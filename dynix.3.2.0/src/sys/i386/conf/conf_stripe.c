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
static  char    rcsid[] = "$Header: conf_stripe.c 1.3 1991/06/25 16:10:15 $";
#endif

/*
 * conf_stripe.c
 *
 * disk striping binary configuration file.
 */

/* $Log: conf_stripe.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../stripe/stripe.h"

int nstripebufs = 100;			/* # of bufs in the driver local pool */
int stripe_maxbufs = 50;                /* Maximum number of bufs a request may 
				         * have at once from the local pool */
gate_t stripegate = 61;			/* Arbitrary */
