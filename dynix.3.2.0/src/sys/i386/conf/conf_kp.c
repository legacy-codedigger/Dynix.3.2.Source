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
static	char	rcsid[] = "$Header: conf_kp.c 1.1 86/10/07 $";
#endif

/* $Log:	conf_kp.c,v $
 */

/*
 * Configuration info for NMI based kernel profiling
 */

int kp_binshift = 3;		/* log2(binsize) (eg, bin is 8 bytes => 3) */
