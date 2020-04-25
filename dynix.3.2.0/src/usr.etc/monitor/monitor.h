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
 * $Header: monitor.h 2.1 87/04/10 $
 */

/*
 * monitor.h
 */

#define MAX_PROC_LINES		10	/* default max processors per line */
#define MAX_PROC_COLS		3
#define COL_SPACING		4

#define TERSE_TEXT_LENGTH	8
#define VERBOSE_TEXT_LENGTH	18

struct coord {
	int x, y;
};

/*
 * A structure describing a statistic
 * Where the data point is located is an offset from text_pt
 * based on whether terse or not.
 */
struct sstat {
	struct coord text_pt;	/* the (x,y) of this stat text */
	char *fmt;		/* format for printing the data */
	unsigned *datav;	/* pointer to the data value */
	char *verbose;
	char *terse;
	int displayed;
};
