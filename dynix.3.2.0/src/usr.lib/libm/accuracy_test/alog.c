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
static char rcsid[] = "$Header: alog.c 1.2 87/08/25 $";
/*
c     program to test alog
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
c         abs, alog, alog10, amax1, dble, sign, sqrt
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
      double a,ait,albeta,b,beta,c,del,eight,eps,epsneg,half;
      double one,ran(),r6,r7,tenth,w,x,xl,xmax,xmin,xn,x1,y,z,zero,zz;
      double fabs(), log(), log10(), sqrt();

/*
      call machar(ibeta,it,irnd,ngrd,machep,negep,iexp,minexp,
     1            maxexp,eps,epsneg,xmin,xmax)
*/
#include "machar.def"
      setbuf(stdout, NULL);
      beta = (double)ibeta;
      albeta = log(beta);
      ait = (double) (it);
      j = it / 3;
      zero = 0.0;
      half = 0.5;
      eight = 8.0;
      tenth = 0.1;
      one = 1.0;
      c = one;

      for (i = 1; i <= j; i++)
       { c = c / beta; }
 
      b = one + c;
      a = one - c;
      n = 2000;
      xn = (double) n;
      i1 = 0;
/*
c-----------------------------------------------------------------
c     random argument accuracy tests
c-----------------------------------------------------------------
*/
      for (j=1; j <= 4; j++)
         {
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
            if (j == 1) 
               {
               y = (x - half) - half;
               zz = log(x);
               z = one / 3.0;
               z = y * (z - y / 4.0);
               z = (z - half) * y * y + y;
	       }
            else
              if (j == 2) 
	        {
                x = (x + eight) - eight;
                y = x + x / 16.00;
                z = log(x);
                zz = log(y) - 7.7746816434842581e-5;
                zz = zz - 31.0e0/512.0e0;
 		}

      	      else
                if (j == 3) 
                  {
                  x = (x + eight) - eight;
                  y = x + x * tenth;
                  z = log10(x);
                  zz = log10(y) - 3.7706015822504075e-4;
                  zz = zz - 21.0e0/512.0e0;
		  }

                else
     		  {
                  z = log(x*x);
                  zz = log(x);
                  zz = zz + zz;
                  }

            w = one;
            if (z != zero) 
 	      w = (z - zz) / z;

            /* z = sign(w,z) */
 	    z = ((z<0) ? -fabs(w) : fabs(w));

            if (z > zero) 
	      k1 = k1 + 1;
            else
              if (z < 0) 
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
         if (j == 1) 
           printf("test of alog(x) vs t.s. expansion of alog(1+y)\n\n");
         if (j == 2) 
           printf("test of alog(x) vs alog(17x/16)-alog(17/16) \n\n");
         if (j == 3) 
           printf("test of alog10(x) vs alog10(11x/10)-alog10(11/10) \n\n");
         if (j == 4) 
           printf("test of alog(x*x) vs 2 * log(x)  \n\n");
         if (j == 1) 
           printf("%d random arguments were tested from the interval \n (1-eps,1+eps), where eps = (%18.18g)\n\n",n,c);
         if (j != 1) 
 	   printf("%d random arguments were tested from the interval \n (%18.18g, %18.18g)  \n\n", n,a,b);  
         if (j != 3) 
           {
           printf("alog(x) was larger %d times,\n",k1);
           printf("            agreed %d times, and \n",k2);
           printf("       was smaller %d times.\n\n",k3);
           }
         else
           {
           printf("alog10(x) was larger %d times, \n",k1);
           printf("              agreed %d times, and\n",k2);
           printf("         was smaller %d times.\n\n",k3);
           }
         printf("there are %d  base  %d  significant digits in a floating-point number\n\n",it,ibeta);
         w = -999.0e0;
         if (r6 != zero) 
	   w = log(fabs(r6))/albeta;
 	 printf("the maximum relative error of %18.18g = %d ** %18.18g\n",r6,ibeta,w);
         printf("  occurred for x = %18.18g\n",x1);
         w = ((ait+w) > zero) ? (ait+w) : zero;
 	 printf("the estimated loss of base %d  significant digits is %18.18g\n\n",ibeta,w);
         w = -999.0e0;
         if (r7 != zero) 
	   w = log(fabs(r7))/albeta;
 	 printf("the root mean square relative error was %18.18g  = %d  **  %18.18g\n",r7,ibeta,w);
         w = ((ait+w) > zero) ? (ait+w) : zero;
 	 printf("the estimated loss of base %d  significant digits is %18.18g\n\n",ibeta,w);
         if (j == 1) 
           {
           a = sqrt(half);
           b = 15.0e0 / 16.0e0;
 	   }
         else
  	   if (j == 2) 
             {
             a = sqrt(tenth);
             b = 0.9e0;
	     }
	   else
             {
             a = 16.0e0;
             b = 240.0e0;
	     }
      }
/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
   printf(" special tests\n\n");
   printf("the identity  alog(x) = -alog(1/x)  will be tested.\n\n      x         f(x) + f(1/x)\n");

   for (i=1; i <= 5; i++)
      {
      x = ran(i1);
      x = x + x + 15.0e0;
      y = one / x;
      z = log(x) + log(y);
      printf("%18.18g\t%e\n",x,z);
      }
   printf("\n\ntest of special arguments\n\n");
   x = one;
   y = log(x);
   printf("alog(1.0) = %18.18g\n\n",y);
   x = xmin;
   y = log(x);
   printf("alog(xmin) = alog(%18.18g) = %18.18g\n\n",x,y);
   x = xmax;
   y = log(x);
   printf("alog(xmax) = alog(%18.18g) = %18.18g\n\n",x,y);
/*
c-----------------------------------------------------------------
c     test of error returns
c-----------------------------------------------------------------
*/
/*
      stop
      write (iout,1050)
      x = -2.0d0
      write (iout,1052) x
      y = log(x)
      write (iout,1055) y
      x = zero
      write (iout,1052) x
      y = log(x)
      write (iout,1055) y
      write (iout,1100)
      stop
 */

	exit(0);
}
