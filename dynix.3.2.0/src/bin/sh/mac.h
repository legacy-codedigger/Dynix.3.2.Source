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

/* $Header: /usr/src/dynix.3.2.0/src/bin/sh/RCS/mac.h,v 1.2 1993/03/16 09:55:36 bruce Exp $ */

#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

#define LOCAL	static
#define PROC	extern
#define TYPE	typedef
#define STRUCT	TYPE struct
#define UNION	TYPE union
#define REG	register

#define IF	if(
#define THEN	){
#define ELSE	} else {
#define ELIF	} else if (
#define FI	;}

#define BEGIN	{
#define END	}
#define SWITCH	switch(
#define IN	){
#define ENDSW	}
#define FOR	for(
#define WHILE	while(
#define DO	){
#define OD	;}
#define REP	do{
#define PER	}while(
#undef DONE
#define DONE	);
#define LOOP	for(;;){
#define POOL	}


#define SKIP	;
#define DIV	/
#define REM	%
#define NEQ	^
#define ANDF	&&
#define ORF	||

#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377
#define STRIP	0177
#define QUOTE	0200

#define EOF	0
#define NL	'\n'
#define SP	' '
#define LQ	'`'
#define RQ	'\''
#define MINUS	'-'
#define COLON	':'

