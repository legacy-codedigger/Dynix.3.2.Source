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

/* $Header: mailstats.c 2.0 86/01/28 $ */

# include "sendmail.h"

SCCSID(@(#)mailstats.c	4.1		7/25/83);

/*
**  MAILSTATS -- print mail statistics.
**
**	Flags:
**		-Ffile		Name of statistics file.
**
**	Exit Status:
**		zero.
*/

main(argc, argv)
	char  **argv;
{
	register int fd;
	struct statistics stat;
	char *sfile = "/usr/lib/sendmail.st";
	register int i;
	extern char *ctime();

	fd = open(sfile, 0);
	if (fd < 0)
	{
		perror(sfile);
		exit(EX_NOINPUT);
	}
	if (read(fd, &stat, sizeof stat) != sizeof stat ||
	    stat.stat_size != sizeof stat)
	{
		(void) sprintf(stderr, "File size change\n");
		exit(EX_OSERR);
	}

	printf("Statistics from %s", ctime(&stat.stat_itime));
	printf(" M msgsfr bytes_from  msgsto   bytes_to\n");
	for (i = 0; i < MAXMAILERS; i++)
	{
		if (stat.stat_nf[i] == 0 && stat.stat_nt[i] == 0)
			continue;
		printf("%2d ", i);
		printf("%6ld %10ldK ", stat.stat_nf[i], stat.stat_bf[i]);
		printf("%6ld %10ldK\n", stat.stat_nt[i], stat.stat_bt[i]);
	}
}
