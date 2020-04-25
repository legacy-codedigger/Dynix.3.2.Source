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
static char rcsid[] = "$Header: asin.c 1.1 86/09/20 $";
/*
c     program to test asin/acos
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
c                 ibeta  - the radix of the floating-point system
c                 it     - the number of base-ibeta digits in the
c                          significand of a floating-point number
c                 irnd   - 0 if floating-point addition chops,
c                          1 if floating-point addition rounds
c                 minexp - the largest in magnitude negative
c                          integer such that float(ibeta)**minexp
c                          is a positive floating-point number
c
c        ran(k) - a function subprogram returning random real
c                 numbers uniformly distributed over (0,1)
c
c
c     standard fortran subprograms required
c
c         abs, acos, alog, alog10, amax1, asin, float, int, sqrt
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
      int i,ibeta,iexp,iout,irnd,it,i1,j,k,k1,k2,k3,l,m,
              machep,maxexp,minexp,n,negep,ngrd;
      double a,ait,albeta,b,beta,betap,c1,c2,del,eps,epsneg,
       half,r6,r7,s,sum,w,x,xl,xm,xmax,xmin,xn,x1,y,ysq,z,zero,zz;
      double ran(),asin(),sqrt(),fabs(),acos(),log(),log10(),pow();

/*
      iout = 0
      call machar(ibeta,it,irnd,ngrd,machep,negep,iexp,minexp,
     1            maxexp,eps,epsneg,xmin,xmax)
*/
#include "machar.def"
      setbuf(stdout, NULL);
      beta = (double)(ibeta);
      albeta = log(beta);
      zero = 0.0e0;
      half = 0.5e0;
      ait = (double)it;
      k = (log10(pow(beta,(double)it))) + 1;
      if (ibeta == 10) 
         {
         c1 = 1.57e0;
         c2 = 7.9632679489661923132e-4;
         }
      else
        {
        c1 = 201.0e0/128.0e0;
        c2 = 4.8382679489661923132e-4;
        }
      a = -0.125e0;
      b = -a;
      n = 2000;
      xn = (double)n;
      i1 = 0;
      l = -1;
/*
c-----------------------------------------------------------------
c     random argument accuracy tests
c-----------------------------------------------------------------
*/
      for (j=1; j <=5; j++)
         {
         k1 = 0;
         k3 = 0;
         l = -l;
         x1 = zero;
         r6 = zero;
         r7 = zero;
         del = (b - a) / xn;
         xl = a;
 
         for (i=1; i<= n; i++)
            {
            x = del*ran(i1) + xl;
            if (j > 2) 
              {
              ysq = half - half*fabs(x);
              x = (half - (ysq+ysq)) + half;
              if (j == 5) 
                x = -x;
              y = sqrt(ysq);
              y = y + y;
              }
            else
              {
              y = x;
              ysq = y*y;
              }
            sum = zero;
            xm = (double)(k+k+1);
            if (l > 0) 
              z = asin(x);
            else
              if (l < 0) 
                z = acos(x);
 
            for (m=1; m <=k; m++)
               {
               sum = ysq*(sum + 1.0e0/xm);
               xm = xm - 2.0e0;
               sum = sum*(xm/(xm+1.0e0));
               }
 
            sum = sum*y;
            if ((j == 1) || (j == 4)) 
              {
              zz = y + sum;
              sum = (y - zz) + sum;
              if (irnd != 1) 
                zz = zz + (sum+sum);
              }
            else
              {
              s = c1 + c2;
              sum = ((c1 - s) + c2) - sum;
              zz = s + sum;
              sum = ((s - zz) + sum) - y;
              s = zz;
              zz = s + sum;
              sum = (s - zz) + sum;
              if (irnd != 1) 
                zz = zz + (sum+sum);
              }
            w = 1.0e0;
            if (z != zero) 
              w = (z-zz)/z;
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
            r7 = r7 + w*w;
            xl = xl + del;
            }
 
         k2 = n - k3 - k1;
         r7 = sqrt(r7/xn);
         if (l >= 0)
           { 
           printf("test of asin(x) vs taylor series\n\n");
 	   printf("%d random arguments were tested from the interval\n",n); 
           printf("    ( %18.18g, %18.18g)\n\n",a,b);
 	   printf(" asin(x) was larger %d times,\n",k1);
           printf("             agreed %d times, and\n",k2);
           printf("        was smaller %d times.\n\n",k3);
           }
         else
           {
 	   printf("test of acos(x) vs taylor series\n\n"); 
 	   printf("%d random arguments were tested from the interval\n",n); 
           printf("    ( %18.18g, %18.18g)\n\n",a,b);
 	   printf(" acos(x) was larger %d times,\n",k1);
           printf("             agreed %d times, and\n",k2);
           printf("        was smaller %d times.\n\n",k3);
           }
 	 printf("there are %d base %d significant digits in a floating-point number\n\n",it,ibeta);
         w = -999.0e0;
         if (r6 != zero) 
           w = log(fabs(r6))/albeta;
 	 printf("the maximum relative error of %18.18g = %d  ** %18.18g\n",r6,ibeta,w);
         printf("  occurred for x = %18.18g\n",x1);
         w = (ait+w > zero) ? (ait+w) : zero;
 	 printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         w = -999.0e0;
         if (r7 != zero)
           w = log(fabs(r7))/albeta;
 	 printf("the root mean square relative error was %18.18g = %d ** %18.18g\n",r7,ibeta,w);
         w = (ait+w > zero) ? (ait+w) : zero;
 	 printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         if (j == 2) 
           {
           a = 0.75e0;
           b = 1.0e0;
           }
         if (j == 4) 
           {
           b = -a;
           a = -1.0e0;
           c1 = c1 + c1;
           c2 = c2 + c2;
           l = -l;
           }
       }
/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
      printf("special tests\n\n");
      printf("the identity  asin(-x) = -asin(x)  will be tested.\n\n");
      printf("      x          f(x) + f(-x)\n");
 
      for (i=1; i <= 5; i++)
         {
         x = ran(i1)*a;
         z = asin(x) + asin(-x);
 	 printf("%e\t %e\n",x,z);
         }
 
      printf("\nthe identity asin(x) = x, x small will be tested.\n\n");
      printf("     x           x - f(x)\n");
      betap = pow(beta,(double)it);
      x = ran(i1) / betap;
 
      for (i=1; i <= 5; i++)
         {
         z = x - asin(x);
 	 printf("%e\t %e\n",x,z);
         x = x / beta;
         }
 
      printf("\ntest of underflow for very small argument.\n");
      x = pow(beta,((double)minexp)) * 0.75e0;
      y = asin(x);
      printf("    asin( %18.18g) = %18.18g \n",x,y);


/*
c-----------------------------------------------------------------
c     test of error returns
c-----------------------------------------------------------------
      stop
      write (iout,1050)
      x = 1.2d0
      write (iout,1052) x
      y = asin(x)
      write (iout,1055) y
      write (iout,1100)
      stop
*/

	exit(0);
}
