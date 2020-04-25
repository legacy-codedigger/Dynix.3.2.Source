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

/*
 * $Header: kp_util.h 1.1 86/10/07 $
 */

/* $Log:	kp_util.h,v $
 */

/*
 * kp_util.h
 *
 * Local definitions for kernel profiling utilities
 */

#define TIMESTAMP	0
#define ELAPSED		1

struct kp_hdr {
	int tod_flag;		/* 0 => time stamp, 1 => elapsed time */
	struct timeval tod;
	int engines;
	int bins;
	int binshift;
	unsigned b_text;
	unsigned e_text;
};
