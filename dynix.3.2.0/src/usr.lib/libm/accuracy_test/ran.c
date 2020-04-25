/*$Copyright:	$
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
static char rcsid[] = "$Header: ran.c 1.1 86/09/20 $";
      double ran(k)
/*
c
c     random number generator - based on algorithm 266 by pike and
c      hill (modified by hansson), communications of the acm,
c      vol. 8, no. 10, october 1965.
c
c     this subprogram is intended for use on computers with
c      fixed point wordlength of at least 29 bits.  it is
c      best if the floating point significand has at most
c      29 bits.
c
*/
int k;
{
      int j;
      static long iy = 100001;

      j = k;
      iy = iy * 125;
      iy = iy - (iy/2796203) * 2796203;
      return ((double)iy / 2796203.0);
}
