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
static char rcsid[] = "$Header: tan.c 1.1 86/09/20 $";
/*
c     program to test tan/cotan
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
c                 be deleted provided the following three
c                 parameters are assigned the values indicated
c
c                 ibeta  - the radix of the floating-point system
c                 it     - the number of base-ibeta digits in the
c                          significand of a floating-point number
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
c         abs, alog, amax1, cotan, float, tan, sqrt
c
c
c     latest revision - december 6, 1979
c
c     author - w. j. cody
c              argonne national laboratory
c
c
*/

  double ran();

#include <stdio.h>
main()
 {
      int ibeta,iexp,iout,irnd,i,it,i1,j,k1,k2,k3,machep,
              maxexp,minexp,n,negep,ngrd;
      double a,ait,albeta,b,beta,betap,c1,c2,del,eps,epsneg,
           half,pi,r6,r7,w,x,xl,xmax,xmin,xn,x1,y,z,zero,zz;
      double cotan(),fabs(), log(), sqrt(),tan(),pow();

/*
  c
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
      ait = (double)(it);
      pi = 3.14159265e0;
      a = zero;
      b = pi * 0.25e0;
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
 
	 for(i = 1; i <= n; i++)
	    {
            x = del * ran(i1) + xl;
            y = x * half;
            x = y + y;
            if (j != 4)
              {
              z = tan(x);
              zz = tan(y);
              w = 1.0e0;
              if ( z != zero)
	        {
                w = ((half-zz)+half)*((half+zz)+half);
                w = (z - (zz+zz)/w) / z;
                }
              }
            else
              {
              z = cotan(x);
              zz = cotan(y);
              w = 1.0e0;
 	      if (z != zero)
                {
                w = ((half-zz)+half)*((half+zz)+half);
                w = (z+w/(zz+zz))/z;
                }
              }
            if (w > zero)
	      k1 = k1 + 1;
            else
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
         if (j != 4) 
 	   printf("test of tan(x) vs 2*tan(x/2)/(1-tan(x/2)**2)\n\n");
         else
           printf("test of cot(x) vs (cot(x/2)**2-1)/(2*cot(x/2))\n\n");
         printf("    %d random arguments were tested from the interval \n",n);
         printf("     ( %18.18g,  %18.18g)\n\n",a,b);
         if (j != 4)
	   {
 	   printf("tan(x) was larger %d times,\n",k1);
           printf("           agreed %d times, and \n",k2);
           printf("           smaller %d times.\n\n",k3);
           }
         else
           {
 	   printf("cot(x) was larger %d times,\n",k1);
           printf("           agreed %d times, and \n",k2);
           printf("           smaller %d times.\n\n",k3);
           }
 	 printf("there are  %d base %d significant digits in a floating-point number\n\n",it,ibeta);
         w = -999.0e0;
         if (r6 != zero) 
	    w = log(fabs(r6))/albeta;
 	 printf("the maximum relative error of %18.18g = %d ** %18.18g\n",r6,ibeta,w);
         printf("   occurred for x = %18.18g\n",x1);
         w = ((ait+w) > zero) ? (ait+w) : zero;
         printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         w = -999.0e0;
         if (r7 != zero) 
	   w = log(fabs(r7))/albeta;
         printf("the root mean square relative error was %18.18g = %d ** %18.18g\n",r7,ibeta,w);
         w = ((ait+w) > zero) ? (ait+w) : zero;
         printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);
         if (j == 1) 
            {
            a = pi * 0.875e0;
            b = pi * 1.125e0;
            }
         else
            {
            a = pi * 6.0e0;
            b = a + pi * 0.25e0;
            }
         }
/*
c-----------------------------------------------------------------
c     special tests
c-----------------------------------------------------------------
*/
      printf("special tests\n\n");
      printf("\nthe identity  tan(-x) = -tan(x)  will be tested.\n\n");
      printf("   x            f(x) + f(-x)\n");
 
      for (i = 1; i <= 5; i++)  
         {
         x = ran(i1) * a;
         z = tan(x) + tan(-x);
         printf("%e \t%e\n",x,z);
         }
 
      printf("\nthe identity tan(x) = x , x small, will be tested.\n\n");
      printf("   x            f(x) + f(-x)\n");
      betap = pow(beta, (double)it);
      x = ran(i1) / betap;
 
      for (i=1; i <=5; i++)
         {
         z = x - tan(x);
         printf("%e \t%e\n",x,z);
         x = x / beta;
         }

 
      printf("\ntest of underflow for very small argument. \n");
      x = pow(beta, (((double)minexp)*0.75e0));
      y = tan(x);
      printf("     tan( %18.18g) = %18.18g\n",x,y);
      c1 = -225.0e0;
      c2 = -.950846454195142026e0;
      x = 11.0e0;
      y = tan(x);
      w = ((c1-y)+c2)/(c1+c2);
      z = log(fabs(w))/albeta;
      printf("\nthe relative error in tan(11) is  %18.18g = %d ** %18.18g where \n",w,ibeta,z);
      printf("     tan(%18.18g) = %18.18g\n\n",x,y);
      w = (ait+z>zero) ? (ait+z) : zero;
      printf("the estimated loss of base %d significant digits is %18.18g\n\n",ibeta,w);

/*
c-----------------------------------------------------------------
c     test of error returns
c-----------------------------------------------------------------
      stop
      write (iout,1050)
      x = beta ** (it/2)
      write (iout,1051) x
      y = tan(x)
      write (iout,1055) y
      x = betap
      write (iout,1052) x
      y = tan(x)
      write (iout,1055) y
      write (iout,1100)
      stop
*/
	exit(0);
}

/*
c
c--------- cotan added 6-mar-85 by dale
c
*/
 	double cotan(x)
           double x;
 
        {
	if (x != 0) 
	  return(1.0 / tan(x));
        else
          return(0.0);
        }

