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

#ifdef RCS
static	char rcsid[] = "$Header: ls.c 2.1 86/04/27 $";
#endif

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/dir.h>
#include <sys/fs.h>
#include "saio.h"

char line[100];

main()
{
	int i;

	printf("ls\n");
	printf("*** type \"exit\" to exit ***\n");
	for (;;) {
		do  {
			printf(": "); gets(line);
			if (strcmp("exit", line) == 0)
				exit(0);
			i = open(line, 0);
		} while (i < 0);

		ls(i);
		close(i);
	}
}

ls(io)
register io;
{
	struct direct *d;
	register int i, n, count, offset;
	char b[DIRBLKSIZ];

	printf("Inode      Name\n");
	while ((count=read(io, (char *)b, sizeof b)) > 0) {
		offset = 0;
		d=(struct direct *)b;
		for (; offset<count; d=(struct direct *)&b[offset]) {
			offset += d->d_reclen;
			if (d->d_ino == 0)
				continue;
			n = d->d_ino;
			printf("%d", n);
			for (i=0; i < 10; i++) {
				if (n /= 10) 
					continue;
				printf(" ");
			}
			printf("%s\n", d->d_name);
		}
	}
}
