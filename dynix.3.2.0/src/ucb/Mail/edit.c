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
static char *rcsid = "$Header: edit.c 2.3 87/04/02 $";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)edit.c	5.2 (Berkeley) 6/21/85";
#endif not lint

#include "rcv.h"
#include <stdio.h>
#include <sys/stat.h>

/*
 * Mail -- a mail program
 *
 * Perform message editing functions.
 */

/*
 * Edit a message list.
 */

editor(msgvec)
	int *msgvec;
{
	char *edname;

	if ((edname = value("EDITOR")) == NOSTR)
		edname = EDITOR;
	return(edit1(msgvec, edname));
}

/*
 * Invoke the visual editor on a message list.
 */

visual(msgvec)
	int *msgvec;
{
	char *edname;

	if ((edname = value("VISUAL")) == NOSTR)
		edname = VISUAL;
	return(edit1(msgvec, edname));
}

/*
 * Edit a message by writing the message into a funnily-named file
 * (which should not exist) and forking an editor on it.
 * We get the editor from the stuff above.
 */

edit1(msgvec, ed)
	int *msgvec;
	char *ed;
{
	register char *cp, *cp2;
	register int c;
	register int newline;
	int *ip, pid, mesg, lines;
	long ms;
	int (*sigint)(), (*sigquit)(), (*sigtstp)();
	FILE *ibuf, *obuf;
	char edname[15], nbuf[10];
	struct message *mp;
	extern char tempEdit[];
	off_t fsize(), size;
	struct stat statb;
	long modtime;

	/*
	 * Set signals; locate editor.
	 */

	sigint = sigset(SIGINT, SIG_IGN);
	sigquit = sigset(SIGQUIT, SIG_IGN);
# ifdef VMUNIX
	sigtstp = sigset(SIGTSTP, SIG_IGN);
# endif VMUNIX

	/*
	 * Deal with each message to be edited . . .
	 */

	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		mp->m_flag |= MODIFY;

		/*
		 * Make up a name for the edit file of the
		 * form "Message%d" and make sure it doesn't
		 * already exist.
		 */

		cp = &nbuf[10];
		*--cp = 0;
		while (mesg) {
			*--cp = mesg % 10 + '0';
			mesg /= 10;
		}
		cp2 = copy("Message", edname);
		while (*cp2++ = *cp++)
			;
		if (!access(edname, 2)) {
			printf("%s: file exists\n", edname);
			goto out;
		}

		/*
		 * Copy the message into the edit file.
		 */

		close(creat(edname, 0600));
		if ((obuf = fopen(edname, "w")) == NULL) {
			perror(edname);
			goto out;
		}
		if (send(mp, obuf, 0) < 0) {
			perror(edname);
			fclose(obuf);
			remove(edname);
			goto out;
		}
		fflush(obuf);
		if (ferror(obuf)) {
			remove(edname);
			fclose(obuf);
			goto out;
		}
		fclose(obuf);

		/*
		 * If we are in read only mode, make the
		 * temporary message file readonly as well.
		 */

		if (readonly)
			chmod(edname, 0400);

		/*
		 * Fork/execlp the editor on the edit file.
		 */

# ifdef VMUNIX
		sigsys(SIGTSTP, SIG_DFL);
# endif VMUNIX
		if (stat(edname, &statb) < 0)
			modtime = 0;
		modtime = statb.st_mtime;
		pid = vfork();
		if (pid == -1) {
			perror("fork");
			remove(edname);
			goto out;
		}
		if (pid == 0) {
			sigchild();
			if (sigint != SIG_IGN)
				sigsys(SIGINT, SIG_DFL);
			if (sigquit != SIG_IGN)
				sigsys(SIGQUIT, SIG_DFL);
			execlp(ed, ed, edname, (char *)0);
			perror(ed);
			_exit(1);
		}
		while (wait(&mesg) != pid)
			;
# ifdef VMUNIX
		sigsys(SIGTSTP, SIG_IGN);
# endif VMUNIX

		/*
		 * If in read only mode, just remove the editor
		 * temporary and return.
		 */

		if (readonly) {
			remove(edname);
			continue;
		}

		/*
		 * Now copy the message to the end of the
		 * temp file.
		 */

		if (stat(edname, &statb) < 0) {
			perror(edname);
			goto out;
		}
		if (modtime == statb.st_mtime) {
			remove(edname);
			goto out;
		}
		if ((ibuf = fopen(edname, "r")) == NULL) {
			perror(edname);
			remove(edname);
			goto out;
		}
		remove(edname);
		fflush(otf);
		size = fsize(otf);
		mp->m_block = blockof(size);
		mp->m_offset = offsetof(size);
		fseek(otf, (long) 0, 2);
		ms = 0L;
		lines = 0;
		newline = 0;
		while ((c = getc(ibuf)) != EOF) {
			if (c == '\n') {
				lines++;
				newline++;
			}
			else
				newline = 0;
			putc(c, otf);
			if (ferror(otf))
				break;
			ms++;
		}
		while (newline < 2) {
			putc('\n', otf);
			if (ferror(otf))
				break;
			ms++;
			lines++;
			newline++;
		}
		mp->m_size = ms;
		mp->m_lines = lines;
		if (ferror(otf))
			perror("/tmp");
		fclose(ibuf);
	}

	/*
	 * Restore signals and return.
	 */

out:
	sigset(SIGINT, sigint);
	sigset(SIGQUIT, sigquit);
# ifdef VMUNIX
	sigset(SIGTSTP, sigtstp);
# endif VMUNIX
}
