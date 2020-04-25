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

/* $Header: condevs.h 1.2 87/05/27 $ */

/*	condevs.h	4.6	86/02/13	*/

#include "uucp.h"
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <sgtty.h>
#ifdef VMSDTR	/* Modem control on vms(works dtr) */
#include <eunice/eunice.h>
#define TT$M_MODEM	0x00200000 /* These should be in a '.h' somewhere */
#define SS$_NORMAL	0x00000001
#define IO$_SETMODE	0x00000023
#define IO$_SENSEMODE	0x00000027
#endif

extern char devSel[];	/* name to pass to delock() in close */
extern int errno, next_fd;
extern jmp_buf Sjbuf;
extern int alarmtr();
int nulldev(), nodev(), Acuopn(), diropn(), dircls();

#ifdef DATAKIT
int dkopn();
#endif DATAKIT

#ifdef DN11
int dnopn(), dncls();
#endif DN11

#ifdef HAYES
int hyspopn(), hystopn(), hyscls();
#endif HAYES

#ifdef HAYES2400
int hyspopn24(), hystopn24(), hyscls24();
#endif HAYES2400

#ifdef HAYESQ
int hysqopn(), hysqcls();  /* a version of hayes that doesn't use ret codes */
#endif HAYESQ

#ifdef NOVATION
int novopn(), novcls();
#endif NOVATION

#ifdef CDS224
int cdsopn224(), cdscls224();
#endif CDs224

#ifdef DF02
int df2opn(), df2cls();
#endif DF02

#ifdef DF112
int df12popn(), df12topn(), df12cls();
#endif DF112

#ifdef PNET
int pnetopn();
#endif PNET

#ifdef VENTEL
int ventopn(), ventcls();
#endif VENTEL

#ifdef PENRIL
int penopn(), pencls();
#endif PENRIL

#ifdef	UNETTCP
#define TO_ACTIVE	0
int unetopn(), unetcls();
#endif UNETTCP

#ifdef BSDTCP
int bsdtcpopn(), bsdtcpcls();
#endif BSDTCP

#ifdef VADIC
int vadopn(), vadcls();
#endif VADIC

#ifdef VA212
int va212opn(), va212cls();
#endif VA212

#ifdef VA811S
int va811opn(), va811cls();
#endif VA811S

#ifdef VA820
int va820opn(), va820cls();
#endif VA820

#ifdef	RVMACS
int rvmacsopn(), rvmacscls();
#endif

#ifdef	VMACS
int vmacsopn(), vmacscls();
#endif

#ifdef MICOM
int micopn(), miccls();
#endif MICOM

#ifdef SYTEK
int sykopn(), sykcls();
#endif SYTEK

#ifdef ATT2224
int attopn(), attcls();
#endif	ATT2224

