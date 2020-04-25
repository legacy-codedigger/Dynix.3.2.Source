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

/* @(#)$Header: host.h 1.3 84/12/18 $ */

int errno;

#define BIT(n) (1<<(n))		/* for the select */

/*
 * control variables for the escape mechanism
 */
int	escape;		/* escape character */
int	newline;	/* newline seen */
int	esc;		/* escape seen */

FILE *runfile;		/* file of commands to execute */
char promptbuf[80];	/* buffer for prompt to wait for */
char *prompt;		/* ptr into prompt buffer */
int fileonly;		/* input coming only from file */

FILE *scriptfp;		/* script file pointer */

char *myname;		/* name of prog, av[0] */

/*
 * global command line flags 
 */
int nflag;		/* flag for no lock file */
int mflag;		/* flag for MMU */
int bflag;		/* script file buffering flag */
int lecho;		/* local echo flag */
int baud;		/* set port to baud rate */

/*
 * remote port buffer
 */
unsigned char  rbuf[BUFSIZ];		/* buffer */
unsigned char *rptr;			/* pointer to it */
int            rcount;			/* count of chars */
int            port; 			/* port file descriptor */
char          *portname;		/* port name */

int	process;	/* process remote file server requests */
