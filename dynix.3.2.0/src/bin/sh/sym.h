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

/* $Header: sym.h 2.0 86/01/28 $ */

#
/*
 *	UNIX shell
 */


/* symbols for parsing */
#define DOSYM	0405
#define FISYM	0420
#define EFSYM	0422
#define ELSYM	0421
#define INSYM	0412
#define BRSYM	0406
#define KTSYM	0450
#define THSYM	0444
#define ODSYM	0441
#define ESSYM	0442
#define IFSYM	0436
#define FORSYM	0435
#define WHSYM	0433
#define UNSYM	0427
#define CASYM	0417

#define SYMREP	04000
#define ECSYM	(SYMREP|';')
#define ANDFSYM	(SYMREP|'&')
#define ORFSYM	(SYMREP|'|')
#define APPSYM	(SYMREP|'>')
#define DOCSYM	(SYMREP|'<')
#define EOFSYM	02000
#define SYMFLG	0400

/* arg to `cmd' */
#define NLFLG	1
#define MTFLG	2

/* for peekc */
#define MARK	0100000

/* odd chars */
#define DQUOTE	'"'
#define SQUOTE	'`'
#define LITERAL	'\''
#define DOLLAR	'$'
#define ESCAPE	'\\'
#define BRACE	'{'
