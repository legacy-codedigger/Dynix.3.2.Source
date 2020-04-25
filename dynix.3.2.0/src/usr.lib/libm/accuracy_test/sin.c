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
static char rcsid[] = "$Header: sin.c 1.2 87/08/25 $";
/*
c     program to test sin/cos
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
c                 be deleted provided the following five
c                 parameters are assigned the values indicated
c
c                 ibeta  - the radix of the floating-point system
c                 it     - the number of base-ibeta digits in the
c                          significand of a floating-point number
c                 minexp - the largest in magnitude negative
c                          integer such that  float(ibeta)**minexp
c                          is a positive floating-point number
c                 eps    - the smallest positive floating-point
c                          number such that 1.0+eps .ne. 1.0
c                 epsneg - the smallest positive floating-point
c                          number such that 1.0-epsneg .ne. 1.0
c
c        ran(k) - a function subprogram returning random real
c                 numbers uniformly distributed over (0,1)
c
c
c     standard fortran subprograms required
c
c         abs, alog, amax1, cos, float, sin, sqrt
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
      double a,ait,albeta,b,beta,betap,c,del,eps,epsneg;
      double expon,one,r6,r7,three,w,x,xl,xmax,xmin,xn,x1,y,z,zero,zz;
      double sqrt(),pow(),log(),ran(),fabs(),cos(),sin();

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
      zero = 0.0;
      three = 3.0;
      a = zero;
      b = 1.570796327;
      c = b;
      n = 2000;
      xn = (double)n;
      i1 = 0;
/*
c-----------------------------------------------------------------
c     random argument accuracy tests
c-----------------------------------------------------------------
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
            y = x / three;
            y = (x + y) - x;
            x = three * y;
            if (j != 3) {
               z = sin(x);
               zz = sin(y);
               w = one;
               if (z != zero) w = (z - zz*(three-4.0*zz*zz)) / z;
	       }
	    else {
               z = cos(x);
               zz = cos(y);
               w = one;
               if (z != zero) w = (z + zz*(three-4.0*zz*zz)) / z;
	       }
            if (w > zero) k1 = k1 + 1;
            if (w < zero) k3 = k3 + 1;
            w = fabs(w);
            if (w > r6) {
               r6 = w;
               x1 = x;
	       }
            r7 = r7 + w * w;
            xl = xl + del;
	    }

         k2 = n - k3 - k1;
         r7 = sqrt(r7/xn);
         if (j != 3) {
            printf("test of sin(x) vs 3*sin(x/3)-4*sin(x/3)**3\n\n");
            printf("%d random arguments were tested from the interval\n",n);
            printf("      (%18.18g , %18.18g)\n\n",a,b);
	    printf("sin(x) was larger %d times\n",k1);
            printf("           agreed %d times, and\n",k2);
            printf("      was smaller %d times.\n\n",k3);
	    }
	 else {
	    printf("test of cos(x) vs 4*cos(x/3)**3-3*cos(x/3)\n\n");
            printf("%d random arguments were tested from the interval\n",n);
            printf("      (%18.18g , %18.18g)\n\n",a,b);
	    printf("cos(x) was larger %d times\n",k1);
            printf("           agreed %d times, and\n",k2);
            printf("      was smaller %d times.\n\n",k3);
	    }
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
printf("the root mean square relative error was %18.18g = %d ** %18.18g\n",r7,ibeta,w);
	 if ((ait+w) > zero)
	    w = ait + w;
	 else
            w = zero;
printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         a = 18.84955592;
         if (j == 2) a = b + c;
         b = a + c;
	 }

/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
      printf("special tests\n\n");
      c = pow((one/beta),(double)(it/2));
      z = (sin(a+c) - sin(a-c)) / (c + c);
      printf("if %le is not almost 1.0e0, sin has the wrong period.\n\n",z);

      printf("the identity sin(-x) = -sin(x) will be tested.\n");
      printf("     x			f(x) + f(-x)\n");

      for (i=1; i<=5; i++) {
         x = ran(i1) * a;
         z = sin(x) + sin(-x);
	 printf("%le		%le\n",x,z);
	 }

      printf("\nthe identity sin(x) = x , x small, will be tested.\n");
      printf("     x		         x - f(x)\n");
      betap = pow(beta,(double)it);
      x = ran(i1) / betap;

      for (i=1; i<=5; i++) {
         z = x - sin(x);
	 printf("%le		%le\n",x,z);
         x = x / beta;
	 }

      printf("\nthe identity cos(-x) = cos(x) will be tested.\n");
      printf("     x			f(x) - f(-x)\n");

      for (i=1; i<=5; i++) {
         x = ran(i1) * a;
         z = cos(x) - cos(-x);
	 printf("%le		%le\n",x,z);
	 }

      printf("\ntest of underflow for very small argument.\n");
      expon = (double)minexp * 0.75;
      x = pow(beta,expon);
      y = sin(x);
      printf("sin( %18.18g ) = %18.18g\n\n",x,y);
      printf("the following three lines illustrate the loss in significance\n");
      printf(" for large arguments.  the arguments are consecutive.\n\n");
      z = sqrt(betap);
      x = z * (one - epsneg);
      y = sin(x);
      printf("sin( %18.18g ) = %18.18g\n",x,y);
      y = sin(z);
      printf("sin( %18.18g ) = %18.18g\n",x,y);
      x = z * (one + eps);
      y = sin(x);
      printf("sin( %18.18g ) = %18.18g\n",x,y);
/*
c-----------------------------------------------------------------
c     test of error returns
c-----------------------------------------------------------------
      stop
      write (iout,1050)
      x = betap
      write (iout,1052) x
      y = sin(x)
      write (iout,1055) y
      write (iout,1100)
      stop
 1050 format(22h1test of error returns//)
 1052 format(37h sin will be called with the argument,e15.4/
     1       37h this should trigger an error message//)
 1055 format(23h sin returned the value,e15.4///)
 1100 format(25h this concludes the tests )
*/
	exit(0);
}
