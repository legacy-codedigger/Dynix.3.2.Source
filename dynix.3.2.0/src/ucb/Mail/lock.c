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

#ifndef	lint
static char *rcsid = "$Header: lock.c 1.2 90/04/05 $";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

extern int errno;
extern char *malloc();

static char devname[128];	/* Lock file name */
static int hold_lock = 0;	/* != 0, means we hold a locked file */
static char *mfilename = NULL;	/* Name of file we're locking */
static int mfilestate;		/* Flag whether mailbox or edit file */

/*
 * lock.c--routines to implement mutual-exclusion locking of the mail files.
 *
 * These routines try to solve the problems of accidentally hosing your
 * mail file by running multiple sessions.  The second and successive
 * sessions are allowed to start, but are run in read-only mode.
 */

/*
 * Flag a file as busy.  This routine assumes that flock() has been used by
 * the caller to provide mutual exclusion on the corresponding file.  It then
 * creates a lock status file in /tmp based on the device/inode, and stores
 * our existence in it.  When a second or more mail session starts on the same
 * file, we don't apply a lock, reporting the collision instead.  This then
 * allows the upper levels to process the file read-only.
 */
flag_busy(fname, isedit)
	char *fname;
	int isedit;
{
	struct stat sb;
	char buf[128];
	int pid, fd;

	/*
	 * Get rid of any old cache of the filename
	 */
	if (mfilename)
		free(mfilename);

	/*
	 * Stash the new file name
	 */
	mfilename = malloc(strlen(fname)+1);
	strcpy(mfilename, fname);
	mfilestate = isedit;

	/*
	 * Release a previous lock if we're switching to another file
	 */
	if (hold_lock) {
		unlink(devname);
		hold_lock = 0;
	}

	/*
	 * Look up st_dev/st_ino to uniquely identify the file
	 */
	if (stat(fname, &sb)) {
		perror(fname);
		panic("fstat failed on file");
	}

	/* Create file */
	sprintf(devname, "/usr/tmp/M%x,%x", sb.st_dev, sb.st_ino);
	if ((fd = open(devname, O_RDWR|O_CREAT, 0666)) < 0) {
#ifdef DEBUG
		perror(devname);
		fprintf(stderr,
		 "Can't apply soft lock to file, opening read-only instead.\n");
#endif
		return(0);
	}
	fchmod(fd, 0666);

	/* Read first word (process id) */
	if (read(fd, &pid, sizeof(pid)) == sizeof(pid)) {
		if ((kill(pid, 0) < 0) && (errno == ESRCH)) {
#ifdef DEBUG
			fprintf(stderr,
			  "[Clearing old lock of mailbox, pid %d]\n", pid);
#else
			/* Usually not a big deal */
#endif
		} else {
			read(fd, buf, sizeof(buf)-1);
			buf[sizeof(buf)-1] = '\0';
			fprintf(stderr,
			  "[Mailbox is in use by %s (pid %d)]\n", buf, pid);
			close(fd);
			return(1);
		}
	}

	/* We get the lock, so clear any old junk and write us in */
	lseek(fd, 0L, 0);
	ftruncate(fd, 0L);
	pid = getpid();
	write(fd, &pid, sizeof(pid));
	if (!getenv("USER"))
		strcpy(buf, "<unknown>");
	else
		strcpy(buf, getenv("USER"));
	write(fd, buf, strlen(buf)+1);
	close(fd);
	hold_lock = 1;
	return(0);
}

/*
 * Release the file for use by others
 */
void
flag_free()
{
	if (!hold_lock)
		return;
	unlink(devname);
}

/*
 * A user has requested that we override the lock--do so
 */
u_unlock()
{
	extern int readonly;

	if (!readonly) {
		printf("There is no lock to clear.\n");
		return;
	}
	printf("Clearing lock on %s\n", mfilename);
	(void) unlink(devname);
	setfile(mfilename, mfilestate);
}
