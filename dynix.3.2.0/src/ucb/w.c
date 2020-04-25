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
static char rcsid[] = "$Header: w.c 2.12 90/03/08 $";
#endif

/*
 * w - print system status (who and what)
 *
 * This program is similar to the systat command on Tenex/Tops 10/20
 * It needs read permission on /dev/mem, /dev/kmem, and /dev/drum.
 * TODO:  speedup by use of procMAX
 */
#include <sys/param.h>
#include <nlist.h>
#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/vm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/pte.h>

#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)

#define ARGWIDTH	33	/* # chars left on 80 col crt for args */

struct pr {
	short	w_pid;			/* proc.p_pid */
	char	w_flag;			/* proc.p_flag */
	short	w_size;			/* proc.p_size */
	short	w_uid;			/* proc.p_uid */
	long	w_seekaddr;		/* where to find args */
	long	w_lastpg;		/* disk address of stack */
	int	w_igintr;		/* INTR+3*QUIT, 0=die, 1=ign, 2=catch */
	time_t	w_time;			/* CPU time used by this process */
	time_t	w_stime;		/* start time of process */
	time_t	w_ctime;		/* CPU time used by children */
	dev_t	w_tty;			/* tty device of process */
	char	w_comm[15];		/* user.u_comm, null terminated */
	char	w_args[ARGWIDTH+1];	/* args if interesting process */
} *pr;
int	nproc;

struct	nlist nl[] = {
	{ "_proc" },
#define	X_PROC		0
	{ "_nswap" },
#define	X_NSWAP		1
	{ "_avenrun" },
#define	X_AVENRUN	2
	{ "_boottime" },
#define	X_BOOTTIME	3
	{ "_nproc" },
#define	X_NPROC		4
	{ "_dmmin" },
#define	X_DMMIN		5
	{ "_dmmax" },
#define	X_DMMAX		6
	{ "" },
};

FILE	*ps;
FILE	*ut;
FILE	*bootfd;
int	kmem;
int	mem;
int	swap;			/* /dev/kmem, mem, and swap */
int	nswap;
int	dmmin, dmmax;
int	uid;
dev_t	tty;
char	doing[520];		/* process attached to terminal */
time_t	proctime;		/* cpu time of process in doing */
long	avenrun[3];
struct	proc *aproc;

#define	DIV60(t)	((t+30)/60)    /* x/60 rounded */ 
#define IGINT		(1+3*1)		/* ignoring both SIGINT & SIGQUIT */

char	*getargs();
char	*fread();
char	*ctime();
char	*rindex();
char	*savestr();
char	*valloc();
FILE	*popen();
struct	tm *localtime();

int	debug;			/* true if -d flag: debugging output */
int	header = 1;		/* true if -h flag: don't print heading */
int	lflag = 1;		/* true if -l flag: long style output */
int	login;			/* true if invoked as login shell */
int	idle;			/* number of minutes user is idle */
int	nusers;			/* number of users logged in now */
char *	sel_user;		/* login of particular user selected */
char firstchar;			/* first char of name of prog invoked as */
time_t	jobtime;		/* total cpu time visible */
time_t	now;			/* the current time of day */
struct	tm *nowt;		/* current time as time struct */
struct	timeval boottime;
time_t	uptime;			/* time of last reboot & elapsed time since */
int	np;			/* number of processes currently active */
struct	utmp utmp;
struct	proc mproc;
struct	user *up;
struct dmapext *dmep;
int sizeof_dmapext;
long sizeof_dmapext_disk;

main(argc, argv)
	char **argv;
{
	int days, hrs, mins;
	register int i, j;
	char *cp;
	register int curtime, empty;
	char obuf[BUFSIZ];

	setbuf(stdout, obuf);
	up = (struct user *)valloc(NBPG*UPAGES);
	if (up == NULL) {
		fprintf(stderr, "ps: Cannot valloc up.\n");
		exit(1);
	}
	login = (argv[0][0] == '-');
	cp = rindex(argv[0], '/');
	firstchar = login ? argv[0][1] : (cp==0) ? argv[0][0] : cp[1];
	cp = argv[0];	/* for Usage */

	while (argc > 1) {
		if (argv[1][0] == '-') {
			for (i=1; argv[1][i]; i++) {
				switch(argv[1][i]) {

				case 'd':
					debug++;
					break;

				case 'h':
					header = 0;
					break;

				case 'l':
					lflag++;
					break;

				case 's':
					lflag = 0;
					break;

				case 'u':
				case 'w':
					firstchar = argv[1][i];
					break;

				default:
					if (firstchar != 'u') {
						fprintf(stderr, "Usage: w [ -dhlsuw ] [ user ]\n");
					} else {
						fprintf(stderr,"Usage: uptime [-w]\n");
					}
					exit(1);
				}
			}
		} else {
			if (!isalnum(argv[1][0]) || argc > 2) {
				fprintf(stderr, "Usage: %s [ -hlsuw ] [ user ]\n", cp);
				exit(1);
			} else
				sel_user = argv[1];
		}
		argc--; argv++;
	}

	if ((kmem = open("/dev/kmem", 0)) < 0) {
		fprintf(stderr, "No kmem\n");
		exit(1);
	}
	nlist("/dynix", nl);
	if (nl[0].n_type==0) {
		fprintf(stderr, "No namelist\n");
		exit(1);
	}

	if (firstchar != 'u')
		readpr();

	ut = fopen("/etc/utmp","r");
	if (header) {
		/* Print time of day */
		time(&now);
		nowt = localtime(&now);
		prtat(nowt);

		/*
		 * Print how long system has been up.
		 * (Found by looking for "boottime" in kernel)
		 */
		lseek(kmem, (long)nl[X_BOOTTIME].n_value, 0);
		read(kmem, &boottime, sizeof (boottime));

		uptime = now - boottime.tv_sec;
		uptime += 30;
		days = uptime / (60*60*24);
		uptime %= (60*60*24);
		hrs = uptime / (60*60);
		uptime %= (60*60);
		mins = uptime / 60;

		printf("  up");
		if (days > 0)
			printf(" %d day%s,", days, days>1?"s":"");
		if (hrs > 0 && mins > 0) {
			printf(" %2d:%02d,", hrs, mins);
		} else {
			if (hrs > 0)
				printf(" %d hr%s,", hrs, hrs>1?"s":"");
			if (mins > 0)
				printf(" %d min%s,", mins, mins>1?"s":"");
		}

		/* Print number of users logged in to system */
		while (fread(&utmp, sizeof(utmp), 1, ut)) {
			if (utmp.ut_name[0] != '\0' && !nonuser(utmp))
				nusers++;
		}
		rewind(ut);
		printf("  %d users", nusers);

		/*
		 * Print 1, 5, and 15 minute load averages.
		 * (Found by looking in kernel for avenrun).
		 */
		printf(",  load average:");
		lseek(kmem, (long)nl[X_AVENRUN].n_value, 0);
		read(kmem, avenrun, sizeof(avenrun));
		for (i = 0; i < (sizeof(avenrun)/sizeof(avenrun[0])); i++) {
			if (i > 0)
				printf(",");
			printf(" %.2f", (double)avenrun[i]/FSCALE);
		}
		printf("\n");
		if (firstchar == 'u')
			exit(0);

		/* Headers for rest of output */
		if (lflag)
			printf("User     tty       login@  idle   JCPU   PCPU  what\n");
		else
			printf("User    tty  idle  what\n");
		fflush(stdout);
	}


	for (;;) {	/* for each entry in utmp */
		if (fread(&utmp, sizeof(utmp), 1, ut) == NULL) {
			fclose(ut);
			exit(0);
		}
		if (utmp.ut_name[0] == '\0')
			continue;	/* that tty is free */
		if (sel_user && strncmp(utmp.ut_name, sel_user, NMAX) != 0)
			continue;	/* we wanted only somebody else */

		gettty();
		jobtime = 0;
		proctime = 0;
		strcpy(doing, "-");	/* default act: normally never prints */
		empty = 1;
		curtime = -1;
		idle = findidle();
		for (i=0; i<np; i++) {	/* for each process on this tty */
			if (tty != pr[i].w_tty)
				continue;
			if (uid != pr[i].w_uid)
				continue;
			jobtime += pr[i].w_time + pr[i].w_ctime;
			proctime += pr[i].w_time;
			if (debug) {
				printf("\t\t%d\t%d\t%s",
					pr[i].w_pid,
					pr[i].w_stime,
					pr[i].w_args);
				if ((j=pr[i].w_igintr) > 0)
					if (j==IGINT)
						printf(" &");
					else
						printf(" & %d %d", j%3, j/3);
				printf("\n");
			}
			if (empty && pr[i].w_igintr!=IGINT) {
				empty = 0;
				curtime = -1;
			}
			if(pr[i].w_stime>curtime && (pr[i].w_igintr!=IGINT || empty)){
				curtime = pr[i].w_stime;
				strcpy(doing, lflag ? pr[i].w_args : pr[i].w_comm);
#ifdef notdef
				if (doing[0]==0 || doing[0]=='-' && doing[1]<=' ' || doing[0] == '?') {
					strcat(doing, " (");
					strcat(doing, pr[i].w_comm);
					strcat(doing, ")");
				}
#endif
			}
		}
		putline();
	}
}

/*
 * figure out the major/minor device # pair for this tty
 * figure out tty owner
 */
gettty()
{
	char ttybuf[20];
	struct stat statbuf;

	ttybuf[0] = 0;
	strcpy(ttybuf, "/dev/");
	strcat(ttybuf, utmp.ut_line);
	stat(ttybuf, &statbuf);
	tty = statbuf.st_rdev;
	uid = statbuf.st_uid;
}

/*
 * putline: print out the accumulated line of info about one user.
 */
putline()
{
	register int tm;

	/* print login name of the user */
	printf("%-*.*s ", NMAX, NMAX, utmp.ut_name);

	/* print tty user is on */
	if (lflag)
		/* long form: all (up to) LMAX chars */
		printf("%-*.*s", LMAX, LMAX, utmp.ut_line);
	else {
		/* short form: 2 chars, skipping 'tty' if there */
		if (utmp.ut_line[0]=='t' && utmp.ut_line[1]=='t' && utmp.ut_line[2]=='y')
			printf("%-2.2s", &utmp.ut_line[3]);
		else
			printf("%-2.2s", utmp.ut_line);
	}

	if (lflag)
		/* print when the user logged in */
		prtat(localtime(&utmp.ut_time));

	/* print idle time */
	prttime(idle," ");

	if (lflag) {
		/* print CPU time for all processes & children */
		prttime(jobtime," ");
		/* print cpu time for interesting process */
		prttime(proctime," ");
	}

	/* what user is doing, either command tail or args */
	printf(" %-.32s\n",doing);
	fflush(stdout);
}

/* find & return number of minutes current tty has been idle */
findidle()
{
	struct stat stbuf;
	long lastaction, diff;
	char ttyname[20];

	strcpy(ttyname, "/dev/");
	strncat(ttyname, utmp.ut_line, LMAX);
	stat(ttyname, &stbuf);
	time(&now);
	lastaction = stbuf.st_atime;
	diff = now - lastaction;
	diff = DIV60(diff);
	if (diff < 0) diff = 0;
	return(diff);
}

/*
 * prttime prints a time in hours and minutes.
 * The character string tail is printed at the end, obvious
 * strings to pass are "", " ", or "am".
 */
prttime(tim, tail)
	time_t tim;
	char *tail;
{
	register int didhrs = 0;

	if (tim >= 60) {
		printf("%3d:", tim/60);
		didhrs++;
	} else {
		printf("    ");
	}
	tim %= 60;
	if (tim > 0 || didhrs) {
		printf(didhrs&&tim<10 ? "%02d" : "%2d", tim);
	} else {
		printf("  ");
	}
	printf("%s", tail);
}

/* prtat prints a 12 hour time given a pointer to a time of day */
prtat(p)
	struct tm *p;
{
	register int t, pm;

	t = p -> tm_hour;
	pm = (t > 11);
	if (t > 11)
		t -= 12;
	if (t == 0)
		t = 12;
	prttime(t*60 + p->tm_min, pm ? "pm" : "am");
}

/*
 * readpr finds and reads in the array pr, containing the interesting
 * parts of the proc and user tables for each live process.
 */
readpr()
{
	int pn, mf, addr, c;
	int i;
	struct pte *pteaddr, apte;
	struct dblock db;

	if((mem = open("/dev/mem", 0)) < 0) {
		fprintf(stderr, "No mem\n");
		exit(1);
	}
	if ((swap = open("/dev/drum", 0)) < 0) {
		fprintf(stderr, "No drum\n");
		exit(1);
	}
	/*
	 * Find base of and parameters of swap
	 */
	lseek(kmem, (long)nl[X_NSWAP].n_value, 0);
	read(kmem, &nswap, sizeof(nswap));
	lseek(kmem, (long)nl[X_DMMIN].n_value, 0);
	read(kmem, &dmmin, sizeof(dmmin));
	lseek(kmem, (long)nl[X_DMMAX].n_value, 0);
	read(kmem, &dmmax, sizeof(dmmax));
	/*
	 * Locate proc table
	 */
	lseek(kmem, (long)nl[X_NPROC].n_value, 0);
	read(kmem, &nproc, sizeof(nproc));
	pr = (struct pr *)calloc(nproc, sizeof (struct pr));
	np = 0;
	lseek(kmem, (long)nl[X_PROC].n_value, 0);
	read(kmem, &aproc, sizeof(aproc));
	init_dmap();	/* need dmmin, dmmax before init */
	for (pn=0; pn<nproc; pn++) {
		lseek(kmem, (int)(aproc + pn), 0);
		read(kmem, &mproc, sizeof mproc);
		/* decide if it's an interesting process */
		if (mproc.p_stat==0 || mproc.p_pgrp==0)
			continue;
		/* find & read in the user structure */
		if ((mproc.p_flag & SLOAD) == 0) {
			/* not in memory - get from swap device */
			addr = dtob(mproc.p_swaddr);
			lseek(swap, (long)addr, 0);
			if (read(swap, up, NBPG*UPAGES) != NBPG*UPAGES) {
#if 0
		fprintf(stderr, "READ ERROR on swap dev, size is %d\n", size);
#endif
				continue;
			}
			if (up->u_smap.dm_ext) {
				/* bring in smap extension */
				addr = dtob(up->u_smap.dm_daddr);
				lseek(swap, (long)addr, 0);
				if (read(swap, dmep, dtob(sizeof_dmapext_disk))
				    != dtob(sizeof_dmapext_disk)) {
#if 0
			fprintf(stderr, "READ ERROR on swap dev, size is %d\n", size);
#endif
					continue;
				}
				up->u_smap.dm_ext = dmep;
			}
		} else {
			(void) lseek(kmem, (long) mproc.p_uarea, 0);
			if (read(kmem, up,  NBPG*UPAGES) != NBPG*UPAGES) {
#if 0
				fprintf(stderr, "READ ERROR on kmem dev, sizeof (up) is %d\n", NBPG*UPAGES);
#endif
			}
			if (up->u_smap.dm_ext) {
				/* bring in smap extension */
				lseek(kmem, (long)up->u_smap.dm_ext, 0);
				if (read(kmem, dmep, sizeof_dmapext)
				    != sizeof_dmapext) {
#if 0
				fprintf(stderr, "READ ERROR on kmem dev, sizeof (dmapext) is %d\n", sizeof_dmapext);
#endif
					continue;
				}
				up->u_smap.dm_ext = dmep;
			}
			if (mproc.p_pid == 0 && (mproc.p_flag&SSYS))
				continue;
			/*
			 * For a22 NS32032 chip bug workaround, the stack starts
			 * USRSTACKADJ bytes below the highest mapped address.
			 * Thus, add USRSTACKADJ below to get start of 1st K
			 * of users stack.
			 */
			pteaddr = sptopte(&mproc, USRSTACKADJ/NBPG+CLSIZE-1);
			(void) lseek(kmem, (long)pteaddr, 0);
			if (read(kmem, (char *)&apte, sizeof(apte)) != sizeof(apte)) {
#if 0
				/* proc 0 blows up here... */
				fprintf(stderr, "w: bad pte (0x%x) for stack, (pid %d)\n",
					pteaddr, mproc.p_pid);
#endif
				continue;
			}
			pr[np].w_seekaddr = PTETOPHYS(apte);
		}
		/*
		 * For a22 NS32032 chip bug work around, the stack starts
		 * USRSTACKADJ bytes below the highest mapped address.
		 * Thus, we pass a vsbase of USRSTACKADJ/NBPG to vstodb()
		 * instead of 0.
		 */
		vstodb(ctod(USRSTACKADJ/NBPG), ctod(CLSIZE), 
			&up->u_smap, &db, 1);
		pr[np].w_lastpg = dtob(db.db_base);
		if (up->u_ttyp == NULL)
			continue;

		/* save the interesting parts */
		pr[np].w_pid = mproc.p_pid;
		pr[np].w_uid = mproc.p_uid;
		pr[np].w_flag = mproc.p_flag;
		pr[np].w_size = mproc.p_dsize + mproc.p_ssize;
		pr[np].w_igintr = (((int)up->u_signal[2]==1) +
		    2*((int)up->u_signal[2]>1) + 3*((int)up->u_signal[3]==1)) +
		    6*((int)up->u_signal[3]>1);
		pr[np].w_time =
		    up->u_ru.ru_utime.tv_sec + up->u_ru.ru_stime.tv_sec;
		pr[np].w_ctime =
		    up->u_cru.ru_utime.tv_sec + up->u_cru.ru_stime.tv_sec;
		pr[np].w_stime = up->u_start.tv_sec;
		pr[np].w_tty = up->u_ttyd;
		up->u_comm[14] = 0;	/* Bug: This bombs next field. */
		strcpy(pr[np].w_comm, up->u_comm);
		/*
		 * Get args if there's a chance we'll print it.
		 * Cant just save pointer: getargs returns static place.
		 * Cant use strcpyn: that crock blank pads.
		 */
		pr[np].w_args[0] = 0;
		strncat(pr[np].w_args,getargs(&pr[np]),ARGWIDTH);
		if (pr[np].w_args[0]==0 || pr[np].w_args[0]=='-' && pr[np].w_args[1]<=' ' || pr[np].w_args[0] == '?') {
			strcat(pr[np].w_args, " (");
			strcat(pr[np].w_args, pr[np].w_comm);
			strcat(pr[np].w_args, ")");
		}
		np++;
	}
}

/*
 * init_dmap()
 *	Initialize dmap related values.
 */

init_dmap()
{
	register int blk;
	register u_int vaddr;
	register int dm_nemap = 0;

	/* 
	 * Determine number of entries in dmapext array (dm_nemap),
	 * and size of dmapext object (sizeof_dmapext).
	 */

	for (blk = dmmin, vaddr = 0; vaddr < MAXADDR; ) {
		++dm_nemap;
		vaddr += ctob(dtoc(blk));
		if (blk < dmmax)
			blk *= 2;
	}

#ifdef	i386
	/*
	 * Kludge to get sizeof_dmapext to not overflow into next page by
	 * only a few bytes -- assuming dmmin == 16k, dmmax == 256k, need
	 * to compensate for 6 longs (2 from struct dmapext and 4 from
	 * the short blocks).  In practice, this drops about 1.5Meg from
	 * the max size segment, and since stack consumes at least 4Meg
	 * of virtual space (due to virtual hole, data can't grow into
	 * top 4Meg of 256Meg), this is a don't care.
	 *
	 * Not an issue on ns32000, since 16Meg address space and 256k max
	 * chunk doesn't fill a HW page.
	 */
	dm_nemap -= 6;
#endif	i386

	sizeof_dmapext = sizeof(struct dmapext) + sizeof(swblk_t)*(dm_nemap-1);
	sizeof_dmapext_disk = ctod(clrnd(btoc(sizeof_dmapext)));
	dmep = (struct dmapext *)valloc(dtob(sizeof_dmapext_disk));
}

/*
 * getargs: given a pointer to a proc structure, this looks at the swap area
 * and tries to reconstruct the arguments. This is straight out of ps.
 */
char *
getargs(p)
	struct pr *p;
{
	char cmdbuf[CLSIZE*NBPG];
	char pagealign[CLSIZE*NBPG+(CLBYTES-1)];
	union argunion {
		char	argc[CLSIZE*NBPG];
		int	argi[CLSIZE*NBPG/sizeof (int)];
	} *argspac = (union argunion *)(((int)pagealign + (CLBYTES-1)) &~ (CLBYTES-1));
	int c, addr;
	register int *ip;
	register char *cp;

	if ((p->w_flag & SLOAD) == 0) {
		lseek(swap, p->w_lastpg, 0);
		if (read(swap, argspac, sizeof(union argunion)) != sizeof(union argunion))
			return(p->w_comm);
	} else {
		lseek(mem, (long) p->w_seekaddr,0);
		if (read(mem, argspac, CLSIZE*NBPG) != CLSIZE*NBPG) {
#if DEBUG
			printf("w: bad read of args from mem dev, sizeof argspac = %d\n", CLSIZE*NBPG);
#endif
			return(p->w_comm);
		}
	}
	ip = &argspac->argi[CLSIZE*NBPG/sizeof (int)];
	ip -= 2;		/* last arg word and .long 0 */
	while (*--ip)
		if (ip == argspac->argi)
			goto retucomm;
	*(char *)ip = ' ';
	ip++;
	for (cp = (char *)ip; cp < &argspac->argc[CLSIZE*NBPG]; cp++) {
		c = *cp & 0177;
		if (c == 0) {
			*cp = ' ';
		} else if (c < ' ' || c > 0176) {
			goto retucomm;
		} else if (c == '=') {
			while (*--cp != ' ')
				if (cp <= (char *)ip)
					break;
			break;
		}
	}
	*cp = 0;
	while (*--cp == ' ')
		*cp = 0;
	cp = (char *)ip;
	(void) strncpy(cmdbuf, cp, &argspac->argc[CLSIZE*NBPG] - cp);
	return (savestr(cmdbuf));

retucomm:
	return (p->w_comm);
}


char *
savestr(cp)
	char *cp;
{
	register int len;
	register char *dp;

	len = strlen(cp);
	dp = (char *)calloc(1, len+1);
	(void) strcpy(dp, cp);
	return (dp);
}


/*
 * vstodb()
 *	Given a base/size pair in virtual swap area,
 *	return a physical base/size pair which is the
 *	(largest) initial, physically contiguous block.
 */

vstodb(vsbase, vssize, dmp, dbp, rev)
	register int	vsbase;
	register int	vssize;
	struct dmap	*dmp;
	register struct dblock *dbp;
	int		rev;
{
	register swblk_t *ip;
	register int blk;

	if (dmp->dm_ext)
		ip = dmp->dm_ext->dme_map;
	else
		ip = dmp->dm_map;

	blk = dmmin;
	while (vsbase >= blk) {
		ip++;
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		else {				/* reached constant size blks */
			ip += (vsbase / blk);
			vsbase %= blk;
			break;
		}
	}
	dbp->db_size = min(vssize, blk - vsbase);
	dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
}

min(a, b)
{

	return (a < b ? a : b);
}
