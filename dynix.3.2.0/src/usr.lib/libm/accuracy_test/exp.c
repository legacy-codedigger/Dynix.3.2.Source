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
static char rcsid[] = "$Header: exp.c 1.2 87/08/25 $";
/*
c     program to test exp
c
c     data required
c
c        none
c
c     subprograms required from this package
c
c        machar - an environmental inquiry program providing
c                 information on the floating-point arithmetic
c                 system.  note that the call to machar can
c                 be deleted provided the following four
c                 parameters are assigned the values indicated
c
c                 ibeta - the radix of the floating-point system
c                 it    - the number of base-ibeta digits in the
c                         significand of a floating-point number
c                 xmin  - the smallest non-vanishing floating-point
c                         power of the radix
c                 xmax  - the largest finite floating-point no.
c
c        ran(k) - a function subprogram returning random real
c                 numbers uniformly distributed over (0,1)
c
c
c     standard fortran subprograms required
c
c         abs, aint, alog, amax1, exp, float, sqrt
c
c
c     latest revision - december 6, 1979
c
c     author - w. j. cody
c              argonne national laboratory
c
c
*/
#include <stdio.h>
main()
{
      long ix;
      int i,ibeta,iexp,iout,irnd,it,i1,j,k1,k2,k3,machep;
      int maxexp,minexp,n,negep,ngrd;
      double a,ait,albeta,b,beta,d,del,eps,epsneg,one;
      double r6,r7,two,ten,v,w,x,xl,xmax,xmin,xn,x1,y,z,zero,zz;
      double pow(),ran(),fabs(),log(),exp(),sqrt();

/*
      iout = 0;
      call machar(ibeta,it,irnd,ngrd,machep,negep,iexp,minexp,
     1            maxexp,eps,epsneg,xmin,xmax)
*/
#include "machar.def"
      setbuf(stdout, NULL);
      beta = (double)ibeta;
      albeta = log(beta);
      ait = (double)it;
      one = 1.0;
      two = 2.0;
      ten = 10.0;
      zero = 0.0;
      v = 0.0625;
      a = two;
      b = log(a) * 0.5;
      a = -b + v;
      d = log(0.9*xmax);
      n = 2000;
      xn = (double)n;
      i1 = 0;
/*
c---------------------------------------------------------------------
c     random argument accuracy tests
c---------------------------------------------------------------------
*/
      for (j=1; j<=3; j++) {
         k1 = 0;
         k3 = 0;
         x1 = zero;
         r6 = zero;
         r7 = zero;
         del = (b - a) / xn;
         xl = a;

	 for (i=1; i<=n; i++) {
            x = del * ran(i1) + xl;
/*
c---------------------------------------------------------------------
c     purify arguments
c---------------------------------------------------------------------
*/
            y = x - v;
            if (y < zero) x = y + v;
            z = exp(x);
            zz = exp(y);
            if (j != 1) {
               if (ibeta != 10)
		  z = z * .0625 - z * 2.4453321046920570389e-3;
	       else
		  z = z * 6.0e-2 + z * 5.466789530794296106e-5;
	       }
	    else
               z = z - z * 6.058693718652421388e-2;
            w = one;
            if (zz != zero) w = (z - zz) / zz;
            if (w < zero) k1 = k1 + 1;
            if (w > zero) k3 = k3 + 1;
            w = fabs(w);
            if (w > r6) {
               r6 = w;
               x1 = x;
	       }
            r7 = r7 + w*w;
            xl = xl + del;
	    }

         k2 = n - k3 - k1;
         r7 = sqrt(r7/xn);
         printf("test of exp(x- %18.18g ) vs exp(x)/exp( %18.18g )\n\n",v,v);
         printf("%d random arguments were tested from the interval\n",n);
         printf("     ( %18.18g,%18.18g)\n\n",a,b);
         printf("exp(x-v) was larger %d times\n",k1);
         printf("             agreed %d times, and\n",k2);
         printf("        was smaller %d times.\n\n",k3);
 printf("there are %d base %d significant digits in a flt-pt no.\n\n",it,ibeta);
         w = -999.0;
         if (r6 != zero) w = log(fabs(r6))/albeta;
         printf("the maximum relative error of %18.18g = %d ** %18.18g\n",r6,ibeta,w);
	 printf("    occurred for x = %18.18g\n",x1);
	 if ((ait+w) > zero)
	    w = ait + w;
	 else
	    w = zero;
  printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         w = -999.0;
         if (r7 != zero) w = log(fabs(r7))/albeta;
 printf("the root mean square relative error was %18.18g = %d **%18.18g\n",r7,ibeta,w);
	 if ((ait+w) > zero)
	    w = ait + w;
	 else
	    w = zero;
  printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         if (j != 2) {
            v = 45.0 / 16.0;
            a = -ten * b;
            b = 4.0 * xmin * pow(beta,(double)it);
            b = log(b);
            }
	 else {
            a = -two * a;
            b = ten * a;
            if (b < d) b = d;
	    }
	 }
/*
c---------------------------------------------------------------------
c     special tests
c---------------------------------------------------------------------
*/
      printf("\nspecial tests\n\n");
      printf("the identity  exp(x)*exp(-x) = 1.0  will be tested.\n\n");
      printf("	x		f(x)*f(-x) - 1\n");

      for (i=1; i<=5; i++) {
         x = ran(i1) * beta;
         y = -x;
         z = exp(x) * exp(y) - one;
         printf("	%18.18g		%18.18g\n",x,z);
	 }

      printf("\n\ntest of special arguments\n\n");
      x = zero;
      y = exp(x) - one;
      printf("exp(0.0) - 1.0d0 = %18.18g\n",y);
      ix = (long)log(xmin);
      x = (double)ix;
      y = exp(x);
      printf("exp( %18.18g ) =	%18.18g\n",x,y);
      ix = (long)log(xmax);
      x = (double)ix;
      y = exp(x);
      printf("exp( %18.18g ) =	%18.18g\n",x,y);
      x = x / two;
      v = x / two;
      y = exp(x);
      z = exp(v);
      z = z * z;
      printf("\nif exp( %18.18g )   = %18.18g is not about\n",x,y);
      printf(" exp( %18.18g )**2 = %18.18g there is an arg red error\n",v,z);
/*
c---------------------------------------------------------------------
c     test of error returns
c---------------------------------------------------------------------
      stop
      write (iout,1050)
      x = -one / sqrt(xmin)
      write (iout,1052) x
      y = exp(x)
      write (iout,1061) y
      x = -x
      write (iout,1052) x
      y = exp(x)
      write (iout,1061) y
      write (iout,1100)
      stop
 1050 format(22h1test of error returns  //)
 1052 format(37h0exp will be called with the argument,e15.4/
     1       37h this should trigger an error message//)
 1061 format(23h exp returned the value,e15.4///)
 1100 format(25h this concludes the tests )
*/
	exit(0);
}
