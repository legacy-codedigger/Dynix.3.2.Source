/* $Header: mkdate.c 2.3 1991/05/31 18:49:16 $ */

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

/*
 * $Log: mkdate.c,v $
 *
 */

#include <stdio.h>
#include <pwd.h>
#include <time.h>

struct	passwd *getpwuid();

main(argc, argv)
	char **argv;
{
	register struct passwd *pp;
	struct passwd dummy;
	struct tm *t;
	time_t clock;
#ifdef BSD
	char host[100];
#endif

	clock = time(0);
	t = localtime(&clock);
#ifdef BSD
	gethostname(host, sizeof host);
#endif
	pp = getpwuid(getuid());
	if (pp == NULL)
		pp = &dummy, pp->pw_name = "unknown";
	printf("char Version[] = \"%s\";\n", argv[1]);
	printf("char *Date = \"%d/%d/%d %d:%02d",
		t->tm_mon + 1, t->tm_mday, t->tm_year % 100,
		t->tm_hour, t->tm_min);
#ifdef BSD
	printf("(%s!%s)\";\n", host, pp->pw_name);
#else
	printf("(%s)\";\n", pp->pw_name);
#endif
	exit(0);
}
