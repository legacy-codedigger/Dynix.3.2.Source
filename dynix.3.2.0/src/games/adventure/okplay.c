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

/* $Header: okplay.c 2.0 86/01/28 $ */

static char sccsid[] = "	okplay.c	4.1	82/05/11	";

main(argc, argv)
int argc; char *argv[];
{
	argv[argc] = 0;
	okplay();
	if (argc == 1)
		advmotd();
	execv("/usr/games/lib/adventure", argv);
	write(2, "No adventure just now\n", 22);
	exit(1);
}

okplay()
{
	int tvec[2];
	register struct local {
		int seconds,minutes,hours,daymo,month,year,daywk,dayyr,dst;
	} *local;

	return;
	time(tvec);
	local = localtime(tvec);
	local->month++;
	if (local->daywk == 6||local->daywk == 0)
		return;
	if (local->hours>=9&&local->hours<18)
		if (!holiday(local->month, local->daymo))
			notinprime();
}

holiday(mo,da)
	int mo,da;
{
	int stbuf[30];
	char *np, *op;

	np = "/usr/games/holiday/XXXX";
	for(op=np;*op;op++);
	*--op=(da%10)|'0';
	*--op=(da/10)|'0';
	*--op=(mo%10)|'0';
	*--op=(mo/10)|'0';
	return(stat(np,stbuf) == 0);
}
char notinm[] "No adventure in prime time (M-F 9am-5pm) except on holidays\n";

notinprime()
{
	write(2, notinm, sizeof notinm);
	exit(1);
}

advmotd()
{
	char ch;
	int fd;

	fd = open("/usr/games/lib/advmotd", 0);
	if (fd < 0) return;
	while(read(fd,&ch,1) ==1) write(1,&ch,1);
	close(fd);
}
