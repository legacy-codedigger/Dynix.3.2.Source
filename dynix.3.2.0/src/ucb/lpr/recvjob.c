/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
static char rcsid[] = "$Header: recvjob.c 2.2 1991/05/10 23:22:37 $";
#endif

/*
 * Receive printer jobs from the network, queue them and
 * start the printer daemon.
 */

#include "lp.h"

static char    tfname[40];	/* tmp copy of cf before linking */
static char    *dfname;		/* data files */

recvjob()
{
	struct stat stb;
	char *bp = pbuf;
	int status;

	/*
	 * Perform lookup for printer name or abbreviation
	 */
	if ((status = pgetent(line, printer)) < 0)
		fatal("cannot open printer description file");
	else if (status == 0)
		fatal("unknown printer");
	if ((LF = pgetstr("lf", &bp)) == NULL)
		LF = DEFLOGF;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;

	(void) close(2);
	(void) open(LF, O_WRONLY|O_APPEND);
	if (chdir(SD) < 0)
		fatal("cannot chdir to %s", SD);
	if (stat(LO, &stb) == 0 && (stb.st_mode & 010)) {
		/* queue is disabled */
		putchar('\1');		/* return error code */
		exit(1);
	}

	if (readjob())
		printjob();
}

char	*sp = "";
#define ack()	(void) write(1, sp, 1);

/*
 * Read printer jobs sent by lpd and copy them to the spooling directory.
 * Return the number of jobs successfully transfered.
 */
static
readjob(printer)
	char *printer;
{
	register int size, nfiles;
	register char *cp;

	ack();
	nfiles = 0;
	for (;;) {
		/*
		 * Read a command to tell us what to do
		 */
		cp = line;
		do {
			if (cp > &line[(sizeof line) - 1]) {
				fatal("buffer overflow");
			}
			if ((size = read(1, cp, 1)) != 1) {
				if (size < 0)
					fatal("Lost connection");
				return(nfiles);
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = line;
		switch (*cp++) {
		case '\1':	/* cleanup because data sent was bad */
			cleanup();
			continue;

		case '\2':	/* read cf file */
			size = 0;
			while (*cp >= '0' && *cp <= '9')
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			if (badpath(cp)) {
				fatal("refusing to create %s", cp);
			}
			strcpy(tfname, cp);
			tfname[0] = 't';
			if (!readfile(tfname, size, 1)) {
				cleanup();
				continue;
			}
			if (link(tfname, cp) < 0)
				fatal("cannot rename %s", tfname);
			(void) unlink(tfname);
			tfname[0] = '\0';
			nfiles++;
			continue;

		case '\3':	/* read df file */
			size = 0;
			while (*cp >= '0' && *cp <= '9')
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			/*
			 * Refuse to write a data file with a control
			 * file name.  This is ugly, but necessary if
			 * we are to reliably place "Y" commands in
			 * control files from the network.  See comments
			 * in readfile() concerning this.
			 */
			if ((cp[0] == 'c' || cp[0] == 't') && cp[1] == 'f') {
				fatal("refusing to create %s as data file", cp);
			}
			if (badpath(cp)) {
				fatal("refusing to create %s", cp);
			}
			(void) readfile(dfname = cp, size, 0);
			continue;
		}
		fatal("protocol screwup");
	}
}

/*
 * Read files send by lpd and copy them to the spooling directory.
 */
static
readfile(file, size, cf)
	char *file;
	int size;
	int cf;		/* are we writing a cf file? */
{
	register char *cp;
	char buf[BUFSIZ];
	register int i, j, amt;
	int fd, err;
	struct stat stbuf;

	/*
	 * If lstat fails, we can open the file.  If it succeeds,
	 * we have either a file or a link.  In either case we don't
	 * want to open since we could be writing on something
	 * outside of the spooldir.  Note that there is a race here between
	 * the lstat and open syscalls, but it is sufficiently
	 * small as to be safe.  Also, if the window is actually
	 * caught, the only thing we can do is write on something
	 * that doesn't exist (O_EXCL).
	 */
	if (lstat(file, &stbuf) < 0) {
		fd = open(file, O_WRONLY|O_CREAT|O_EXCL, FILMOD);
	} else {
		fatal("%s already exists", file);
	}
	if (fd < 0)
		fatal("cannot create %s", file);
	ack();
	err = 0;
	/*
	 * If we are writing a control file, add a line at the
	 * beginning that says the file came from the network.
	 * When printjob processes this, it will perform extra
	 * checks on unlink commands.  This prevents someone
	 * that we trust from telling us to unlink things
	 * outside the spool directory.
	 */
	if (cf) {
		sprintf(buf, "Y\n");
		if (write(fd, buf, strlen(buf)) != strlen(buf)) {
			fatal("%s: write error", file);
		}
	}
	for (i = 0; i < size; i += BUFSIZ) {
		amt = BUFSIZ;
		cp = buf;
		if (i + amt > size)
			amt = size - i;
		do {
			j = read(1, cp, amt);
			if (j <= 0)
				fatal("Lost connection");
			amt -= j;
			cp += j;
		} while (amt > 0);
		amt = BUFSIZ;
		if (i + amt > size)
			amt = size - i;
		if (write(fd, buf, amt) != amt) {
			err++;
			break;
		}
	}
	(void) close(fd);
	if (err)
		fatal("%s: write error", file);
	if (noresponse()) {		/* file sent had bad data in it */
		(void) unlink(file);
		return(0);
	}
	ack();
	return(1);
}

static
noresponse()
{
	char resp;

	if (read(1, &resp, 1) != 1)
		fatal("Lost connection");
	if (resp == '\0')
		return(0);
	return(1);
}

/*
 * Remove all the files associated with the current job being transfered.
 */
static
cleanup()
{
	if (tfname[0])
		(void) unlink(tfname);
	if (dfname)
		do {
			do
				(void) unlink(dfname);
			while (dfname[2]-- != 'A');
			dfname[2] = 'z';
		} while (dfname[0]-- != 'd');
	dfname = (char *)0;
}

static
fatal(msg, a1)
	char *msg;
{
	cleanup();
	log(msg, a1);
	putchar('\1');		/* return error code */
	exit(1);
}
