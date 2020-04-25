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
static char rcsid[] = "$Header: tanh.c 1.1 86/09/20 $";
/*
c     program to test tanh
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
c                          integer such that float(ibeta)**minexp
c                          is a positive floating-point number
c                 xmin   - the smallest non-vanishing floating-point
c                          power of the radix
c                 xmax   - the largest finite floating-point no.
c
c        ran(k) - a function subprogram returning random real
c                 numbers uniformly distributed over (0,1)
c
c
c     standard fortran subprograms required
c
c         abs, alog, amax1, float, sqrt, tanh
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
      int i,ibeta,iexp,iout,irnd,it,i1,j,k1,k2,k3,machep,
              maxexp,minexp,n,negep,ngrd;
      double a,ait,albeta,b,beta,betap,c,d,del,eps,epsneg,
           expon,half, one,r6,r7,w,x,xl,xmax,xmin,xn,x1,y,z,zero,zz;
      double log(),ran(),tanh(),sqrt(),fabs(),pow();
 
/*
      iout = 0
      call machar(ibeta,it,irnd,ngrd,machep,negep,iexp,minexp,
     1            maxexp,eps,epsneg,xmin,xmax)
*/
#include "machar.def"
      setbuf(stdout, NULL);
      beta = (double)(ibeta);
      albeta = log(beta);
      ait = (double)(it);
      zero = 0.0e0;
      one = 1.0e0;
      half = 0.5e0;
      a = 0.125e0;
      b = log(3.0e0) * half;
      c = 1.2435300177159620805e-1;
      d = log(2.0e0) + (ait+one) * log(beta) * half;
      n = 2000;
      xn = (double)(n);
      i1 = 0;
/*
c-----------------------------------------------------------------
c     random argument accuracy tests
c-----------------------------------------------------------------
*/
      for (j=1; j <= 2; j++)
         {
         k1 = 0;
         k3 = 0;
         x1 = zero;
         r6 = zero;
         r7 = zero;
         del = (b - a) / xn;
         xl = a;
 
         for (i=1; i<= n; i++)
            {
            x = del * ran(i1) + xl;
            z = tanh(x);
            y = x - 0.125e0;
            zz = tanh(y);
            zz = (zz + c) / (one + c*zz);
            w = one;
            if (z != zero) 
	      w = (z - zz) / z;
            if (w > zero)
              k1 = k1 + 1;
            if (w < zero) 
 	      k3 = k3 + 1;
            w = fabs(w);
            if (w > r6) 
              {
              r6 = w;
              x1 = x;
              }
            r7 = r7 + w * w;
            xl = xl + del;
            } 
 
         k2 = n - k3 - k1;
         r7 = sqrt(r7/xn);
 	 printf("test of tanh(x) vs tanh(x-1/8)+tanh(1/8)/(1+tanh(x-1/8)tanh(1/8)\n\n");
 	 printf("%d random arguments were tested from the interval \n",n);
         printf("    ( %18.18g,  %18.18g) \n\n",a,b);
 	 printf("tanh(x) was larger %d times \n",k1);
         printf("            agreed %d times and \n",k2);  
         printf("       was smaller %d times.\n\n",k3);
 	 printf("there are %d base %d significant digits in a floating-point number\n\n",it,ibeta);
         w = -999.0e0;
         if (r6 != zero) 
           w = log(fabs(r6))/albeta;
 	 printf("the maximum relative error of %18.18g = %d ** %18.18g\n",r6,ibeta,w);
         printf("   occurred for x = %18.18g\n",x1);
         w = (ait+w > zero) ? (ait+w) : zero;
 	 printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         w = -999.0e0;
         if (r7 != zero) 
	   w = log(fabs(r7))/albeta;
 	 printf("the root mean square relative error was %18.18g = %d ** %18.18g\n",r7,ibeta,w);
         w = (ait+w > zero) ? (ait+w) : zero;
 	 printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         a = b + a;
         b = d;
         }
/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
      printf("special tests\n\n");
      printf("\nthe identity   tanh(-x) = -tanh(x)   will be tested.\n\n");
      printf("      x         f(x) + f(-x)\n\n");
 
      for (i = 1; i <= 5; i++)
         {
         x = ran(i1);
         z = tanh(x) + tanh(-x);
         printf("%e\t%e\n",x,z);
         }
 
      printf("\nthe identity   tanh(-x) = x , x small,   will be tested.\n\n");
      printf("      x          x - f(x)\n\n");
      betap = pow(beta,(double)it);
      x = ran(i1) / betap;
 
      for (i=1; i <=5; i++) 
         {
         z = x - tanh(x);
         printf("%e\t%e\n",x,z);
         x = x / beta;
         }
 
      printf("\nthe identity tanh(x) = 1 , x large, will be tested.\n\n");
      printf("        x        1- f(x)\n\n");  
      x = d;
      b = 4.0e0;
 
      for (i=1; i <= 5; i++)
         {
         z = (tanh(x) - half) - half;
         printf("%e\t%e\n",x,z);
         x = x + b;
         }
 
      printf("\ntest of underflow for very small argument. \n\n");
      expon = ((double)minexp) * 0.75e0;
      x = pow(beta,expon);
      z = tanh(x);
      printf("tanh( %18.18g ) = %18.18g\n\n",x,z);
      printf("the function tanh will be called with the argument %18.18g\n\n",xmax);
      z = tanh(xmax);
      printf("tanh( %18.18g ) = %18.18g\n\n",xmax,z);
      printf("the function tanh will be called with the argument %18.18g\n\n",xmin);
      z = tanh(xmin);
      printf("tanh( %18.18g ) = %18.18g\n\n",xmin,z);
      x = zero;
      printf("the function tanh will be called with the argument %18.18g\n\n",x);
      z = tanh(x);
      printf("tanh( %18.18g ) = %18.18g\n\n",x,z);
      printf("this concludes the tests\n\n" );
      exit(0);
}
