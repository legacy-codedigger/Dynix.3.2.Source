#ident "$Header: util.c 1.3 1991/07/17 02:54:22 $"

/*
 * $Copyright: $
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
 * $Log: util.c,v $
 *
 *
 */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * This file contains code for utilities used by more than one crash function.
 */

#include <stdio.h>
#include <setjmp.h>
#ifdef _SEQUENT_
#include <sys/param.h>
#include <a.out.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/var.h>
#include <crash.h>


#define ARGLEN 40	/* max length of argument */

extern char *args[];		/* argument array */
extern int argcnt;		/* argument count */
extern int optind;	/* argument index */
extern char *optarg;	/* getopt argument */
extern char *strpbrk();


/* print error message */
/*VARARGS1*/
int
prerrmes(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{
	printf(string,arg1,arg2,arg3);
}

struct sym *
symsrch(s)
	char *s;
{
	return(searchbynam(s));
}

/* string to hexadecimal long conversion */
long
hextol(s)
char *s;
{
	int	i,j;
		
	for(j = 0; s[j] != '\0'; j++)
		if((s[j] < '0' || s[j] > '9') && (s[j] < 'a' || s[j] > 'f')
			&& (s[j] < 'A' || s[j] > 'F'))
			break ;
	if(s[j] != '\0' || sscanf(s, "%x", &i) != 1) {
		prerrmes("%c is not a digit or letter a - f\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to decimal long conversion */
long
stol(s)
char *s;
{
	int	i,j;
		
	for(j = 0; s[j] != '\0'; j++)
		if((s[j] < '0' || s[j] > '9'))
			break ;
	if(s[j] != '\0' || sscanf(s, "%d", &i) != 1) {
		prerrmes("%c is not a digit 0 - 9\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to octal long conversion */
long
octol(s)
char *s;
{
	int	i,j;
		
	for(j = 0; s[j] != '\0'; j++)
		if((s[j] < '0' || s[j] > '7')) 
			break ;
	if(s[j] != '\0' || sscanf(s, "%o", &i) != 1) {
		prerrmes("%c is not a digit 0 - 7\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to binary long conversion */
long
btol(s)
char *s;
{
	int	i,j;
		
	i = 0;
	for(j = 0; s[j] != '\0'; j++)
		switch(s[j]) {
			case '0' :	i = i << 1;
					break;
			case '1' :	i = (i << 1) + 1;
					break;
			default  :	prerrmes("%c is not a 0 or 1\n",s[j]);
					return(-1);
		}
	return(i);
}

/* string to number conversion */
long
strcon(string,format)
char *string;
char format;
{
	char *s;

	s = string;
	if(*s == '0') {
		if(strlen(s) == 1)
			return(0); 
		switch(*++s) {
			case 'X' :
			case 'x' :	format = 'h';
					s++;
					break;
			case 'B' :
			case 'b' :	format = 'b';
					s++;
					break;
			case 'D' :
			case 'd' :	format = 'd';
					s++;
					break;
			default  :	format = 'o';
		}
	}
	if(!format)
		format = 'd';
	switch(format) {
		case 'h' :	return(hextol(s));
		case 'd' :	return(stol(s));
		case 'o' :	return(octol(s));
		case 'b' :	return(btol(s));
		default  :	return(-1);
	}
}



/* simple arithmetic expression evaluation ( +  - & | * /) */
long
eval(string)
char *string;
{
	int j,i;
	char rand1[ARGLEN];
	char rand2[ARGLEN];
	char *op;
	long addr1,addr2;
	struct sym *sp;

	if(string[strlen(string)-1] != ')') {
		prerrmes("(%s is not a well-formed expression\n",string);
		return(-1);
	}
	if(!(op = strpbrk(string,"+-&|*/"))) {
		prerrmes("(%s is not an expression\n",string);
		return(-1);
	}
	for(j=0,i=0; string[j] != *op; j++,i++) {
		if(string[j] == ' ')
			--i;
		else rand1[i] = string[j];
	}
	rand1[i] = '\0';
	j++;
	for(i = 0; string[j] != ')'; j++,i++) {
		if(string[j] == ' ')
			--i;
		else rand2[i] = string[j];
	}
	rand2[i] = '\0';
	if(!strlen(rand2) || strpbrk(rand2,"+-&|*/")) {
		prerrmes("(%s is not a well-formed expression\n",string);
		return(-1);
	}
	if(sp = symsrch(rand1))
		addr1 = sp->sym_value;
	else if((addr1 = strcon(rand1,NULL)) == -1)
		return(-1);
	if(sp = symsrch(rand2))
		addr2 = sp->sym_value;
	else if((addr2 = strcon(rand2,NULL)) == -1)
		return(-1);
	switch(*op) {
		case '+' : return(addr1 + addr2);
		case '-' : return(addr1 - addr2);
		case '&' : return(addr1 & addr2);
		case '|' : return(addr1 | addr2);
		case '*' : return(addr1 * addr2);
		case '/' : if(addr2 == 0) {
				prerrmes("cannot divide by 0\n");
				return(-1);
			   }
			   return(addr1 / addr2);
	}
	return(-1);
}
		



/*
 * putch() recognizes escape sequences as well as characters and prints the
 * character or equivalent action of the sequence.
 */
int
putch(c)
char  c;
{
	c &= 0377;
	if(c < 040 || c > 0176) {
		printf("\\");
		switch(c) {
		case '\0': c = '0'; break;
		case '\t': c = 't'; break;
		case '\n': c = 'n'; break;
		case '\r': c = 'r'; break;
		case '\b': c = 'b'; break;
		default:   c = '?'; break;
		}
	}
	else printf(" ");
	printf("%c ",c);
}

/* check to see if string is a symbol or a hexadecimal number */
int
isasymbol(string)
char *string;
{
	int i;

	for(i = strlen(string); i > 0; i--)
		if(!isxdigit(*string++))
			return(1);
	return(0);
}


/* convert a string into a range of slot numbers */
int
range(max,begin,end)
int max;
long *begin,*end;
{
	int i,j,len,pos;
	char string[ARGLEN];
	char temp1[ARGLEN];
	char temp2[ARGLEN];

	strcpy(string,args[optind]);
	len = strlen(string);
	if((*string == '-') || (string[len-1] == '-')){
		printf("%s is an invalid range\n",string);
		*begin = -1;
		return;
	}
	pos = strcspn(string,"-");
	for(i = 0; i < pos; i++)
		temp1[i] = string[i];
	temp1[i] = '\0';
	for(j = 0, i = pos+1; i < len; j++,i++)
		temp2[j] = string[i];
	temp2[j] = '\0';
	if((*begin = (int)stol(temp1)) == -1)
		return;
	if((*end = (int)stol(temp2)) == -1) {
		*begin = -1;
		return;
	}
	if(*begin > *end) {
		printf("%d-%d is an invalid range\n",*begin,*end);
		*begin = -1;
		return;
	}
	if(*end >= max) 
		*end = max - 1;
}


/* get slot number in table from address */
int
getslot(addr,base,size,phys,max)
long addr,base,max;
int size,phys; /* phys is ignored for now */
{
	int slot;

	slot = (addr - base) / size;
	if ((slot >= 0) && (slot < max))
		return(slot);
	else
		return(-1);
}


/* argument processing from **args */
long
getargs1(max,arg1,arg2)
	int max;
	long *arg1,*arg2;
{	
	struct sym *sp;
	long slot;

	/* range */
	if(strpbrk(args[optind],"-")) {
		range(max,arg1,arg2);
		return;
	}
	/* expression */
	if(*args[optind] == '(') {
		*arg1 = (eval(++args[optind]));
		return;
	}
	/* symbol */
	if((sp = symsrch(args[optind])) != NULL) {
		*arg1 = (sp->sym_value);
		return;
	}
/*
	if(isasymbol(args[optind])) {
		prerrmes("%s not found in symbol table\n",args[optind]);
		*arg1 = -1;
		return;
	}
*/
	/* slot number */
	if((slot = strcon(args[optind],'d')) == -1) {
		*arg1 = -1;
		return;
	}
	if((slot < max) && (slot >= 0)) {
		*arg1 = slot;
		return;
	}
	/* address */
	*arg1 = slot;
} 

fixup_args()
{
	argcnt = 0;
	optind = 0;
	while ((args[argcnt++] = token()) != NULL) {
	}
}

#endif

/* error handling */
/*VARARGS1*/
int
error(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{
#ifdef _SEQUENT_
	extern sigjmp_buf jmp;		/* syntax error label */ 
#else
	extern jmp_buf jmp;		/* syntax error label */ 
#endif
	extern int Eve;

	printf(string,arg1,arg2,arg3);
#ifdef _SEQUENT_
	siglongjmp(jmp, string);
#else
	longjmp(jmp, string);
#endif
	Eve = -1;
}
