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
static char rcsid[] = "$Header: sqrt.c 1.2 87/08/25 $";
/*
c     program to test sqrt
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
c                 be deleted provided the following six
c                 parameters are assigned the values indicated
c
c                 ibeta  - the radix of the floating-point system
c                 it     - the number of base-ibeta digits in the
c                          significand of a floating-point number
c                 eps    - the smallest positive floating-point
c                          number such that 1.0+eps .ne. 1.0
c                 epsneg - the smallest positive floating-point
c                          number such that 1.0-epsneg .ne. 1.0
c                 xmin   - the smallest non-vanishing floating-point
c                          power of the radix
c                 xmax   - the largest finite floating-point no.
c
c      randl(x) - a function subprogram returning logarithmically
c                 distributed random real numbers.  in particular,
c                        a * randl(alog(b/a))
c                 is logarithmically distributed over (a,b)
c
c        ran(k) - a function subprogram returning random real
c                 numbers uniformly distributed over (0,1)
c
c
c     standard fortran subprograms required
c
c         abs, alog, amax1, float, sqrt
c
c
c     latest revision - august 2, 1979
c
c     author - w. j. cody
c              argonne national laboratory
c
c
*/
#include <stdio.h>
main()
{
      int i,ibeta,iexp,iout,irnd,it,j,k1,k2,k3,machep;
      int maxexp,minexp,n,negep,ngrd;
      double a,ait,albeta,b,beta,c,eps,epsneg,one;
      double r6,r7,sqbeta,w,x,xmax,xmin,xn,x1,y,z,zero;
      double log(),fabs(),randl(),sqrt(),ran();

/*
      iout = 0;
      machar(ibeta,it,irnd,ngrd,machep,negep,iexp,minexp,maxexp,eps,epsneg,
	xmin,xmax);
*/
#include "machar.def"
      setbuf(stdout, NULL);
      beta = (double)ibeta;
      sqbeta = sqrt(beta);
      albeta = log(beta);
      ait = (double)it;
      one = 1.0;
      zero = 0.0;
      a = one / sqbeta;
      b = one;
      n = 2000;
      xn = (double)n;
/*
c-----------------------------------------------------------------
c     random argument accuracy tests
c-----------------------------------------------------------------
*/
      for (j=1; j<=2; j++) {
         c = log(b/a);
         k1 = 0;
         k3 = 0;
         x1 = zero;
         r6 = zero;
         r7 = zero;

	 for (i=1; i <= n; i++) {
            x = a * randl(c);
            y = x * x;
            z = sqrt(y);
            w = (z - x) / x;
            if (w > zero) k1 = k1 + 1;
            if (w < zero) k3 = k3 + 1;
            w = fabs(w);
            if (w > r6) {
               r6 = w;
               x1 = x;
	       }
  	    r7 = r7 + w * w;
	    }

         k2 = n - k1 - k3;
         r7 = sqrt(r7/xn);
printf(" test of sqrt(x*x) - x \n\n");
printf(" %d random arguments were tested from the interval\n",n);
printf("      (%18.18g,%18.18g)\n\n",a,b);
printf(" sqrt(x) was larger %d times\n",k1);
printf("             agreed %d times, and\n",k2);
printf("        was smaller %d times.\n\n",k3);
printf(" there are %d base %d significant digits in a flt pt no.\n\n",it,ibeta);
         w = -999.0;
         if (r6 != zero) w = log(fabs(r6))/albeta;
printf(" the maximum relative error of %18.18g = %d ** %18.18g\n",r6,ibeta,w);
printf("    occurred for x = %18.18g\n",x1);
	 if ((ait+w) > 0.0)
	   w = ait + w;
	 else
	   w = 0.0;
printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         w = -999.0;
         if (r7 != zero) w = log(fabs(r7))/albeta;
printf("the root mean square relative error was %18.18g = %d **%18.18g\n",r7,ibeta,w);
	 if ((ait+w) > 0.0)
	   w = ait + w;
	 else
	   w = 0.0;
printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         a = one;
         b = sqbeta;
	 }
/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
      printf (" test of special arguments\n\n");
      x = xmin;
      y = sqrt(x);
      printf(" sqrt(xmin) = sqrt( %18.18g    ) = %18.18g\n\n",x,y);
      x = one - epsneg;
      y = sqrt(x);
      printf(" sqrt(1-epsneg) = sqrt(1- % 18.18g ) = %18.18g\n\n",epsneg,y);
      x = one;
      y = sqrt(x);
      printf(" sqrt(1.0) = sqrt( %18.18g ) = %18.18g\n\n",x,y);
      x = one + eps;
      y = sqrt(x);
      printf(" sqrt(1+eps) = sqrt(1+ %18.18g ) = %18.18g\n\n",eps,y);
      x = xmax;
      y = sqrt(x);
      printf(" sqrt(xmax) = sqrt( %18.18g ) = %18.18g\n\n",x,y);
/*
c-----------------------------------------------------------------
c     test of error returns
c-----------------------------------------------------------------
      stop
      write (iout,1050)
      x = zero
      write (iout,1051) x
      y = sqrt(x)
      write (iout,1055) y
      x = -one
      write (iout,1052) x
      y = sqrt(x)
      write (iout,1055) y
      write (iout,1100)
      stop
 1050 format(22h1test of error returns//)
 1051 format(38h sqrt will be called with the argument,e15.4/
     1       41h this should not trigger an error message//)
 1052 format(38h0sqrt will be called with the argument,e15.4/
     1       37h this should trigger an error message//)
 1055 format(24h sqrt returned the value,e15.4///)
 1100 format(25h this concludes the tests )
*/
	exit(0);
}
