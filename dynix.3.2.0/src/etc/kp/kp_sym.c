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

#ifndef	lint
static	char	rcsid[] = "$Header: kp_sym.c 1.2 86/10/07 $";
#endif

/* $Log:	kp_sym.c,v $
 */

/*
 * kp_sym.c
 *
 * Stolen/modified from disas
 */
#include <stdio.h>
#include <sys/types.h>
#include <a.out.h>
#include <stab.h>
#include "kp_sym.h"

#define SANITY(c, m, f)  (!(c) || symerror(m, f))

#define MIN(a,b)	((a)>(b)?(b):(a))
#define TRUE		1
#define FALSE		0

/*
 * NUM_STE is set for 8K reads
 */
#define NUM_STE	 (8*BUFSIZ/sizeof (struct nlist))

extern char *whoiam;	/* ptr to argv[0], i.e., name of this prog */
extern char *file;	/* current file */

struct	sym *symtable, **symtablepv, **symtablepn;
static struct sym *symp;
static	int cmpsymbyval(), cmpsymbynam();
static	char *stringtable;

struct exec exec;
int total_symbols, glb_symbols;

extern char *malloc(), *realloc(), *strcpy();

getsymboltable(fd)
int fd;
{
	register ret;
	int strsz;

	ret = lseek(fd, (off_t)0L, 0);
	SANITY(ret == -1, "bad lseek on", file);

	ret = read(fd, (char *)&exec, sizeof (struct exec));
	SANITY(ret != sizeof(struct exec), "can't read a.out header for", file);

	SANITY(N_BADMAG(exec), file, ": bad magic number");
	if (exec.a_syms <= 0)
		return;
	total_symbols = exec.a_syms / sizeof (struct nlist);
	symtable = (struct sym *) malloc((unsigned)total_symbols * sizeof(struct sym));
	SANITY(symtable == NULL, "malloc of symbol table failed", (char *)NULL);
	symp = symtable;

	ret = lseek(fd, (off_t) N_STROFF(exec), 0);
	SANITY(ret == -1, "N_STROFF lseek failed on", file);

	ret = read(fd, (char *)&strsz, sizeof(strsz));
	SANITY(ret != sizeof(strsz), "can't read string table size for", file);

	stringtable = malloc((unsigned) strsz);
	SANITY(stringtable == NULL, "malloc of string table failed", (char *)NULL);

	ret = read(fd, stringtable+sizeof(strsz), strsz-sizeof(strsz));
	SANITY(ret != strsz-sizeof(strsz), "can't read string table for", file);

	process(fd);
}

process(fd)
int fd;
{
	extern char *rindex();
	struct nlist lnlist[ NUM_STE ];
	register struct nlist *s;
	register int n, ret;

	
	ret = lseek(fd, (off_t) N_SYMOFF(exec), 0);
	SANITY(ret == -1, "N_SYMOFF lseek failed on", file);

	for (n = total_symbols; n > 0; n -= ret) {
		ret = read(fd, (char *)lnlist, sizeof(struct nlist) * MIN(NUM_STE, n));
		SANITY(ret <= 0, "bad read of symbol table from", file);
		ret /= sizeof (struct nlist);
		for (s=lnlist; s < &lnlist[ret]; s++) {
			if ((s->n_type & N_STAB) == 0 &&
			    (s->n_type & N_TYPE) == N_TEXT) {
				s->n_un.n_strx += (unsigned long) stringtable;
				if (*(char *)s->n_un.n_strx != 'L')
					sym_add(s);
			}
		}
	}

	/* allocate memory for arrays of pointers */
	symtablepn = (struct sym **) malloc((unsigned) glb_symbols * sizeof(struct sym *));
	SANITY(symtablepn == NULL, "malloc failed", (char *)NULL);
	symtablepv = (struct sym **) malloc((unsigned) glb_symbols * sizeof(struct sym *));
	SANITY(symtablepv == NULL, "malloc failed", (char *)NULL);

	/* set all pointers before sorting them */
	for (n=0; n < glb_symbols; n++)
		symtablepn[n] = symtablepv[n] = &symtable[n];

	/* sort global symbols by name and value */
	qsort((char *)symtablepn, glb_symbols, sizeof(struct sym *), cmpsymbynam);
	qsort((char *)symtablepv, glb_symbols, sizeof(struct sym *), cmpsymbyval);
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
	symp->sym_name = (char *)s->n_un.n_strx;
	symp->sym_value = s->n_value;
	symp->sym_type = s->n_type;
	++glb_symbols;
	++symp;
}

symerror(msg, f)
	char *msg;
	char *f;
{
	fprintf(stderr, "%s: %s %s\n", whoiam, msg, (f != NULL) ? f : "");
	exit(1);
}
