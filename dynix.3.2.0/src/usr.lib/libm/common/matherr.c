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

/* This prints the error messages for math library errors.
 * It may be replaced by a user written routine having the
 * entry point "MATHERR".
 *
 *  $Header: matherr.c 2.6 90/04/23 $
 */

#include <stdio.h>
#include <errno.h>
#include <math.h>

extern int errno;
int _matherr;

/*
 *
 *  union dbl {
 *	int d_low;
 *	int d_high;
 *  };
 *
 *  union dbl nan;
 *
 *  nan.d_low  = 0x00000000;
 *  nan.d_high = 0x7ff00000;
 *
 */

double
MATHERR(loc, code)
  int *loc;
  int *code;
{

  switch (*code) {
  case 1:
     if (!_matherr) {
	errno = EDOM;
	return;
     }
     fprintf(stderr, "Square root of nan: sqrt");
     break;
  case 2:
     if (!_matherr) {
	errno = EDOM;
	return(0.0);
     }
     fprintf(stderr, "Square root of negative number: sqrt");
     break;
  case 3:
     if (!_matherr) {
	errno = EDOM;
	return;
     }
     fprintf(stderr, "Square root of infinity: sqrt");
     break;
  case 4:
     if (!_matherr) {
	errno = EDOM;
	return(0.0);
     }
     fprintf(stderr, "Argument exceeds maximum value: sincos");
     break;
  case 5:
     if (!_matherr) {
	errno = ERANGE;
	return(0.0);
     }
     fprintf(stderr, "Argument too small:exp");
     break;
  case 6:
     if (!_matherr) {
	errno = ERANGE;
	return(HUGE);
     }
     fprintf(stderr, "Argument exceeds maximum value: exp");
     break;
  case 7:
     if (!_matherr) {
	errno = EDOM;
	return;
     }
     fprintf(stderr, "Argument infinite: log");
     break;
  case 8:
     if (!_matherr) {
	errno = EDOM;
	return(-HUGE);
     }
     fprintf(stderr, "Argument less than or equal 0: log");
     break;
  case 9:
     if (!_matherr) {
	errno = EDOM;
	return;
     }
     fprintf(stderr, "Argument nan: log");
     break;
  case 11:
     if (!_matherr) {
	errno = EDOM;
	return(0.0);
     }
     fprintf(stderr, "Raising a negative number to a non-integer value: pow");
     break;
  case 12:
     if (!_matherr) {
	errno = EDOM;
	return(0.0);
     }
     fprintf(stderr, "Raising 0 to a non-positive value: pow");
     break;
/*
 * Domain error for atan2 not presently implemented for compatibility reasons.
 *
 * case 14:
 *    fprintf(stderr, "Both parameters equal 0: atan2");
 *    break;
 */
   case 16:
     if (!_matherr) {
	errno = EDOM;
	return(0.0);
     }
     fprintf(stderr, "Argument exceeds maximum value: _tan");
     break;
   case 18:
     if (!_matherr) {
	errno = EDOM;
	return(0.0);
     }
     fprintf(stderr, "Argument out of range: asin, acos");
     break;
  case 19:
     if (!_matherr) {
	errno = EDOM;
	return;
     }
     fprintf(stderr, "Argument out of range: sinh, cosh");
     break;
  default:
     fprintf(stderr, "MATHERR called with unknown code of %d", *code);
     break;
  }
  fprintf(stderr, "\nAddress 0x%x\n", *loc);
  abort();
}
