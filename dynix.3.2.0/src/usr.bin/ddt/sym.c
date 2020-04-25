/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
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
static char rcsid[] = "$Header: sym.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <a.out.h>
#include "sym.h"

#define SANITY(c, m, p)  (!(c) || symerror(m, p))

#define MIN(a,b)	((a)>(b)?(b):(a))
#define TRUE		1
#define FALSE		0

/*
 * Tune these for speed
 * NUM_STE is set for 8K reads
 * PERCENT_USE is an estimate of the percentage of symbols that
 * will be kept
 * >>>>>>>>>>>> I seem to have broken the realloc scheme so default to keep
 *		all symbols!
 */
#define NUM_STE	 (8*BUFSIZ/sizeof (struct nlist))
#define PERCENT_USE	 1.0

static	struct	mod modtable;	/* one entry mod table */
struct	sym *symtable, **symtablepv, **symtablepn;
static	int cmpsymbyval(), cmpsymbynam();
static	char *stringtable;
static int symtblallocsize;

static	int dodots;		/* print "Reading a.out symbols.." ? */
static	int symcount;		/* count of syms read so far */

struct exec ddtheader;
int total_symbols, glb_symbols;

extern char *malloc(), *realloc(), *bsearch();
extern char imagename[];

getsymboltable(fd)
int fd;
{
	register ret;
	struct stat sbuf;
	int stringtblsize;

	ret = lseek(fd, 0L, 0);
	SANITY(ret == -1, "bad lseek", imagename);

	fstat(fd, &sbuf);

	ret = read(fd, &ddtheader, sizeof (struct exec));
	SANITY(ret != sizeof (struct exec), "can't read a.out header", imagename);

	SANITY(N_BADMAG(ddtheader), "bad magic number", imagename);
	if ( ddtheader.a_syms <= 0 )
		return;
	total_symbols = ddtheader.a_syms / sizeof (struct nlist);
	symtblallocsize = PERCENT_USE * total_symbols;

	stringtblsize = sbuf.st_size - N_STROFF(ddtheader);
	stringtable = malloc( (unsigned)stringtblsize );
	SANITY(stringtable == NULL, "malloc of string table failed", (char *)0);

	ret = lseek(fd, (long) N_STROFF(ddtheader), 0);
	SANITY(ret == -1, "N_STROFF lseek failed", imagename);

	ret = read(fd, stringtable, stringtblsize);
	SANITY(ret != stringtblsize, "can't read string table", imagename);

	if (isatty(0)) {
		dodots = 1;
		symcount = 0;
		printf("reading a.out symbols.");
		fflush(stdout);
	}

	process(fd);

	if (dodots)
		printf(" found %d\r\n", symcount);
}

process(fd)
int fd;
{
	struct nlist lnlist[ NUM_STE ];
	register struct nlist *s;
	register n, ret, flush;
	register char *p;
	extern char *rindex();

	
	ret = lseek(fd, (long) N_SYMOFF(ddtheader), 0);
	SANITY(ret == -1, "N_SYMOFF lseek failed", imagename);

	flush = 0;
	for (n = total_symbols; n > 0; n -= ret) {
		ret = read(fd, (char *)lnlist, sizeof(struct nlist) * MIN(NUM_STE, n));
		SANITY(ret <= 0, "bad read of symbol table", imagename);
		ret /= sizeof (struct nlist);
		for (s=lnlist; s < &lnlist[ret]; s++) {
			if ((s->n_type & N_STAB) == 0 && s->n_type != N_FN) {
				s->n_un.n_strx += (unsigned long) stringtable;
				sym_add(s);
			}
		}
	}

	/* allocate memory for for arrays of pointers */
	symtablepn = (struct sym **) malloc ( (unsigned) glb_symbols * sizeof (struct sym *) );
	SANITY(symtablepn == NULL, "symtablepn malloc failed", (char *)0);
	symtablepv = (struct sym **) malloc ( (unsigned) glb_symbols * sizeof (struct sym *) );
	SANITY(symtablepv == NULL, "symtablepv malloc failed", (char *)0);

	/* set all pointers before sorting them */
	for (n=0; n < glb_symbols; n++)
		symtablepn[n] = symtablepv[n] = &symtable[n];

	/* sort global symbols by name and value */
	qsort((char *)symtablepn, glb_symbols, sizeof (struct sym *), cmpsymbynam);
	qsort((char *)symtablepv, glb_symbols, sizeof (struct sym *), cmpsymbyval);

/*
	printf("%d total symbols (%d=global) %d%% useful\n", 
		total_symbols, glb_symbols,
		(glb_symbols*100)/total_symbols);
 */
}

static int
cmpsymbyval(a, b)
	struct sym **a, **b;
{

	return  (*a)->sym_value - (*b)->sym_value;
}

static int
cmpsymbynam(a, b)
	struct sym **a, **b;
{
	register char *s1, *s2;

	/* expanded strcmp() */
	s1 = (*a)->sym_name;
	s2 = (*b)->sym_name;
	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return 0;
	return (*s1 - *--s2);
}

sym_add(s)
	struct nlist *s;
{
	static struct sym *syme, *symp;

	if (symp == NULL || symp >= syme) {
		unsigned size = (unsigned)syme - (unsigned)symp + (symtblallocsize * sizeof (struct sym));
		if (symp == NULL)
			symtable = (struct sym *) malloc ( size );
		else
			symtable = (struct sym *) realloc( (char *) symtable, size );
		SANITY(symtable == NULL, "malloc for ste symbol table failed", (char *)0);
		size /= sizeof (struct sym);
		symp = &symtable[ size - symtblallocsize ];
		syme = &symtable[ size ];
	}
	symp->sym_name = (char *)s->n_un.n_strx;
	symp->sym_value = s->n_value;
	/* symp->sym_type = s->n_type; */
	++glb_symbols;
	++symp;

	/*
	 * put a '.' for every 50 symbols
	 */
	symcount++;
	if (dodots && (symcount % 50) == 0) {
		write(1, ".", 1);
	}
}

/*
 * Search routines
 */

/* given a value, looks up the name */
static struct sym *
searchbyval(value, offset)
	register unsigned value;
	unsigned *offset;
{
	register unsigned tval;
	register int low, middle, high;

	*offset = 0;
	if ( symtablepv == NULL )
		return((struct sym *) NULL);

	low = -1;
	high = glb_symbols;
	do {
		middle = low + (high - low) / 2;
		tval = symtablepv[middle]->sym_value;
		if ( value == tval ) {
			*offset = 0;
			return(symtablepv[middle]);
		} else if ( value < tval ) {
			high = middle;
		} else {	/* value > tval */
			low = middle;
		}
	} while ( high > low+1 );
	/*
	 * No exact match.  Try to determine best match
	 */
	tval = symtablepv[low]->sym_value;
	if ( value > tval ) {
		*offset = value - tval;
		return(symtablepv[low]);
	} else {
		return((struct sym *)NULL);
	}
}

/* given a name, look up a value */
struct sym *
searchbynam(s)
	char *s;
{
	struct sym lsym, *p = &lsym, **pp;
	char buf[ MAXSTRLEN + 1 ];

	if ( symtablepn == NULL )
		return((struct sym *) NULL);
	p->sym_name = s;
	pp = (struct sym **) bsearch((char *)&p, (char *)symtablepn, glb_symbols, sizeof (struct sym *), cmpsymbynam);
	if (pp == NULL)  {	/* try '_'name */
		buf [ 0 ] = '_';
		strcpy(&buf[1], s);
		p->sym_name = buf;
		pp = (struct sym **) bsearch((char *)&p, (char *)symtablepn, glb_symbols, sizeof (struct sym *), cmpsymbynam);
		if (pp == NULL)
			return (struct sym *)NULL;
	}
	return *pp;
}

/* Global interfaces */
char * 
lookbyval(addr, offsetp)
	unsigned addr, *offsetp;
{
	struct sym *p;

	if ( symtablepv == NULL ||
	     addr < symtablepv[0]->sym_value ||
	     addr > symtablepv[glb_symbols-1]->sym_value)
		return NULL;
	p = searchbyval(addr, offsetp);
	if (p == NULL)
		return NULL;
	return p->sym_name;
}

int
lookbysym(symbol, value)
	char *symbol;
	unsigned *value;
{
	struct sym *p;
	
	if ( symtablepn == NULL || symbol == 0 || *symbol == '\0')
		return FALSE;
	*value = 0;
	p = searchbynam(symbol);
	if ( p ) {
		 *value = p->sym_value;
		return TRUE;
	}
	return FALSE;
}

symerror(msg, name)
	char *msg;
	char *name;
{
	if (name != (char *)0)
		fprintf(stderr, "%s: ", name);
	fprintf(stderr, "%s\r\n", msg);
	ddtdied();
}
