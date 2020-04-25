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
static char *rcsid = "$Header: bp.c 1.3 87/04/27 $";
#endif

/*
 * bp.c
 *	Binary patch an a.out or other file.
 *
 *	Original author: Bob Beck
 *	Mods by Len Wyatt and Phil Hochstetler
 */

/* $Log:	bp.c,v $
 */

#include <stdio.h>
#include <a.out.h>
#include <sys/types.h>
#include <sys/file.h>

#define	USAGE \
"usage: %s [-qdh] [-s symfile] filename symbol[+offset] [new_value]\n"

int	quiet;
int	readonly;
int	numbase;

char	*index();
char	*malloc();
char	*pvalue();

/*
 * Print usage message.
 */
usage(prog)
	char	*prog;
{
	fprintf(stderr, USAGE, prog);
	exit(1);
}

main(argc, argv)
	int	argc;
	char	**argv;
{
	register char *argp;
	char	*prog = argv[0];
	char	*file;
	char	*symfile = NULL;
	char	*symbol;
	char	*plus;
	int	value = 0;
	int	offset = 0;

	while(--argc) {
		argp = *++argv;
		if (*argp == '-') while(*++argp) switch(*argp) {
		case 'q':
			++quiet;
			break;
		case 's':
			if (--argc <= 0)
				usage(prog);
			symfile = *++argv;
			break;
		case 'd':
			numbase = 10;
			break;
		case 'h':
			numbase = 16;
			break;
		default:
			usage(prog);
		} else
			break;
	}
	if (argc < 2 || argc > 3)
		usage(prog);

	file = argv[0];
	symbol = argv[1];
	if (plus = index(symbol, '+')) {
		offset = getnum(plus+1);
		*plus = '\0';
		if (plus == symbol)
			symbol = NULL;
	}
	if (argc == 3)
		value = getnum(argv[2]);
	else
		++readonly;
	exit(modfile(file, symfile, symbol, offset, value));
}

/*
 * modfile()
 *	Modify a file at given symbol offset.
 *
 * if symbol is NULL, use only offset.
 * If symfile non-null, use this for symbols.
 *
 * Returns appropriate exit status (0 ==> hunkey dorey).
 */

modfile(file, symfile, symbol, offset, value)
	char	*file;
	char	*symfile;
	char	*symbol;
{
	int	fd;
	int	symval = 0;
	int	start_off = 0;
	long	pos;
	int	oldval;
	struct	exec	hdr;

	/*
	 * Open file.
	 */
	fd = open(file, readonly ? O_RDONLY : O_RDWR);
	if (fd < 0) {
		perror(file);
		return(1);
	}

	/*
	 * If need a symbol, look it up.
	 * Use symfile if explicitly given, else the file itself.
	 */
	if (symbol != NULL) {
		if (symfile == NULL) {
			if (read(fd, (char *)&hdr, sizeof(hdr)) != sizeof(hdr)) {
				fprintf(stderr, "%s read error\n", file);
				(void) close(fd);
				return(1);
			}
			if (N_BADMAG(hdr)) {
				fprintf(stderr, "%s bad format\n", file);
				(void) close(fd);
				return(1);
			}
			start_off = N_TXTOFF(hdr) - N_ADDRADJ(hdr);
			symfile = file;
		}
		if (!getsymval(symfile, symbol, &symval)) {
			(void) close(fd);
			return(1);
		}
	}

	/*
	 * Seek in file to where we want to 
	 * patch, and write the new value.
	 */
	pos = start_off + symval + offset;
	if (pos < 0) {
		fprintf(stderr, "Offset out of range: 0x%x\n", pos);
		(void) close(fd);
		return(1);
	}
	(void) lseek(fd, (off_t)pos, L_SET);
	if (!quiet || readonly) {
		if (read(fd, (char *)&oldval, sizeof(oldval)) != sizeof(oldval)) {
			perror(file);
			(void) close(fd);
			return(1);
		}
		if (readonly)
			value = oldval;
		(void) lseek(fd, (off_t)pos, L_SET);
	}
	if (!readonly &&
	    write(fd, (char *)&value, sizeof(value)) != sizeof(value)) {
			perror(file);
			(void) close(fd);
			return(1);

	}
	if (!quiet) {
		printf("%s: ", file);
		if (symbol == NULL)
			printf("%s", pvalue(symval+offset));
		else if (offset == 0)
			printf("%s", symbol);
		else
			printf("%s+%s", symbol, pvalue(offset));
		if (!readonly)
			printf(" was %s", pvalue(oldval));
		printf(" is %s\n", pvalue(value));
	}
	(void) close(fd);
	return(0);
}

/*
 * pvalue()
 *	printf a number in numbase format
 *
 * Returns pointer to static area containing
 * the printed number
 */

char *
pvalue(n)
	int n;
{
	static char lbuf[256];
	
	switch (numbase) {
	case 10:
		(void) sprintf(lbuf, "%d", n);
		break;
	case 16:
	default:
		(void) sprintf(lbuf, "0x%x", n);
		break;
	}
	return (lbuf);
}

/*
 * getsymval()
 *	Given name of file containing symbol table and name of
 *	symbol, return it's value.
 *
 * Returns non-zero for sucess, else zero for failure.
 */

getsymval(symfile, sym_name, symvalp)
	char	*symfile;
	char	*sym_name;
	int	*symvalp;
{
	register struct	nlist	*sym;
	register char	*symstr;
	register char	*strings;
	register struct	nlist	*lastsym;
	struct	nlist	*symbols;
	int	fd;
	int	ssize;
	struct	exec	hdr;

	fd = open(symfile, O_RDONLY);
	if (fd < 0) {
		perror(symfile);
		return(0);
	}

	if (read(fd, (char *)&hdr, sizeof(hdr)) != sizeof(hdr)) {
		fprintf(stderr, "%s read error\n", symfile);
		(void) close(fd);
		return(0);
	}
	if (N_BADMAG(hdr)) {
		fprintf(stderr, "%s bad format\n", symfile);
		(void) close(fd);
		return(0);
	}

	(void) lseek(fd, (off_t)N_STROFF(hdr), 0);
	if (read(fd, (char *)&ssize, sizeof(ssize)) != sizeof(ssize)) {
		perror("read");
		(void) close(fd);
		return (0);
	}

	strings = malloc((unsigned)(ssize + sizeof(ssize)));
	if (strings == NULL) {
		fprintf(stderr, "out of memory.\n");
		(void) close(fd);
		return (0);
	}

	if (read(fd, (char *)(strings+sizeof(ssize)), ssize-sizeof(ssize)) != ssize-sizeof(ssize)) {
		fprintf(stderr, "%s: too small\n", symfile);
		(void) close(fd);
		return(0);
	}

	symbols = (struct nlist *) malloc((unsigned)hdr.a_syms);
	if (symbols == NULL) {
		fprintf(stderr, "out of memory.\n");
		(void) close(fd);
		return (0);
	}
	(void) lseek(fd, (off_t)N_SYMOFF(hdr), 0);
	if (read(fd, (char *)symbols, (int)hdr.a_syms) != (int)hdr.a_syms) {
		fprintf(stderr, "%s: too small\n", symfile);
		(void) close(fd);
		return(0);
	}
	(void) close(fd);
	lastsym = symbols + hdr.a_syms / sizeof(struct nlist);

	/*
	 * Read symbols, looking for the one we want.
	 */
	for (sym = symbols; sym < lastsym; sym++) {
		if (sym->n_type & N_STAB)	/* skip sdb symbols */
			continue;
		symstr = &strings[sym->n_un.n_strx];
		if (strcmp(sym_name, symstr) == 0 
		 || (*symstr == '_' && strcmp(sym_name, symstr+1) == 0)) {
			/*
			 * Found it!  Return value.
			 */
			*symvalp = sym->n_value;
			return(1);
		}
	}

	fprintf(stderr, "No %s in %s.\n", sym_name, symfile);
	return(0);
}

/*
 * getnum()
 *	Return an integer value converted from a string.
 *
 * Leading "0x" returns hex value, else decimal.
 */

getnum(s)
	register char *s;
{
	int	value;

	if (s[0] == '0' && s[1] == 'x') {
		if (numbase == 0)
			numbase = 16;
		(void) sscanf(&s[2], "%x", &value);
		return(value);
	}
	if (numbase == 0)
		numbase = 10;
	return(atoi(s));
}
