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
static char rcsid[] = "$Header: sinh.c 1.1 86/09/20 $";
/*
c     program to test sinh/cosh
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
c                 eps    - the smallest positive floating-point
c                          number such that 1.0+eps .ne. 1.0
c                 xmax   - the largest finite floating-point no.
c
c        ran(k) - a function subprogram returning random real
c                 numbers uniformly distributed over (0,1)
c
c
c     standard fortran subprograms required
c
c         abs, alog, amax1, cosh, float, int, sinh, sqrt
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
      int i,ibeta,iexp,ii,iout,irnd,it,i1,i2,j,k1,k2,k3,
              machep,maxexp,minexp,n,negep,ngrd,nit;
      double a,aind,ait,albeta,alxmax,b,beta,betap,c,c0,del,
           den,eps,epsneg,five,one,r6,r7,three,w,x,xl,xmax,xmin;
      double  xn,x1,xsq,y,z,zero,zz;
      double log(),fabs(),sqrt(),ran(),sinh(),pow(),cosh();

/* 
      iout = 0
      call machar(ibeta,it,irnd,ngrd,machep,negep,iexp,minexp,
     1            maxexp,eps,epsneg,xmin,xmax)
*/
#include "machar.def"
      setbuf(stdout, NULL);
      beta = (double)(ibeta);
      albeta = log(beta);
      alxmax = log(xmax);
      ait = (double)(it);
      zero = 0.0e0;
      one = 1.0e0;
      three = 3.0e0;
      five = 5.0e0;
      c0 = five/16.0e0 + 1.152713683194269979e-2;
      a = zero;
      b = 0.5e0;
      c = (ait + one) * 0.35e0;
      if (ibeta == 10) 
        c = c * three;
      n = 2000;
      xn = (double)(n);
      i1 = 0;
      i2 = 2;
      nit = 2 - ((int)(log(eps)*three))/20;
      aind = (double)(nit+nit+1);
/*
c-----------------------------------------------------------------
c     random argument accuracy tests
c-----------------------------------------------------------------
*/
      for (j=1; j<=4; j++)
         {
         if (j == 2) 
           {
           aind = aind - one;
           i2 = 1;
           }
         k1 = 0;
         k3 = 0;
         x1 = zero;
         r6 = zero;
         r7 = zero;
         del = (b - a) / xn;
         xl = a;
 
         for (i = 1; i <= n; i++)
            {
            x = del * ran(i1) + xl;
            if (j <= 2) 
              {
              xsq = x * x;
              zz = one;
              den = aind;
 
              for (ii=i2; ii <= nit; ii++)
                 {
                 w = zz * xsq/(den*(den-one));
                 zz = w + one;
                 den = den - 2.0e0;
                 }
 
              if (j != 2) 
                 {
                 w = x*xsq*zz/6.0e0;
                 zz = x + w;
                 z = sinh(x);
                 if (irnd == 0) 
                   {
                   w = (x - zz) + w;
                   zz = zz + (w + w);
                   }
                 }
              else
                 {
                 z = cosh(x);
                 if (irnd == 0) 
                   {
                   w = (one - zz) + w;
                   zz = zz + (w + w);
                   }
                 }
              }
            else
              {
              y = x;
              x = y - one;
              w = x - one;
              if (j != 4) 
                {
                z = sinh(x);
                zz = (sinh(y) + sinh(w)) * c0;
                }
              else
                {
                z = cosh(x);
                zz = (cosh(y) + cosh(w)) * c0;
                }
              } 

            w = one;
            if (z != zero) 
               w = (z - zz)/z;
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
         i = (j/2) * 2;
         if (j == 1) 
 	   printf("test of sinh(x) vs t.s. expansion of sinh(x)\n\n");
         if (j == 2) 
           printf("test of cosh(x) vs t.s. expansion of cosh(x)\n\n"); 
         if (j == 3) 
 	   printf("test of sinh(x) vs c*(sinh(x+1)+sinh(x-1))\n\n"); 
         if (j == 4) 
 	   printf("test of cosh(x) vs c*(cosh(x+1)+cosh(x-1))\n\n"); 
 	 printf("%d random arguments were tested from the interval \n",n);
         printf("   ( %18.18g,  %18.18g)\n\n",a,b);
         if (i != j) 
           {
 	   printf("  sinh(x) was larger %d times, \n",k1);
           printf("              agreed %d times, and \n",k2);
           printf("         was smaller %d times.\n\n",k3);
           }
         else
           {
 	   printf("  cosh(x) was larger %d times, \n",k1);
           printf("              agreed %d times, and \n",k2);
           printf("         was smaller %d times.\n\n",k3);
           }
         printf("there are %d base %d significant digits in a floating-point number\n\n",it,ibeta);
         w = -999.0e0;
         if (r6 != zero) 
	   w = log(fabs(r6))/albeta;
 	 printf("the maximum relative error of %18.18g = %d ** %18.18g \n",r6,ibeta,w);
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
           b = alxmax;
           a = three;
           }
     }
/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
      printf("special tests\n\n");
      printf("\nthe identity  sinh(-x) = -sinh(x)  will be tested.\n\n");
      printf("    x           f(x) + f(-x)\n\n");
 
      for (i=1; i <= 5; i++)
         {
         x = ran(i1) * a;
         z = sinh(x) + sinh(-x);
 	 printf("%e\t%e\n",x,z);
         }
 
      printf("\nthe identity sinh(x) = x , x small, will be tested.\n\n");
      printf("    x            x - f(x)\n\n");
      betap = pow(beta,(double)it);
      x = ran(i1) / betap;
 
      for (i=1; i <= 5; i++)
         {
         z = x - sinh(x);
 	 printf("%e\t%e\n",x,z);
         x = x / beta;
         }
 
      printf("\nthe identity  cosh(-x) = cosh(x)  will be tested.\n\n");
      printf("   x            f(x) - f(-x)\n\n");
 
      for (i = 1; i <= 5; i++)
         {
         x = ran(i1) * a;
         z = cosh(x) - cosh(-x);
 	 printf("%e\t%e\n",x,z);
         }
 
      printf("\ntest of underflow for very small argument.\n\n");
      x = pow(beta,(((double)minexp)*0.75e0));
      y = sinh(x);
      printf("  sinh( %18.18g ) = %18.18g\n",x,y);
/*
c-----------------------------------------------------------------
c     test of error returns
c-----------------------------------------------------------------
      stop
      write (iout,1050)
      x = alxmax + 0.125d0
      write (iout,1051) x
      y = sinh(x)
      write (iout,1055) y
      x = betap
      write (iout,1052) x
      y = sinh(x)
      write (iout,1055) y
      write (iout,1100)
      stop
*/
	exit(0);
}
