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

#ident	"$Header: getchar.c 1.4 90/08/15 $"

/*
 * These control the getting and putting of chars
 */


#include <sys/types.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>
#include <machine/hwparam.h>
#include <machine/slic.h>
#include <machine/mftpr.h>

int init_getchar();
static int (*char_in)() = init_getchar;

/* 
 * init_getchar()
 * 	A one-shot function invoked the first time a console 
 *	character is to be read.  Determine the source of 
 *	console input, setup and invoke the device 
 *	dependent routine for this.
 */
static int
init_getchar(wait)
	int wait;
{
	register struct config_desc *cd = CD_LOC;
	extern int sec_getchar(); 
	extern int ssm_getchar(); 
	extern int kd_getchar();

	if (cd->c_cons->cd_type == SLB_SCSIBOARD) 
		char_in = sec_getchar;
	else
		char_in = ssm_getchar;
	return((*char_in)(wait));
}

getchar()
{
	return((*char_in)(1));
}

/* non blocking getchar */
igetchar()
{
	return((*char_in)(0));
}

/*
 * SEC dependent console input functions.
 * They are included here because including
 * them in the sec.c module makes some
 * bootstraps too big.
 */

#define RBUFSIZE	80
static char _ring[RBUFSIZE];
static char *_getp = _ring;
static char *_putp = _ring;

static int
sec_getchar(wait)
	int wait;
{
	register struct cpuslic *sl = (struct cpuslic *)LOAD_CPUSLICADDR;
	register int c;

	if (_putp != _getp) {
		c = *_getp++;
		if (_getp >= &_ring[RBUFSIZE])
			_getp -= RBUFSIZE;
		return ((unsigned char)c);
	}
	if (wait) {
		while ((sl->sl_ictl & SL_HARDINT) == 0)
			/* spin */;
	} else {
		if ((sl->sl_ictl & SL_HARDINT) == 0)
			return(-1);
	}
	c = sl->sl_binint; 
	sl->sl_binint = 0;
	return((unsigned char)c);
}

sec_put(c)
{
	if ((_putp == _getp - 1) 
	||  (_getp == _ring && _putp == &_ring[RBUFSIZE-1]))
		return;	/* discard */
	*_putp++ = (unsigned char) c;
	if (_putp >= &_ring[RBUFSIZE])
		_putp -= RBUFSIZE;
}
