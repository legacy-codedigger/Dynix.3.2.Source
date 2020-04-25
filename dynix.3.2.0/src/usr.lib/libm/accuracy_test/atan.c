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
static char rcsid[] = "$Header: atan.c 1.1 86/09/20 $";
/*
c     program to test atan, atan2
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
c                 irnd   - 0 if floating-point addition chops,
c                          1 if floating-point addition rounds
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
c     standard fortran subprograms required
c
c         abs, alog, amax1, atan, atan2, float, sqrt
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
      int i,ibeta,iexp,iout,irnd,ii,it,i1,j,k1,k2,k3,machep,
              maxexp,minexp,n,negep,ngrd;
      double a,ait,albeta,b,beta,betap,del,em,eps,epsneg,
        expon,half,ob32,one,r6,r7,sum,two,w,x,xl,xmax,xmin,xn,
        xsq,x1,y,z,zero,zz;
      double log(),ran(),atan(),fabs(),sqrt(),pow(),atan2();

/*
      iout = 0
      call machar(ibeta,it,irnd,ngrd,machep,negep,iexp,minexp,
     1            maxexp,eps,epsneg,xmin,xmax)
*/
#include "machar.def"
      setbuf(stdout, NULL);
      beta = (double)(ibeta);
      albeta = log(beta);
      ait = (double)it;
      one = 1.0e0;
      half = 0.5e0;
      two = 2.0e0;
      zero = 0.0e0;
      a = -0.0625e0;
      b = -a;
      ob32 = b * half;
      n = 2000;
      xn = (double)(n);
      i1 = 0;
/*
c-----------------------------------------------------------------
c     random argument accuracy tests
c-----------------------------------------------------------------
*/
      for (j=1; j<=4; j++)
         {
         k1 = 0;
         k3 = 0;
         x1 = zero;
         r6 = zero;
         r7 = zero;
         del = (b - a) / xn;
         xl = a;
 
         for (i=1; i <= n; i++)
            {
            x = del * ran(i1) + xl;
            if (j == 2) 
              x = ((1.0e0+x*a)-one)*16.0e0;
            z = atan(x);

            if (j == 1)
              {
              xsq = x * x;
              em = 17.0e0;
              sum = xsq / em;
 
              for (ii = 1; ii <= 7; ii++)
                {
                em = em - two;
                sum = (one/em - sum) * xsq;
                }
 
              sum = -x * sum;
              zz = x + sum;
              sum = (x - zz) + sum;
              if (irnd == 0) 
                zz = zz + (sum + sum);
              }
            else
              {
              if (j == 2)
                {
                y = x - .0625e0;
                y = y / (one + x*a);
                zz = (atan(y) - 8.1190004042651526021e-5) + ob32;
                zz = zz + ob32;
                }
              else
 		{
                z = z + z;
                y = x / ((half + x * half)*((half - x) + half));
                zz = atan(y);
            	}
              }
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
         if (j == 1) 
 	   printf("test of atan(x) vs truncated taylor series\n\n");
         if (j == 2) 
 	   printf("test of atan(x) vs  atan(1/16) + atan((x - 1/16)/(1+x/16))\n\n");
         if (j > 2) 
 	   printf("test of 2*atan(x) vs atan(2x/(1-x*x))\n\n");
 	 printf("%d random arguments were tested from the interval \n",n);
         printf("  (  %18.18g, %18.18g)\n\n",a,b);
 	 printf("atan(x) was larger %d times,\n",k1);
         printf("            agreed %d times, and\n",k2);
	 printf("	    smaller %d times.\n\n",k3);
 	 printf(" there are %d base %d significant digits in a floating-point number\n\n",it,ibeta);
         w = -999.0e0;
         if (r6 != zero) 
	   w = log(fabs(r6))/albeta;
 	 printf("the maximum relative error of %18.18g = %d ** %18.18g\n",r6,ibeta,w);
         printf("  occurred for x = %18.18g\n",x1);
         w = (ait+w > zero) ? (ait+w) : zero;
 	 printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);	
         w = -999.0e0;
         if (r7 != zero) 
	   w= log(fabs(r7))/albeta;
 	 printf("the root mean square relative error was %18.18g = %d ** %18.18g\n",r7,ibeta,w);
         w = (ait+w > zero) ? (ait+w) : zero;
 	 printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);	
         a = b;
         if (j == 1) 
	   b = two - sqrt(3.0e0);
         if (j == 2) 
	   b = sqrt(two) - one;
         if (j == 3) 
           b = one;
       }
/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
      printf("special tests\n\n");
      printf("\nthe identity   atan(-x) = -atan(x)   will be tested.\n\n");
      printf("      x         f(x) + f(-x)\n");
      a = 5.0e0;
 
      for (i = 1; i <= 5; i++)
         {
         x = ran(i1) * a;
         z = atan(x) + atan(-x);
 	 printf("  %e\t%e \n",x,z);
         }
 
      printf("\nthe identity atan(x) = x , x small, will be tested.\n\n");
      printf("     x          x - f(x)\n\n");
      betap = pow(beta,(double)it);
      x = ran(i1) / betap;
 
      for (i=1; i <= 5; i++)
         {
         z = x - atan(x);
 	 printf("  %e\t%e \n",x,z);
         x = x / beta;
         }
 
      printf("\nthe identity atan(x/y) = atan2(x,y) will be tested \n");
      printf("the first column of results should be 0, the second +-pi\n\n");
      printf("   x               y          f1(x/y)-f2(x,y)  f1(x/y)-f2(x/(-y))\n\n");
      a = -two;
      b = 4.0e0;
 
      for (i=1; i<=5; i++)
         {
         x = ran(i1) * b + a;
         y = ran(i1);
         w = -y;
         z = atan(x/y) - atan2(x,y);
         zz = atan(x/w) - atan2(x,w);
         printf(" %e\t%e\t%e\t%e\n",x,y,z,zz);
         }
 
      printf("\ntest of underflow for very small argument.\n\n");
      expon = (double)(minexp) * 0.75e0;
      x = pow(beta,expon);
      y = atan(x);
      printf("    atan( %18.18g ) = %18.18g\n",x,y);
/*
c-----------------------------------------------------------------
c     test of error returns
c-----------------------------------------------------------------
      stop
      write (iout,1050)
      write (iout,1051) xmax
      z = atan(xmax)
      write (iout,1061) xmax, z
      x = one
      y = zero
      write (iout,1053) x, y
      z = atan2(x,y)
      write (iout,1062) x, y, z
      write (iout,1053) xmin, xmax
      z = atan2(xmin,xmax)
      write (iout,1062) xmin, xmax, z
      write (iout,1053) xmax, xmin
      z = atan2(xmax,xmin)
      write (iout,1062) xmax, xmin, z
      x = zero
      write (iout,1054) x, y
      z = atan2(x,y)
      write (iout,1062) x, y, z
      write (iout,1100)
      stop
*/
	exit(0);
  }
