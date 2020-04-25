/*
 * $Copyright:	$
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
 static char rcsid[] = "$Header: ipcrm.c 2.1 88/03/24 $";
#endif lint

/*
 * $Log:	ipcrm.c,v $
 */

/* @(#)ipcrm.c	1.5 */
/*
**	ipcrm - IPC remove
**	Remove specified message queues, semaphore sets and shared memory ids.
*/

#include	"sys/types.h"
#include	"sys/ipc.h"
#include	"sys/msg.h"
#include	"sys/sem.h"
#include	"sys/errno.h"
#include	"stdio.h"

char opts[] = "q:m:s:Q:M:S:";	/* allowable options for getopt */
extern char	*optarg;	/* arg pointer for getopt */
extern int	optind;		/* option index for getopt */
extern int	errno;		/* error return */

main(argc, argv)
int	argc;	/* arg count */
char	**argv;	/* arg vector */
{
	register int	o;	/* option flag */
	register int	err;	/* error count */
	register int	ipc_id;	/* id to remove */
	register key_t	ipc_key;/* key to remove */
	extern	long	atol();

	/* Go through the options */
	err = 0;
	while ((o = getopt(argc, argv, opts)) != EOF)
		switch(o) {

		case 'q':	/* message queue */
			ipc_id = atoi(optarg);
			if (msgctl(ipc_id, IPC_RMID, 0) == -1)
				oops("msqid", (long)ipc_id);
			break;

		case 'm':	/* shared memory */
			fprintf(stderr,"This command does not support shared memory\n");
			break;

		case 's':	/* semaphores */
			ipc_id = atoi(optarg);
			if (semctl(ipc_id, IPC_RMID, 0) == -1)
				oops("semid", (long)ipc_id);
			break;

		case 'Q':	/* message queue (by key) */
			if((ipc_key = (key_t)getkey(optarg)) == 0)
				break;
			if ((ipc_id=msgget(ipc_key, 0)) == -1
				|| msgctl(ipc_id, IPC_RMID, 0) == -1)
				oops("msgkey", ipc_key);
			break;

		case 'M':	/* shared memory (by key) */
			fprintf(stderr,"This command does not support shared memory\n");
			break;

		case 'S':	/* semaphores (by key) */
			if((ipc_key = (key_t)getkey(optarg)) == 0)
				break;
			if ((ipc_id=semget(ipc_key, 0, 0)) == -1
				|| semctl(ipc_id, IPC_RMID, 0) == -1)
				oops("semkey", ipc_key);
			break;

		default:
		case '?':	/* anything else */
			err++;
			break;
		}
	if (err || (optind < argc)) {
		fprintf(stderr,
		   "usage: ipcrm [ [-q msqid] [-m shmid] [-s semid]\n%s\n",
		   "	[-Q msgkey] [-M shmkey] [-S semkey] ... ]");
		exit(1);
	}
}

oops(s, i)
char *s;
long   i;
{
	char *e;

	switch (errno) {

	case	ENOENT:	/* key not found */
	case	EINVAL:	/* id not found */
		e = "not found";
		break;

	case	EPERM:	/* permission denied */
		e = "permission denied";
		break;
	default:
		e = "unknown error";
	}

	fprintf(stderr, "ipcrm: %s(%ld): %s\n", s, i, e);
}

key_t
getkey(kp)
register char *kp;
{
	key_t k;

	if((k = (key_t)atol(kp)) == IPC_PRIVATE)
		printf("illegal key: %s\n", kp);
	return(k);
}

