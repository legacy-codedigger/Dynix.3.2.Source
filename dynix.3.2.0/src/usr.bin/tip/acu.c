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
static char rcsid[] = "$Header: acu.c 2.0 86/01/28 $";
#endif

#include "tip.h"

static acu_t *acu = NOACU;
static int conflag;
static int acuabort();
static acu_t *acutype();
static jmp_buf jmpbuf;
/*
 * Establish connection for tip
 *
 * If DU is true, we should dial an ACU whose type is AT.
 * The phone numbers are in PN, and the call unit is in CU.
 *
 * If the PN is an '@', then we consult the PHONES file for
 *   the phone numbers.  This file is /etc/phones, unless overriden
 *   by an exported shell variable.
 *
 * The data base files must be in the format:
 *	host-name[ \t]*phone-number
 *   with the possibility of multiple phone numbers
 *   for a single host acting as a rotary (in the order
 *   found in the file).
 */
char *
connect()
{
	register char *cp = PN;
	char *phnum, string[256];
	FILE *fd;
	int tried = 0;

	if (!DU) {		/* regular connect message */
		if (CM != NOSTR) {
			cp = CM;
			pwrite(FD, cp, size(CM));
		}
		return (NOSTR);
	}
	/*
	 * @ =>'s use data base in PHONES environment variable
	 *        otherwise, use /etc/phones
	 */
	signal(SIGINT, acuabort);
	signal(SIGQUIT, acuabort);
	if (setjmp(jmpbuf)) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		printf("\ncall aborted\n");
		logent(value(HOST), "", "", "call aborted");
		if (acu != NOACU) {
			boolean(value(VERBOSE)) = FALSE;
			if (conflag)
				disconnect(NOSTR);
			else
				(*acu->acu_abort)();
		}
		delock(uucplock);
		exit(1);
	}
	if ((acu = acutype(AT)) == NOACU)
		return ("unknown ACU type");
	if (*cp != '@') {
		while (*cp) {
			for (phnum = cp; any(*cp, "0123456789-*=K&%,"); cp++)
				;
			if (*cp)
				*cp++ = '\0';
			
			if (conflag = (*acu->acu_dialer)(phnum, CU)) {
				logent(value(HOST), phnum, acu->acu_name,
					"call completed");
				return (NOSTR);
			} else
				logent(value(HOST), phnum, acu->acu_name,
					"call failed");
			tried++;
		}
	} else {
		if ((fd = fopen(PH, "r")) == NOFILE) {
			printf("%s: ", PH);
			return ("can't open phone number file");
		}
		while (fgets(string, sizeof(string), fd) != NOSTR) {
			for (cp = string; !any(*cp, " \t\n"); cp++)
				;
			if (*cp == '\n') {
				fclose(fd);
				return ("unrecognizable host name");
			}
			*cp++ = '\0';
			if (strcmp(string, value(HOST)))
				continue;
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\n') {
				fclose(fd);
				return ("missing phone number");
			}
			for (phnum = cp; any(*cp, "0123456789-*=K&%,"); cp++)
				;
			*cp = '\0';
			
			if (conflag = (*acu->acu_dialer)(phnum, CU)) {
				fclose(fd);
				logent(value(HOST), phnum, acu->acu_name,
					"call completed");
				return (NOSTR);
			} else
				logent(value(HOST), phnum, acu->acu_name,
					"call failed");
			tried++;
		}
		fclose(fd);
	}
	if (!tried)
		logent(value(HOST), "", acu->acu_name, "missing phone number");
	else
		(*acu->acu_abort)();
	return (tried ? "call failed" : "missing phone number");
}

disconnect(reason)
	char *reason;
{
	if (!conflag)
		return;
	if (reason == NOSTR) {
		logent(value(HOST), "", acu->acu_name, "call terminated");
		if (boolean(value(VERBOSE)))
			printf("\r\ndisconnecting...");
	} else 
		logent(value(HOST), "", acu->acu_name, reason);
	(*acu->acu_disconnect)();
}

static int
acuabort(s)
{
	signal(s, SIG_IGN);
	longjmp(jmpbuf, 1);
}

static acu_t *
acutype(s)
	register char *s;
{
	register acu_t *p;
	extern acu_t acutable[];

	for (p = acutable; p->acu_name != '\0'; p++)
		if (!strcmp(s, p->acu_name))
			return (p);
	return (NOACU);
}
