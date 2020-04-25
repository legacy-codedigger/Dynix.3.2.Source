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
static char rcsid[] = "$Header: refer0.c 2.0 86/01/28 $";
#endif

#include "refer..c"

FILE *in = stdin;
FILE *fo = stdout;
FILE *ftemp = stdout;
int endpush = 0;
int sort = 0;
int labels = 0;
int keywant = 0;
int bare = 0;
int biblio = 0;
int science = 0;
int postpunct = 0;
int authrev = 0;
char *smallcaps = "";
char *keystr = "AD";
char *convert = "X.AP";
int nmlen = 0, dtlen = 0;
char *rdata[NSERCH];
char **search = rdata;
int refnum = 0;
char reftext[NRFTXT];
char *reftable[NRFTBL];
char *rtp = reftext;
int sep = '\n';
char tfile[NTFILE];
char ofile[NTFILE];
char gfile[NTFILE];
char hidenam[NTFILE];
char *Ifile = "standard input";
int Iline = 0;
