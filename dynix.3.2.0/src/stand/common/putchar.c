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

#ident	"$Header: putchar.c 2.7 90/08/15 $"

/*
 * Control the putting of chars
 */


#include <sys/types.h>
#include <machine/cfg.h>
#include <machine/hwparam.h>
#include <machine/slic.h>
#include <machine/slicreg.h>
#include <machine/mftpr.h>

static init_putchar();
static (*char_out)() = init_putchar;
int _slscsi = -1;

putchar(c)
	int c;
{
	(*char_out)(c);
}

/*
 * init_putchar()
 * 	A one-shot function invoked the first time a console 
 *	character is to be printed.  Determine the location 
 *	of the console output, setup and invoke the device 
 *	dependent function for this. 
 */
static 
init_putchar(c)
	int c;
{
#ifdef BOOTXX 
	extern sec_putchar();
	char_out = sec_putchar;
	_slscsi = CD_LOC->c_cons->cd_slic;
	putchar(c);
#endif BOOTXX


#ifndef BOOTXX
	register struct config_desc *cd = CD_LOC;
	extern sec_putchar();
	extern ssm_putchar();

	if (cd->c_cons->cd_type == SLB_SCSIBOARD) 
		char_out = sec_putchar;
	 else 	
		char_out = ssm_putchar;
	_slscsi = cd->c_cons->cd_slic;
	putchar(c);
#endif BOOTXX
}

/*
 * SEC dependent console output function.
 * Included here since inclusion in the
 * sec.c modules causes some bootstraps
 * to be too big.
 */
sec_putchar(c)
	int c;
{
#ifndef BOOTXX
	register struct cpuslic *sl = (struct cpuslic *)LOAD_CPUSLICADDR;
#endif /* BOOTXX */

	if (c == '\n') putchar('\r');
	mIntr(_slscsi, 1, c);

#ifndef BOOTXX
	/* still receive chars when putting them out */
	if (sl->sl_ictl & SL_HARDINT) {
		c = sl->sl_binint;
		sl->sl_binint = 0;
		sec_put(c);
	}
#endif
}
