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
static char rcsid[] = "$Header: enroll.c 2.1 87/01/27 $";
#endif

#include "xmail.h"
#include "pwd.h"
#include "sys/types.h"
MINT *a[42], *x, *b, *one, *c64, *t45, *z, *q, *r, *two, *t15;
char buf[256];
char maildir[] = { "/usr/spool/secretmail"};
main()
{
	int uid, i;
	FILE *fd;
	char *myname, fname[128];
	uid = getuid();
	myname = (char *) getlogin();
	if(myname == NULL)
		myname = getpwuid(uid)->pw_name;
	sprintf(fname, "%s/%s.key", maildir, myname);
	comminit();
	setup(getpass("Gimme key: "));
	mkb();
	mkx();
#ifdef debug
	omout(b);
	omout(x);
#endif
	mka();
	umask(0);
	i = creat(fname, 0644);
	if(i<0)
	{	perror(fname);
		exit(1);
	}
	close(i);
	fd = fopen(fname, "w");
	for(i=0; i<42; i++)
		nout(a[i], fd);
	exit(0);
}
