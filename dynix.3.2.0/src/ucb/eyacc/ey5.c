/* $Copyright:	$
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

#ifndef lint
static char rcsid[] = "$Header: ey5.c 2.0 86/01/28 $";
#endif

/* (c) 1979 Regents of the University of California */
/* fake portable I/O routines, for those
    sites so backward as to not have the
     port. library */
/* munged for standard i/o library: peter and louise 5 may 80 */
#include <stdio.h>

FILE *cin, *cout;

FILE *copen( s, c )
    char *s;
    char c;
  {
  FILE *f;

	  if( c == 'r' ){
	    f = fopen( s, "r" );
  } else  if( c == 'a' ){
	    f = fopen( s, "a" );
	    fseek( f, 0L, 2 );
  } else {  /* c == w */
	    f = fopen( s, "w" );
  }

  return( f );
  }

cflush(x) FILE *x; { /* fake! sets file to x */
  fflush( cout );
  cout = x;
  }

system(){
  error( "The function \"system\" is called" );
  }

cclose(i) FILE *i; {
  fclose(i);
  }

cexit(i){
  fflush( cout );
  if ( i != 0 ) {
    abort();
  }
  exit(i);
  }
