/* @(#)$Copyright:	$
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

/* @(#)$Header: bootstrap.c 1.7 86/03/12 $
 *
 * This routine puts a bootstrap or test into the linked list.
 * It always treats it as a bootstrap (since the EPROM firmware doesn't
 * really care).
 */

#include <stdio.h>
#include <a.out.h>
#include <data.h>
#include "host.h"

bootstrap(name, type)
char *name;
int type;
{
	PERMPTR;		/* this gets the addresses of data */
	unsigned addrlast, last, ramboot;
	int textdata, bss;
	short lname;
	char tbuf[80];

	(void)write(port, "\r", 1);
	getprompt();		/* sync with the monitor */
	/*
	 * get the address of the link pointer, and its value
	 */
	addrlast = (int)&prm->ramboot;
	(void)sprintf(tbuf, "@pmd%x\r", addrlast);
	(void)write(port, tbuf, strlen(tbuf));
	last = getval();
	getprompt();
	/*
	 * If there are already boots there, link this one into the
	 * list, otherwise, start a new list.
	 */
	if (last) {
		printf("list=");
		while (last) {
			addrlast = last;
			(void)sprintf(tbuf, "@pmw%x\r", last+4);
			(void)write(port, tbuf, strlen(tbuf));
			lname = getval();
			getprompt();
			printf(" %c%c", lname & 0xff, lname >> 8);

			(void)sprintf(tbuf, "@pmd%x\r", last);
			(void)write(port, tbuf, strlen(tbuf));
			last = getval();
			getprompt();
		}
		printf("\n");
		(void)sprintf(tbuf, "@pmd%x\r", addrlast + 8);
		(void)write(port, tbuf, strlen(tbuf));
		last = getval();
		getprompt();
		ramboot = addrlast + sizeof (struct bootstrap) + last;
	} else
		/*
		 * NOTE: This assumes that the vax and ns16k C compilers
		 * build structures exactly the same.  If they don't
		 * you would have to use (and trust) the "hm" command,
		 * but it could get quite confused if anything goes
		 * wrong!
		 */
		ramboot = (int)prm + sizeof (struct prm);
	lname = strlen(name) - 2;
	printf("load %s at %x\n", &name[lname], ramboot);
	(void)fflush(stdout);

	download(name, ramboot + sizeof (struct bootstrap), &textdata, &bss);
	if (bss < 0)
		goto err;
	/*
	 * write struct bootstrap header
	 */
	(void)sprintf(tbuf, "@cmd%x=0\r", ramboot);	/* next pointer */
	(void)write(port, tbuf, strlen(tbuf));
	getprompt();
	(void)sprintf(tbuf, "@cmd%x=%x\r", ramboot+4,	/* name and type */
		name[lname] | (name[lname+1] << 8) | (type << 16));
	(void)write(port, tbuf, strlen(tbuf));
	getprompt();
	(void)sprintf(tbuf, "@cmd%x=%x\r", ramboot+8, textdata); /* size field */
	(void)write(port, tbuf, strlen(tbuf));
	getprompt();
	(void)sprintf(tbuf, "@cmd%x=%x\r", ramboot+12, bss); /* bss field */
	(void)write(port, tbuf, strlen(tbuf));
	getprompt();
	/*
	 * update linked list to include this one
	 */
	(void)sprintf(tbuf, "@cmd%x=%x\r", addrlast, ramboot);
	(void)write(port, tbuf, strlen(tbuf));
	getprompt();
	/*
	 * issue a couple of commands which will cause the checksum on
	 * non-volatile ram to be restablished.
	 */
	(void)write(port, "@rm\r", 4);
	(void)sprintf(tbuf, "@wm=%d\r", getval());
	getprompt();
	(void)write(port, tbuf, strlen(tbuf));
	getprompt();
err:
	(void)write(port, "\r", 1);
	(void)fflush(stdout);
}

int
getval()
{
	char tbuf[80];
	register int i;
	unsigned val;

	i = 0;
	do {
		(void)read(port, &tbuf[i], 1);
	} while (tbuf[i++] != '\r');
	tbuf[i] = 0;
	(void)sscanf(tbuf, " =%x", &val);
	return(val);
}

bootzero()
{
	PERMPTR;		/* this gets the addresses of data */
	char tbuf[80];

	(void)write(port, "\r", 1);
	getprompt();		/* sync with the monitor */
	/*
	 * zero the list
	 */
	(void)sprintf(tbuf, "@cmd%x=0\r", &prm->ramboot);
	(void)write(port, tbuf, strlen(tbuf));
	getprompt();

	/*
	 * issue a couple of commands which will cause the checksum on
	 * non-volatile ram to be restablished.
	 */
	(void)write(port, "@rm\r", 4);
	(void)sprintf(tbuf, "@wm=%d\r", getval());
	getprompt();
	(void)write(port, tbuf, strlen(tbuf));
	getprompt();
	(void)write(port, "\r", 1);
	(void)fflush(stdout);
}
