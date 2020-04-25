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
static char gmon_rcsid[] = "$Header: gmon.c 1.4 1991/04/15 01:17:15 $";
#endif

/*
 * gmon.c -- gprof monitor
 *
 * $Log: gmon.c,v $
 *
 *
 */

#ifdef DEBUG
#include <stdio.h>
#endif DEBUG

#include "gmon.h"
#define MAXLONG 0x7fffffff

    /*
     *	froms is actually a bunch of unsigned shorts indexing tos
     */
static int		profiling = 3;
static unsigned short	*froms;
static struct tostruct	*tos = 0;
static long		tolimit = 0;
static char		*s_lowpc = 0;
static char		*s_highpc = 0;
static unsigned long	s_textsize = 0;

static int	ssiz;
static char	*sbuf;
static int	s_scale;
    /* see profil(2) where this is describe (incorrectly) */
#define		SCALE_1_TO_1	0x10000L

/*
 * to support parallel program monitoring, move the name here.
 */

char * _mon_file = "gmon.out";

#define	MSG "No space for monitor buffer(s)\n"

monstartup(lowpc, highpc)
    char	*lowpc;
    char	*highpc;
{
    int			monsize;
    char		*buffer;
    char		*sbrk();
    char		*_profbrk();
    extern char		*_minbrk;

	/*
	 *	round lowpc and highpc to multiples of the density we're using
	 *	so the rest of the scaling (here and in gprof) stays in ints.
	 */
    lowpc = (char *)
	    ROUNDDOWN((unsigned)lowpc, HISTFRACTION*sizeof(HISTCOUNTER));
    s_lowpc = lowpc;
    highpc = (char *)
	    ROUNDUP((unsigned)highpc, HISTFRACTION*sizeof(HISTCOUNTER));
    s_highpc = highpc;
    s_textsize = highpc - lowpc;
    monsize = (s_textsize / HISTFRACTION) + sizeof(struct phdr);
    buffer = _profbrk( monsize );
    if ( buffer == (char *) -1 ) {
	write( 2 , MSG , sizeof(MSG) );
	return;
    }
    froms = (unsigned short *) _profbrk( s_textsize / HASHFRACTION );
    if ( froms == (unsigned short *) -1 ) {
	write( 2 , MSG , sizeof(MSG) );
	froms = 0;
	return;
    }
    tolimit = s_textsize * ARCDENSITY / 100;
    if ( tolimit < MINARCS ) {
	tolimit = MINARCS;
    } else if ( tolimit > 65534 ) {
	tolimit = 65534;
    }
    tos = (struct tostruct *) _profbrk( tolimit * sizeof( struct tostruct ) );
    if ( tos == (struct tostruct *) -1 ) {
	write( 2 , MSG , sizeof(MSG) );
	froms = 0;
	tos = 0;
	return;
    }
    _minbrk = sbrk(0);
    tos[0].link = 0;
    monitor( lowpc , highpc , buffer , monsize , tolimit );
}

_mcleanup()
{
    int			fd;
    int			fromindex;
    int			endfrom;
    char		*frompc;
    int			toindex;
    struct rawarc	rawarc;

    fd = creat( _mon_file , 0666 );
    if ( fd < 0 ) {
	perror( "mcount: gmon.out" );
	return;
    }
#   ifdef DEBUG
	fprintf( stderr , "[mcleanup] sbuf 0x%x ssiz %d\n" , sbuf , ssiz );
#   endif DEBUG
    write( fd , sbuf , ssiz );
    endfrom = s_textsize / (HASHFRACTION * sizeof(*froms));
    for ( fromindex = 0 ; fromindex < endfrom ; fromindex++ ) {
	if ( froms[fromindex] == 0 ) {
	    continue;
	}
	frompc = s_lowpc + (fromindex * HASHFRACTION * sizeof(*froms));
	for (toindex=froms[fromindex]; toindex!=0; toindex=tos[toindex].link) {
#	    ifdef DEBUG
		fprintf( stderr ,
			"[mcleanup] frompc 0x%x selfpc 0x%x count %d\n" ,
			frompc , tos[toindex].selfpc , tos[toindex].count );
#	    endif DEBUG
	    rawarc.raw_frompc = (unsigned long) frompc;
	    rawarc.raw_selfpc = (unsigned long) tos[toindex].selfpc;
	    rawarc.raw_count = tos[toindex].count;
	    write( fd , &rawarc , sizeof rawarc );
	}
    }
    close( fd );
}

mcount()
{
	register char			*selfpc;	/* %edi */
	register unsigned short		*frompcindex;	/* %esi */
	register struct tostruct	*top;		/* %ebx */
	register struct tostruct	*prevtop;
	register long			toindex;

#ifdef lint
	selfpc = (char *)0;
	frompcindex = 0;
#else not lint

#ifdef	notdef
	/*
 	 *	save register r1 and r2 to support PASCAL
	 */
	asm("	movd	r1,tos");	/* save R1 */
	asm("	movd	r2,tos");	/* save R2 */
#endif

	/*
	 *	find the return address for mcount,
	 *	and the return address for mcount's caller.
	 */
	asm("	movl	4(%ebp),%edi");	/* selfpc = caller's pc */
	asm("	movl	0(%ebp),%eax");
	asm("	movl	4(%eax),%esi"); /* frompcindex = his caller */
#endif not lint
	/*
	 *	check that we are profiling
	 *	and that we aren't recursively invoked.
	 */
	if (profiling) {
		goto out;
	}
	profiling++;
	/*
	 *	check that frompcindex is a reasonable pc value.
	 *	for example:	signal catchers get called from the stack,
	 *			not from text space.  too bad.
	 */
	frompcindex = (unsigned short *)((long)frompcindex - (long)s_lowpc);
	if ((unsigned long)frompcindex > s_textsize) {
		goto done;
	}
	frompcindex =
	    &froms[((long)frompcindex) / (HASHFRACTION * sizeof(*froms))];
	toindex = *frompcindex;
	if (toindex == 0) {
		/*
		 *	first time traversing this arc
		 */
		toindex = ++tos[0].link;
		if (toindex >= tolimit) {
			goto overflow;
		}
		*frompcindex = toindex;
		top = &tos[toindex];
		top->selfpc = selfpc;
		top->count = 1;
		top->link = 0;
		goto done;
	}
	top = &tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 *	arc at front of chain; usual case.
		 */
		if (top->count < MAXLONG) {
			/* Make sure no overflow */
			top->count++;
		}
		goto done;
	}
	/*
	 *	have to go looking down chain for it.
	 *	top points to what we are looking at,
	 *	prevtop points to previous top.
	 *	we know it is not at the head of the chain.
	 */
	for (; /* goto done */; ) {
		if (top->link == 0) {
			/*
			 *	top is end of the chain and none of the chain
			 *	had top->selfpc == selfpc.
			 *	so we allocate a new tostruct
			 *	and link it to the head of the chain.
			 */
			toindex = ++tos[0].link;
			if (toindex >= tolimit) {
				goto overflow;
			}
			top = &tos[toindex];
			top->selfpc = selfpc;
			top->count = 1;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
		/*
		 *	otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 *	there it is.
			 *	increment its count
			 *	move it to the head of the chain.
			 */
			if (top->count < MAXLONG) {
				/* Make sure no overflow */
				top->count++;
			}
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}

	}
done:
	profiling--;
	/* and fall through */
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
	profiling++; /* halt further profiling */
#   define	TOLIMIT	"mcount: tos overflow\n"
	write(2, TOLIMIT, sizeof(TOLIMIT));
	goto out;
}

/*VARARGS1*/
monitor( lowpc , highpc , buf , bufsiz , nfunc )
    char	*lowpc;
    char	*highpc;
    char	*buf;	/* declared ``short buffer[]'' in monitor(3) */
    int		bufsiz;
    int		nfunc;	/* not used, available for compatability only */
{
    register o;

    if ( lowpc == 0 ) {
	moncontrol(0);
	_mcleanup();
	return;
    }
    sbuf = buf;
    ssiz = bufsiz;
    ( (struct phdr *) buf ) -> lpc = lowpc;
    ( (struct phdr *) buf ) -> hpc = highpc;
    ( (struct phdr *) buf ) -> ncnt = ssiz;
    bufsiz -= sizeof(struct phdr);
    if ( bufsiz <= 0 )
	return;
    o = highpc - lowpc;
    if( bufsiz < o )
	s_scale = ( (float) bufsiz / o ) * SCALE_1_TO_1;
    else
	s_scale = SCALE_1_TO_1;
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
	profil(sbuf + sizeof(struct phdr), ssiz - sizeof(struct phdr),
		s_lowpc, s_scale);
	profiling = 0;
    } else {
	/* stop */
	profil((char *)0, 0, 0, 0);
	profiling = 3;
    }
}
