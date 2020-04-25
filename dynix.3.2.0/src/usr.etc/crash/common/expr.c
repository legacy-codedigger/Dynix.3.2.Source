/* $Header: expr.c 2.21 1991/07/29 23:26:49 $ */

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * $Log: expr.c,v $
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

#ifndef lint
static char rcsid[] = "$Header: expr.c 2.21 1991/07/29 23:26:49 $";
#endif

#include "crash.h"
#ifdef CROSS
#include	"/usr/include/setjmp.h"
#else
#include <setjmp.h>
#endif

#ifndef NOSTR
#include <string.h>
#endif
#ifdef BSD
#include <sdb.h>
#else
#include <sys/types.h>
#endif
#include <a.out.h>

#ifdef BSD
#define error(s) longjmp(jmp, s)
static jmp_buf jmp;
#else
#define error(s) siglongjmp(jmp, s)
static sigjmp_buf jmp;
#endif
char *err_atoi;

static char *str;
static int nbkt, nbrc;
static struct val term(), lval(), qqqq(), factor(), _expr(), getnum(), getsym();
struct val dot;
static int	is_cast;
static int	for_effect;
extern int	(*Read)();

#define	ID(c)	(isalpha(c) || (c) == '_' || isdigit(c))
#define	ID2(c)	(isalpha(c) || (c) == '_' || (c) == '.' || (c) == ':' || isdigit(c))

expr()
{
	register char *s;
	int n;
	char *how;

	s = token();
	if (debug[15]) {
	    if (s == NULL)
		printf("expr:1: s = token() returns NULL, replacing s with \".\"\n");
	    else
		printf("expr:1: s = token() returns '%s'\n", s);
	}
	if (s == NULL) {
		s = ".";
	} else {
		how = token();
		if (how == NULL)
			how = "X";
	}
	n = atoi(s);
	if (debug[15]) {
	    if (err_atoi)
		printf("expr:2: atoi() returns error\n");
	    else
		printf("expr:2: atoi() returns 0x%x\n", n);
	}
	if ( err_atoi )
		printf("'%s', %s?\n", s, err_atoi);
	else {
		printf("= ", n);
		prstyle(n, how);
		printf("\n");
		if (dot.v_sdb.sdb_name && *dot.v_sdb.sdb_name != ' ') {
			printf("(sdb info) ");
			show_sdb(&dot.v_sdb);
			printf("\n");
		}
	}
}

prstyle(value, style)
	int	value;
	char	*style;
{
	extern	struct	prmode	prm[];
	register  struct  prmode  *pp;
	char	*type;

	for(pp = prm; pp->pr_sw != 0; pp++) {
		if(strcmp(pp->pr_name, style) == 0)
			break;
	}
	switch(pp->pr_sw) {
	case SDB:
#ifdef _SEQUENT_
		{ extern struct sdb **sdbmap;
			printf("coff %d maps to 0x%x\n", value, sdbmap[value]);
			dot.v_sdb = *sdbmap[value];
		}
#endif
		return;
	case OCT2:
	case OCT4:
		printf("%#o",value);
		return;
	case DEC2:
	case DEC4:
		printf("%u",value);
		return;
	case HEX2:
	case HEX4:
		printf("0x%x",value);
		return;
	case CHAR:
	case BYTE:
		printf("'%c'",value);
		return;
	case STRING:
		printf("'%s'",&value);
		return;
	}
}

set()
{
	register char *s, *p;
	register n;
	register struct set_val *v;

	s = token();
	if (s == NULL) {
		for (v = &set_val; v->sv_name; v = v->sv_next)
			printf("%s\tvalue '%#x', ",
				v->sv_name, v->sv_val.v_value);
			show_sdb(&v->sv_val.v_sdb);
			printf("\n");
		return;
	}
	if ((p=strchr(s, '=')) == NULL) {
		printf("'%s', missing '='\n", s);
		return;
	}
	*p = 0;
	if (isalpha(*s) == 0 && *s != '_') {
		printf("'%s', illegal lhs for a token\n", s);
		return;
	}
	if (searchbynam(s)) {
		printf("'%s' already exists\n", s);
		return;
	}
	for (p=s+1; *p; p++) {
		if ( ID(*p) == 0 ) {
			printf("'%s', illegal lhs for token\n", s);
			return;
		}
	}
	(void) atoi(p+1);	/* void cause value stored in dot */
	if ( err_atoi ) {
		printf("'%s', %s?\n", p+1, err_atoi);
		return;
	}
	add_value(s);
}

struct set_val set_val;

add_value(s)
	char *s;
{
	register struct set_val *v;

	v = searchsetval(s);
	if (v) {  /* already exists */
		v->sv_val = dot;
		return;
	}
	/*
	 * Name not in list, so add it
	 */
	for (v = &set_val; v->sv_name; v = v->sv_next)
		/* void */;
	v->sv_next = (struct set_val *) malloc(sizeof (struct set_val));
	bzero(v->sv_next, sizeof (struct set_val));
	v->sv_name = malloc( strlen(s) + 1 );
	strcpy(v->sv_name, s);
	v->sv_val = dot;
	return;
}

struct set_val *
searchsetval(s)
	register char *s;
{
	register struct set_val *v;

	for (v = &set_val; v->sv_name; v = v->sv_next)
		if (strcmp(s, v->sv_name) == 0)
			return v;
	return NULL;
}

atoi(s)
char *s;
{
	char *ret;
	struct val v;

	str = s;
	nbkt = nbrc = 0;
	err_atoi = 0;
	is_cast = 0;
	bzero(&sdbinfo, sizeof(sdbinfo));
	if (debug[15])
	    printf("atoi:1: set str = '%s'\n", s);
#ifdef BSD
	if((ret = (char *)setjmp(jmp)) == 0) {
#else
	if((ret = (char *)sigsetjmp(jmp, 1)) == 0) {
#endif
		v = _expr();
		if( nbkt == 0 && nbrc == 0) {
			dot = v;
			for_effect = 0;
			return v.v_value;	/* normal return */
		}
		ret = "unbalanced parens";
		/* and fall into ... */
	}
	err_atoi = ret;
	for_effect = 0;
	return 0;	/* error */
}


/*
 * like atoi but return final address of object dereferencing any pointers.
 * If no STAB information treat as a pointer.
 */
ato_addr(s, point)
	char *s;
	int point;
{
	atoi(s);
	if (err_atoi)
		return 0;
	if (dot.v_sdb.sdb_desc){
		deref(&dot);
	} else while(point--) {
		(*Read)(dot.v_value, &dot.v_value, sizeof (int *));
	}
	return dot.v_value;
}


static struct val 
_expr()
{ 
	struct val lhs, v;

	if (debug[15])
	    if (str == NULL)
	        printf("_expr:1: str= NULL\n");
	    else
	        printf("_expr:1: str= '%s'\n", str);
	lhs = factor();
	if (debug[15]) {
	    if (str == NULL)
	        printf("_expr:2: str= NULL, lhs=> ", str);
	    else
	        printf("_expr:2: str= '%s', lhs=> ", str);
	    show_val(&lhs);
	    printf("\n");
	}
	for(;;) switch (*str++) {
		case ' ':
		case '\t':
			continue;
		case '+':
			v = factor(); 
			lhs.v_value += v.v_value;
			if (debug[15]) {
			    printf("_expr:3: '+', v.value= 0x%x, new lhs val = 0x%x\n",
				v.v_value, lhs.v_value);
			}
			continue;
		case '-':
			v = factor(); 
			lhs.v_value -= v.v_value;
			if (debug[15]) {
			    printf("_expr:3: '-', v.value= 0x%x, new lhs val = 0x%x\n",
				v.v_value, lhs.v_value);
			}
			continue;
		case '&':
			v = factor(); 
			lhs.v_value &= v.v_value;
			if (debug[15]) {
			    printf("_expr:3: '&', v.value= 0x%x, new lhs val = 0x%x\n",
				v.v_value, lhs.v_value);
			}
			continue;
		case '|':
			v = factor();
			lhs.v_value |= v.v_value;
			if (debug[15]) {
			    printf("_expr:3: '|', v.value= 0x%x, new lhs val = 0x%x\n",
				v.v_value, lhs.v_value);
			}
			continue;
		default:
			str--;
			if (debug[15]) {
			    printf("_expr:3: default case, decremented str, = ");
			    if (str == NULL)
				printf("NULL\n");
			    else
				printf("'%s'\n", str);
			    printf("_expr:4: returning lhs=> ");
			    show_val(&lhs);
			    printf("\n");
			}
			return lhs;
	}
}

static struct val 
factor()
{
	struct val lhs, v;

	if (debug[15])
	    if (str == NULL)
	        printf("factor:1: str= NULL\n", str);
	    else
	        printf("factor:1: str= '%s'\n", str);
	lhs = term();
	if (debug[15]) {
	    if (str == NULL)
		printf("factor:2: str= NULL, lhs=> ");
	    else
	        printf("factor:2: str= '%s', lhs=> ", str);
	    show_val(&lhs);
	    printf("\n");
	}
	for(;;) switch( *str++ ) {
		case ' ':
		case '\t':
			continue;
		case '*':
			v = term();
			lhs.v_value *= v.v_value;
			if (debug[15]) {
			    printf("factor:3: '*', v.value= 0x%x, new lhs val = 0x%x\n",
				v.v_value, lhs.v_value);
			}
			continue;
		case '/':
			v = term();
			lhs.v_value /= v.v_value;
			if (debug[15]) {
			    printf("factor:3: '/', v.value= 0x%x, new lhs val = 0x%x\n",
				v.v_value, lhs.v_value);
			}
			continue;
		case '%':
			v = term();
			lhs.v_value %= v.v_value;
			if (debug[15]) {
			    printf("factor:3: '%', v.value= 0x%x, new lhs val = 0x%x\n",
				v.v_value, lhs.v_value);
			}
			continue;
		default:
			str--;
			if (debug[15]) {
			    printf("factor:3: default case, decremented str, = ");
			    if (str == NULL)
				printf("NULL\n");
			    else
				printf("'%s'\n", str);
			    printf("factor:4: returning lhs=> ");
			    show_val(&lhs);
			    printf("\n");
			}
			return lhs;
	}
}

static struct val 
term()
{
	struct val v;

	if (debug[15])
	    if (str == NULL)
	        printf("term:1: str= NULL\n", str);
	    else
	        printf("term:1: str= '%s'\n", str);
	for(;;) switch ( *str++ ) {
		case ' ':
		case '\t':
			continue;
		case '.':
			if (debug[15]) {
			    printf("term:2: '.', returning dot, => ");
			    show_val(&dot);
			    printf("\n");
			}
			return dot;
		case '*':
			if (debug[15]) {
			    printf("term:2: '*', indirecting though addr\n");
			}
			v = term();
			if (debug[15]) {
			    printf("term:3: address computed as 0x%x\n", v.v_value);
			}
			(*Read)(v.v_value, &v.v_value, sizeof (int *));
			if (debug[15]) {
			    printf("term:4: indirection gives value 0x%x\n", v.v_value);
			}
			return v;
		case '-':
			if (debug[15]) {
			    printf("term:2: '-', returning neg value\n");
			}
			v = term();
			if (debug[15]) {
			    printf("term:3: value computed as 0x%x = %d\n",
			        v.v_value, v.v_value);
			}
			v.v_value = -v.v_value;
			if (debug[15]) {
			    printf("term:4: returning neg as 0x%x = %d\n",
			        v.v_value, v.v_value);
			}
			return v;
		case '~':
			if (debug[15]) {
			    printf("term:2: '~', returning complement value\n");
			}
			v = term();
			if (debug[15]) {
			    printf("term:3: value computed as 0x%x\n", v.v_value);
			}
			v.v_value = ~v.v_value;
			if (debug[15]) {
			    printf("term:4: returning complement as 0x%x\n", v.v_value);
			}
			return v;
		case '&':	/* address of */
			for_effect = 1;
			v = term();
			for_effect = 0;
			if (debug[15]) {
			    printf("term:4: returning address of as 0x%x\n", v.v_value);
			}
			return v;
		case '(':
			nbkt++;
			if (cast()) {
				str++;
				nbkt--;
				if (debug[15]) {
				    printf("term:2: end of cast str='%s'\n",str);
				}
				continue;
			}
			if (debug[15]) {
			    printf("term:2: '(', returning expr\n");
			}
			v = _expr();
			if (*str != ')')
				error("unbalanced parens");
			else { 
				str++;
				nbkt--;
				if (debug[15]) {
				    printf("term:3: returning v=> ");
				    show_val(&v);
				    if (str == NULL)
					printf(", str= NULL\n");
				    else
					printf(", str= '%s'\n", str);
				}
				return v;
			}
		default:
			str--;
			if (debug[15]) {
			    printf("term:3: default case, decremented str, = ");
			    if (str == NULL)
				printf("NULL\n");
			    else
				printf("'%s'\n", str);
			}
			if (0) ;	/* assembler bug workaround! */
			v = qqqq();
			if (!for_effect && *str != '\0')
				deref(&v);
			return (v);
	}
}

static struct val 
qqqq()
{
	struct val lhs, v;

	if (debug[15])
	    if (str == NULL)
	        printf("qqqq:1: str= NULL\n", str);
	    else
	        printf("qqqq:1: str= '%s'\n", str);
	lhs = lval(0);
	if (debug[15]) {
	    if (str == NULL)
		printf("qqqq:2: str= NULL, lhs=> \n\t");
	    else
	        printf("qqqq:2: str= '%s', lhs=> \n\t", str);
	    show_val(&lhs);
	    printf("\n");
	}

	for (;;) {
		switch( *str++ ) {
		case ' ':
		case '\t':
			if (debug[15])
			    printf("qqqq:3: case ' ' or '\t', continuing\n");
			continue;
		case '[':
			nbrc++;
			if (debug[15])
			    printf("qqqq:3: '[', caling _expr\n");
			deref(&lhs);	/* if pointer derefrence */
			v = _expr();
			if (debug[15]) {
			    printf("qqqq:4: expr returns v=> ");
			    show_val(&v);
			    if (str == NULL)
				printf(", str= NULL\n");
			    else
				printf(", str= '%s'\n", str);
			}
			if (*str!=']')
				error("unbalanced brackets");
			else {
				str++;
				nbrc--;
				if (debug[15]) {
				    printf("qqqq:4: adding \"lhs.v_sdb.sdb_size * v.v_value\" to lhs.v_value\n");
				    printf("   lhs=> ");
				    show_val(&lhs);
				    printf("\n    v=> ");
				    show_val(&v);
				    printf("\n");
				}
				lhs.v_value += lhs.v_sdb.sdb_size * v.v_value;
				if (debug[15]) {
				    printf("qqqq:5: set lhs=> ");
				    show_val(&lhs);
				    printf("\n");
				}
				continue;
			}
		 case '.':
			if (debug[15])
			    printf("qqqq:3: '.', calling lval\n");
			deref(&lhs);	/* if pointer derefrence */
			v = lval(&lhs);
			if (debug[15]) {
			    printf("qqqq:4: v.v_value = lhs.v_value + v.v+sdb.sdb_soff\n");
			    printf("   lhs=>");
			    show_val(&lhs);
			    printf("\n    v=>");
			    show_val(&v);
			    printf("\n");
			}
			v.v_value = lhs.v_value + v.v_sdb.sdb_soff;
			lhs = v;
			if (debug[15]) {
			    printf("qqqq:5: set lhs=> ");
			    show_val(&lhs);
			    printf("\n");
			}
			continue;
		case '-':
			if( *str == '>' ) {
				if (debug[15]) {
				    printf("qqqq:3: '->', calling lval\n");
				}
				str++;
				deref(&lhs);	/* if pointer derefrence */
				v = lval(&lhs);
				if (debug[15]) {
				    printf("qqqq:4: v.v_value = lhs.v_value + v.v_sdb.sdb_soff\n");
				    printf("   lhs=> ");
				    show_val(&lhs);
				    printf("\n    v=> ");
				    show_val(&v);
				    printf("\n");
				}
				v.v_value = lhs.v_value + v.v_sdb.sdb_soff;
				lhs = v;
				if (debug[15]) {
			    	    printf("qqqq:5: set lhs=> ");
			    	    show_val(&lhs);
			    	    printf("\n");
				}
				continue;
			}
			if (debug[15])
			    printf("qqqq:3: '-', falls into default\n");
		default:
			str--;
			if (debug[15]) {
			    printf("qqqq:3: default case, decremented str, = ");
			    if (str == NULL)
				printf("NULL\n");
			    else
				printf("'%s'\n", str);
			    printf("qqqq:4: returning lhs=> ");
			    show_val(&lhs);
			    printf("\n");
			}
			return lhs;
		}
	}
}


/*
 * process simple casts.
 * (interal types, structs, unions, pointers)
 * does not deal with arrays or functions or parenthasised expressions.
 */
cast()
{
	int	is_struct;
	char	name;
	char symbol[ MAXSTRLEN ], *p = symbol;

	if (debug[15]) {
		printf("cast:1: str, = ");
		if (str == NULL)
			printf("NULL\n");
		else
			printf("'%s'\n", str);
	}
	is_struct = 0;
	while (*str != ')') {
		if (*str == ' ' || *str == '\t') {
			str++;
			continue;
		}
		if (*str == '\0') {
			error("badly formed cast");
			return (0);
		}
		if (strncmp(str, "struct", 6) == 0) {
			str += 6;
			is_struct = 1;
		} else if (strncmp(str, "union", 5) == 0) {
			str += 5;
			is_struct = 1;
		} else if (strncmp(str, "int", 3) == 0) {
			dot.v_sdb.sdb_name = " int";
			dot.v_sdb.sdb_size = 4;
			dot.v_sdb.sdb_type = N_GSYM;
#ifdef BSD
			dot.v_sdb.sdb_desc = PCCT_INT;
#else
			dot.v_sdb.sdb_desc = T_INT;
#endif
			str += 3;
		} else if (strncmp(str, "char", 4) == 0) {
			dot.v_sdb.sdb_name = " char";
			dot.v_sdb.sdb_size = 1;
			dot.v_sdb.sdb_type = N_GSYM;
#ifdef BSD
			dot.v_sdb.sdb_desc = PCCT_CHAR;
#else
			dot.v_sdb.sdb_desc = T_CHAR;
#endif
			str += 4;
		} else if (strncmp(str, "short", 5) == 0) {
			dot.v_sdb.sdb_name = " short";
			dot.v_sdb.sdb_size = 2;
			dot.v_sdb.sdb_type = N_GSYM;
#ifdef BSD
			dot.v_sdb.sdb_desc = PCCT_SHORT;
#else
			dot.v_sdb.sdb_desc = T_SHORT;
#endif
			str += 5;
		} else if (strncmp(str, "long", 4) == 0) {
			dot.v_sdb.sdb_name = " long";
			dot.v_sdb.sdb_size = 4;
			dot.v_sdb.sdb_type = N_GSYM;
#ifdef BSD
			dot.v_sdb.sdb_desc = PCCT_LONG;
#else
			dot.v_sdb.sdb_desc = T_LONG;
#endif
			str += 4;
		} else if (strncmp(str, "*", 1) == 0) {
			dot.v_sdb.sdb_desc = INCREF(dot.v_sdb.sdb_desc);
			str += 1;
		} else if (is_struct) {
			while (ID(*str) && p < &symbol[ MAXSTRLEN -1])
				*p++ = *str++;
			*p = '\0';
			if (search_stag(symbol)){
				dot.v_sdb = sdbinfo;
				is_struct = 0;
			} else {
				error("unknown structure in cast");
				return (0);
			}
		} else {
			/*
			 * might be typecast
			 */
			while (ID(*str) && p < &symbol[ MAXSTRLEN -1])
				*p++ = *str++;
			*p = '\0';
			if (!search_stag(symbol)){
				return (0);
			}
			dot.v_sdb = sdbinfo;
		}
	}
	if (debug[15]) {
		printf("cast:2: :");
		show_sdb(&dot.v_sdb);
		printf("\n");
	}
	is_cast = 1;
	return (1);
}

static struct val 
lval(lhs)
	struct	val	*lhs;
{
	if (debug[15]) {
		printf("lval: str= '%s'\n", str);
	}

	if( isdigit(*str) )
		return getnum();
	if( isalpha(*str) || *str == '_')
		return getsym(lhs);
	error("badly formed expression");
}


static struct val 
getsym(lhs)
	struct	val *lhs;
{
	static char symbol[MAXSTRLEN];
	char 	*p = symbol;
	char	*q;
	struct val v;

	q = str;
	while (ID(*str) && p < &symbol[ MAXSTRLEN -1])
		*p++ = *str++;
	*p = '\0';
	if (debug[15]) {
		printf("getsym: str= '%s' is_cast=%d\n", symbol, is_cast);
		if (lhs) {
			show_val(lhs);
			printf("\n");
		}
	}
#ifdef BSD
	if (lhs && (lhs->v_sdb.sdb_name != NULL)) {
		v.v_value = search_mos(symbol, is_cast, &lhs->v_sdb);
	} else {
		v.v_value = search(symbol);
		if (err_search)
			v.v_value = search_mos(symbol, is_cast, &dot.v_sdb);
	}
#else
	if (lhs && (lhs->v_sdb.sdb_name != NULL)) {
		v.v_value = search_mos(symbol, lhs->v_sdb.sdb_inx);
	} else {
		v.v_value = search(symbol);
		if (err_search)
			v.v_value = search_mos(symbol, 0);
	}
#endif
	/*
	 * might have embedded file name for static global
	 */
	if (err_search  && !lhs) {
		str = q;
		p = symbol;
		while (ID2(*str) && p < &symbol[ MAXSTRLEN -1])
			*p++ = *str++;
		*p = '\0';
		if (debug[15]) {
			printf("getsym: str= '%s' is_cast=%d\n", symbol, is_cast);
			if (lhs) {
				show_val(lhs);
				printf("\n");
			}
		}
		v.v_value = search(symbol);
	}

	if ( err_search ) 
		error(symbol);
	if (is_cast && (lhs == 0)) {
		v.v_sdb = dot.v_sdb;
	} else
		v.v_sdb = sdbinfo;
	/*
	 * Certain globals have BSS entries but no GSYM stab entries.
	 * So hack around the problem by assinging them something here.
	 */
	if (v.v_sdb.sdb_name == NULL)
		v.v_sdb.sdb_name = q;
	if (xdebug) {
		printf("\n..getsym('%s'): value '%#x' ", symbol, v.v_value);
		show_sdb(&v.v_sdb);
		printf("\n");
	}
	return v;
}

static struct val 
getnum()
{
	register int c;
	register unsigned int n = 0;
	struct val v;

	if (debug[15]) {
		printf("getnum: str = '%s' is_cast= %d\n", str, is_cast);
	}
	c = *str++;
	if( (c=='0') && ( (*str=='x') || (*str=='X') ) ) {
		str++;
		while( isxdigit( c = *str++ ) ) {
			if( isdigit(c) )
				c = c - '0';
			else if( isupper(c) )
				c = c - 'A' + 10;
			else
				c = c - 'a' + 10;
			n = n * 16 + c;
		}
	} else for (n =  c - '0'; isdigit(c = *str++); ) {
		n = n * 10 + c - '0';
	}
	str--;
	v.v_value = n;
	if (debug[15]) {
		printf("getnum: value 0x%x (%d) str = '%s'\n", n, n, str);
	}
	if (is_cast) {
		v.v_sdb = dot.v_sdb;
	} else {
		bzero(&v.v_sdb, sizeof (struct sdb));
		v.v_sdb.sdb_size = sizeof (int);
		v.v_sdb.sdb_name = " int";
		v.v_sdb.sdb_type = N_GSYM;
#ifdef BSD
		v.v_sdb.sdb_desc = PCCT_INT;
		v.v_sdb.sdb_sym = 0;
#else
		v.v_sdb.sdb_desc = T_INT;
#endif
	}
	return v;
}

show_val(v)
	struct val *v;
{
	printf(" val= 0x%x", v->v_value);
	if (v->v_sdb.sdb_name == NULL)
		printf(" sdb_name= NULL");
	else {
		show_sdb(&v->v_sdb);
	}
}


deref(v)
	struct val *v;
{
	if (debug[15]) {
		printf("deref: ");
		show_sdb(&v->v_sdb);
		printf("\n");
	}
	/* 
	 * if basic type is a pointer, must de-reference it now
	 */
#ifdef BSD
	while (desc(&v->v_sdb.sdb_desc) == PCCTM_PTR) {
#else
	while (desc(&v->v_sdb.sdb_desc) == DT_PTR) {
#endif
		(*Read)(v->v_value, &v->v_value, sizeof (int *));
		v->v_sdb.sdb_desc = DECREF(v->v_sdb.sdb_desc);
		if (xdebug || debug[15]) 
			printf("de-reference pointer to '%#x'\n", v->v_value);
	}
}

