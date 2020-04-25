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
static char mon_rcsid[] = "$Header: mon.c 1.4 1991/04/15 01:16:59 $";
static char mon_sccsid[] = "@(#)mon.c	4.9 (Berkeley) 7/26/83";
#endif

/*
 * mon.c
 *
 * $Log: mon.c,v $
 *
 *
 */

#define MAXLONG		0x7fffffff
#define ARCDENSITY	5	/* density of routines */
#define MINARCS		50	/* minimum number of counters */
#define	HISTFRACTION	2	/* fraction of text space for histograms */


struct phdr {
	int *lpc;
	int *hpc;
	int ncnt;
};

struct cnt {
	int *pc;
	long ncall;
} *countbase;

static int cntrs = 0;
static int profiling = 3;
static char *s_sbuf;
static int s_bufsiz;
static int s_scale;
static char *s_lowpc;

int numctrs;

/* 
 * to support parallel profiling
 */

char * _mon_file = "mon.out";

#define	MSG "No space for monitor buffer(s)\n"

monstartup(lowpc, highpc)
	char *lowpc;
	char *highpc;
{
	int monsize;
	char *buffer;
	int cntsiz;
	extern char *sbrk();
	extern char *_profbrk();
	extern char *_minbrk;


	cntsiz = (highpc - lowpc) * ARCDENSITY / 100;
	if (cntsiz < MINARCS)
		cntsiz = MINARCS;
	monsize = (highpc - lowpc + HISTFRACTION - 1) / HISTFRACTION
		+ sizeof(struct phdr) + cntsiz * sizeof(struct cnt);
	monsize = (monsize + 1) & ~1;
	buffer = _profbrk(monsize);
	if (buffer == (char *)-1) {
		write(2, MSG, sizeof(MSG));
		return;
	}
	_minbrk = sbrk(0);
	monitor(lowpc, highpc, buffer, monsize, cntsiz);
}

mcount()
{
	register int *selfpc;	/* %edi */
	register long **cntp;   /* %esi */

#ifdef lint
	selfpc = (int *) 0;
#else not lint

#ifdef	notdef
	/*
 	 *	save register r1 and r2 to support PASCAL
	 */
	asm("	movd	r1,tos");	/* save R1 */
	asm("	movd	r2,tos");	/* save R2 */
#endif

	/*
	 * find the return address for mcount,
	 * and address of counter pointer
	 */
	asm("	movl	4(%ebp),%edi");	/* selfpc = caller's pc */
	asm("	movl	%eax,%esi");	/* address of count local */
#endif not lint
	/*
	 * check that we aren't recursively invoked.
	 */
	if (profiling)
		goto out;
	profiling++;
	/*
	 * check that counter is allocated
	 */
	if (*cntp == 0) {
		/*
		 * check that a counter is available
		 */
		if (cntrs++ == numctrs)
			goto overflow;
		countbase->pc = selfpc;
		*cntp = &countbase->ncall;
		countbase++;
	}
	if ((**cntp) < MAXLONG) {
		/* Avoid overflow */
		(**cntp)++;
	}
	profiling--;
out:
#ifdef	notdef
	/*
 	 *	restore register r1 and r2
	 */
	asm("	movd	tos,r2");	/* restore R2 */
	asm("	movd	tos,r1");	/* restore R1 */
#endif
	return;

overflow:
#   define	TOLIMIT	"mcount: counter overflow\n"
	write( 2 , TOLIMIT , sizeof( TOLIMIT ) );
	goto out;
}

monitor(lowpc, highpc, buf, bufsiz, cntsiz)
	char *lowpc, *highpc;
	char *buf;
	int bufsiz, cntsiz;
{
	register int o;
	struct phdr *php;
	static int ssiz;
	static char *sbuf;

	if (lowpc == 0) {
		moncontrol(0);
		o = creat(_mon_file, 0666);
		write(o, sbuf, ssiz);
		close(o);
		return;
	}
	sbuf = buf;
	ssiz = bufsiz;
	php = (struct phdr *)&buf[0];
	php->lpc = (int *)lowpc;
	php->hpc = (int *)highpc;
	php->ncnt = cntsiz;
	numctrs = cntsiz;
	countbase = (struct cnt *)(buf + sizeof(struct phdr));
	o = sizeof(struct phdr) + cntsiz * sizeof(struct cnt);
	buf += o;
	bufsiz -= o;
	if (bufsiz <= 0)
		return;
	o = (highpc - lowpc);
	if(bufsiz < o)
		o = ((float) bufsiz / o) * 65536;
	else
		o = 65536;
	s_scale = o;
	s_sbuf = buf;
	s_bufsiz = bufsiz;
	s_lowpc = lowpc;
	moncontrol(1);
}

/*
 * Control profiling
 *	profiling is what mcount checks to see if
 *	all the data structures are ready.
 */
moncontrol(mode)
    int mode;
{
    if (mode) {
	/* start */
	profil(s_sbuf, s_bufsiz, s_lowpc, s_scale);
	profiling = 0;
    } else {
	/* stop */
	profil((char *)0, 0, 0, 0);
	profiling = 3;
    }
}
