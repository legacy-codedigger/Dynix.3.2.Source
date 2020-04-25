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

#ifndef lint
static char rcsid[] = "$Header: sym.c 2.25 1991/08/30 22:00:58 $";
#endif

#include "crash.h"
#include <sys/stat.h>
#include <a.out.h>
#include <stab.h>
#include "stabstring.h"

#define streq(s1, s2)   (strcmp(s1, s2) == 0)

#define	tprintf		if (debug[2]) printf

/* Tune these for speed */
#define NUM_STE	 (8*BUFSIZ/sizeof (struct nlist))	/* 8k reads */
#define NUM_SYM	 5000
#define NUM_SDB	 5000

char *bsearch();
char *save_name();
static	struct	sym *symtable, **symtablepv, **symtablepn, *searchbyval();
static	struct	sdb *sdbsstable, **sdbsstablepn;
static	struct	sdb *sdbgstable, **sdbgstablepn;
static	int cmpsymbyval(), cmpsymbynam(), cmpsdbbynam();
static	char *stringtable;


static	struct exec exec;
static	int total_symbols, glb_symbols, sdb_sssymbols, sdb_gssymbols;
static  struct sdb *Sdbp;

struct	sdb sdbinfo;

long    text_size;
long    text_offset;
long    text_start;
long    data_size;
long    data_offset;
long    data_start;
long    bss_size;
long    bss_start;

getsymboltable()
{
	register ret;
	struct stat sbuf;
	int stringtblsize;

	ret = fstat(kernfd, &sbuf);
	SANITY(ret == -1, "bad stat on", dynix);

	ret = lseek(kernfd, 0L, 0);
	SANITY(ret == -1, "bad lseek on", dynix);

	ret = read(kernfd, &exec, sizeof (struct exec));
	SANITY(ret != sizeof (struct exec), "can't read a.out header for", dynix);

	SANITY(N_BADMAG(exec), "bad magic number for", dynix);
	total_symbols = exec.a_syms / sizeof (struct nlist);

	ret = lseek(kernfd, (long) N_TXTOFF(exec), 0);
	SANITY(ret == -1, "N_TXTOFF lseek failed on", dynix);

	stringtblsize = sbuf.st_size - N_STROFF(exec);
	stringtable = malloc( (unsigned)stringtblsize );
	SANITY(stringtable == NULL, "malloc of string table failed", "");

	ret = lseek(kernfd, (long) N_STROFF(exec), 0);
	SANITY(ret == -1, "N_STROFF lseek failed", "");

	ret = read(kernfd, stringtable, stringtblsize);
	SANITY(ret != stringtblsize, "can't read string table for", dynix);

	symbols_init();
	process();
}

static
process()
{
	struct nlist lnlist[ NUM_STE ];
	register struct nlist *s;
	register n, ret, type;
	register char *cp;
	
	ret = lseek(kernfd, (long) N_SYMOFF(exec), 0);
	SANITY(ret == -1, "N_SYMOFF lseek failed", "");

	for (n = total_symbols; n > 0; n -= ret) {
		ret = read(kernfd, (char *)lnlist, sizeof (struct nlist) * MINS(NUM_STE, n));
		SANITY(ret <= 0, "bad read of symbol table", "");
		ret /= sizeof (struct nlist);
		for (s=lnlist; s < &lnlist[ret]; s++) {
			if (s->n_un.n_strx == 0) {
				tprintf("%d: skipped			<<<<\n", (total_symbols - n) + (s - lnlist));
				continue;
			}
			if (s->n_type & N_STAB) {
				s->n_un.n_strx += (unsigned long) stringtable;
				tprintf("%d: sdb_add: ", (total_symbols - n) + (s - lnlist));
				sdb_add(s);
				continue;
			}
			type = (s->n_type & N_TYPE);
			tprintf("%d: n_type,type == %#x,%#x: ", (total_symbols - n) + (s - lnlist), s->n_type, type);
			if (type == N_TEXT || type == N_DATA || type == N_BSS ||
				type == N_ABS) {
				s->n_un.n_strx += (unsigned long) stringtable;
				cp = (char *) s->n_un.n_strx;
				if (   (   cp[0] == 'L'
					&& cp[1] >= '0' 
					&& cp[1] <= '9'
				        )
				  ||   (   cp[0] == '.'
				        && cp[1] == 'L'
					)
				  ||    strcmp(cp, "_rcsid") == 0
				  ) { /* skip these symbols */
					tprintf("'%s', value=%#x   SKIPPED <<<<<<<\n", cp, s->n_value);
					continue;
				}
				sym_add(s);
				tprintf("'%s', value=%#x   REG symbol\n", (char *) s->n_un.n_strx, s->n_value);
				continue;
			}
			tprintf("\n");
		}
	}

	/* allocate memory for for arrays of pointers */
	symtablepn = (struct sym **) malloc ( (unsigned) glb_symbols * sizeof (struct sym *) );
	SANITY(symtablepn == NULL, "malloc failed", "");
	symtablepv = (struct sym **) malloc ( (unsigned) glb_symbols * sizeof (struct sym *) );
	SANITY(symtablepv == NULL, "malloc failed", "");
	sdbgstablepn = (struct sdb **) malloc ( (unsigned) sdb_gssymbols * sizeof (struct sdb *) );
	SANITY(sdbgstablepn == NULL, "malloc failed", "");
	sdbsstablepn = (struct sdb **) malloc ( (unsigned) sdb_sssymbols * sizeof (struct sdb *) );
	SANITY(sdbsstablepn == NULL, "malloc failed", "");

	/* set all pointers before sorting them */
	for (n=0; n < glb_symbols; n++) {
		symtablepn[n] = symtablepv[n] = &symtable[n];
		if (debug[1]) {
			printf("symtable[%d] = '%s' @ 0x%x\n", n,
				symtable[n].sym_name, symtable[n].sym_value);
		}
		/*
		 * Now that realloc() will not happen, pin down the adresses.
		 */
		if (symtable[n].sym_sdbi != -1) {
			symtable[n].sym_sdb = &sdbgstable[symtable[n].sym_sdbi];
		}
	}
	for (n=0; n < sdb_sssymbols; n++) {
		sdbsstablepn[n] = &sdbsstable[n];
		/*
		 * Now that realloc() will not happen, pin down the adresses.
		 */
		if (sdbsstable[n].sdb_sym) {
			if (!sdbsstable[n].sdb_sym->type) {
				printf("WARNING no type for ss #%d '%s'\n",
					n,sdbsstable[n].sdb_name); 
			} else {
				sdbsstable[n].sdb_sym->type->sdb = &sdbsstable[n];
			}
		}
	}
	for (n=0; n < sdb_gssymbols; n++) {
		sdbgstablepn[n] = &sdbgstable[n];
		/*
		 * Now that realloc() will not happen, pin down the adresses.
		 */
		if (sdbgstable[n].sdb_sym) {
			if (!sdbgstable[n].sdb_sym->type) {
				printf("WARNING: no type for gs#%d'%s'\n",
					n,sdbgstable[n].sdb_name);
			} else {
				sdbgstable[n].sdb_sym->type->sdb = &sdbgstable[n];
			}
		}
	}

	/* sort global symbols by name and value, sdb by name only */
	qsort((char *)symtablepn, glb_symbols, sizeof (struct sym *), cmpsymbynam);
	qsort((char *)symtablepv, glb_symbols, sizeof (struct sym *), cmpsymbyval);
	qsort((char *)sdbgstablepn, sdb_gssymbols, sizeof (struct sdb *), cmpsdbbynam);
	qsort((char *)sdbsstablepn, sdb_sssymbols, sizeof (struct sdb *), cmpsdbbynam);

#ifdef	VERBOSE
	printf("%d total symbols (%d=global, %d=globals %d=types) %d%% useful\n", 
		total_symbols, glb_symbols, sdb_gssymbols, sdb_sssymbols, 
		((glb_symbols+sdb_sssymbols+sdb_gssymbols)*100)/total_symbols);
#endif
}


/*
 * note can not use return  (*a)->sym_value - (*b)->sym_value
 * as these are unsigned vales.
 */
static int
cmpsymbyval(a, b)
	struct sym **a, **b;
{

	if ((*a)->sym_value > (*b)->sym_value)
		return 1;
	if ((*a)->sym_value < (*b)->sym_value)
		return -1;
	return 0;
}

static int
cmpsymbynam(a, b)
	struct sym **a, **b;
{
	register char *s1, *s2;

	/* expanded strcmp() */
	s1 = (*a)->sym_name;
	s2 = (*b)->sym_name;
	if (debug[5]) printf("cmpsymbynam: '%s' .. '%s'\n", s1, s2);
	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return 0;
	return (*s1 - *--s2);
}

static int
cmpsdbbynam(a, b)
	struct sdb **a, **b;
{
	register char *s1, *s2;

	/* expanded strcmp() */
	s1 = (*a)->sdb_name;
	s2 = (*b)->sdb_name;
	if (debug[5]) printf("cmpsdbbynam: '%s' .. '%s'\n", s1, s2);
	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return 0;
	return (*s1 - *--s2);
}

sym_add(s)
	struct nlist *s;
{
	extern struct sdb;
	static struct sym *syme, *symp;

	if (symp == NULL || symp >= syme) {
		unsigned size = (unsigned)syme - (unsigned)symtable + (NUM_SYM * sizeof (struct sym));
		if (symp == NULL)
			symtable = (struct sym *) malloc ( size );
		else
			symtable = (struct sym *) realloc( (char *) symtable, size );
		SANITY(symtable == NULL, "malloc for ste symbol table failed", "");
		size /= sizeof (struct sym);
		symp = &symtable[ size - NUM_SYM ];
		syme = &symtable[ size ];
	}
	symp->sym_name = (char *)s->n_un.n_strx;
	symp->sym_value = s->n_value;
	symp->sym_sdbi = sdb_add_gs(s);
	symp->sym_sdb = 0;
	++glb_symbols;
	++symp;
}

struct symbol *entersym();

static
sdb_add(s)
	register struct nlist *s;
{
	extern struct symbol *symbol_alloc();
	extern struct symbol *t_int;
	struct symbol *sym;

	if (s->n_type == N_LSYM) {
		if (sym = entersym((char *)s->n_un.n_strx, s)) {
			tprintf("kept Local '%s' @0x%x\n", sym->name, sym);
			sdb_add_ss(sym);
		}
		return;
	}

	if (s->n_type == N_GSYM) {
		(void) sdb_add_gs(s);
		return;
	}

	if (s->n_type == N_LENG || s->n_type == N_SSYM) {
		tprintf("got a bogus symbol type: 0x%x '%s'\n", 
				s->n_type, s->n_un.n_name);
		/*
		 * Support old style STABS
		 */
		if (s->n_type == N_LENG) {
			if (strcmp(Sdbp->sdb_name, s->n_un.n_name) == 0) {
				Sdbp->sdb_size = s->n_value;
			}
		} else {
			sym = symbol_alloc();
			sym->symvalue.field.offset = s->n_value * 8;
			sym->name = s->n_un.n_name;
			sym->symvalue.field.length = 4;
			sym->np = s;
			sym->type = t_int;
			sym->class = 0;
			sdb_add_ss(sym);
		}
		return;
	}
}

sdb_add_ss(s)
	register struct symbol *s;
{
	static struct sdb *sdbe, *sdbp;
	struct symbol *sym;

	if (sdbp == NULL || sdbp >= sdbe) {
		unsigned size = (unsigned)sdbe - (unsigned)sdbsstable + 
				(NUM_SDB * sizeof (struct sdb));
		if (sdbp == NULL)
			sdbsstable = (struct sdb *) malloc ( size );
		else
			sdbsstable = (struct sdb *)
					realloc((char *)sdbsstable, size );
		tprintf("*** MALLOC ***\n");
		SANITY(sdbsstable == NULL, "malloc for sdb symbol table failed",
			"");
		size /= sizeof (struct sdb);
		sdbp = &sdbsstable[ size - NUM_SDB ];
		sdbe = &sdbsstable[ size ];
	}

	sdbp->sdb_soff = s->symvalue.field.offset / 8;	/* convert to bytes */
	sdbp->sdb_type = N_SSYM;
	sdbp->sdb_name = s->name;
	sdbp->sdb_sym = s;
	sdbp->sdb_inx = sdb_sssymbols;
	if (s->np->n_type == N_LSYM) {
		sdbp->sdb_size = size(s);
		sdbp->sdb_desc = set_desc(s);
	} else {
		sdbp->sdb_size = 4;
		sdbp->sdb_desc = s->np->n_desc;
	}
	if (debug[2]) {
		printf("Kept sdb ");
		show_sdb(sdbp);
		printf("\n");
	}
	Sdbp = sdbp;
	++sdb_sssymbols;
	++sdbp;
}

static int
sdb_add_gs(s)
	register struct nlist *s;
{
	static struct sdb *sdbe, *sdbp;
	struct symbol *sym;

	if (sdbp == NULL || sdbp >= sdbe) {
		unsigned size = (unsigned)sdbe - (unsigned)sdbgstable + 
				(NUM_SDB * sizeof (struct sdb));
		if (sdbp == NULL)
			sdbgstable = (struct sdb *) malloc ( size );
		else
			sdbgstable = (struct sdb *)
					realloc((char *)sdbgstable, size );
		tprintf("*** MALLOC ***\n");
		SANITY(sdbgstable == NULL, "malloc for sdb symbol table failed",
			"");
		size /= sizeof (struct sdb);
		sdbp = &sdbgstable[ size - NUM_SDB ];
		sdbe = &sdbgstable[ size ];
	}

	sdbp->sdb_soff = s->n_value;
	sdbp->sdb_type = s->n_type;
	switch (s->n_type) {
	case N_GSYM:
	case N_STSYM:
	case N_LSYM:
	case N_SSYM:
	case N_PSYM:
	case N_RSYM:
		sdbp->sdb_desc = s->n_desc;
		break;
	default:
		sdbp->sdb_desc = 0;
	}

	if (index(s->n_un.n_name, ':')) {
		sym = entersym(s->n_un.n_name, s);
		if (debug[2]) {
			printf("enter sym ");
		}
		sdbp->sdb_name = sym->name;
		if (sym->type->class == PTR) {
			sdbp->sdb_size = size(sym->type->type);
		} else {
			sdbp->sdb_size = size(sym);
		}
		sdbp->sdb_sym = sym;
		sdbp->sdb_desc = set_desc(sym);
	} else {
		sdbp->sdb_name = (char *)(s->n_un.n_strx);
		sdbp->sdb_size = 4;
		sdbp->sdb_sym = 0;
	}

	sdbp->sdb_inx = sdb_gssymbols;

	if (debug[2]) {
		printf("Kept global ");
		show_sdb(sdbp);
		printf("\n");
	}
	Sdbp = sdbp++;
	return (sdb_gssymbols++);
}


/*
 * Search routines
 */

/* given a value, looks up the name */
static struct sym *
searchbyval(value, offset)
	unsigned value, *offset;
{
	struct sym lsym, *p = &lsym;
	register struct sym **pp, **ppend;

	*offset = 0;
	p->sym_value = value;
	pp = (struct sym **) bsearch((char *)&p, (char *)symtablepv, glb_symbols, sizeof (struct sym *), cmpsymbyval);
	if (pp == NULL) {
		/*
		 * Binary search for exact value failed, so try again with linear
		 * search to find value just before so result is "value+offset".
		 */
		for( pp = symtablepv, ppend = &symtablepv[glb_symbols]; pp != ppend; pp++) {
			if ( (*pp)->sym_value < value ) 
				continue;
			--pp;
			*offset = value - (*pp)->sym_value;
			return *pp;
		}
		return (struct sym *)NULL;
	}
	return *pp;
}

/* given a name, look up a value */
struct sym *
searchbynam(s)
	char *s;
{
	struct sym lsym, *p = &lsym, **pp;
	char buf[ MAXSTRLEN + 1 ];

	p->sym_name = s;
	pp = (struct sym **) bsearch((char *)&p, (char *)symtablepn, 
			glb_symbols, sizeof (struct sym *), cmpsymbynam);
	if (pp == NULL)  {	/* try '_'name */
		buf [ 0 ] = '_';
		strcpy(&buf[1], s);
		p->sym_name = buf;
		pp = (struct sym **) bsearch((char *)&p, (char *)symtablepn,
			   glb_symbols, sizeof (struct sym *), cmpsymbynam);
		if (pp == NULL)
			return (struct sym *)NULL;
	}
	return *pp;
}

/* given a name, return any sdb info */
static struct sdb *
searchsdb(s, type)
	char *s;
	unsigned char type;
{
	struct sdb lsdb, *p = &lsdb, **pp;

	p->sdb_name = s;
	if (type == N_GSYM) {
		pp = (struct sdb **) 
			bsearch((char *)&p, (char *)sdbgstablepn, sdb_gssymbols,
			    sizeof (struct sdb *), cmpsdbbynam);
	} else {
		pp = (struct sdb **) 
			bsearch((char *)&p, (char *)sdbsstablepn, sdb_sssymbols,
			    sizeof (struct sdb *), cmpsdbbynam);
	}
	if (pp == NULL)
		return (struct sdb *)NULL;
	if (*(pp-1))
		if (cmpsdbbynam(pp, pp-1) == 0) {
			printf("%s: ambiguous symbol\n", s);
			return (struct sdb *)NULL;
		}
	if (*(pp+1))
		if (cmpsdbbynam(pp, pp+1) == 0) {
			printf("%s: ambiguous symbol\n", s);
			return (struct sdb *)NULL;
		}
	return *pp;
}
	
/* Global interfaces */
char * 
lookbyval(addr, offsetp)
	unsigned addr, *offsetp;
{
	struct sym *p;

	if (addr < symtablepv[0]->sym_value || addr > symtablepv[glb_symbols-1]->sym_value) {
		return NULL;
	}
	p = searchbyval(addr, offsetp);
	if (p == NULL)
		return NULL;
	return p->sym_name;
}


char * 
lookbyptr(addr, offsetp)
	unsigned addr, *offsetp;
{
	register struct sym *p;
	struct sym *best_p;
	unsigned int    best_off;
	unsigned int    off;
	char	   *ptr;

	best_p = 0;
	best_off = -1;
	for (p = symtable; p < &symtable[glb_symbols]; p++) {
		if (p->sym_sdb && ((p->sym_sdb->sdb_desc &~PCCTM_BASETYPE) == PCCTM_PTR)) {
			if (readv(p->sym_value, &ptr, sizeof ptr) != -1) {
				off = addr - (unsigned)ptr;
				if (off < best_off) {
					best_off = off;
					best_p = p;
					if (best_off == 0)
						break;
				}
			}
		}
	}
	if (best_p == 0)
		return NULL;
	*offsetp = best_off;
	return (best_p->sym_name);
}


/*
 * locate the given symbol of type N_GSYM or N_SSYM and set value.
 * return FALSE if no symbol found.
 */
unsigned int
lookbysym(symbol, value, type)
	char *symbol;
	unsigned *value;
	unsigned char type;
{
	struct sym *p;
	struct sdb *pp;
	struct sdb *sdb;
	struct set_val *ppp;
	char	*s;
	
	if (symbol == 0 || *symbol == '\0')
		return FALSE;
	*value = 0;
	p = 0;
	pp = 0;
	sdb = 0;
	s = symbol;
	if (*symbol == '_')
		s++;
	bzero(&sdbinfo,  sizeof sdbinfo);

	ppp = searchsetval(symbol);
	if (ppp) {
		*value = ppp->sv_val.v_value;
		sdbinfo = ppp->sv_val.v_sdb;
		return TRUE;
	} else {
		if (type == N_GSYM) {
			p = searchbynam(symbol);
			*value = p->sym_value;
		} else {
			pp = searchsdb(symbol, type);
		}
	}
	if (p) {
		if ((sdb = p->sym_sdb)) {
			pp = sdb;
			/*
			 * non STAB entry - find one and fix it up.
			 */
			if (sdb->sdb_type != N_GSYM) {
				pp = searchsdb(s, N_GSYM);
				if (pp) {
					if (pp->sdb_soff == 0) {
						pp->sdb_soff = sdb->sdb_soff;
					}	
					if (sdb->sdb_desc == 0)
						sdb->sdb_desc = pp->sdb_desc;
					if (sdb->sdb_size == 4)
						sdb->sdb_size = pp->sdb_size;
					p->sym_sdb = pp;
				}
			} 
		} else {
			pp = searchsdb(s, N_GSYM);
			p->sym_sdb = pp;
		}
	}
	if (pp)
		sdbinfo = *pp;
	else if (sdb)
		sdbinfo = *sdb;
	if (p || pp)
		return TRUE;
	return FALSE;
}

/* return the first descripter in *x */
/* see compilers/pcc/common/manifest.h and /usr/include/pcc.h */
desc(x)
	register short *x;
{
        return(*x & PCCTM_TYPEMASK);
}

/*
 * routine for storing short string variables with little space wasted.
 * there is no way to free a string out of this, unless the entire
 * string pool block is freed.
 */

#define STRINGSIZE ((2048 - sizeof(struct Stringpool *)) / sizeof(char))

struct stringpool {
	char chars[STRINGSIZE];
	struct stringpool *prevpool;
};
typedef struct stringpool Stringpool;
static Stringpool *s_pool, *newpool;
static nextchar = STRINGSIZE + 1;		/* force first alloc */

char *stralloc(n)
	int n;
{
	if (nextchar + n > STRINGSIZE) {
		if (n > STRINGSIZE) {
			printf("stralloc: string too long!");
			printf("string size %d, max is %d\n", n, STRINGSIZE);
			exit(1);
		}
		newpool = (Stringpool *)malloc(sizeof(Stringpool));
		newpool->prevpool = s_pool;
		s_pool = newpool;
		nextchar = 0;
	}
	nextchar += n;
	return(&s_pool->chars[nextchar - n]);
}

char *
save_name(sname)
	char *sname;
{
	register char *p, *q;
	register int len = 0;
	char *ret;

	p = sname;
	while (*p != '\0') {
		len++;
		p++;
	}
	ret = stralloc(len + 1);
	p = sname;
	q = ret;
	while (*p) {
		*q++ = *p++;
	}
	*q = '\0';

	return(ret);
}

/*
 * check for duplicates of same name different values.
 */
check_dup(f)
	int f;
{
	struct sym **p1, **p2;
	struct sdb **p3, **p4;
	int count;
	int w = 0;

	if (glb_symbols < 2)
		return;
	p1 = &symtablepn[0];
	p2 = &symtablepn[1];

	while (p2 < &symtablepn[glb_symbols]) {
	    if (streq((*p1)->sym_name, (*p2)->sym_name)) {
		if ((*p1)->sym_value == (*p2)->sym_value) {
			p1 = p2;
			p2++;
			continue;
		}
		count = 2;
		if (f) {
		    printf("Dup global: '%s', addr = 0x%x,  0x%x\n",
			(*p1)->sym_name, (*p1)->sym_value, (*p2)->sym_value);
		} else {
		    if (! w) {
			printf("Warning: duplicate symbols: ");
			w = 29;
		    }
		    if (w + strlen((*p1)->sym_name) > 79) {
			printf("\n  ");
			w = 3;
		    }
		    printf("%s, ", (*p1)->sym_name);
		    w += strlen((*p1)->sym_name) + 2;
		}
		p2++;
		while (streq((*p1)->sym_name, (*p2)->sym_name)) {
		    count++;
		    p2++;
		    if (p2 >= &symtablepn[glb_symbols])
			break;
		}
		p1 = p2;
		p2++;
	    }
	    p1++;
	    p2++;
	}
	if (w) {
	    printf("\n");
	    w = 0;
	}


	p1 = &symtablepv[0];
	p2 = &symtablepv[1];

	while (p2 < &symtablepv[glb_symbols]) {
	    if ((*p1)->sym_value == (*p2)->sym_value) {
		if ((*p1)->sym_value == (*p2)->sym_value) {
			p1 = p2;
			p2++;
			continue;
		}
		count = 2;
		if (f) {
		    printf("Dup values addr = 0x%x, sym '%s',  '%s'\n",
			(*p1)->sym_value, (*p1)->sym_name, (*p2)->sym_name);
		} else {
		    if (! w) {
			printf("Warning: duplicate address values: ");
			w = 37;
		    }
		    if (w + strlen((*p1)->sym_name) > 79) {
			printf("\n  ");
			w = 3;
		    }
		    printf("%s, ", (*p1)->sym_name);
		    w += strlen((*p1)->sym_name) + 2;
		}
		p2++;
		while ((*p1)->sym_value == (*p2)->sym_value) {
		    count++;
		    p2++;
		    if (p2 >= &symtablepv[glb_symbols])
			break;
		}
		p1 = p2;
		p2++;
	    }
	    p1++;
	    p2++;
	}
	if (w) {
	    printf("\n");
	    w = 0;
	}

	if (sdb_sssymbols < 2)
		return;

	p3 = &sdbsstablepn[0];
	p4 = &sdbsstablepn[1];

	while (p4 < &sdbsstablepn[sdb_sssymbols]) {
	    /*
	     * ignore C_EXT globals set to "" for thier name.
	     */
	    if (*((*p4)->sdb_name) == '\0') {
		p3 = p4;
		p4++;
		continue;
	    }
	    /*
	     * ignore symbols that are exact duplicates.
	     */
	    if (streq((*p3)->sdb_name, (*p4)->sdb_name)) {
		if ((*p3)->sdb_type == (*p4)->sdb_type &&
		    (*p3)->sdb_desc == (*p4)->sdb_desc &&
		    (*p3)->sdb_soff == (*p4)->sdb_soff &&
		    (*p3)->sdb_size == (*p4)->sdb_size ) {
			p3 = p4;
			p4++;
			continue;
		}
		count = 2;
		if (f) {
		    printf("Dup element: '%s', offset = 0x%x,  0x%x\n",
			(*p3)->sdb_name, (*p3)->sdb_soff, (*p4)->sdb_soff);
		} else {
		    if (! w)  {
			printf("Warning: duplicate debug info: ");
			w = 33;
		    }
		    if (w + strlen((*p3)->sdb_name) > 79) {
			printf("\n  ");
			w = 3;
		    }
		    printf("%s, ", (*p3)->sdb_name);
		    w += strlen((*p3)->sdb_name) + 2;
		}
		p4++;
		while (streq((*p3)->sdb_name, (*p4)->sdb_name)) {
		    count++;
		    p4++;
		    if (p4 >= &sdbsstablepn[sdb_sssymbols])
			break;
		}
		p3 = p4;
		p4++;
	    }
	    p3++;
	    p4++;
	}
	if (w)
	    printf("\n");
}
	
/*
 * display an sdb structure.
 */

show_sdb(sdb)
	struct sdb *sdb;
{	
	if (sdb->sdb_name) {
		printf(" sdb_name= '%s'", sdb->sdb_name);
	} else
		printf(" sdb_name= ''");
	printf("\n\t");
	printf(" sdb_inx = %5d", sdb->sdb_inx);
	printf(" sdb_sym = 0x%8.8x", sdb->sdb_sym);
	printf(" sdb_type = 0x%2.2x", sdb->sdb_type);
	if (sdb->sdb_type == N_GSYM)
		printf(" Global");
	printf("\n\t");
	printf(" sdb_soff= %11u(0x%8.8x)", sdb->sdb_soff, sdb->sdb_soff);
	printf(" sdb_size= 0x%x", sdb->sdb_size);
	printf(" sdb_desc = 0x%4.4x", sdb->sdb_desc);
	printf(" ");
}


unsigned int
search_mos(sym, cast, sdb)
	char *sym;
	int cast;
	struct sdb *sdb;
{
	int val;

	err_search = 0;
	if (cast && sdb->sdb_sym) {
		/*
		 * This ought to be quick since we only have to
		 * search the currect structure chain.
		 */
		if ((val = find_field(sdb->sdb_sym, sym)) != -1) {
			sdbinfo.sdb_soff = val;
			return(0);
		}
	}

	/*
	 * Look in the sdb table otherwise.
	 */
	if (lookbysym(sym, &val, N_SSYM) == FALSE) {
		err_search = 1;
		if (debug[18])
			printf("search_mos('%s') failed\n", sym);
		return NULL;
	} 
	return val;
}

/*
 * return sdbinfo for arguments. - currently not supported.
 */
search_arg()
{
	return(0);
}

/*
 * print name of arguments. - currently not supported.
 */
printarg(index, col)
{
	return(0);
}

a_out(s)
	char *s;
{
	register n;
	struct exec x;
	struct stat sb;

	if ((n = open(s, 0)) == -1) {
		perror(s);
		exit(1);
	}
	if (read(n, &x, sizeof x) != sizeof x) {
		close(n);
		return (0);
	}
	close(n);
	if (N_BADMAG(x))
		return FALSE;
	/* catch /dev/kmem here */
	if (stat(s, &sb) < 0 || (sb.st_mode & S_IFMT) != S_IFREG)
		return FALSE;
	Etext = x.a_text;
	return TRUE;
}
