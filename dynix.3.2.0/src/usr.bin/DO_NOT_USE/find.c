/* $Copyright:	$
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
static char rcsid[] = "$Header: find.c 2.9 91/01/08 $";
#endif

/*	find	COMPILE:	cc -o find -s -O -i find.c -lS	*/

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>
#include <ufs/fs.h>
#include <mntent.h>

/* for new dev_t */
#define	devtoshort(a)	((short)(((a)>>8)&0xff00)|((a)&0xff))
#define	shorttodev(a)	((dev_t)(((a)<<8)&0xff0000)|((a)&0xff))

#define A_DAY	86400L /* a day full of seconds */
#define EQ(x, y)	(strcmp(x, y)==0)

int	Randlast;
char	Pathname[MAXPATHLEN+1];

struct anode {
	int (*F)();
	struct anode *L, *R;
} Node[100];
int Nn;			/* number of nodes */
char	*Fname;
char	Needfs;		/* don't compute Fsname unless this is true */
char	*Fsname;
long	Now;
int	Argc,
	Ai,
	Pi;
char	**Argv;
/* cpio stuff */
int	Cpio;
short	*Buf, *Dbuf, *Wp;
int	Bufsize = 5120;
int	Wct = 2560;

long	Newer;
int giveup = 0;		/* abort search in this directory */

struct stat Statb;

struct	anode	*exp(),
		*e1(),
		*e2(),
		*e3(),
		*mk();
char	*nxtarg();
char	Home[MAXPATHLEN];
long	Blocks;
DIR  *extend_fd_opendir();
char *rindex();
char *sbrk();
char *getmntpt();

main(argc, argv)
	char *argv[];
{
	struct anode *exlist;
	int paths;
	register char *cp, *sp = 0;

	time(&Now);
	if (0 == getwd(Home)) {
		fprintf(stderr, "find: %s\n", Home);
		exit(1);
	}
	Argc = argc; Argv = argv;
	if(argc<3) {
usage:		fprintf(stderr, "Usage: find path-list predicate-list\n");
		exit(1);
	}
	for(Ai = paths = 1; Ai < (argc-1); ++Ai, ++paths)
		if(*Argv[Ai] == '-' || EQ(Argv[Ai], "(") || EQ(Argv[Ai], "!"))
			break;
	if(paths == 1) /* no path-list */
		goto usage;
	if(!(exlist = exp())) { /* parse and compile the arguments */
		fprintf(stderr, "find: parsing error\n");
		exit(1);
	}
	if(Ai<argc) {
		fprintf(stderr, "find: missing conjunction\n");
		exit(1);
	}
	for(Pi = 1; Pi < paths; ++Pi) {
		sp = 0;
		chdir(Home);
		strcpy(Pathname, Argv[Pi]);
		if(cp = rindex(Pathname, '/')) {
			sp = cp + 1;
			*cp = '\0';
			if(chdir(*Pathname? Pathname: "/") == -1) {
				fprintf(stderr, "find: bad starting directory\n");
				exit(2);
			}
			*cp = '/';
		}
		Fname = sp? sp: Pathname;
		if (Needfs)
			Fsname = getmntpt(Fname);
		/* to find files that match  */
		descend(Pathname, Fname, Fsname, 0, exlist);
	}
	if(Cpio) {
		strcpy(Pathname, "TRAILER!!!");
		Statb.st_size = 0;
		cpio();
		printf("%D blocks\n", Blocks*10);
	}
	exit(0);
}

/* compile time functions:  priority is  exp()<e1()<e2()<e3()  */

struct anode *exp() { /* parse ALTERNATION (-o)  */
	int or();
	register struct anode * p1;

	p1 = e1() /* get left operand */ ;
	if(EQ(nxtarg(), "-o")) {
		Randlast--;
		return(mk(or, p1, exp()));
	}
	else if(Ai <= Argc) --Ai;
	return(p1);
}
struct anode *e1() { /* parse CONCATENATION (formerly -a) */
	int and();
	register struct anode * p1;
	register char *a;

	p1 = e2();
	a = nxtarg();
	if(EQ(a, "-a")) {
And:
		Randlast--;
		return(mk(and, p1, e1()));
	} else if(EQ(a, "(") || EQ(a, "!") || (*a=='-' && !EQ(a, "-o"))) {
		--Ai;
		goto And;
	} else if(Ai <= Argc) --Ai;
	return(p1);
}
struct anode *e2() { /* parse NOT (!) */
	int not();

	if(Randlast) {
		fprintf(stderr, "find: operand follows operand\n");
		exit(1);
	}
	Randlast++;
	if(EQ(nxtarg(), "!"))
		return(mk(not, e3(), (struct anode *)0));
	else if(Ai <= Argc) --Ai;
	return(e3());
}
struct anode *e3() { /* parse parens and predicates */
	int exeq(), ok(), glob(),  mtime(), atime(), user(),
		group(), size(), perm(), links(), print(),
		type(), fstype(), ino(), cpio(), newer(), prune();
	struct anode *p1;
	int i;
	struct passwd *pw;
	struct group *gp;
	register char *a, *b, s;

	a = nxtarg();
	if(EQ(a, "(")) {
		Randlast--;
		p1 = exp();
		a = nxtarg();
		if(!EQ(a, ")")) goto err;
		return(p1);
	}
	else if(EQ(a, "-print")) {
		return(mk(print, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-prune")) {
		return(mk(prune, (struct anode *)0, (struct anode *)0));
	}
	b = nxtarg();
	s = *b;
	if(s=='+') b++;
	if(EQ(a, "-name"))
		return(mk(glob, (struct anode *)b, (struct anode *)0));
	else if(EQ(a, "-mtime"))
		return(mk(mtime, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-atime"))
		return(mk(atime, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-user")) {
		if((pw=getpwnam(b)) == NULL) {
			if(gmatch(b, "[0-9]*"))
				return mk(user, (struct anode *)atoi(b), (struct anode *)s);
			fprintf(stderr, "find: cannot find -user name\n");
			exit(1);
		}
		i = pw->pw_uid;
		return(mk(user, (struct anode *)i, (struct anode *)s));
	}
	else if(EQ(a, "-inum"))
		return(mk(ino, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-group")) {
		if((gp=getgrnam(b)) == NULL) {
			if(gmatch(b, "[0-9]*"))
				return mk(group, (struct anode *)atoi(b), (struct anode *)s);
			fprintf(stderr, "find: cannot find -group name\n");
			exit(1);
		}
		i = gp->gr_gid;
		return(mk(group, (struct anode *)i, (struct anode *)s));
	} else if(EQ(a, "-size"))
		return(mk(size, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-links"))
		return(mk(links, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-perm")) {
		for(i=0; *b ; ++b) {
			if(*b=='-') continue;
			i <<= 3;
			i = i + (*b - '0');
		}
		return(mk(perm, (struct anode *)i, (struct anode *)s));
	}
	else if(EQ(a, "-type")) {
		i = s=='d' ? S_IFDIR :
		    s=='b' ? S_IFBLK :
		    s=='c' ? S_IFCHR :
		    s=='f' ? S_IFREG :
		    s=='l' ? S_IFLNK :
		    0;
		return(mk(type, (struct anode *)i, (struct anode *)0));
	}
	else if(EQ(a, "-fstype")) {
		Needfs = 1;
		return(mk(fstype, (struct anode *)b, (struct anode *)0));
	}
	else if (EQ(a, "-exec")) {
		i = Ai - 1;
		while(!EQ(nxtarg(), ";"));
		return(mk(exeq, (struct anode *)i, (struct anode *)0));
	}
	else if (EQ(a, "-ok")) {
		i = Ai - 1;
		while(!EQ(nxtarg(), ";"));
		return(mk(ok, (struct anode *)i, (struct anode *)0));
	}
	else if(EQ(a, "-cpio")) {
		if((Cpio = creat(b, 0666)) < 0) {
			fprintf(stderr, "find: cannot create < %s >\n", b);
			exit(1);
		}
		Buf = (short *)sbrk(512);
		Wp = Dbuf = (short *)sbrk(5120);
		return(mk(cpio, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-newer")) {
		if(stat(b, &Statb) < 0) {
			fprintf(stderr, "find: cannot access < %s >\n", b);
			exit(1);
		}
		Newer = Statb.st_mtime;
		return mk(newer, (struct anode *)0, (struct anode *)0);
	}
err:	fprintf(stderr, "find: bad option < %s >\n", a);
	exit(1);
}
struct anode *mk(f, l, r)
int (*f)();
struct anode *l, *r;
{
	Node[Nn].F = f;
	Node[Nn].L = l;
	Node[Nn].R = r;
	return(&(Node[Nn++]));
}

char *nxtarg() { /* get next arg from command line */
	static strikes = 0;

	if(strikes==3) {
		fprintf(stderr, "find: incomplete statement\n");
		exit(1);
	}
	if(Ai>=Argc) {
		strikes++;
		Ai = Argc + 1;
		return("");
	}
	return(Argv[Ai++]);
}

/* execution time functions */
and(p)
register struct anode *p;
{
	return(((*p->L->F)(p->L)) && ((*p->R->F)(p->R))?1:0);
}
or(p)
register struct anode *p;
{
	 return(((*p->L->F)(p->L)) || ((*p->R->F)(p->R))?1:0);
}
not(p)
register struct anode *p;
{
	return( !((*p->L->F)(p->L)));
}
glob(p)
register struct { int f; char *pat; } *p; 
{
	return(gmatch(Fname, p->pat));
}
print()
{
	puts(Pathname);
	return(1);
}
mtime(p)
register struct { int f, t, s; } *p; 
{
	return(scomp((int)((Now - Statb.st_mtime) / A_DAY), p->t, p->s));
}
atime(p)
register struct { int f, t, s; } *p; 
{
	return(scomp((int)((Now - Statb.st_atime) / A_DAY), p->t, p->s));
}
user(p)
register struct { int f, u, s; } *p; 
{
	return(scomp(Statb.st_uid, p->u, p->s));
}
ino(p)
register struct { int f, u, s; } *p;
{
	return(scomp((int)Statb.st_ino, p->u, p->s));
}
group(p)
register struct { int f, u; } *p; 
{
	return(p->u == Statb.st_gid);
}
links(p)
register struct { int f, link, s; } *p; 
{
	return(scomp(Statb.st_nlink, p->link, p->s));
}
size(p)
register struct { int f, sz, s; } *p; 
{
	return(scomp((int)((Statb.st_size+511)>>9), p->sz, p->s));
}
perm(p)
register struct { int f, per, s; } *p; 
{
	register i;
	i = (p->s=='-') ? p->per : 07777; /* '-' means only arg bits */
	return((Statb.st_mode & i & 07777) == p->per);
}
type(p)
register struct { int f, per, s; } *p;
{
	return((Statb.st_mode&S_IFMT)==p->per);
}
fstype(p)
register struct { int f; char *typename; } *p;
{
	return(!strcmp(Fsname, p->typename));
}
prune(p)
register struct { int f, per, s; } *p;
{
	giveup = 1;
	return(1);
}
exeq(p)
register struct { int f, com; } *p;
{
	fflush(stdout); /* to flush possible `-print' */
	return(doex(p->com));
}
ok(p)
struct { int f, com; } *p;
{
	char c;  int yes;
	yes = 0;
	fflush(stdout); /* to flush possible `-print' */
	fprintf(stderr, "< %s ... %s > ?   ", Argv[p->com], Pathname);
	fflush(stderr);
	if((c=getchar())=='y') yes = 1;
	while(c!='\n') c = getchar();
	if(yes) return(doex(p->com));
	return(0);
}


#define	LINKS	500

static
long
GetLinkId()
{
	register long i;
	static struct ml {
		dev_t	m_dev;
		ino_t	m_ino;
	} **ml = 0;
	register struct ml	*mlp;
	static long mlsize = 0;
	static long mlinks = 0;


	if( !ml ) {
		mlsize = LINKS;
		ml = (struct ml **) malloc(mlsize * sizeof(struct ml *));
	}
	else if( mlinks == mlsize ) {
		mlsize += LINKS;
		ml = (struct ml **) realloc((char *) ml,
		    mlsize * sizeof(struct ml *));
	}
	if (ml == NULL) {
		fprintf(stderr, "find: Out of memory for links");
		exit(2);
	}
	for(i = 0; i < mlinks; i++) {
		mlp = ml[i];
		if(mlp->m_ino == Statb.st_ino && mlp->m_dev == Statb.st_dev)
			return i;
	}
	if( !(ml[mlinks] = (struct ml *)malloc(sizeof(struct ml)))) {
		fprintf(stderr, "find: Out of memory for links");
		exit(2);
	}
	ml[mlinks]->m_dev = Statb.st_dev;
	ml[mlinks]->m_ino = Statb.st_ino;
	return (mlinks++);
}
#define MKSHORT(v, lv) {U.l=1L;if(U.c[0]) U.l=lv, v[0]=U.s[1], v[1]=U.s[0]; else U.l=lv, v[0]=U.s[0], v[1]=U.s[1];}
union { long l; short s[2]; char c[4]; } U;
long mklong(v)
short v[];
{
	U.l = 1;
	if(U.c[0] /* VAX */)
		U.s[0] = v[1], U.s[1] = v[0];
	else
		U.s[0] = v[0], U.s[1] = v[1];
	return U.l;
}
cpio()
{
#define MAGIC 070707
	struct header {
		short	h_magic,
			h_dev,
			h_ino,
			h_mode,
			h_uid,
			h_gid,
			h_nlink,
			h_rdev;
		short	h_mtime[2];
		short	h_namesize;
		short	h_filesize[2];
		char	h_name[256];
	} hdr;
	register ifile, ct;
	static long fsz;
	register i;
	long tlong;
	int fstype;

	hdr.h_magic = MAGIC;
	strcpy(hdr.h_name, !strncmp(Pathname, "./", 2)? Pathname+2: Pathname);
	hdr.h_namesize = strlen(hdr.h_name) + 1;
	hdr.h_uid = Statb.st_uid;
	hdr.h_gid = Statb.st_gid;
	fstype = Statb.st_mode & S_IFMT;
	if (fstype != S_IFDIR && Statb.st_nlink > 1) {
		tlong = GetLinkId();
		hdr.h_dev = (tlong & 0xffff);
		hdr.h_ino = ((tlong >> 16) & 0xffff);
	} else {
		hdr.h_dev = devtoshort(Statb.st_dev);
		hdr.h_ino = Statb.st_ino;
	}
	hdr.h_mode = Statb.st_mode;
	MKSHORT(hdr.h_mtime, Statb.st_mtime);
	hdr.h_nlink = Statb.st_nlink;
	fsz = (fstype == S_IFREG) ? Statb.st_size: 0L;
	MKSHORT(hdr.h_filesize, fsz);
	if ((fstype == S_IFBLK || fstype == S_IFCHR || fstype == S_IFIFO) 
	 && (Statb.st_rdev & 0xff00ff00)) {
		fprintf(stderr, "find: < %s > : long dev_t. ignoring.\n",
			hdr.h_name);
		return;
	}
	hdr.h_rdev = devtoshort(Statb.st_rdev);
	if(EQ(hdr.h_name, "TRAILER!!!")) {
		bwrite((short *)&hdr, (sizeof hdr-256)+hdr.h_namesize);
		for(i = 0; i < 10; ++i)
			bwrite(Buf, 512);
		return;
	}
	if(!mklong(hdr.h_filesize))
		return;
	if((ifile = open(Fname, 0)) < 0) {
cerror:
		fprintf(stderr, "find: cannot copy < %s >\n", hdr.h_name);
		return;
	}
	bwrite((short *)&hdr, (sizeof hdr-256)+hdr.h_namesize);
	for(fsz = mklong(hdr.h_filesize); fsz > 0; fsz -= 512) {
		ct = fsz>512? 512: fsz;
		if(read(ifile, (char *)Buf, ct) < 0)
			goto cerror;
		bwrite(Buf, ct);
	}
	close(ifile);
	return;
}
newer()
{
	return Statb.st_mtime > Newer;
}

/* support functions */
scomp(a, b, s) /* funny signed compare */
register a, b;
register char s;
{
	if(s == '+')
		return(a > b);
	if(s == '-')
		return(a < (b * -1));
	return(a == b);
}

doex(com)
{
	register np;
	register char *na;
	static char *nargv[50];
	static ccode;

	ccode = np = 0;
	while (na=Argv[com++]) {
		if(strcmp(na, ";")==0) break;
		if(strcmp(na, "{}")==0) nargv[np++] = Pathname;
		else nargv[np++] = na;
	}
	nargv[np] = 0;
	if (np==0) return(9);
	if(fork()) /*parent*/ {
#include <signal.h>
		int (*old)() = signal(SIGINT, SIG_IGN);
		int (*oldq)() = signal(SIGQUIT, SIG_IGN);
		wait(&ccode);
		signal(SIGINT, old);
		signal(SIGQUIT, oldq);
	} else { /*child*/
		chdir(Home);
		execvp(nargv[0], nargv, np);
		exit(1);
	}
	return(ccode ? 0:1);
}

descend(name, fname, pfsname, pfsnum, exlist)
	struct anode *exlist;
	dev_t pfsnum;		/* parent's dev */
	char *name, *fname;
	char *pfsname;		/* parent's fsname */
{
	DIR	*dir = NULL;
	register struct direct	*dp;
	register char *c1;
	int rv = 0;
	char *endofname;
	dev_t fsnum;		/* current dev */
	char *fsname;		/* current fsname */

	if (lstat(fname, &Statb)<0) {
		if (errno != ENOENT) {
			fprintf(stderr, "find: bad status < %s >\n", name);
		}
		return(0);
	}
	fsnum = Statb.st_dev;
	if (Needfs) {
		if (fsnum != pfsnum)
			Fsname = getmntpt(Fname);
		else
			Fsname = pfsname;
		fsname = Fsname;	/* squirrel this away on stack */
	}
	(*exlist->F)(exlist);
	if (giveup) {
		giveup = 0;
		return(1);
	}
	if((Statb.st_mode&S_IFMT)!=S_IFDIR)
		return(1);

	for (c1 = name; *c1; ++c1);
	if (*(c1-1) == '/')
		--c1;
	endofname = c1;

	if (chdir(fname) == -1)
		return(0);
	if ((dir = extend_fd_opendir(".")) == NULL) {
		fprintf(stderr, "find: cannot open < %s >\n", name);
		rv = 0;
		goto ret;
	}
	for (dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
		if ((dp->d_name[0]=='.' && dp->d_name[1]=='\0') ||
		    (dp->d_name[0]=='.' && dp->d_name[1]=='.' && dp->d_name[2]=='\0'))
			continue;
		c1 = endofname;
		*c1++ = '/';
		strcpy(c1, dp->d_name);
		Fname = endofname+1;
		if(!descend(name, Fname, fsname, fsnum, exlist)) {
			*endofname = '\0';
			chdir(Home);
			if(chdir(Pathname) == -1) {
				fprintf(stderr, "find: bad directory tree\n");
				exit(1);
			}
		}
	}
	rv = 1;
ret:
	if(dir)
		closedir(dir);
	if(chdir("..") == -1) {
		*endofname = '\0';
		fprintf(stderr, "find: bad directory <%s>\n", name);
		rv = 0;
	}
	return(rv);
}

gmatch(s, p) /* string match as in glob */
register char *s, *p;
{
	if (*s=='.' && *p!='.') return(0);
	return amatch(s, p);
}

amatch(s, p)
register char *s, *p;
{
	register cc;
	int scc, k;
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
				/*
				 * observation:
				 * k is being used as a boolean value.
				 * the k++ may as be an = 1;
				 * the expression below could use logical
				 * rather than bitwise operations.
				 */
				k |= ((lc <= scc) & (scc <= (cc=(p[1]))));
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

umatch(s, p)
register char *s, *p;
{
	if(*p==0) return(1);
	while(*s)
		if (amatch(s++, p)) return(1);
	return(0);
}

bwrite(rp, c)
register short *rp;
register c;
{
	register short *wp = Wp;

	c = (c+1) >> 1;
	while(c--) {
		if(!Wct) {
again:
			if(write(Cpio, (char *)Dbuf, Bufsize)<0) {
				Cpio = chgreel(1, Cpio);
				goto again;
			}
			Wct = Bufsize >> 1;
			wp = Dbuf;
			++Blocks;
		}
		*wp++ = *rp++;
		--Wct;
	}
	Wp = wp;
}
chgreel(x, fl)
{
	register f;
	char str[22];
	FILE *devtty;
	struct stat statb;
	extern errno;

	switch (errno) {
	case 0:
	case ENOSPC:
		fprintf(stderr,"End of reel\n");
		break;
	default:
		fprintf(stderr,"errno: %d, ", errno);
		fprintf(stderr,"Can't %s\n", x? "write output": "read input");
		break;
	}

	fstat(fl, &statb);
	if((statb.st_mode&S_IFMT) != S_IFCHR)
		exit(1);
	close(fl);
again:
	fprintf(stderr, "If you want to go on, type device/file name %s\n",
		"when ready");
	devtty = fopen("/dev/tty", "r");
	fgets(str, 20, devtty);
	str[strlen(str) - 1] = '\0';
	if(!*str)
		exit(1);
	if((f = open(str, x? 1: 0)) < 0) {
		fprintf(stderr, "That didn't work");
		fclose(devtty);
		goto again;
	}
	return f;
}

/*
 * Given a name like /usr/src/etc/foo.c returns the mntent
 * structure for the file system it lives in.
 */
char *
getmntpt(file)
	char *file;
{
	FILE *mntp;
	struct mntent *mnt;
	struct stat filestat, dirstat;
	char *mnttype;

	if (stat(file, &filestat) < 0) {
		perror(file);
		return(NULL);
	}

	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		perror(MOUNTED);
		exit(1);
	}

	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
		    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
			continue;
		if (strcmp(mnt->mnt_fsname, file) == 0) {
			endmntent(mntp);
			return(mnt->mnt_type);
		}
		if (stat(mnt->mnt_dir, &dirstat) < 0) {
			perror(mnt->mnt_dir);
			endmntent(mntp);
			return(NULL);
		}
		if (filestat.st_dev == dirstat.st_dev) {
			endmntent(mntp);
			mnttype = (char *)malloc(strlen(mnt->mnt_type+1));
			strcpy(mnttype, mnt->mnt_type);
			return(mnttype);
		}
	}
	fprintf(stderr, "Couldn't find mount point for %s\n", file);
	exit(1);
	/*NOTREACHED*/
}
