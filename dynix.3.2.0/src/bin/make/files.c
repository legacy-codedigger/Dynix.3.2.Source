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
static char rcsid[] = "$Header: files.c 2.3 86/06/16 $";
#endif
#include <fcntl.h>

/* UNIX DEPENDENT PROCEDURES */


/* DEFAULT RULES FOR UNIX */

char *builtin[] =
	{
#ifdef pwb
	".SUFFIXES : .L .out .o .c .f .e .r .y .yr .ye .l .s .z .x .t .h .cl",
#else
#if defined(sequent)
	".SUFFIXES : .out .o .c .f .for .e .r .y .yr .ye .l .s .cl .p .pas",
#else
	".SUFFIXES : .out .o .c .F .f .e .r .y .yr .ye .l .s .cl .p",
#endif
#endif
	"YACC=yacc",
	"YACCR=yacc -r",
	"YACCE=yacc -e",
	"YFLAGS=",
	"LEX=lex",
	"LFLAGS=",
	"CC=cc",
#if defined(vax) || defined(sun) || defined(sequent)
	"AS=as",
#else
	"AS=as -",
#endif
#if defined(sequent)
	"PC=pascal",
#else
	"PC=pc",
#endif
	"PFLAGS=",
	"CFLAGS=",
	"RC=f77",
	"RFLAGS=",
#if defined(sequent)
	"FC=fortran",
#else
	"FC=f77",
#endif
	"EFLAGS=",
	"FFLAGS=",
	"LOADLIBES=",
#ifdef pwb
	"SCOMP=scomp",
	"SCFLAGS=",
	"CMDICT=cmdict",
	"CMFLAGS=",
#endif

	".c.o :",
	"\t$(CC) $(CFLAGS) -c $<",

#if defined(sequent)
	".p.o .pas.o :",
#else
	".p.o :",
#endif
	"\t$(PC) $(PFLAGS) -c $<",

	".cl.o :",
	"\tclass -c $<",

#if defined(sequent)
	".e.o .r.o .f.o .for.o :",
#else
	".e.o .r.o .F.o .f.o :",
#endif
	"\t$(FC) $(RFLAGS) $(EFLAGS) $(FFLAGS) -c $<",

	".s.o :",
	"\t$(AS) -o $@ $<",

	".y.o :",
	"\t$(YACC) $(YFLAGS) $<;$(CC) $(CFLAGS) -c y.tab.c;rm y.tab.c;mv y.tab.o $@",

	".yr.o:",
	"\t$(YACCR) $(YFLAGS) $<;$(RC) $(RFLAGS) -c y.tab.r;rm y.tab.r;mv y.tab.o $@",

	".ye.o :",
	"\t$(YACCE) $(YFLAGS) $<;$(EC) $(RFLAGS) -c y.tab.e;rm y.tab.e;mv y.tab.o $@",

	".l.o :",
	"\t$(LEX) $(LFLAGS) $<;$(CC) $(CFLAGS) -c lex.yy.c;rm lex.yy.c;mv lex.yy.o $@",

	".y.c :",
	"\t$(YACC) $(YFLAGS) $<;mv y.tab.c $@",

	".l.c :",
	"\t$(LEX) $(LFLAGS) $<;mv lex.yy.c $@",

	".yr.r:",
	"\t$(YACCR) $(YFLAGS) $<;mv y.tab.r $@",

	".ye.e :",
	"\t$(YACCE) $(YFLAGS) $<;mv y.tab.e $@",

#ifdef pwb
	".o.L .c.L .t.L:",
	"\t$(SCOMP) $(SCFLAGS) $<",

	".t.o:",
	"\t$(SCOMP) $(SCFLAGS) -c $<",

	".t.c:",
	"\t$(SCOMP) $(SCFLAGS) -t $<",

	".h.z .t.z:",
	"\t$(CMDICT) $(CMFLAGS) $<",

	".h.x .t.x:",
	"\t$(CMDICT) $(CMFLAGS) -c $<",
#endif

	".s.out .c.out .o.out :",
	"\t$(CC) $(CFLAGS) $< $(LOADLIBES) -o $@",

	".f.out .F.out .r.out .e.out :",
	"\t$(FC) $(EFLAGS) $(RFLAGS) $(FFLAGS) $< $(LOADLIBES) -o $@;-rm $*.o",

	".y.out :",
	"\t$(YACC) $(YFLAGS) $<;$(CC) $(CFLAGS) y.tab.c $(LOADLIBES) -ly -o $@;rm y.tab.c",

	".l.out :",
	"\t$(LEX) $(LFLAGS) $<;$(CC) $(CFLAGS) lex.yy.c $(LOADLIBES) -ll -o $@;rm lex.yy.c",

	0 };

#include "defs"
#ifdef CCS
#include "/usr/include/sys/stat.h"
#else
#include <sys/stat.h>
#endif


TIMETYPE 
exists(pname)
struct nameblock *pname;
{
struct stat buf;
register char *s, *filename;
TIMETYPE lookarch();
extern char *findfl();

filename = pname->namep;

for(s = filename ; *s!='\0' && *s!='(' ; ++s)
	;

if(*s == '(')
	return(lookarch(filename));

if (stat(filename, &buf) < 0)
{
	s = findfl(filename);
	if(s != (char *)-1)
	{
		pname->alias = copys(s);
		if(stat(pname->alias, &buf) == 0)
			return(buf.st_mtime);
	}
	return(0);
}
else	return(buf.st_mtime);
}


TIMETYPE prestime()
{
TIMETYPE t;
time(&t);
return(t);
}



FSTATIC char nbuf[MAXNAMLEN + 1];
FSTATIC char *nbufend	= &nbuf[MAXNAMLEN];



struct depblock *srchdir(pat, mkchain, nextdbl)
register char *pat; /* pattern to be matched in directory */
int mkchain;  /* nonzero if results to be remembered */
struct depblock *nextdbl;  /* final value for chain */
{
DIR *dirf;
register int i;
int nread, cldir;
char *dirname, *dirpref, *endir, *filepat, *p, temp[BUFSIZ];
char fullname[BUFSIZ], *p1, *p2;
struct nameblock *q;
struct depblock *thisdbl;
struct dirhdr *od;
struct pattern *patp;
struct varblock *cp, *varptr();
char *path, pth[BUFSIZ], *strcpy();
struct direct *dptr;


thisdbl = 0;

if(mkchain == NO)
	for(patp=firstpat ; patp ; patp = patp->nxtpattern)
		if(! unequal(pat, patp->patval)) return(0);

patp = ALLOC(pattern);
patp->nxtpattern = firstpat;
firstpat = patp;
patp->patval = copys(pat);

endir = 0;

for(p=pat; *p!='\0'; ++p)
	if(*p=='/') endir = p;

if(endir==0)
	{
	dirpref = "";
	filepat = pat;
	cp = varptr("VPATH");
	if (cp->varval == 0 || cp->varval[0] == '\0') path = ".";
	else {
	       path = pth; 
	       strcpy(pth,".:");
	       strcat(pth, cp->varval);
	       }
	}
else	{
	*endir = '\0';
	path = strcpy(pth, pat);
	dirpref = concat(pat, "/", temp);
	filepat = endir+1;
	}

while (*path) {			/* Loop thru each VPATH directory */
  dirname = path;
  for (; *path; path++)
    if (*path == ':') {
      *path++ = '\0';
      break;
      }

dirf = NULL;
cldir = NO;

for(od = firstod; od; od = od->nxtopendir)
	if(! unequal(dirname, od->dirn) )
		{
		dirf = od->dirfc;
		if (dirf != NULL)
			rewinddir(dirf); /* start over at the beginning  */
		break;
		}

if(dirf == NULL)
	{
	dirf = opendir(dirname);
	if(nopdir >= MAXDIR)
		cldir = YES;
	else	{
		++nopdir;
		od = ALLOC(dirhdr);
		od->nxtopendir = firstod;
		firstod = od;
		od->dirfc = dirf;
		od->dirn = copys(dirname);
		fcntl(dirf->dd_fd, F_SETFD, 1);
		}
	}

if(dirf == NULL)
	{
	fprintf(stderr, "Directory %s: ", dirname);
	fatal("Cannot open");
	}

else for (dptr = readdir(dirf); dptr != NULL; dptr = readdir(dirf))
	{
	p1 = dptr->d_name;
	p2 = nbuf;
	while( (p2<nbufend) && (*p2++ = *p1++)!='\0' )
		/* void */;
	if( amatch(nbuf,filepat) )
		{
		concat(dirpref,nbuf,fullname);
		if( (q=srchname(fullname)) ==0)
			q = makename(copys(fullname));
		if(mkchain)
			{
			thisdbl = ALLOC(depblock);
			thisdbl->nxtdepblock = nextdbl;
			thisdbl->depname = q;
			nextdbl = thisdbl;
			}
		}
	}

if(endir != 0)  *endir = '/';

if(cldir) {
	closedir(dirf);
	dirf = NULL;
}
} /* End of VPATH loop */
return(thisdbl);
}

/* stolen from glob through find */

static amatch(s, p)
char *s, *p;
{
	register int cc, scc, k;
	int c, lc;

	scc = *s;
	lc = 077777;
	switch (c = *p) {

	case '[':
		k = 0;
		while (cc = *++p) {
			switch (cc) {

			case ']':
				if (k)
					return(amatch(++s, ++p));
				else
					return(0);

			case '-':
				k |= (lc <= scc)  & (scc <= (cc=p[1]) ) ;
			}
			if (scc==(lc=cc)) k++;
		}
		return(0);

	case '?':
	caseq:
		if(scc) return(amatch(++s, ++p));
		return(0);
	case '*':
		return(umatch(s, ++p));
	case 0:
		return(!scc);
	}
	if (c==scc) goto caseq;
	return(0);
}

static umatch(s, p)
char *s, *p;
{
	if(*p==0) return(1);
	while(*s)
		if (amatch(s++,p)) return(1);
	return(0);
}

#ifdef METERFILE
#include <pwd.h>
int meteron	= 0;	/* default: metering off */

meter(file)
char *file;
{
TIMETYPE tvec;
char *p, *ctime();
FILE * mout;
struct passwd *pwd, *getpwuid();

if(file==0 || meteron==0) return;

pwd = getpwuid(getuid());

time(&tvec);

if( (mout=fopen(file,"a")) != NULL )
	{
	p = ctime(&tvec);
	p[16] = '\0';
	fprintf(mout,"User %s, %s\n",pwd->pw_name,p+4);
	fclose(mout);
	}
}
#endif


/* look inside archives for notations a(b) and a((b))
	a(b)	is file member   b   in archive a
	a((b))	is entry point  _b  in object archive a
*/

#ifdef ASCARCH
#	include <ar.h>
#else
#	include <ar.h>
#endif
#include <a.out.h>

static char *strtab = (char *)0;	/* pointer to a.out string table */
static long arflen;
static long arfdate;
static char arfname[80];		/* should depend on length of arname*/
FILE *arfd;
long int arpos, arlen;

static struct exec objhead;

static struct nlist objentry;


TIMETYPE lookarch(filename)
char *filename;
{
char *p, *q, *send, psave, *s;
int i, nsym, objarch;

for(p = filename; *p!= '(' ; ++p)
	;
*p = '\0';
openarch(filename);
*p++ = '(';

if(*p == '(')
	{
	objarch = YES;
	++p;
	}
else
	{
	objarch = NO;
	}

/*
 * point s at null terminated name of file or entry point
 */
s = p;
while( *p != ')' && *p != '\0' )
	p++;
psave = *p;
*p = '\0';

/*
 * try and find s
 */
while(getarch())
	{
	if(objarch)
		{
		if (!getobj())
			continue;
		nsym = objhead.a_syms / sizeof(objentry);
		for(i = 0; i<nsym ; ++i)
			{
			fread( (char *) &objentry, sizeof(objentry),1,arfd);
			if( (objentry.n_type & N_EXT)
			   && ((objentry.n_type & ~N_EXT) || objentry.n_value)
			   && !strcmp(&strtab[objentry.n_un.n_strx],s))
				{
				*p = psave;
				clarch();
				return(arfdate);
				}
			}
		}

	else if( !strcmp(arfname, s) )
		{
		*p = psave;
		clarch();
		return(arfdate);
		}
	}

*p = psave;
clarch();
return( 0L);
}


clarch()
{
fclose( arfd );
}


openarch(f)
register char *f;
{
#ifdef ASCARCH
char magic[SARMAG];
#endif
int word;
#ifdef CCS
#include "/usr/include/sys/stat.h"
#else
#include <sys/stat.h>
#endif
struct stat buf;

stat(f, &buf);
arlen = buf.st_size;

arfd = fopen(f, "r");
if(arfd == NULL)
	fatal1("cannot open %s", f);

	fread( (char *) &word, sizeof(word), 1, arfd);
#ifdef ASCARCH
	fseek(arfd, 0L, 0);
	fread(magic, SARMAG, 1, arfd);
	arpos = SARMAG;
	if( ! eqstr(magic, ARMAG, SARMAG) )
#else
	arpos = sizeof(word);
	if(word != ARMAG)
#endif
		fatal1("%s is not an archive", f);

arflen = 0;
}



getarch()
{
	struct ar_hdr arhead;
	register char *ap, *np;
	long atol();

arpos += (arflen + 1) & ~1L;	/* round archived file length up to even */
if(arpos >= arlen)
	return(0);
fseek(arfd, arpos, 0);

	fread( (char *) &arhead, sizeof(arhead), 1, arfd);
	arpos += sizeof(arhead);
#ifdef ASCARCH
	arflen = atol(arhead.ar_size);
	arfdate = atol(arhead.ar_date);
#else
	arflen = arhead.ar_size;
	arfdate = arhead.ar_date;
#endif
	ap = arfname;
	np = arhead.ar_name;
	while(  *np != ' ' && np < &arhead.ar_name[sizeof(arhead.ar_name)])
		*(ap++) = *(np++);
	*ap = '\0';
return(1);
}


getobj()
{
	long int skip;

	fread( (char *) &objhead, sizeof(objhead), 1, arfd);
	if (N_BADMAG(objhead))
		return(0);
	/*fatal1("%s is not an object module", arfname);*/
	skip = objhead.a_text + objhead.a_data;
#if defined(vax) || defined(sun) || defined(sequent)
	skip += objhead.a_trsize + objhead.a_drsize;
#else
	if(! objhead.a_flag )
		skip *= 2;
#endif
	get_strtab(skip);		/* read in string table */
	fseek(arfd, skip, 1);
	return(1);
}

/*
 * read in string table, located 'where' bytes from current
 * file position.
 */
get_strtab( where )
	int where;
{
	long fsave;
	long strsiz;

	/*
	 * remember where we are 
	 */
	fsave = ftell(arfd);

	/*
	 * allocate memory for string table
	 */
	if (strtab != (char *)0)
		free(strtab);

	fseek(arfd, where+objhead.a_syms, 1);	
	fread(&strsiz, sizeof(strsiz), 1, arfd);
	strtab = (char *)malloc(strsiz);
	if (strtab == (char *)0) 
		fatal1("can't malloc %d bytes for string table in get_strtab\n",
			strsiz);
	/* 
	 * suck in the string table
	 */
	fread(strtab+sizeof(strsiz), strsiz-sizeof(strsiz), 1, arfd);

	/*
	 * back to previous position
	 */
	fseek(arfd, fsave, 0);
}


eqstr(a,b,n)
register char *a, *b;
int n;
{
register int i;
for(i = 0 ; i < n ; ++i)
	if(*a++ != *b++)
		return(NO);
return(YES);
}


/*
 *	findfl(name)	(like execvp, but does path search and finds files)
 */
static char fname[128];

char *execat();

char *findfl(name)
register char *name;
{
	register char *p;
	register struct varblock *cp;
	struct stat buf;

	for (p = name; *p; p++) 
		if(*p == '/') return(name);

	cp = varptr("VPATH");
	if(cp->varval == NULL || *cp->varval == 0)
		p = ":";
	else
		p = cp->varval;

	do
	{
		p = execat(p, name, fname);
		if(stat(fname,&buf) >= 0)
			return(fname);
	} while (p);
	return((char *)-1);
}

char *execat(s1, s2, si)
register char *s1, *s2;
char *si;
{
	register char *s;

	s = si;
	while (*s1 && *s1 != ':' && *s1 != '-')
		*s++ = *s1++;
	if (si != s)
		*s++ = '/';
	while (*s2)
		*s++ = *s2++;
	*s = '\0';
	return(*s1? ++s1: 0);
}


/* copy s to d, changing file names to file aliases */
fixname(s, d)
char *s, *d;
{
	register char *r, *q;
	struct nameblock *pn;
	char name[BUFSIZ];

	while (*s) {
		if (isspace(*s)) *d++ = *s++;
		else {
			r = name;
			while (*s) {
				if (isspace(*s)) break; 
				*r++ = *s++;
				}
			*r = '\0';
 		
			if (((pn = srchname(name)) != 0) && (pn->alias))
				q = pn->alias;
			else q = name;
	
			while (*q) *d++ = *q++;
			}
		}
	*d = '\0';
}
