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

/* $Header: compact.h 2.1 86/06/18 $ */

#if defined(vax) || defined(sun) || defined(ns32000) || defined(i386)
typedef int longint;
#else
typedef long longint;
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdio.h>

#define LNAME (MAXPATHLEN+1)
#define NEW flist; flist = flist -> next
#define LLEAF 010
#define RLEAF 04
#define SEEN 02
#define FBIT 01
#define COMPACTED 017777
#define PACKED 017437
#define EF 0400
#define NC 0401

struct charac {
#if defined(vax) || defined(pdp11) || defined(ns32000) || defined(i386)
	char lob, hib;
#else
	char hib, lob;
#endif
};

union cio {
	struct charac chars;
	short integ;
};

struct fpoint {
	struct node *fp;
	int flags;
} in [258];

#define MAXDIR 1024
struct index {
	struct node *pt;
	struct index *next;
} dir [MAXDIR], *head, *flist, *dirp, *dirq;

union treep {
	struct node *p;
	int ch;
};

struct node {
	struct fpoint fath;
	union treep sp [2];
	struct index *top [2];
	longint count [2];
} dict [258], *bottom;

longint oc;

FILE *cfp, *uncfp;

struct stat status;
