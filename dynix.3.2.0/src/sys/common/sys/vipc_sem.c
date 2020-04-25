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
static char rcsid[]="$Header: vipc_sem.c 2.7 88/03/17 $";
#endif lint

/*
**	Inter-Process Communication semaphore Facility.
*/

/*
 * $Log:	vipc_sem.c,v $
 */


#ifdef SVSEMA
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ipc.h"
#include "../h/sem.h"
#include "../h/map.h"

#include "../machine/intctl.h"

#ifdef DEBUG
int vsem_debug = 0;
#endif DEBUG

#define SPLSEM	SPL1


/*
 * sema_lock must provide mutex to sema[] during lookup,
 * creation, and deletion of semaphore id's.  This include functions:
 *	semconv()
 *	semget()
 *	semctl()	(only in IPC_RMID case).
 *
 *	sema_lock must always be grabbed BEFORE semid_lock's.
 */
extern struct semid_ds	sema[];		/* semaphore data structures */
lock_t	sema_lock;

/*
 * semmap mutex protected by semmap_lock.  No additional locks are
 * asserted while semma_lock is held, so no deadlock problems here.
 */
extern struct map	semmap[];	/* sem allocation map */
lock_t	semmap_lock;			/* mutex for sem allocation map */
extern struct sem	sem[];		/* semaphores */

/*
 * wake up all processes sleeping on the system V semaphore
 * which contains the DYNIX semaphore (wait_sema).
 * The semid_ds structure containing the system V semaphore
 * must be locked during this operation.
 */
#define SEMWAKEUP(wait_sema)	vall_sema((wait_sema))

/*
 * sleep on the system V semaphore contining the DYNIX semaphore
 * (wait_sema).  Also atmoically release the semid_ds structure lock.
 */
#define SEMSLEEP(wait_sema, semid, pri) \
	p_sema_v_lock((wait_sema), (pri), &((semid)->semid_lock), SPLSEM)

/*
 * No mutex is needed on access to this table, as each entry is
 * effectively owned by a single process.  This array is always
 * indexed using the proc-slot number of the current process.
 */
extern struct sem_undo	*sem_undo[];	/* undo table pointers */
extern int	semu[];			/* operation adjust on exit table */

/*
 * The chain of active undo pointers is mutex'd by semunp_lock.
 * Note that the chain is really a list of blocks of undo structures.
 * This lock must be held when manipulating the chain of blocks AND when
 * manipulating the list inside a block which is on this chain.
 *
 * If this proves a bottle-neck, this can be broken up more.
 * deadlock avoidance demands that when manipulating an undo structure
 * associated with a system V semaphore, the semid_ds lock associated
 * with that semaphore must be held first.  The only place this proves
 * a problem is in semexit().
 */
struct sem_undo	*semunp;		/* ptr to head of undo chain */
lock_t	semunp_lock;

/*
 *
 */
struct sem_undo	*semfup;		/* ptr to head of free undo chain */
lock_t	semfup_lock;

extern struct seminfo seminfo;		/* param information structure */
/*
 * define sem temporary structure for use on stack 
 *	numbers below are chose to make this stucture under 100 bytes
 */
#define	SSEMOPM	16
#define SSEMMSL	48
extern union ssemtmp {
	ushort			semvals[SSEMMSL]; /* set semaphore values */
	struct	semid_ds	ds;	/* set permission values */
	struct sembuf		semops[SSEMOPM];	/* operation holding area */
};

/*
 * This is the global sem temporary structure, for use with very
 * large semaphore operations.  This also serializes much of
 * the code below.  Access to this structure is protected with a
 * semaphore.
 */
union proto_semtmp {
	ushort			semvals[1]; /* set semaphore values */
	struct	semid_ds	ds;	/* set permission values */
	struct sembuf		semops[1];	/* operation holding area */
};
extern union proto_semtmp semtmp;
sema_t	gsemtmp_mutex;

extern struct timeval	time;			/* system idea of date */
extern long	rmalloc();

struct ipc_perm	*ipcget();
struct semid_ds *semconv();

/*
**	semaoe - Create or update adjust on exit entry.
**		assume the semaphore associated with this is
**		locked by the caller.
*/

static
semaoe(val, id, num)
short	val,	/* operation value to be adjusted on exit */
	num;	/* semaphore # */
int	id;	/* semid */
{
	register struct undo		*uup,	/* ptr to entry to update */
					*uup2;	/* ptr to move entry */
	register struct sem_undo	*up,	/* ptr to process undo struct */
					*up2;	/* ptr to undo list */
	register int			i,	/* loop control */
					found;	/* matching entry found flag */

	if(val == 0)
		return(0);
	if(val > seminfo.semaem || val < -seminfo.semaem) {
#ifdef DEBUG
		if (vsem_debug) {
			printf("semaoe: semaem %d val %d\n",
				seminfo.semaem, val);
		}
#endif DEBUG
		u.u_error = ERANGE;
		return(1);
	}
	if((up = sem_undo[u.u_procp - proc]) == NULL) {
		/*
		 * lock the free list and get a new undo struct
		 */
		(void)p_lock(&semfup_lock, SPLSEM);
		if (up = semfup) {
			semfup = up->un_np;
			v_lock(&semfup_lock, SPLSEM);
			up->un_np = NULL;
			sem_undo[u.u_procp - proc] = up;
		} else {
			v_lock(&semfup_lock, SPLSEM);
			u.u_error = ENOSPC;
			return(1);
		}
	}
	/*
	 *  lock the undo structure.
	 */
	(void)p_lock(&up->un_lock, SPLSEM);
	for(uup = up->un_ent, found = i = 0;i < up->un_cnt;i++) {
		if(uup->un_id < id || (uup->un_id == id && uup->un_num < num)) {
			uup++;
			continue;
		}
		if(uup->un_id == id && uup->un_num == num)
			found = 1;
		break;
	}
	if(!found) {
		if(up->un_cnt >= seminfo.semume) {
			v_lock(&up->un_lock, SPLSEM);
			u.u_error = EINVAL;
			return(1);
		}
		/*
		 * lock the undo chain, to add a new udno
		 * structure to the chain.
		 *
		 * must release lock on undo structure first, to
		 * avoid deadlock.  Note, there are no races or
		 * deadlocks with other accesses to this undo block
		 * via this semaphore, because a lock on this semaphore
		 * was asserted before calling this routine.
		 */
		if(up->un_cnt == 0) {
			v_lock(&up->un_lock, SPLSEM);
			(void)p_lock(&semunp_lock, SPLSEM);
			up->un_np = semunp;
			semunp = up;
			v_lock(&semunp_lock, SPLSEM);
			(void)p_lock(&up->un_lock, SPLSEM);
		}
		uup2 = &up->un_ent[up->un_cnt++];
		while(uup2-- > uup)
			*(uup2 + 1) = *uup2;
		uup->un_id = id;
		uup->un_num = num;
		uup->un_aoe = -val;
		v_lock(&up->un_lock, SPLSEM);
		return(0);
	}
	uup->un_aoe -= val;
	if(uup->un_aoe > seminfo.semaem || uup->un_aoe < -seminfo.semaem) {
#ifdef DEBUG
		if (vsem_debug) {
			printf("semaoe: un_aoe %d semaem\n", uup->un_aoe, 
					seminfo.semaem);
		}
#endif DEBUG
		uup->un_aoe += val;
		v_lock(&up->un_lock, SPLSEM);
		u.u_error = ERANGE;
		return(1);
	}
	if(uup->un_aoe == 0) {
		uup2 = &up->un_ent[--(up->un_cnt)];
		while(uup++ < uup2)
			*(uup - 1) = *uup;
		if(up->un_cnt == 0) {

			/* Remove process from undo list. */
			v_lock(&up->un_lock, SPLSEM);
			(void)p_lock(&semunp_lock, SPLSEM);
			if(semunp == up)
				semunp = up->un_np;
			else
				for(up2 = semunp;up2 != NULL;up2 = up2->un_np)
					if(up2->un_np == up) {
						up2->un_np = up->un_np;
						break;
					}
			v_lock(&semunp_lock, SPLSEM);
			(void)p_lock(&up->un_lock, SPLSEM);
			up->un_np = NULL;
		}
	}
	v_lock(&up->un_lock, SPLSEM);
	return(0);
}

/*
**	semconv - Convert user supplied semid into a ptr to the associated
**		semaphore header.
**
**		added new flag argument, "lflag" to assist removing
**		semaphores (ugh!!).  It makes deadlock avoidence
**		easier by leaving the sema_lock locked on exit when
**		we know we may to delete the semaphore.  The only
**		time this flag is non-zero is during the IPC_RM case
**		of semctl().
*/

static struct semid_ds *
semconv(s, lflag)
register int	s;	/* semid */
int lflag;
{
	register struct semid_ds	*sp;	/* ptr to associated header */


 	if (s < 0)	{
 		u.u_error = EINVAL;
 		return(NULL);
 	}
	sp = &sema[s % seminfo.semmni];
retry:
	(void)p_lock(&sema_lock, SPLSEM);
	if((sp->sem_perm.mode & IPC_ALLOC) == 0 ||
		s / seminfo.semmni != sp->sem_perm.seq) {
		v_lock(&sema_lock, SPL0);
		u.u_error = EINVAL;
		return(NULL);
	}
	(void)p_lock(&(sp->semid_lock), SPLSEM);
	if (sp->semid_flag == STRUE) {
		v_lock(&sema_lock, SPLSEM);
		/* Non-signal-able sleep */
		p_sema_v_lock(&(sp->semid_mutex), PSEM,
			&(sp->semid_lock), SPL0);
		goto retry;
	}
	if (lflag == 0)
		v_lock(&sema_lock, SPLSEM);
	return(sp);
}

/*
**	semctl - Semctl system call.
*/

#define GETSEMTMPS(n)	if ((n) > SSEMMSL) { \
				p_sema(&gsemtmp_mutex, PSEM); \
				semtmpp = (union proto_semtmp *)&semtmp; \
			} else { \
				semtmpp = (union proto_semtmp *)&lsemtmp; \
			}

#define FREESEMTMPS	if (semtmpp == (union proto_semtmp *)&semtmp) \
				v_sema(&gsemtmp_mutex)

static
semctl(uap)
	register struct {
		int	semid;
		u_int	semnum;
		int	cmd;
		int	arg;
	}	*uap;
{
	register struct	semid_ds	*sp;	/* ptr to semaphore header */
	register struct sem		*p;	/* ptr to semaphore */
	register int			i;	/* loop control */
	caddr_t	tcp;
	short	tmp;
	int	rmflag;		/* in process of removing semaphore? */
	union ssemtmp lsemtmp;	/* stack-based semaphore temp space */
	register union proto_semtmp *semtmpp;	/* pointer to temp space */


	rmflag = 0;
	if (uap->cmd == IPC_RMID)
		rmflag = 1;
	if((sp = semconv(uap->semid, rmflag)) == NULL) 
		return;
	u.u_r.r_val1 = 0;
	switch(uap->cmd) {

	/* Remove semaphore set. */
	case IPC_RMID:
		if(u.u_uid != sp->sem_perm.uid && u.u_uid != sp->sem_perm.cuid
			&& !suser()) {
			v_lock(&sema_lock, SPLSEM);
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		if(uap->semid + seminfo.semmni < 0)
			sp->sem_perm.seq = 0;
		else
			sp->sem_perm.seq++;
		sp->sem_perm.mode = 0;
		/*
		 * semaphore is effectively removed at this point.
		 * Now, just clean up.
		 */
		v_lock(&sema_lock, SPLSEM);
		semunrm(uap->semid, 0, sp->sem_nsems);
		for(i = sp->sem_nsems, p = sp->sem_base;i--;p++) {
			p->semval = p->sempid = 0;
			/*
			 * when these processes awake, they will find
			 * the semaphore has gone away.
			 */
			SEMWAKEUP(&p->semncnt_wait);
			SEMWAKEUP(&p->semzcnt_wait);
		}
		(void)p_lock(&semmap_lock, SPLSEM);
		rmfree(semmap, (long)sp->sem_nsems, (long)(sp->sem_base - sem) + 1);
		v_lock(&semmap_lock, SPLSEM);
		v_lock(&(sp->semid_lock), SPL0);
		return;

	/* Set ownership and permissions. */
	case IPC_SET:
		if(u.u_uid != sp->sem_perm.uid && u.u_uid != sp->sem_perm.cuid
			 && !suser()) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		sp->semid_flag = STRUE;
		v_lock(&(sp->semid_lock), SPL0);
		semtmpp = (union proto_semtmp *)&lsemtmp;
		if(copyin((caddr_t)uap->arg, (caddr_t)&(*semtmpp).ds.semid_ic, 
				sizeof((*semtmpp).ds.semid_ic))) {
			(void)p_lock(&(sp->semid_lock), SPLSEM);
			sp->semid_flag = SFALSE;
			vall_sema(&(sp->semid_mutex));
			v_lock(&(sp->semid_lock), SPL0);
			u.u_error = EFAULT;
			return;
		}
		(void)p_lock(&(sp->semid_lock), SPLSEM);
		sp->sem_perm.uid = (*semtmpp).ds.sem_perm.uid;
		sp->sem_perm.gid = (*semtmpp).ds.sem_perm.gid;
		sp->sem_perm.mode = (*semtmpp).ds.sem_perm.mode & 0777 | IPC_ALLOC;
		sp->sem_ctime = (time_t)time.tv_sec;
		sp->semid_flag = SFALSE;
		vall_sema(&(sp->semid_mutex));
		v_lock(&(sp->semid_lock), SPL0);
		return;

	/* Get semaphore data structure. */
	case IPC_STAT:
		if(ipcaccess(&sp->sem_perm, SEM_R)) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		lsemtmp.ds.semid_ic = sp->semid_ic;
		v_lock(&(sp->semid_lock), SPL0);
		if(copyout((caddr_t)&lsemtmp.ds.semid_ic, (caddr_t)uap->arg,
			sizeof(lsemtmp.ds.semid_ic))) {
			u.u_error = EFAULT;
			return;
		}
		return;

	/* Get # of processes sleeping for greater semval. */
	case GETNCNT:
		if(ipcaccess(&sp->sem_perm, SEM_R)) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		if(uap->semnum >= sp->sem_nsems) {
			v_lock(&(sp->semid_lock), SPL0);
			u.u_error = EINVAL;
			return;
		}
		tmp = -sema_count(&(sp->sem_base + uap->semnum)->semncnt_wait);
		v_lock(&(sp->semid_lock), SPL0);
		u.u_r.r_val1 = (tmp > 0 ? tmp : 0);
		return;

	/* Get pid of last process to operate on semaphore. */
	case GETPID:
		if(ipcaccess(&sp->sem_perm, SEM_R)) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		if(uap->semnum >= sp->sem_nsems) {
			v_lock(&(sp->semid_lock), SPL0);
			u.u_error = EINVAL;
			return;
		}
		u.u_r.r_val1 = (sp->sem_base + uap->semnum)->sempid;
		v_lock(&(sp->semid_lock), SPL0);
		return;

	/* Get semval of one semaphore. */
	case GETVAL:
		if(ipcaccess(&sp->sem_perm, SEM_R)) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		if(uap->semnum >= sp->sem_nsems) {
			v_lock(&(sp->semid_lock), SPL0);
			u.u_error = EINVAL;
			return;
		}
		u.u_r.r_val1 = (sp->sem_base + uap->semnum)->semval;
		v_lock(&(sp->semid_lock), SPL0);
		return;

	/* Get all semvals in set. */
	case GETALL:
		if(ipcaccess(&sp->sem_perm, SEM_R)) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		tcp = (caddr_t)uap->arg;
		sp->semid_flag = STRUE;
		v_lock(&(sp->semid_lock), SPLSEM);
		for(i = sp->sem_nsems, p = sp->sem_base;i--;p++) {
			if(copyout((caddr_t)&p->semval, (caddr_t)tcp, 
					sizeof(p->semval)))
			{
				(void)p_lock(&(sp->semid_lock), SPLSEM);
				sp->semid_flag = SFALSE;
				vall_sema(&(sp->semid_mutex));
				v_lock(&(sp->semid_lock), SPL0);
				u.u_error = EFAULT;
				return;
			}
			tcp += sizeof(p->semval);
		}
		(void)p_lock(&(sp->semid_lock), SPLSEM);
		sp->semid_flag = SFALSE;
		vall_sema(&(sp->semid_mutex));
		v_lock(&(sp->semid_lock), SPL0);
		return;

	/* Get # of processes sleeping for semval to become zero. */
	case GETZCNT:
		if(ipcaccess(&sp->sem_perm, SEM_R)) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		if(uap->semnum >= sp->sem_nsems) {
			v_lock(&(sp->semid_lock), SPL0);
			u.u_error = EINVAL;
			return;
		}
		tmp = -sema_count(&(sp->sem_base + uap->semnum)->semzcnt_wait);
		v_lock(&(sp->semid_lock), SPL0);
		u.u_r.r_val1 = (tmp > 0 ? tmp : 0);
		return;

	/* Set semval of one semaphore. */
	case SETVAL:
		if(ipcaccess(&sp->sem_perm, SEM_A)) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		if(uap->semnum >= sp->sem_nsems) {
			v_lock(&(sp->semid_lock), SPL0);
			u.u_error = EINVAL;
			return;
		}
		if((unsigned)uap->arg > seminfo.semvmx) {
#ifdef DEBUG
			if (vsem_debug) {
				printf("SETVAL: semvmx %d arg %d\n",
					seminfo.semvmx, uap->arg);
			}
#endif DEBUG
			v_lock(&(sp->semid_lock), SPL0);
			u.u_error = ERANGE;
			return;
		}
		if((p = sp->sem_base + uap->semnum)->semval = uap->arg) {
			SEMWAKEUP(&p->semncnt_wait);
		} else
			SEMWAKEUP(&p->semzcnt_wait);
		p->sempid = u.u_procp->p_pid;
		semunrm(uap->semid, uap->semnum, uap->semnum);
		v_lock(&(sp->semid_lock), SPL0);
		return;

	/* Set semvals of all semaphores in set. */
	case SETALL:
		if(ipcaccess(&sp->sem_perm, SEM_A)) {
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		sp->semid_flag = STRUE;
		v_lock(&(sp->semid_lock), SPL0);
		GETSEMTMPS(sp->sem_nsems);
		u.u_error = copyin((caddr_t)uap->arg, (caddr_t)(*semtmpp).semvals,
			sizeof((*semtmpp).semvals[0]) * sp->sem_nsems);
		(void)p_lock(&(sp->semid_lock), SPLSEM);
		sp->semid_flag = SFALSE;
		vall_sema(&(sp->semid_mutex));
		if(u.u_error) {
			FREESEMTMPS;
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		for(i = 0;i < sp->sem_nsems;)
			if((*semtmpp).semvals[i++] > seminfo.semvmx) {
#ifdef DEBUG
				if (vsem_debug) {
					printf("SETALL: semvals %d semvmx %d\n",
					(*semtmpp).semvals[i-1], seminfo.semvmx);
				}
#endif DEBUG
				FREESEMTMPS;
				v_lock(&(sp->semid_lock), SPL0);
				u.u_error = ERANGE;
				return;
			}
		semunrm(uap->semid, 0, sp->sem_nsems);
		for(i = 0, p = sp->sem_base;i < sp->sem_nsems;
			(p++)->sempid = u.u_procp->p_pid) {
			if(p->semval = (*semtmpp).semvals[i++]) 
				SEMWAKEUP(&p->semncnt_wait);
			else
				SEMWAKEUP(&p->semzcnt_wait);
		}
		FREESEMTMPS;
		v_lock(&(sp->semid_lock), SPL0);
		return;
	default:
		v_lock(&(sp->semid_lock), SPL0);
		u.u_error = EINVAL;
		return;
	}
}

/*
**	semexit - Called by exit(sys1.c) to clean up on process exit.
*/
semexit()
{
	register struct sem_undo	*up,	/* process undo struct ptr */
					*p;	/* undo struct ptr */
	register struct semid_ds	*sp;	/* semid being undone ptr */
	register long			v;	/* adjusted value */
	register struct sem		*semp;	/* semaphore ptr */
	register struct undo		*uup;	/* ptr to entry to update */
	int i;

	if((up = sem_undo[u.u_procp - proc]) == NULL) {
		return;
	}
	(void)p_lock(&semunp_lock, SPLSEM);
	if(semunp == up)
		semunp = up->un_np;
	else
		for(p = semunp;p != NULL;p = p->un_np)
			if(p->un_np == up) {
				p->un_np = up->un_np;
				break;
			}
	v_lock(&semunp_lock, SPLSEM);
	sem_undo[u.u_procp - proc] = NULL;
	if(up->un_cnt == 0)
		goto cleanup;
	/*
	 * The undo block for this process is off the undo chain.
	 *  So, no locking of undo chain is needed at this point.
	 */
	for(i = up->un_cnt; i-- ;) {
		uup = &up->un_ent[i];
		if((sp = semconv(uup->un_id, 0)) == NULL) 
			continue;
		v = (long)(semp = sp->sem_base + uup->un_num)->semval +
			uup->un_aoe;
		if(v < 0 || v > seminfo.semvmx) {
			v_lock(&(sp->semid_lock), SPLSEM);
			continue;
		}
		semp->semval = v;
		if(v == 0) {
			SEMWAKEUP(&semp->semzcnt_wait);
		}
		if(uup->un_aoe > 0 ) {
			SEMWAKEUP(&semp->semncnt_wait);
		}
		v_lock(&(sp->semid_lock), SPLSEM);
	}
	up->un_cnt = 0;
cleanup:
	/*
	 * put undo block back onto free chain
	 */
	(void)p_lock(&semfup_lock, SPLSEM);
	up->un_np = semfup;
	semfup = up;
	v_lock(&semfup_lock, SPL0);
}

/*
**	semget - Semget system call.
*/

static
semget(uap)
	register struct {
		key_t	key;
		int	nsems;
		int	semflg;
	}	*uap;
{
	register struct semid_ds	*sp;	/* semaphore header ptr */
	register unsigned int		i;	/* temp */
	int				s;	/* ipcget status return */

	(void)p_lock(&sema_lock, SPLSEM);
	if ((sp = (struct semid_ds *)ipcget(uap->key, uap->semflg, 
		(struct ipc_perm *)sema, seminfo.semmni, sizeof(*sp), &s))
			== NULL)  {
		v_lock(&sema_lock, SPL0);
		return;
	}
	if(s) {

		/* This is a new semaphore set.  Finish initialization. */
		if(uap->nsems <= 0 || uap->nsems > seminfo.semmsl) {
			sp->sem_perm.mode = 0;
			v_lock(&sema_lock, SPL0);
			u.u_error = EINVAL;
			return;
		}
		(void)p_lock(&semmap_lock, SPLSEM);
		if((i = (int)rmalloc(semmap, (long)uap->nsems)) == NULL) {
			v_lock(&semmap_lock, SPLSEM);
			sp->sem_perm.mode = 0;
			v_lock(&sema_lock, SPL0);
			u.u_error = ENOSPC;
			return;
		}
		v_lock(&semmap_lock, SPLSEM);
		(void)p_lock(&(sp->semid_lock), SPLSEM);
		sp->sem_base = sem + (i - 1);
		sp->sem_nsems = uap->nsems;
		sp->sem_ctime = (time_t)time.tv_sec;
		sp->sem_otime = 0;
		v_lock(&(sp->semid_lock), SPLSEM);
	} else {
		/*
		 * no need to lock the semaphore here to test sem_nsems.
		 * Reasons:
		 *	By the time semaphore has been created (this case),
		 *	this value has been initialized and is stable.
		 *
		 *	During removal of semaphore, the semaphore is
		 *	marked deleted before this value is disturbed.
		 *	So again, this case wouldn't be entered with
		 *	an invalid sem_nsems value.
		 */
		if(uap->nsems && sp->sem_nsems < uap->nsems) {
			v_lock(&sema_lock, SPL0);
			u.u_error = EINVAL;
			return;
		}
	}
	u.u_r.r_val1 = sp->sem_perm.seq * seminfo.semmni + (sp - sema);
	v_lock(&sema_lock, SPL0);
}

/*
**	seminit - Called by main(main.c) to initialize the semaphore map.
*/

seminit()
{
	register i;

	init_sema(&gsemtmp_mutex, 1, 0, SEMGATE);
	init_lock(&sema_lock, SEMGATE);
	init_lock(&semfup_lock, SEMGATE);
	init_lock(&semunp_lock, SEMGATE);
	init_lock(&semmap_lock, SEMGATE);
	rminit(semmap, (long)seminfo.semmns, (long)1, "semmap", seminfo.semmap);

	for (i = 0; i < seminfo.semmns; i++) {
		init_sema(&sem[i].semncnt_wait, 0, 0, SEMGATE);
		init_sema(&sem[i].semzcnt_wait, 0, 0, SEMGATE);
	}

	for (i = 0; i < seminfo.semmni  ; i++) {
		init_sema(&sema[i].semid_mutex, 0, 0, SEMGATE);
		init_lock(&sema[i].semid_lock, SEMGATE);
		sema[i].semid_flag = SFALSE;
	}

	semfup = (struct sem_undo *)semu;
	for (i = 0; i < seminfo.semmnu - 1; i++) {
		init_lock(&semfup->un_lock, SEMGATE);
		semfup->un_np = (struct sem_undo *)((u_int)semfup+seminfo.semusz);
		semfup = semfup->un_np;
	}
	init_lock(&semfup->un_lock, SEMGATE);
	semfup->un_np = NULL;
	semfup = (struct sem_undo *)semu;
}

/*
**	semop - Semop system call.
*/

#define GETSEMTMP(n)	if ((n) > SSEMOPM) { \
				p_sema(&gsemtmp_mutex, PSEM); \
				semtmpp = (union proto_semtmp *)&semtmp; \
			} else { \
				semtmpp = (union proto_semtmp *)&lsemtmp; \
			}

#define FREESEMTMP	if (semtmpp == (union proto_semtmp *)&semtmp) \
				v_sema(&gsemtmp_mutex)

static
semop(uap)
	register struct {
		int		semid;
		struct sembuf	*sops;
		u_int		nsops;
	}	*uap;
{
	register struct sembuf		*op;	/* ptr to operation */
	register int			i;	/* loop control */
	register struct semid_ds	*sp;	/* ptr to associated header */
	register struct sem		*semp;	/* ptr to semaphore */
	int	again;
	union ssemtmp lsemtmp;
	union proto_semtmp *semtmpp;
	char tu_error;

	GETSEMTMP(uap->nsops);
	if(uap->nsops > seminfo.semopm) {
		tu_error = E2BIG;
	} else {
		tu_error = copyin((caddr_t)uap->sops, (caddr_t)(*semtmpp).semops
			, uap->nsops * sizeof(*op));
	}

	if((sp = semconv(uap->semid, 0)) == NULL)  {
		FREESEMTMP;
		return;
	}

	u.u_error = tu_error;
	if(u.u_error) {
		v_lock(&(sp->semid_lock), SPL0);
		FREESEMTMP;
		return;
	}

	/* Verify that sem #s are in range and permissions are granted. */
	for(i = 0, op = (*semtmpp).semops;i++ < uap->nsops;op++) {
		if(ipcaccess(&sp->sem_perm, (ushort)(op->sem_op ? SEM_A : SEM_R)))  {
			v_lock(&(sp->semid_lock), SPL0);
			FREESEMTMP;
			return;
		}
		
		if(op->sem_num >= sp->sem_nsems) {
			v_lock(&(sp->semid_lock), SPL0);
			FREESEMTMP;
			u.u_error = EFBIG;
			return;
		}
	}
	again = 0;
check:
	/* Loop waiting for the operations to be satisified atomically. */
	/* Actually, do the operations and undo them if a wait is needed
		or an error is detected. */
	if (again) {
		if (semtmpp == &semtmp) {
			GETSEMTMP(uap->nsops);
			tu_error = copyin((caddr_t)uap->sops,
				(caddr_t)(*semtmpp).semops,
				uap->nsops * sizeof(*op));
		}
		/* Verify that the semaphores haven't been removed. */
		if(semconv(uap->semid, 0) == NULL) {
			FREESEMTMP;
			v_lock(&(sp->semid_lock), SPL0);
			u.u_error = EIDRM;
			return;
		}
		/* copy in user operation list after sleep */
		u.u_error = tu_error;
		if(u.u_error) {
			FREESEMTMP;
			v_lock(&(sp->semid_lock), SPL0);
			return;
		}
		
	}
	again = 1;

	for(i = 0, op = (*semtmpp).semops;i < uap->nsops;i++, op++) {
		semp = sp->sem_base + op->sem_num;
		if(op->sem_op > 0) {
			if(op->sem_op + (long)semp->semval > seminfo.semvmx ||
				((op->sem_flg & SEM_UNDO) &&
				semaoe(op->sem_op, uap->semid, (int)op->sem_num))) {
				if(u.u_error == 0) {
#ifdef DEBUG
					if (vsem_debug) {
						printf("semop: sem_op %d semval %d semvmx %d\n",
						op->sem_op, semp->semval,
						seminfo.semvmx);
					}
#endif DEBUG
					u.u_error = ERANGE;
				}
				if(i)
					semundo((*semtmpp).semops, i, uap->semid, sp);
				FREESEMTMP;
				v_lock(&(sp->semid_lock), SPL0);
				return;
			}
			semp->semval += op->sem_op;
			SEMWAKEUP(&semp->semncnt_wait);

			if(!semp->semval) {
				SEMWAKEUP(&semp->semzcnt_wait);
			}
			continue;
		}
		if(op->sem_op < 0) {
			if(semp->semval >= -op->sem_op) {
				if(op->sem_flg & SEM_UNDO &&
					semaoe(op->sem_op, uap->semid, (int)op->sem_num)) {
					if(i)
						semundo((*semtmpp).semops, i, uap->semid, sp);
					v_lock(&(sp->semid_lock), SPL0);
					FREESEMTMP;
					return;
				}
				semp->semval += op->sem_op;
				if(!semp->semval) {
					SEMWAKEUP(&semp->semzcnt_wait);
				}
				continue;
			}
			if(i)
				semundo((*semtmpp).semops, i, uap->semid, sp);
			if(op->sem_flg & IPC_NOWAIT) {
				v_lock(&(sp->semid_lock), SPL0);
				FREESEMTMP;
				u.u_error = EAGAIN;
				return;
			}
			FREESEMTMP;
			SEMSLEEP(&semp->semncnt_wait, sp, PSEMN);
			goto check;
		}
		if(semp->semval) {
			if(i)
				semundo((*semtmpp).semops, i, uap->semid, sp);
			if(op->sem_flg & IPC_NOWAIT) {
				v_lock(&(sp->semid_lock), SPL0);
				FREESEMTMP;
				u.u_error = EAGAIN;
				return;
			}
			FREESEMTMP;
			SEMSLEEP(&semp->semzcnt_wait, sp, PSEMZ); 
			goto check;
		}
	}

	/* All operations succeeded.  Update sempid for accessed semaphores. */
	for(i = 0, op = (*semtmpp).semops;i++ < uap->nsops;
		(sp->sem_base + (op++)->sem_num)->sempid = u.u_procp->p_pid);
	FREESEMTMP;
	sp->sem_otime = (time_t)time.tv_sec;
	v_lock(&(sp->semid_lock), SPL0);
	u.u_r.r_val1 = 0;
}


/*
**	semsys - System entry point for semctl, semget, and semop system calls.
*/

semsys()
{
	int	semctl(),
		semget(),
		semop();
	static int	(*calls[])() = {semctl, semget, semop};
	register struct a {
		u_int	id;	/* function code id */
	}	*uap = (struct a *)u.u_ap;

	if (uap->id > 2) {
		u.u_error = EINVAL;
		return;
	}
	(*calls[uap->id])(&u.u_arg[1]);
}

/*
**	semundo - Undo work done up to finding an operation that can't be done.
		assume semaphore locked by caller.
*/

static
semundo(op, n, id, sp)
register struct sembuf		*op;	/* first operation that was done ptr */
register int			n,	/* # of operations that were done */
				id;	/* semaphore id */
register struct semid_ds	*sp;	/* semaphore data structure ptr */
{
	register struct sem	*semp;	/* semaphore ptr */

	for(op += n - 1;n--;op--) {
		if(op->sem_op == 0)
			continue;
		semp = sp->sem_base + op->sem_num;
		semp->semval -= op->sem_op;
		if(op->sem_flg & SEM_UNDO)
			(void)semaoe(-op->sem_op, id, (int)op->sem_num);
	}
}

/*
**	semunrm - Undo entry remover.
**
**	This routine is called to clear all undo entries for a set of semaphores
**	that are being removed from the system or are being reset by SETVAL or
**	SETVALS commands to semctl.
*/

static
semunrm(id, low, high)
int	id;	/* semid */
ushort	low,	/* lowest semaphore being changed */
	high;	/* highest semaphore being changed */
{
	register struct sem_undo	*pp,	/* ptr to predecessor to p */
					*p;	/* ptr to current entry */
	register struct undo		*up;	/* ptr to undo entry */
	register int			i,	/* loop control */
					j;	/* loop control */

	pp = NULL;
	(void)p_lock(&semunp_lock, SPLSEM);
	p = semunp;
	while(p != NULL) {

		/* Search through current structure for matching entries. */
		(void)p_lock(&p->un_lock, SPLSEM);
		for(up = p->un_ent, i = 0;i < p->un_cnt;) {
			if(id < up->un_id)
				break;
			if(id > up->un_id || low > up->un_num) {
				up++;
				i++;
				continue;
			}
			if(high < up->un_num)
				break;
			for(j = i;++j < p->un_cnt;
				p->un_ent[j - 1] = p->un_ent[j]);
			p->un_cnt--;
		}

		/* Reset pointers for next round. */
		if(p->un_cnt == 0) {
			v_lock(&p->un_lock, SPLSEM);
			/* Remove from linked list. */
			if(pp == NULL) {
				semunp = p->un_np;
				p->un_np = NULL;
				p = semunp;
			} else {
				pp->un_np = p->un_np;
				p->un_np = NULL;
				p = pp->un_np;
			}
		} else {
			v_lock(&p->un_lock, SPLSEM);
			pp = p;
			p = p->un_np;
		}
	}
	v_lock(&semunp_lock, SPLSEM);
}

#endif SVSEMA
