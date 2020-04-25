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

/*
 * $Header: tmp_ctl.h 2.4 1991/07/15 16:05:58 $
 */

/* $Log: tmp_ctl.h,v $
 *
 *
 */

/*
 * Processor/process affinity.
 */

#define AFF_ERROR	-1	/* Error occurred - check errno */
#define AFF_NONE	-2	/* No affinity */
#define AFF_QUERY	-3	/* Query current affinity */

/*
 * Processor information and control commands.
 */
#define TMP_ONLINE	0	/* Turn processor online */
#define TMP_OFFLINE	1	/* Turn processor offline */
#define TMP_QUERY	2	/* Query processor status */
#define TMP_NENG	3	/* Return No. of processors configured */
#define TMP_GETFLAGS    4       /* get flags */
#define TMP_TYPE	5	/* get processor type */
#define TMP_SLIC	6	/* get processor's SLIC address */
#define TMP_RATE	7	/* get processor's cpu rate in MHz */
#define TMP_WPT		8	/* Turn on processors's watchpoints */


/*
 * watchpoint structure
 */
struct kwpt {
	int	kwpt_reg;
	int	kwpt_val;
	int	kwpt_act;
};

/*
 * TMP_GETTYPE return values
 */

#define TMP_TYPE_NS32000 0	
#define TMP_TYPE_I386	 1
#define TMP_TYPE_I486    2
#define TMP_TYPE_I586    3

/*
 * TMP_GETFLAGS return flag.  Must be sure that all of these
 * flags don't equal "-1" when they're all "or"'ed together.
 */
#define TMP_FLAGS_ONLINE 1      /* engine online */
#define TMP_FLAGS_BAD   2       /* processor is bad */
#define TMP_FLAGS_FPA   4       /* has FPA when set */

/*
 * TMP_QUERY return values
 */
#define TMP_ENG_ONLINE	0
#define TMP_ENG_OFFLINE	1
