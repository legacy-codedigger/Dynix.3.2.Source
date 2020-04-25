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

/* @(#)$Header: parse.c 1.5 86/03/12 $ */

#include <stdio.h>
#include <signal.h>
#include <sgtty.h>
#include <sys/wait.h>
#include "remote.h"
#include "host.h"

extern struct sgttyb otty;
extern struct sgttyb ctty;

extern int shellflag;

extern int maxpacket;
extern FILE *packetdebug;
extern unsigned char esc_list[];

extern char *rindex();
extern char *getenv();

parse(c)
char c;
{
	register char *p, *q;
	char *cp, *shell;
	int retval, sigint;
	int textdata, bss;
	char getbuf[100];

#define START	if (esc) {\
			esc = 0;\
			putchar(escape)

#define END		newline++;\
			break;\
		} else\
			goto nonewline

	if (c == escape) {
		if (newline) {
			newline = 0;
			esc++;
			return;
		} else {
			esc = 0;
			goto nonewline;
		}
	}
	switch (c) {
	case '\r':		/* line terminator */
	case '\n':
		newline++;
		esc = 0;
		goto sendit;
	case '?':
		START;
		help();
		END;
	case '.':
		START;
		printf("\r\n[EOT]\r\n");
		quit(0);
		END;
	case '|':
		START;
		printf("| not yet\r\n");
		fflush(stdout);
		END;
	case '!':
		START;
		printf("!\r\n");
		fflush(stdout);
		ioctl(0, TIOCSETP, &otty);
		shell = getenv("SHELL");
		if (shell == 0)
			shell = "/bin/sh";
		if ((cp = rindex(shell, '/')) == NULL)
			cp = shell;
		else
			cp++;
		sigint = (int)signal(SIGINT, SIG_IGN);
		shellflag = 1;
		if (!(retval = vfork())) {
			execl(shell, cp, 0);
			printf("\r\ncan't execl!\r\n");
			exit(1);
		}
		if (retval < 0) {
			printf("sorry\n");
			fflush(stdout);
		} else {
			wait(&retval);
			signal(SIGINT, sigint);
		}
		shellflag = 0;
		ioctl(0, TIOCSETP, &ctty);
		printf("!\r\n");
		END;
	case '#':		/* send break */
		START;
		putchar('#');
		ioctl(port, TIOCSBRK, 0);
		sleep(1);
		ioctl(port, TIOCCBRK, 0);
		END;
	case '>':		/* send file */
		START;
		printf("> file: ");
		fflush(stdout);
		ioctl(0, TIOCSETP, &otty);
		gets(getbuf);
		p = getbuf;
		while (*p == ' ')
			p++;
		sendfile(p);
		ioctl(0, TIOCSETP, &ctty);
		END;
	case 'b':
		START;
		if (bflag) {
			bflag = 0;
			printf("buffer on\r\n");
			fflush(stdout);
		} else {
			bflag = 1;
			printf("buffer off\r\n");
			fflush(stdout);
			if (scriptfp)
				fflush(scriptfp);
		}
		END;
	case 'd':
		START;
		if (packetdebug) {
			fclose(packetdebug);
			packetdebug = 0;
			printf("packet debug off\r\n");
		} else {
			packetdebug = fopen("PACKETS", "a");
			if (!packetdebug)
				printf("can't open PACKETS\r\n");
			else
				printf("packet debug on\r\n");
		}
		fflush(stdout);
		END;
	case 'e':		/* local echo toggle */
		START;
		if (lecho) {
			lecho = 0;
			printf("echo off\r\n");
			fflush(stdout);
		} else {
			lecho = 1;
			printf("echo on\r\n");
			fflush(stdout);
		}
		END;
	case 'f':		/* runcom file */
		START;
		printf("cmdfile: ");
		fflush(stdout);
		ioctl(0, TIOCSETP, &otty);
		gets(getbuf);
		p = getbuf;
		while (*p == ' ')
			p++;
		ioctl(0, TIOCSETP, &ctty);
		if (runfile != NULL)
			fclose(runfile);
		if ((runfile=fopen(p, "r"))==0) {
			printf("\r\n%s: cannot open: %s\r\n", myname, p);
			fflush(stdout);
		}
		END;
#ifdef DROPPED_IN_V10
	case 'l':		/* down load file */
		START;
		printf("load file: ");
		fflush(stdout);
		ioctl(0, TIOCSETP, &otty);
		gets(getbuf);
		p = getbuf;
		while (*p == ' ')
			p++;
		download(p, 0, &textdata, &bss);
		ioctl(0, TIOCSETP, &ctty);
		END;
#endif
	case 'p':
		START;
		ioctl(0, TIOCSETP, &otty);
	pks:
		printf("packet size: ");
		fflush(stdout);
		gets(getbuf);
		sscanf(getbuf, "%d", &maxpacket);
		if (maxpacket < 1 || maxpacket > MAXPACKET) {
			printf("allowable range is: 1 to %d\n", MAXPACKET);
			fflush(stdout);
			goto pks;
		}
		fflush(stdout);
		ioctl(0, TIOCSETP, &ctty);
		END;
	case 'r':		/* script file toggle */
		START;
		if (process) {
			process = 0;
			printf("remote process: off\r\n");
			fflush(stdout);
		} else {
			process = 1;
			printf("remote process: on\r\n");
			fflush(stdout);
		}
		END;
	case 's':		/* script file toggle */
		START;
		if (scriptfp) {
			fclose(scriptfp);
			scriptfp = 0;
			printf("script: off\r\n");
			fflush(stdout);
		} else {
			printf("script: ");
			fflush(stdout);
			ioctl(0, TIOCSETP, &otty);
			p = getbuf;
			gets(p);
			while (*p == ' ')
				p++;
			ioctl(0, TIOCSETP, &ctty);
			if ((scriptfp=fopen(p, "a"))==0) {
				printf("\r\n%s: cannot open: %s\r\n", myname,p);
				fflush(stdout);
			}
		}
		END;
#ifdef DROPPED_IN_V10
	case 'w':		/* write bootstrap */
		START;
		printf("write bootstrap: ");
		fflush(stdout);
		ioctl(0, TIOCSETP, &otty);
		p = getbuf;
		gets(p);
		while (*p == ' ')
			p++;
		q = p;
		while (*q && *q != ' ')
			q++;
		if (*q)
			*q++ = 0;
		while (*q == ' ')
			q++;
		bootstrap(p, *q ? 1 : 0);	/* *q indicates boot or test */
		ioctl(0, TIOCSETP, &ctty);
		END;
#endif
	case 'v':		/* virtual escape */
		START;
		printf("virtual escape: '");
		p = (char *)esc_list;
		while (*p) {
			if (*p < ' ')
				printf("^%c", *p++ | 0x40);
			else if (*p == '\177')
				{ printf("^?"); p++; }
			else
				putchar(*p++);
		}
		printf("' ");
		fflush(stdout);
		ioctl(0, TIOCSETP, &otty);
		p = getbuf;
		gets(p);
		while (*p == ' ')
			p++;
		cp = (char *)esc_list;
		while (*cp)
			*cp++;
		while (*p)
			*cp++ = *p++;
		ioctl(0, TIOCSETP, &ctty);
		END;
#ifdef DROPPED_IN_V10
	case 'z':		/* zero bootstrap */
		START;
		printf("zero bootstraps\r\n");
		fflush(stdout);
		bootzero();
		END;
#endif
	default:
		esc = 0;
	nonewline:
		newline = 0;
	sendit:
		if (lecho) {
			if (scriptfp)
				putc(c, scriptfp);
			putchar(c);
			if (c == '\r')
				putchar('\n');
			if (c == '\n')
				putchar('\r');
			fflush(stdout);
		}
		write(port, &c, 1);
		break;
	}
}
