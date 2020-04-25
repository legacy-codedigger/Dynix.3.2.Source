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
static char rcsid[] = "$Header: conf_if_se.c 2.6 87/01/23 $";
#endif lint

/*
 * if_se_conf.c
 *
 * This file contains the binary configuration data for the
 * Ether interface driver for SCSI/Ether.
 */

/* $Log:	conf_if_se.c,v $
 */

#include "../h/param.h"
#include "../h/socket.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

int se_watch_interval = 10;	/* seconds between stats collection */
int se_write_iats = 5;		/* number of write IATs; 5 should be OK */
int se_bin = 5;			/* bin number for mIntr to interrupt SEC */
short se_mtu = ETHERMTU;	/* max transfer unit for se transmit */

#ifdef DEBUG
int se_ibug = 0;		/* input debug level */
int se_obug = 0;		/* output debug level */
#endif DEBUG
