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

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 * static char sccsid[] = "telebit.c	5.1 (Berkeley) 4/30/85";
 */

#ifndef lint
static char rcsid[] = "$Header: telebit.c 1.1 89/10/26 $";
#endif

/*
 * Routines for calling up on a Telebit Modem
 * (based on the old Hayes driver).
 * The modem is expected to be strapped for "echo".
 * Also, the switches enabling the DTR and CD lines
 * must be set correctly. That is, DTR up and CD down.
 * NOTICE:
 * The easy way to hang up a modem is always simply to
 * clear the DTR signal. However, if the +++ sequence
 * (which switches the modem back to local mode) is sent
 * before modem is hung up, removal of the DTR signal
 * has no effect (except that it prevents the modem from
 * recognizing commands).
 * (by Helge Skrivervik, Calma Company, Sunnyvale, CA. 1984) 
 * The original Hayes driver has been modified to support
 * Telebit Trailblazer modems with revision 4 firmware.
 * All that needs to be done to support rev 4 is to send 3 'A's
 * during synchronization.  This should be fixed with rev 5.
 * Another difference is that this driver needs to be able to
 * set the desired transmission speed for telebit modems.  When
 * ACU type is telebit, the transmission speed is set to duplicate
 * the interface speed (br in the remote file).  
 */
/* Another patch is needed for rev 5 firmware.  It appears that
 * the rev 5 firmware will not autobaud on a capital 'AT' but only
 * with a lower case 'at'.  Since rev 4 firmware will also work
 * the lower case commands, change all command sequences to use
 * use lower case.

 * Also the telebit will to autobaud at 19200 on anything but
 * even parity "at".  telebit sync output an even parity at
 * telebit_dialer modified to "x0" before dialing so only
 * status codes checked for will be output. 
 */
/*
 * TODO:
 * It is probably not a good idea to switch the modem
 * state between 'verbose' and terse (status messages).
 * This should be kicked out and we should use verbose 
 * mode only. This would make it consistent with normal
 * interactive use thru the command 'tip dialer'.
 */
#include "tip.h"

#define	min(a,b)	((a < b) ? a : b)

static	int sigALRM();
static	int goodbye();
static	int timeout = 0;
static	jmp_buf timeoutbuf;
static 	char gobble();
#define DUMBUFLEN	40
static char dumbuf[DUMBUFLEN];

#define	DIALING		1
#define IDLE		2
#define CONNECTED	3
#define	FAILED		4
static	int state = IDLE;

/*ARGSUSED*/
telebit_dialer(num, acu)
	register char *num;
	char *acu;
{
	register int connected = 0;
	char dummy;
#ifdef ACULOG
	char line[80];
#endif
	if (telebit_sync() == 0) {	/* make sure we can talk to the modem */
#ifdef ACULOG
		logent(value(HOST), num, "telebit", "can't synch up");
#endif
		return(0);
	}
	if (boolean(value(VERBOSE)))
		printf("\ndialing...");
	fflush(stdout);
	ioctl(FD, TIOCHPCL, 0);
	ioctl(FD, TIOCFLUSH, 0);	/* get rid of garbage */
	write(FD, "atx0v0\r", 7);	/* tell modem to use short stat codes */
	(void) gobble("\r");	/* eat up the echo of above command */
	(void) gobble("\r");	/* eat up the OK response */
	switch (BR) {		/* Set transmission speed */
		case 300:
			write(FD, "ats50=1\r", 8);	/* 300 baud */
			(void) gobble("\r");		/* ignore echo */
			(void) gobble("\r");		/* ignore OK */
			break;
		case 1200:
			write(FD, "ats50=2\r", 8);	/* 1200 baud */
			(void) gobble("\r");		/* ignore echo */
			(void) gobble("\r");		/* ignore OK */
			break;
		case 2400:
			write(FD, "ats50=3\r", 8);	/* 2400 baud */
			(void) gobble("\r");		/* ignore echo */
			(void) gobble("\r");		/* ignore OK */
			break;
		default:
			write(FD, "ats50=255\r", 10);	/* PEP Mode */
			(void) gobble("\r");		/* ignore echo */
			(void) gobble("\r");		/* ignore OK */
			break;
	}
	write(FD, "atdt", 4);	/* send dial command */
	write(FD, num, strlen(num));
	state = DIALING;
	write(FD, "\r", 1);
	connected = 0;
	if (gobble("\r")) {
		dummy = gobble("012345");
		if (dummy != '1' && dummy != '5')
			errr_rep(dummy);
		else
			connected = 1;
	}
	if (connected)
		state = CONNECTED;
	else {
		state = FAILED;
		return (connected);	/* lets get out of here.. */
	}
	ioctl(FD, TIOCFLUSH, 0);
#ifdef ACULOG
	if (timeout) {
		sprintf(line, "%d second dial timeout",
			number(value(DIALTIMEOUT)));
		logent(value(HOST), num, "telebit", line);
	}
#endif
	if (timeout)
		telebit_disconnect();	/* insurance */
	return (connected);
}


telebit_disconnect()
{

	/* first hang up the modem*/
#ifdef DEBUG
	printf("\rtelebit_disconnect....\n\r");
#endif
/*	ioctl(FD, TIOCCDTR, 0);   */
	nap(50);
	ioctl(FD, TIOCSDTR, 0);
	nap(50);
	goodbye();
}

telebit_abort()
{

#ifdef DEBUG
	printf("\rtelebit_abort...\n\r");
#endif
	write(FD, "\r", 1);	/* send anything to abort the call */
	telebit_disconnect();
}

static int
sigALRM()
{

	printf("\07timeout waiting for reply\n\r");
	timeout = 1;
	longjmp(timeoutbuf, 1);
}

static char
gobble(match)
	register char *match;
{
	char c;
	int (*f)();
	int i, status = 0;

	f = signal(SIGALRM, sigALRM);
	timeout = 0;
#ifdef DEBUG
	printf("\ngobble: waiting for %s\n", match);
#endif
	do {
		if (setjmp(timeoutbuf)) {
			signal(SIGALRM, f);
			return (0);
		}
		alarm(number(value(DIALTIMEOUT)));
		read(FD, &c, 1);
		alarm(0);
		c &= 0177;
#ifdef DEBUG
		printf("%c 0x%x ", c, c);
#endif
		for (i = 0; i < strlen(match); i++)
			if (c == match[i])
				status = c;
	} while (status == 0);
	signal(SIGALRM, f);
#ifdef DEBUG
	printf("\n");
#endif
	return (status);
}

        errr_rep(c)
	register char c;
{
	printf("\n\r");
	switch (c) {

	case '0':
		printf("OK");
		break;

	case '1':
		printf("CONNECT");
		break;
	
	case '2':
		printf("RING");
		break;
	
	case '3':
		printf("NO CARRIER");
		break;
	
	case '4':
		printf("ERROR in input");
		break;
	
	case '5':
		printf("CONNECT 1200");
		break;
	
	default:
		printf("Unknown Modem error: %c (0x%x)", c, c);
	}
	printf("\n\r");
	return;
}

/*
 * set modem back to normal verbose status codes.
 */
static int
goodbye()
{
	int len;
	char c;
#ifdef	DEBUG
	int rlen;
        printf("goodbye\n\r"); 
#endif	DEBUG

	ioctl(FD, TIOCFLUSH, &len);	/* get rid of trash */
	if (telebit_sync()) {
		sleep(1);
#ifdef DEBUG
	        printf("\r we synced....\n\r");
#endif
#ifndef DEBUG
		ioctl(FD, TIOCFLUSH, 0);
#endif
		write(FD, "ath0\r", 5);		/* insurance */
#ifndef DEBUG
		c = gobble("03");
		if (c != '0' && c != '3') {
			printf("cannot hang up modem\n\r");
			printf("please use 'tip dialer' to make sure the line is hung up\n\r");
		}
#endif
		sleep(1);
		ioctl(FD, FIONREAD, &len);
#ifdef DEBUG
		printf("goodbye1: len=%d -- ", len);
		rlen = read(FD, dumbuf, min(len, DUMBUFLEN));
		dumbuf[rlen] = '\0';
		printf("read (%d): %s\r\n", rlen, dumbuf);
#endif
		write(FD, "atv1\r", 5);
		sleep(1);
#ifdef DEBUG
		ioctl(FD, FIONREAD, &len);
		printf("goodbye2: len=%d -- ", len);
		rlen = read(FD, dumbuf, min(len, DUMBUFLEN));
		dumbuf[rlen] = '\0';
		printf("read (%d): %s\r\n", rlen, dumbuf);
#endif
	}
#ifdef DEBUG
       else
       		printf ( "no sync...\n\r");
#endif
	ioctl(FD, TIOCFLUSH, 0);	/* clear the input buffer */
	ioctl(FD, TIOCCDTR, 0);		/* clear DTR (insurance) */
#ifdef DEBUG
        printf("its closed...\n\r");
#endif
	close(FD);
}

#define MAXRETRY	5

telebit_sync()
{
	int len, retry = 0;
	int ii;
        char evenparchar[5];

#ifdef DEBUG
	printf("\rat telebit_sync....\n\r");
#endif
/* even parity "a" */
        evenparchar[0]= 0741;
	while (retry++ <= MAXRETRY) {
		write(FD, evenparchar,1);
		sleep(1);
		ioctl(FD, FIONREAD, &len);
		if (len) {
			retry = 0;
#ifdef DEBUG
                        printf("got %d chars back \n\r", len);
#endif
			break;
		}
	}
#ifdef DEBUG
        printf( "retry= %d \n\r", retry);
#endif
	ioctl(FD, TIOCFLUSH, 0);/* Ignore any garbage modem spits back */
/* even parity "at\r" */
        evenparchar[0]= 0741;
        evenparchar[1]='t';
        evenparchar[2]= 0615;
	while (retry++ <= MAXRETRY) {
		write(FD, evenparchar,3);
		sleep(1);
		ioctl(FD, FIONREAD, &len);
#ifdef DEBUG
                printf("telebit_sync: len = %d \n\r", len ) ;
#endif
		if (len) {
			len = read(FD, dumbuf, min(len, DUMBUFLEN));
			for (ii = 0; ii < len; ii++) dumbuf[ii] &= 0x7f;
			if (index(dumbuf, '0') || 
		   	(index(dumbuf, 'O') && index(dumbuf, 'K')))
				return(1);
#ifdef DEBUG
			dumbuf[len] = '\0';
			printf("telebit_sync: (\"%s\") %d\n\r", dumbuf, retry);
#endif
		}
#ifdef DEBUG
                printf("len = 0, clearing DTR");
#endif
/*		ioctl(FD, TIOCCDTR, 0);  */
		nap(50);
		ioctl(FD, TIOCSDTR, 0);
		nap(50);
	}
	printf("Cannot synchronize with telebit...\n\r");
	return(0);
}
