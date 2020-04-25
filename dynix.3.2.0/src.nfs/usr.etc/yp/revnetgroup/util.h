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

/* $Header: util.h 1.1 86/12/18 $ */

/*  @(#)util.h 1.1 86/02/05 (C) 1985 Sun Microsystems, Inc. */
/* @(#)util.h	2.1 86/04/16 NFSSRC */ 


#define EOS '\0'

#ifndef NULL 
#	define NULL ((char *) 0)
#endif


#define MALLOC(object_type) ((object_type *) malloc(sizeof(object_type)))

#define FREE(ptr)	free((char *) ptr) 

#define ALLOCA(object_type) ((object_type *) alloca(sizeof(object_type)))

#define STRCPY(dst,src) \
	(dst = malloc((unsigned)strlen(src)+1), (void) strcpy(dst,src))

#define STRNCPY(dst,src,num) \
	(dst = malloc((unsigned)(num) + 1),\
	(void)strncpy(dst,src,num),(dst)[num] = EOS) 

extern char *malloc();
extern char *alloca();

char *getline();
void fatal();


