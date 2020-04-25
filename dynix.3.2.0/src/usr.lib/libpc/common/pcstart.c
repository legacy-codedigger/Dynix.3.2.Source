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

/* $Header: pcstart.c 1.1 89/03/12 $ */
#include <signal.h>
#include "h00vars.h"
#include "libpc.h"

/*
 * program variables
 */
struct display	_disply[MAXLVL];
int		_argc;
char		**_argv;
long		_stlim = 500000;
long		_stcnt = 0;
long		_seed = 7774755;
#ifdef ADDR32
char		*_minptr = (char *)0x7fffffff;
#endif ADDR32
#ifdef ADDR16
char		*_minptr = (char *)0xffff;
#endif ADDR16
char		*_maxptr = (char *)0;

/*
 * file record variables
 */
long		_filefre = PREDEF;
struct iorechd	_fchain = {
	0, 0, 0, 0,		/* only use fchain field */
	INPUT			/* fchain  */
};
IOREC	*_actfile[MAXFILES] = {
	INPUT,
	OUTPUT,
	ERR
};

/*
 * standard files
 */
char		_inwin, _outwin, _errwin;
struct iorechd	input = {
	&_inwin,		/* fileptr */
	0,			/* lcount  */
	0x7fffffff,		/* llimit  */
	&_iob[0],		/* fbuf    */
	OUTPUT,			/* fchain  */
	STDLVL,			/* flev    */
	"standard input",	/* pfname  */
	FTEXT|FREAD|SYNC|EOLN,	/* funit   */
	0,			/* fblk    */
	1			/* fsize   */
};
struct iorechd	output = {
	&_outwin,		/* fileptr */
	0,			/* lcount  */
	0x7fffffff,		/* llimit  */
	&_iob[1],		/* fbuf    */
	ERR,			/* fchain  */
	STDLVL,			/* flev    */
	"standard output",	/* pfname  */
	FTEXT | FWRITE | EOFF,	/* funit   */
	1,			/* fblk    */
	1			/* fsize   */
};
struct iorechd	_err = {
	&_errwin,		/* fileptr */
	0,			/* lcount  */
	0x7fffffff,		/* llimit  */
	&_iob[2],		/* fbuf    */
	FILNIL,			/* fchain  */
	STDLVL,			/* flev    */
	"Message file",		/* pfname  */
	FTEXT | FWRITE | EOFF,	/* funit   */
	2,			/* fblk    */
	1			/* fsize   */
};

PCSTART(mode)
int mode;
{
	/*
	 * necessary only on systems which do not initialize
	 * memory to zero
	 */
#ifdef NOTZEROED
	register IOREC	**ip;
	for (ip = &_actfile[3]; ip < &_actfile[MAXFILES]; *ip++ = FILNIL)
		/* empty */;
#endif NOTZEROED

	/*
	 * if running with runtime tests enabled, give more
	 * coherent error messages for FPEs
	 */
	if (mode) {
		signal(SIGFPE, EXCEPT);
	}
}
