
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

/* 
 * $Header: semsys.c 2.0 86/01/28 $
 */

/*
 * $Log:	semsys.c,v $
 */


#include	"sys/types.h"
#include	"sys/ipc.h"
#include	"sys/sem.h"
#include	<syscall.h>

#define	SEMCTL	0
#define	SEMGET	1
#define	SEMOP	2

semctl(semid, semnum, cmd, arg)
int semid, cmd;
int semnum;
union semun {
	int val;
	struct semid_ds *buf;
	ushort array[1];
} arg;
{
	return(syscall(SYS_semsys, SEMCTL, semid, semnum, cmd, arg));
}

semget(key, nsems, semflg)
key_t key;
int nsems, semflg;
{
	return(syscall(SYS_semsys, SEMGET, key, nsems, semflg));
}

semop(semid, sops, nsops)
int semid;
struct sembuf (*sops)[];
int nsops;
{
	return(syscall(SYS_semsys, SEMOP, semid, sops, nsops));
}
