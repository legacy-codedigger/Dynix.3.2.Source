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
static char rcsid[] = "$Header: randl.c 1.1 86/09/20 $";
double randl(x)
double x;
/*
c
c     returns pseudo random numbers logarithmically distributed
c     over (1,exp(x)).  thus a*randl(ln(b/a)) is logarithmically
c     distributed in (a,b).
c
c     other subroutines required
c
c        exp(x) - the exponential routine
c
c        ran(k) - a function program returning random real
c                 numbers uniformly distributed over (0,1).
c                 the argument k is a dummy.
c
c
*/
{
      double ran(), exp();
      int k;
      k = 1;
      return (exp(x*ran(k)));
}
