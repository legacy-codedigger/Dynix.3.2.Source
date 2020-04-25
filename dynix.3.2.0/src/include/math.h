/* $Copyright:	$
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

/* $Header: math.h 2.2 91/02/12 $ */

extern double fabs(), floor(), ceil(), modf(), ldexp(), frexp();
extern double sqrt(), hypot(), atof(), cabs();
extern double sin(), cos(), tan(), asin(), acos(), atan(), atan2();
extern double exp(), log(), log10(), pow();
extern double sinh(), cosh(), tanh();
extern double gamma();
extern double j0(), j1(), jn(), y0(), y1(), yn();

/* 
 * Single Precision Constant (derived from bc function):
 * 	scale = 90
 *	use 2^127*(2-2^(-23)) 
 *      and 2^127*(2-2^(-60)) to nail down the precison
 *
 * Double Precision Constant could be used but what if the poor
 * little user was using single precision and we tried to return
 * a HUGE constant into a single precision.  Poof! a double trap!
 *
 * Double wants to be:	#define HUGE	1.797693134862315e308
 * VAX 4.2 BSD is:	#define HUGE	1.701411733192644270e38
 */
#define HUGE	3.4028234e38
