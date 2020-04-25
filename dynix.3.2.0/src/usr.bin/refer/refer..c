/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: refer..c 2.1 1991/05/20 00:16:51 $ */

#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define FLAG 003
#define AFLAG 007
#define NRFTXT 10000
#define NRFTBL 500
#define NTFILE 20
#define QLEN 512
#define ANSLEN 1024
#define TAGLEN 400
#define NSERCH 20
#define MXSIG 200		/* max bytes in aggregate signal */

extern FILE *in;
extern int endpush, sort, labels, keywant, bare;
extern int biblio, science, postpunct;
extern char *smallcaps;
extern char comname;
extern char *keystr;
extern char *convert;
extern int authrev;
extern int nmlen, dtlen;
extern char *rdata[], **search;
extern int refnum;
extern char *reftable[];
extern char *rtp, reftext[];
extern int sep;
extern char tfile[];
extern char gfile[];
extern char ofile[];
extern char hidenam[];
extern char *Ifile; extern int Iline;
extern FILE *fo, *ftemp;
extern char *input(),*lookat();
extern char *class(),*caps(),*revauth();
extern char *artskp(),*fpar();
extern char *trimnl();

extern char *getenv(), *strcpy(), *strcat();
