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
static char rcsid[] = "$Header: atrun.c 2.0 86/01/28 $";
#endif
/*
 * Run programs submitted by at.
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pwd.h>

# define ATDIR "/usr/spool/at"
# define PDIR	"past"
# define LASTF "/usr/spool/at/lasttimedone"

int	nowtime;
int	nowdate;
int	nowyear;

main(argc, argv)
char **argv;
{
	int tt, day, year, uniq;
	struct direct *dirent;
	DIR *dirp;

	chdir(ATDIR);
	makenowtime();
	if ((dirp = opendir(".")) == NULL) {
		fprintf(stderr, "Cannot read at directory\n");
		exit(1);
	}
	while ((dirent = readdir(dirp)) != NULL) {
		if (dirent->d_ino==0)
			continue;
		if (sscanf(dirent->d_name, "%2d.%3d.%4d.%2d", &year, &day, &tt, &uniq) != 4)
			continue;
		if (nowyear < year)
			continue;
		if (nowyear==year && nowdate < day)
			continue;
		if (nowyear==year && nowdate==day && nowtime < tt)
			continue;
		run(dirent->d_name);
	}
	closedir(dirp);
	updatetime(nowtime);
	exit(0);
}

makenowtime()
{
	long t;
	struct tm *localtime();
	register struct tm *tp;

	time(&t);
	tp = localtime(&t);
	nowtime = tp->tm_hour*100 + tp->tm_min;
	nowdate = tp->tm_yday;
	nowyear = tp->tm_year;
}

updatetime(t)
{
	FILE *tfile;

	tfile = fopen(LASTF, "w");
	if (tfile == NULL) {
		fprintf(stderr, "can't write lastfile\n");
		exit(1);
	}
	fprintf(tfile, "%04d\n", t);
}

run(file)
char *file;
{
	struct stat stbuf;
	register pid, i;
	char sbuf[64];

	/* printf("running %s\n", file); */
	if (fork()!=0)
		return;
	for (i=0; i<15; i++)
		close(i);
	dup(dup(open("/dev/null", 0)));
	sprintf(sbuf, "%s/%s", PDIR, file);
	link(file, sbuf);
	unlink(file);
	chdir(PDIR);
	if (stat(file, &stbuf) == -1)
		exit(1);
	if (pid = fork()) {
		if (pid == -1)
			exit(1);
		wait((int *)0);
		unlink(file);
		exit(0);
	}
	setgid(stbuf.st_gid);
	initgroups((getpwuid(stbuf.st_uid))->pw_name, stbuf.st_gid);
	setuid(stbuf.st_uid);
	execl("/bin/sh", "sh", file, 0);
	execl("/usr/bin/sh", "sh", file, 0);
	fprintf(stderr, "Can't execl shell\n");
	exit(1);
}
