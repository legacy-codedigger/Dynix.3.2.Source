
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
 * $Header: sem.h 2.2 88/03/29 $
 */

/* 
 * $Log:	sem.h,v $
 */

/*
 * 	system V IPC semaphore facility
 */

/*
**	Implementation Constants.
*/

#define PSEM	(PZERO - 4)	/* block priority waiting for mutex */
#define	PSEMN	(PZERO + 3)	/* sleep priority waiting for greater value */
#define	PSEMZ	(PZERO + 2)	/* sleep priority waiting for zero */

/*
**	Permission Definitions.
*/

#define	SEM_A	0200	/* alter permission */
#define	SEM_R	0400	/* read permission */

/*
**	Semaphore Operation Flags.
*/

#define	SEM_UNDO	010000	/* set up adjust on exit entry */

/*
**	Semctl Command Definitions.
*/

#define	GETNCNT	3	/* get semncnt */
#define	GETPID	4	/* get sempid */
#define	GETVAL	5	/* get semval */
#define	GETALL	6	/* get all semval's */
#define	GETZCNT	7	/* get semzcnt */
#define	SETVAL	8	/* set semval */
#define	SETALL	9	/* set all semval's */

/*
**	Structure Definitions.
*/

/*
**	There is one semaphore id data structure for each set of semaphores
**		in the system.
*/

#ifdef KERNEL

struct semid_ds {
	struct lsemid_ds {
		struct ipc_perm	lsem_perm; /* operation permission struct */
		struct sem	*lsem_base; /* ptr to first semaphore in set */
		ushort		lsem_nsems; /* # of semaphores in set */
		time_t		lsem_otime; /* last semop time */
		time_t		lsem_ctime; /* last change time */
	}	semid_ic;
	sema_t	semid_mutex;
	char	semid_flag;
	lock_t	semid_lock;
};
#define STRUE	1
#define SFALSE	0
#define	sem_perm	semid_ic.lsem_perm
#define	sem_base	semid_ic.lsem_base
#define	sem_nsems	semid_ic.lsem_nsems
#define	sem_otime	semid_ic.lsem_otime
#define	sem_ctime	semid_ic.lsem_ctime

#else

struct semid_ds {
	struct ipc_perm	sem_perm;	/* operation permission struct */
	struct sem	*sem_base;	/* ptr to first semaphore in set */
	ushort		sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	time_t		sem_ctime;	/* last change time */
};

#endif KERNEL

/*
**	There is one semaphore structure for each semaphore in the system.
*/

struct sem {
	ushort	semval;		/* semaphore text map address */
	short	sempid;		/* pid of last operation */
	short	semncnt;	/* # awaiting semval > cval */
	short	semzcnt;	/* # awaiting semval = 0 */
#ifdef KERNEL
	sema_t	semncnt_wait;	/* block here */
	sema_t	semzcnt_wait;	/* block here */
#endif KERNEL
};

/*
**	There is one undo structure per process in the system.
*/

struct sem_undo {
	struct sem_undo	*un_np;	/* ptr to next active undo structure */
	short		un_cnt;	/* # of active entries */
#ifdef KERNEL
	lock_t		un_lock;
#endif KERNEL
	struct undo {
		short	un_aoe;	/* adjust on exit values */
		short	un_num;	/* semaphore # */
		int	un_id;	/* semid */
	}	un_ent[1];	/* undo entries (one minimum) */
};

/*
** semaphore information structure
*/
struct	seminfo	{
	int	semmap,		/* # of entries in semaphore map */
		semmni,		/* # of semaphore identifiers */
		semmns,		/* # of semaphores in system */
		semmnu,		/* # of undo structures in system */
		semmsl,		/* max # of semaphores per id */
		semopm,		/* max # of operations per semop call */
		semume,		/* max # of undo entries per process */
		semusz,		/* size in bytes of undo structure */
		semvmx,		/* semaphore maximum value */
		semaem;		/* adjust on exit max value */
};

/*
**	User semaphore template for semop system calls.
*/

struct sembuf {
	ushort	sem_num;	/* semaphore # */
	short	sem_op;		/* semaphore operation */
	short	sem_flg;	/* operation flags */
};

#define SEMGATE		63
