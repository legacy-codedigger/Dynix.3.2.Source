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
static	char	rcsid[] = "$Header: conf_led.c 2.1 87/01/23 $";
#endif

/*
 * Configuration of front panel led light show
 */

/* $Log:	conf_led.c,v $
 */

#include "../h/types.h"

#include "../balance/slicreg.h"
#include "../balance/clkarb.h"

#define	FP_LED(i)	(SL_C_FP_LIGHT + ((i) * 2))

/*
 * The front panel has 48 programmable leds. These are arranged in 12 columns
 * with 4 leds in each row. The front panel led's are addressed from
 * left to right, top to bottom.
 */

/*
 * Currently assumes only processors will turn on lights.
 * Table is indexed by processor number. The first MAXNUMPROC entries
 * are reserved for processor use.
 */
u_char	fp_lightmap[FP_NLIGHTS] = {
	FP_LED(0),
	FP_LED(1),
	FP_LED(2),
	FP_LED(3),
	FP_LED(4),
	FP_LED(5),
	FP_LED(6),
	FP_LED(7),
	FP_LED(8),
	FP_LED(9),
	FP_LED(10),
	FP_LED(11),
	FP_LED(12),
	FP_LED(13),
	FP_LED(14),
	FP_LED(15),
	FP_LED(16),
	FP_LED(17),
	FP_LED(18),
	FP_LED(19),
	FP_LED(20),
	FP_LED(21),
	FP_LED(22),
	FP_LED(23),
	FP_LED(24),
	FP_LED(25),
	FP_LED(26),
	FP_LED(27),
	FP_LED(28),
	FP_LED(29),
	FP_LED(30),
	FP_LED(31),
	FP_LED(32),
	FP_LED(33),
	FP_LED(34),
	FP_LED(35),
	FP_LED(36),
	FP_LED(37),
	FP_LED(38),
	FP_LED(39),
	FP_LED(40),
	FP_LED(41),
	FP_LED(42),
	FP_LED(43),
	FP_LED(44),
	FP_LED(45),
	FP_LED(46),
	FP_LED(47)
};
