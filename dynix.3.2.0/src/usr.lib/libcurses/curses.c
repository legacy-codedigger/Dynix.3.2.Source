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

/* $Header: curses.c 2.0 86/01/28 $
 *
 * Define global variables
 */
# include	"curses.h"

bool	_echoit		= TRUE,	/* set if stty indicates ECHO		*/
	_rawmode	= FALSE,/* set if stty indicates RAW mode	*/
	My_term		= FALSE,/* set if user specifies terminal type	*/
	_endwin		= FALSE;/* set if endwin has been called	*/

char	ttytype[10],		/* long name of tty			*/
	*Def_term	= "unknown";	/* default terminal type	*/

int	_tty_ch		= 1,	/* file channel which is a tty		*/
	LINES,			/* number of lines allowed on screen	*/
	COLS,			/* number of columns allowed on screen	*/
	_res_flg;		/* sgtty flags for reseting later	*/

WINDOW	*stdscr		= NULL,
	*curscr		= NULL;

# ifdef DEBUG
FILE	*outf;			/* debug output file			*/
# endif

SGTTY	_tty;			/* tty modes				*/

bool	AM, BS, CA, DA, DB, EO, GT, HZ, IN, MI, MS, NC, OS, UL, XN,
	NONL, UPPERCASE, normtty, _pfast;

char	*AL, *BC, *BT, *CD, *CE, *CL, *CM, *CR, *DC, *DL, *DM, *DO,
	*ED, *EI, *HO, *IC, *IM, *IP, *LL, *MA, *ND, *NL, *SE, *SF,
	*SO, *SR, *TA, *TE, *TI, *UC, *UE, *UP, *US, *VB, *VE, *VS, PC;