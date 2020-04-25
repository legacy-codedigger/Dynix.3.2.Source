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
static char rcsid[] = "$Header: pow.c 1.2 87/08/25 $";
/*
c     program to test power function (**)
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
c                 minexp - the largest in magnitude negative
c                          integer such that  float(ibeta)**minexp
c                          is a positive floating-point number
c                 maxexp - the largest positive integer exponent
c                          for a finite floating-point number
c                 xmin   - the smallest non-vanishing floating-point
c                          power of the radix
c                 xmax   - the largest finite floating-point
c                          number
c
c        ran(k) - a function subprogram returning random real
c                 numbers uniformly distributed over (0,1)
c
c
c     standard fortran subprograms required
c
c         abs, alog, amax1, exp, dble, sqrt
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
      int i,ibeta,iexp,iout,irnd,it,i1,j,k1,k2,k3,machep;
      int maxexp,minexp,n,negep,ngrd;
      double a,ait,albeta,alxmax,b,beta,c,del,dely,eps;
      double epsneg,one,onep5,r6,r7,scale,two,w,x,xl,xmax,xmin,xn;
      double xsq,x1,y,y1,y2,z,zero,zz;
      double pow(),ran(),log(),exp(),sqrt(),fabs();

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
      alxmax = log(xmax);
      zero = 0.0;
      one = 1.0;
      two = one + one;
      onep5 = (two + one) / two;
      scale = one;
      j = (it+1) / 2;

      for (i=1; i<=j; i++)
         scale = scale * beta;

      a = one / beta;
      b = one;
      if (alxmax > -log(xmin))
	 c = -alxmax/log(100.0);
      else
	 c = log(xmin)/log(100.0);
      dely = -c - c;
      n = 2000;
      xn = (double)n;
      i1 = 0;
      y1 = zero;
/*
c-----------------------------------------------------------------
c     random argument accuracy tests
c-----------------------------------------------------------------
*/
      for (j=1; j<=4; j++) {
         k1 = 0;
         k3 = 0;
         x1 = zero;
         r6 = zero;
         r7 = zero;
         del = (b - a) / xn;
         xl = a;

	 for (i=1; i<=n; i++) {
            x = del * ran(i1) + xl;
            if (j == 1) {
               zz = pow(x,one);
               z = x;
	       }
	    else {
               w = scale * x;
               x = (x + w) - w;
               xsq = x * x;
               if (j != 4) {
                  zz = pow(xsq,onep5);
                  z = x * xsq;
		  }
	       else {
                  y = dely * ran(i1) + c;
                  y2 = (y/two + y) - y;
                  y = y2 + y2;
                  z = pow(x,y);
                  zz = pow(xsq,y2);
		  }
	       }
            w = one;
            if (z != zero) w = (z - zz) / z;
            if (w > zero) k1 = k1 + 1;
            if (w < zero) k3 = k3 + 1;
            w = fabs(w);
            if (w > r6) {
               r6 = w;
               x1 = x;
               if (j == 4) y1 = y;
	       }
            r7 = r7 + w * w;
            xl = xl + del;
	    }

         k2 = n - k3 - k1;
         r7 = sqrt(r7/xn);
         if (j == 1) {
            printf("test of x**1.0 vs x\n\n");
	    printf("%d random arguments were tested from the interval\n",n);
     	    printf(" (%18.18g ,%18.18g )\n\n",a,b);
	    printf("x**1.0 was larger %d times,\n",k1);
            printf("           agreed %d times, and\n",k2);
            printf("      was smaller %d times.\n\n",k3);
  	    }
         else if (j != 4) {
	    printf("test of xsq**1.5 vs xsq*x\n\n");
	    printf("%d random arguments were tested from the interval\n",n);
     	    printf(" (%18.18g ,%18.18g )\n\n",a,b);
	    printf("x**1.5 was larger %d times,\n",k1);
            printf("           agreed %d times, and\n",k2);
            printf("      was smaller %d times.\n\n",k3);
	    }
	 else {
	    printf("test of x**y vs xsq**(y/2)\n\n");
            w = c + dely;
	    printf("%d random arguments were tested from the region\n",n);
            printf("  x in (%18.18g ,%18.18g ), y in (%18.18g ,%18.18g )\n\n",a,b,c,w);
	    printf(" x**y  was larger %d times,\n",k1);
            printf("           agreed %d times, and\n",k2);
            printf("      was smaller %d times.\n\n",k3);
	    }
printf("there are %d base %d significant digits in a flt-pt no.\n\n",it,ibeta);
         w = -999.0;
         if (r6 != zero) w = log(fabs(r6))/albeta;
         if (j != 4) {
	   printf("the maximum relative error of %18.18g = %d ** %18.18g\n",r6,ibeta,w);
           printf("  occurred for x = %18.18g\n",x1);
	   }
	 else {
	   printf("the maximum relative error of %18.18g = %d ** %18.18g\n",r6,ibeta,w);
           printf("  occurred for x = %18.18g y = %18.18g\n",x1,y1);
	   }
	 if ((ait+w) > zero)
	     w = ait + w;
	 else
	     w = zero;
printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         w = -999.0;
         if (r7 != zero) w = log(fabs(r7))/albeta;
printf("the root mean square relative error was %18.18g = %d ** %18.18g\n",r7,ibeta,w);
	 if ((ait+w) > zero)
	     w = ait + w;
	 else
	     w = zero;
printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         if (j != 1) {
            b = 10.0;
            a = 0.01;
            if (j != 3) {
               a = one;
               b = exp(alxmax/3.0);
	       }
	    }
	 }
/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
      printf("special tests\n\n");
      printf("the identity  x ** y = (1/x) ** (-y)  will be tested.\n\n");
      printf("    x		    y		(x**y-(1/x)**(-y)) / x**y\n\n");
      b = 10.0;

      for (i=1; i<=5; i++) {
         x = ran(i1) * b + one;
         y = ran(i1) * b + one;
         z = pow(x,y);
         zz = pow((one/x),(-y));
         w = (z - zz) / z;
	 printf(" %le	%le	%le\n",x,y,w);
	 }
/*
c-----------------------------------------------------------------
c     test of error returns
c-----------------------------------------------------------------
      stop
      write (iout,1050)
      x = beta
      y = dble(minexp)
      write (iout,1051) x, y
      z = x ** y
      write (iout,1055) z
      y = dble(maxexp-1)
      write (iout,1051) x, y
      z = x ** y
      write (iout,1055) z
      x = zero
      y = two
      write (iout,1051) x, y
      z = x ** y
      write (iout,1055) z
      x = -y
      y = zero
      write (iout,1052) x, y
      z = x ** y
      write (iout,1055) z
      y = two
      write (iout,1052) x, y
      z = x ** y
      write (iout,1055) z
      x = zero
      y = zero
      write (iout,1052) x, y
      z = x ** y
      write (iout,1055) z
      write (iout,1100)
      stop
 1050 format(22h1test of error returns//)
 1051 format(2h (,e14.7,7h ) ** (,e14.7,20h ) will be computed.,/
     1       41h this should not trigger an error message//)
 1052 format(2h (,e14.7,7h ) ** (,e14.7,20h ) will be computed.,/
     1       37h this should trigger an error message//)
 1055 format(22h the value returned is,e15.4///)
 1100 format(25h this concludes the tests )
*/
	exit(0);
}
