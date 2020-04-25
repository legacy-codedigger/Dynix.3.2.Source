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

/* @(#)$Header: usage.c 1.3 84/12/18 $ */

#include <stdio.h>
#include "host.h"

usage()
{
	printf("usage: %s [ttyname] [options]\noptions:\n", myname);
	printf("      -b baud: set port to baud rate\n");
	printf("      -ec: change escape character\n");
	printf("      -f: runfile: read input from file\n");
	printf("      -m: remote has MMU\n");
	printf("      -n: no lock file\n");
	printf("      -r: no remote file server\n");
	printf("      -s: script: start script file\n");
	exit(1);
}

help()
{
	printf("help\r\n	%c?	print this\r\n", escape);
	printf("	%c.	terminate\r\n", escape);
	printf("        %c!	subshell\r\n", escape);
	printf("	%c|	script to pipe (not implemented)\r\n", escape);
	printf("	%c#	send break\r\n", escape);
	printf("	%c>	send file\r\n", escape);
	printf("	%cb	toggle buffering of script\r\n", escape);
	printf("	%cd	packet debug toggle\r\n", escape);
	printf("	%ce	toggle local echo\r\n", escape);
	printf("	%cf	read runfile\r\n", escape);
	printf("	%cl	load file\r\n", escape);
	printf("	%cp	change max packet size\r\n", escape);
	printf("	%cr	toggle remote server\r\n", escape);
	printf("	%cs	toggle script\r\n", escape);
	printf("	%cw	write bootstrap\r\n", escape);
	printf("	%cv	virtual escape\r\n", escape);
	printf("	%cz	zero bootstrap list\r\n", escape);
	(void)fflush(stdout);
}
