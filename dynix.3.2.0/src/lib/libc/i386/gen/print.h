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

/* $Header: print.h 1.1 86/02/24 $ */

/* Maximum number of digits in any integer representation */
#define MAXDIGS 11

/* Maximum total number of digits in E format */
#define MAXECVT 17

/* Maximum number of digits after decimal point in F format */
#define MAXFCVT 60

/* Maximum significant figures in a floating-point number */
#define MAXFSIG MAXECVT

/* Maximum number of characters in an exponent */
#define MAXESIZ 5

/* Maximum (positive) exponent */
#define MAXEXP 310

/* Data type for flags */
typedef char bool;

/* Convert a digit character to the corresponding number */
#define tonumber(x) ((x)-'0')

/* Convert a number between 0 and 9 to the corresponding digit */
#define todigit(x) ((x)+'0')

/* Max and Min macros */
#define max(a,b) ((a) > (b)? (a): (b))
#define min(a,b) ((a) < (b)? (a): (b))
