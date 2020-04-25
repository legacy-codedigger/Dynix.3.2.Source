/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)tbl:t0.c	1.3"
 /* t0.c: storage allocation */


# include "t..c"
int expflg = 0;
int ctrflg = 0;
int boxflg = 0;
int dboxflg = 0;
int tab = '\t';
int linsize;
int pr1403;
int delim1, delim2;
int evenflg;
int *evenup;
int F1 = 0;
int F2 = 0;
int allflg = 0;
int leftover = 0;
int textflg = 0;
int left1flg = 0;
int rightl = 0;
char *cstore, *cspace;
char *last;
struct colstr *table[MAXLIN];
int stynum[MAXLIN+1];
int fullbot[MAXLIN];
char *instead[MAXLIN];
int linestop[MAXLIN];
int (*style)[MAXHEAD];
char (*font)[MAXHEAD][2];
char (*csize)[MAXHEAD][4];
char (*vsize)[MAXHEAD][4];
int (*lefline)[MAXHEAD];
char (*cll)[CLLEN];
int (*flags)[MAXHEAD];
int qcol;
int *doubled, *acase, *topat;
int nslin, nclin;
int *sep;
int *used, *lused, *rused;
int nlin, ncol;
int iline = 1;
char *ifile = "Input";
int texname = 'a';
int texct = 0;
char texstr[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWYXZ0123456789";
int linstart;
char *exstore, *exlim, *exspace;
FILE *tabin  /*= stdin */;
FILE *tabout  /* = stdout */;