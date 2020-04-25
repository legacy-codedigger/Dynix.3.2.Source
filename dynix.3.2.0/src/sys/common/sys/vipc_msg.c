/*
 * $Copyright: $
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

#ifndef lint
static char rcsid[] = "$Header: vipc_msg.c 2.6 1991/06/21 22:53:02 $";
#endif

/*
 * $Log: vipc_msg.c,v $
 *
 *
 *
 */

/*
**	Inter-Process Communication Message Facility.
*/

#ifdef SVMESG
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ipc.h"
#include "../h/msg.h"
#include "../h/map.h"
#include "../h/buf.h"

#include "../machine/intctl.h"

extern struct map	msgmap[];	/* msg allocation map */
lock_t	msgmap_lock;
sema_t	msgmap_wait;

extern struct msqid_ds	msgque[];	/* msg queue headers */
lock_t	msgque_lock;

extern struct msg	msgh[];		/* message headers */
extern struct msginfo	msginfo;	/* message parameters */
struct msg		*msgfp;	/* ptr to head of free header list */
lock_t	msgfp_lock;
sema_t	msgfp_wait;

extern char Bmsg[];
caddr_t		msg;		/* base address of message buffer */
extern struct timeval	time;	/* system idea of date */

extern long	rmalloc();

struct ipc_perm		*ipcget();
struct msqid_ds		*msgconv();

int msgrlog[3];
#define MSGFP_WAIT	0
#define MSGMAP_WAIT	1
#define MSG_RW		2

int msgqlog[3];

/* Convert bytes to msg segments. */
#define	btoq(X)	(long)((X + msginfo.msgssz - 1) / msginfo.msgssz)

/* Convert spot index from alloc_msg() into addr in memory */
#define MSGADDR(spot) (msg + (msginfo.msgssz * ((spot)-1)))

/*
 * The max # messages which can be put on a queue.  This number is
 * derived from the fact that msg_qnum is an unsigned short.
 */
#define MSGMAXONQ (65535)

/*
 * msgconv()
 *	Convert queue id into a msgid_ds pointer
 *
 * No locked held on entry, locked msgid_ds structure returned on success,
 * NULL on failure.
 */

static struct msqid_ds *
msgconv(id)
	register int id;
{
	register struct msqid_ds *qp;	/* ptr to associated q slot */

	qp = &msgque[(unsigned)id % (unsigned)msginfo.msgmni];
	(void)p_lock(&qp->mutex, SPLMSG);
	(void)p_lock(&msgque_lock, SPLMSG);
	if((qp->msg_perm.mode & IPC_ALLOC) == 0 ||
			id / msginfo.msgmni != qp->msg_perm.seq) {
		v_lock(&msgque_lock, SPLMSG);
		v_lock(&qp->mutex, SPL0);
		u.u_error = EINVAL;
		return(NULL);
	}
	v_lock(&msgque_lock, SPLMSG);
	return(qp);
}

/*
 * alloc_msg
 *	Allocate memory for message body
 *
 * Called with no locks held, returns with no locks.  Return value
 * is message block index on success, 0 on error.  On error, u_error
 * is filled in with the appropriate errno.  Note that because 0 is
 * an error value, the value from this routine on success is offset by
 * -1 before being used.  Only the MSGADDR macro knows this; it is used
 * in the appropriate places.  This routine can sleep if NOWAIT is not
 * specified.  cnt is in bytes.
 */
static
alloc_msg(cnt, msgflg)
	unsigned cnt;
	int msgflg;
{
	int spot;

	/*
	 * Loop trying to get message memory
	 */
	for (;;) {
		/*
		 * Take lock on msgmap
		 */
		(void)p_lock(&msgmap_lock, SPLMSG);

		/*
		 * Try and allocate our space
		 */
		spot = (int)rmalloc(msgmap, btoq(cnt));

		/*
		 * Leave this loop on success
		 */
		if (spot) {
			v_lock(&msgmap_lock, SPL0);
			return(spot);
		}

		/*
		 * If NOWAIT, just return error
		 */
		if (msgflg & IPC_NOWAIT) {
			v_lock(&msgmap_lock, SPL0);
			u.u_error = EAGAIN;
			return(0);
		}

		/*
		 * trap will return EINTR if interrupted from sleep
		 */
		if (setjmp(&u.u_qsave)) {
			msgqlog[MSGMAP_WAIT]++;
			u.u_error = EINTR;
			return(0);
		}
		msgrlog[MSGMAP_WAIT]++;

		/*
		 * Transfer map lock to sleeping semaphore, wait for
		 * some memory to show up.
		 */
		p_sema_v_lock(&msgmap_wait, PMSG, &msgmap_lock, SPL0);
	}
}

/*
 * free_msg()
 *	Free message body back to pool
 *
 * This routine handles its own locking.  "spot" is the value returned by
 * a previous call to alloc_msg; "cnt" is in bytes.
 */
static void
free_msg(spot, cnt)
	int spot;
	unsigned cnt;
{
	spl_t s;

	s = p_lock(&msgmap_lock, SPLMSG);
	rmfree(msgmap, btoq(cnt), (long)spot);
	if (blocked_sema(&msgmap_wait)) {
		vall_sema(&msgmap_wait);
	}
	v_lock(&msgmap_lock, s);
}

/*
 * alloc_mhead()
 *	Allocate a message head
 *
 * This routine handles its own locking.  Returns a struct msg *
 * on success, or NULL on failure.  On failure, u_error is set appropriately.
 * Can sleep if NOWAIT not specified.
 */
static struct msg *
alloc_mhead(msgflg)
	int msgflg;
{
	struct msg *mp;

	/*
	 * Loop trying to allocate the message head
	 */
	for (;;) {
		/*
		 * Take message head pool mutex
		 */
		(void)p_lock(&msgfp_lock, SPLMSG);

		/*
		 * If something in pool, return it
		 */
		if (msgfp)
			break;

		/*
		 * If NOWAIT, return error
		 */
		if (msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			v_lock(&msgfp_lock, SPL0);
			return(NULL);
		}

		/*
		 * trap will return EINTR if interrupted from sleep
		 */
		if (setjmp(&u.u_qsave)) {
			msgqlog[MSGFP_WAIT]++;
			u.u_error = EINTR;
			return(NULL);
		}
		msgrlog[MSGFP_WAIT]++;
		/*
		 * Fall asleep waiting for more
		 */

		p_sema_v_lock(&msgfp_wait, PMSG, &msgfp_lock, SPL0);
	}

	/*
	 * Have at least one, take it off the queue, unlock, and return it
	 */
	mp = msgfp;
	msgfp = mp->msg_next;
	v_lock(&msgfp_lock, SPL0);
	return(mp);
}

/*
 * free_mhead()
 *	Free a message header back to the pool
 *
 * This routine handles its own locking.
 */
static void
free_mhead(mp)
	struct msg *mp;
{
	spl_t s;

	s = p_lock(&msgfp_lock, SPLMSG);
	mp->msg_next = msgfp;
	msgfp = mp;
	if (blocked_sema(&msgfp_wait))
		v_sema(&msgfp_wait);
	v_lock(&msgfp_lock, s);
}

/*
 * unlink_msg()
 *	Remove a message from its message queue, update the message queue
 *
 * Called with queue locked; returns with queue still locked.  The
 * message is unlinked from the queue, but is not freed; the caller
 * is expected to free it when they're completely done with it.
 */
static void
unlink_msg(qp, pmp, mp)
	struct msqid_ds *qp;
	struct msg *mp, *pmp;
{
	if (pmp == NULL)
		qp->msg_first = mp->msg_next;
	else
		pmp->msg_next = mp->msg_next;
	if (mp->msg_next == NULL)
		qp->msg_last = pmp;
	qp->msg_qnum--;
	if (blocked_sema(&qp->msg_writers)) {
		vall_sema(&qp->msg_writers);
	}
}

/*
 * msgfree()
 *	Free up a message from queue, wake sleepers
 *
 * Called with queue locked; returns queue still locked.
 */
static void
msgfree(qp, pmp, mp)
	struct msqid_ds	*qp;	/* ptr to q of mesg being freed */
	struct msg	*mp,	/* ptr to msg being freed */
			*pmp;	/* ptr to mp's predecessor */
{
	/*
	 * Take message off queue
	 */
	unlink_msg(qp, pmp, mp);

	/*
	 * Free up message text.
	 */
	if (mp->msg_ts)
		free_msg((int)mp->msg_spot, (ushort)mp->msg_ts);

	/* Free up header */
	free_mhead(mp);
}

/*
 * msgctl()
 *	Implement the msgctl() system call
 */
static
msgctl(uap)
	register struct {
		int		msgid,
				cmd;
		struct Dmsqid_ds *buf;
	} *uap;
{
	struct Dmsqid_ds		ds;	/* queue work area */
	register struct msqid_ds	*qp;	/* ptr to associated q */

	u.u_r.r_val1 = 0;
	switch(uap->cmd) {

	/*
	 * Remove message queue identifier from system after freeing
	 * all resources under it.
	 */
	case IPC_RMID:
		if ((qp = msgconv(uap->msgid)) == NULL) {
			return;
		}
		if (u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
			&& !suser()) {
			break;
		}
		while (qp->msg_first)
			msgfree(qp, (struct msg *)NULL, qp->msg_first);
		qp->msg_cbytes = 0;
		if (blocked_sema(&qp->msg_readers)) {
			vall_sema(&qp->msg_readers);
		}
		if (blocked_sema(&qp->msg_writers)) {
			vall_sema(&qp->msg_writers);
		}
		(void)p_lock(&msgque_lock, SPLMSG);
		if (uap->msgid + msginfo.msgmni < 0)
			qp->msg_perm.seq = 0;
		else
			qp->msg_perm.seq++;
		qp->msg_perm.mode = 0;
		v_lock(&msgque_lock, SPLMSG);
		break;

	/*
	 * Set uid, gid, mode, qbytes fields of a message queue
	 */
	case IPC_SET:
		if(copyin((caddr_t)uap->buf, (caddr_t)&ds, 
					(unsigned)sizeof(ds))) {
			u.u_error = EFAULT;
			break;
		}
		if((qp = msgconv(uap->msgid)) == NULL) 
			return;
		if(u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
			 && !suser())
			break;
		if(ds.Dmsg_qbytes > qp->msg_qbytes && !suser())
			break;
		qp->msg_perm.uid = ds.Dmsg_perm.uid;
		qp->msg_perm.gid = ds.Dmsg_perm.gid;
		qp->msg_perm.mode = (qp->msg_perm.mode & ~0777) |
			(ds.Dmsg_perm.mode & 0777);
		qp->msg_qbytes = ds.Dmsg_qbytes;
		qp->msg_ctime = (int)time.tv_sec;
		break;

	/*
	 * Provide Dmsqid_ds structure to user
	 */
	case IPC_STAT:
		if((qp = msgconv(uap->msgid)) == NULL) 
			return;
		if(ipcaccess(&qp->msg_perm, MSG_R))
			break;
		ds = qp->Dmsq;
		v_lock(&qp->mutex, SPL0);
		if(copyout((caddr_t)&ds, (caddr_t)uap->buf, 
					(unsigned)sizeof(ds))) {
			u.u_error = EFAULT;
			return;
		}
		return;

	default:
		u.u_error = EINVAL;
		return;
	}
	v_lock(&qp->mutex, SPL0);
}

/*
 * msgget()
 *	Implement msgget() system call
 *
 * Called with no locks held, handles locking within this routine.
 */
static
msgget(uap)
	register struct {
		key_t	key;
		int	msgflg;
	} *uap;
{
	register struct msqid_ds	*qp;	/* ptr to associated q */
	int				s;	/* ipcget status return */

	(void)p_lock(&msgque_lock, SPLMSG);
	if ((qp = (struct msqid_ds *)ipcget(uap->key, uap->msgflg, 
			(struct ipc_perm *)msgque, 
			msginfo.msgmni, sizeof(*qp), &s)) == NULL) {
		v_lock(&msgque_lock, SPL0);
		return;
	}

	if (s) {
		/*
		 * This is a new queue.  Finish initialization.
		 */
		qp->msg_first = qp->msg_last = NULL;
		qp->msg_qnum = 0;
		qp->msg_qbytes = msginfo.msgmnb;
		qp->msg_lspid = qp->msg_lrpid = 0;
		qp->msg_stime = qp->msg_rtime = 0;
		qp->msg_ctime = (int)time.tv_sec;
	}
	
	u.u_r.r_val1 = qp->msg_perm.seq * msginfo.msgmni + (qp - msgque);
	v_lock(&msgque_lock, SPL0);
}

/*
 * msginit()
 *	Called during system startup to initialize message queues.
 */

msginit()
{
	register int		i;	/* loop control */
	register struct msg	*mp;	/* ptr to msg begin linked */

	msg = Bmsg;
	init_lock(&msgque_lock, MSGGATE);
	rminit(msgmap, (long)msginfo.msgseg, (long)1, "msgmap", msginfo.msgmap);
	init_lock(&msgmap_lock, MSGGATE);
	init_sema(&msgmap_wait, 0, 0, MSGGATE);

	for(i = 0, mp = msgfp = msgh;++i < msginfo.msgtql;mp++) 
		mp->msg_next = mp + 1;
	init_lock(&msgfp_lock, MSGGATE);
	init_sema(&msgfp_wait, 0, 0, MSGGATE);

	for(i = 0; i < msginfo.msgmni; i++) {
		init_lock(&msgque[i].mutex, MSGGATE);
		init_sema(&msgque[i].msg_writers, 0, 0, MSGGATE);
		init_sema(&msgque[i].msg_readers, 0, 0, MSGGATE);
	}
}


/*
 * msgrcv()
 *	Implement msgrcv() system call
 */
static
msgrcv(uap)
	register struct {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		long		msgtyp;
		int		msgflg;
	} *uap;
{
	struct msg 	*mp,	/* ptr to msg on q */
			*pmp,	/* ptr to mp's predecessor */
			*smp,	/* ptr to best msg on q */
			*spmp;	/* ptr to smp's predecessor */
	struct msqid_ds	*qp;	/* ptr to associated q */
	int		sz;	/* transfer byte count */

	/*
	 * Sanity check message size
	 */
	if (uap->msgsz < 0) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Validate user buffer
	 */
	if (!useracc((char *)uap->msgp, (u_int)(uap->msgsz + sizeof(uap->msgtyp)), B_WRITE)) {
		u.u_error = EFAULT;
		return;
	}

	if ((qp = msgconv(uap->msqid)) == NULL)
		return;
	smp = spmp = NULL;
	if (ipcaccess(&qp->msg_perm, MSG_R))
		goto leave;
findmsg:
	pmp = NULL;
	mp = qp->msg_first;
	if (uap->msgtyp == 0) {
		smp = mp;
	} else {
		for (; mp; pmp = mp, mp = mp->msg_next) {
			if (uap->msgtyp > 0) {
				if (uap->msgtyp != mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
				break;
			}
			if (mp->msg_type <= -uap->msgtyp) {
				if (smp && smp->msg_type <= mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
			}
		}
	}

	/*
	 * If found a message that met our selection criteria, see about
	 * handing it to the user.
	 */
	if (smp) {
		if (uap->msgsz < (ushort)smp->msg_ts) {
			if(!(uap->msgflg & MSG_NOERROR)) {
				u.u_error = E2BIG;
				smp = NULL;
				goto leave;
			} else {
				sz = uap->msgsz;
			}
		} else {
			sz = (ushort)smp->msg_ts;
		}

		/*
		 * Set return value to number of bytes, remove message
		 * from queue.  We'll actually copy it to the user buffer
		 * once we've dropped our lock--we "know" that it won't
		 * fail due to the useracc() call above.
		 */
		u.u_r.r_val1 = sz;
		qp->msg_cbytes -= (ushort)smp->msg_ts;
		qp->msg_lrpid = u.u_procp->p_pid;
		qp->msg_rtime = (int)time.tv_sec;
		unlink_msg(qp, spmp, smp);
		goto leave;
	}

	/*
	 * No message that we wanted.  If NOWAIT, just return error
	 */
	if (uap->msgflg & IPC_NOWAIT) {
		u.u_error = ENOMSG;
		goto leave;
	}

	/*
	 * Otherwise fall asleep, handle waking up again
	 */
	if (msgsleep(qp, &qp->msg_readers))
		return;
	if (msgconv(uap->msqid) == NULL) {
		u.u_error = EIDRM;
		return;
	}
	goto findmsg;
leave:
	v_lock(&qp->mutex, SPL0);

	/*
	 * If we have taken a message, we now need to copy it out to the
	 * user's buffer.  We "know" that the buffer is OK, as we verified
	 * it with useracc().  Thus, the panic below.
	 */
	if (smp) {
		int err;

		/*
		 * Copy out type, and message body if any
		 */
		err = copyout((caddr_t)&smp->msg_type,
			(caddr_t)uap->msgp, (unsigned)sizeof(smp->msg_type));

		if (sz) {
			err |= copyout( MSGADDR(smp->msg_spot),
				(caddr_t)uap->msgp + sizeof(smp->msg_type), 
				(unsigned)sz);
		}
		ASSERT(!err, "msgrcv: memory changed");
		/*
		 *+ msgrcv() found that the state of a user's buffer changed
		 *+ between function entry and the actual access.  This is an
		 *+ internal inconsistency.
		 */

		/*
		 * Free the message head and body back to their pools
		 */
		if (smp->msg_ts)
			free_msg((int)smp->msg_spot, (ushort)smp->msg_ts);
		free_mhead(smp);
	}
}

/*
 * msgsnd()
 *	Implement the msgsnd() system call.
 */
static
msgsnd(uap)
	register struct {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		int		msgflg;
	} *uap;
{
	struct msqid_ds	*qp;	/* ptr to associated q */
	struct msg	*mp;	/* ptr to allocated msg hdr */
	int		cnt,	/* byte count */
			spot;	/* msg pool allocation spot */
	long		type;	/* msg type */
	int		lckd;

	/*
	 * Sanity on message size
	 */
	if ((cnt = uap->msgsz) < 0 || cnt > msginfo.msgmax) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Get type value, sanity check it
	 */
	u.u_error = copyin((caddr_t)uap->msgp, (caddr_t)&type, 
		(unsigned)sizeof(type));
	if (u.u_error)
		return;
	if (type < 1) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Set initial values for use of out:
	 */
	spot = 0;
	mp = NULL;

	/*
	 * If user is sending message with non-zero number of bytes,
	 * need to get message body memory.
	 */
	if (cnt) {
		/*
		 * Get some space, return if it failed.  Setting of u_error
		 * is done within alloc_msg().
		 */
		spot = alloc_msg((unsigned)cnt, uap->msgflg);
		if (!spot) {
			goto out;
		}

		/*
		 * Copy in the message body, error out on bogus user buffer
		 */
		u.u_error = copyin((caddr_t)uap->msgp + sizeof(type),
			MSGADDR(spot), (unsigned)cnt);
		if (u.u_error)
			goto out;
	}

	/*
	 * Get message head, return if failed.  alloc_mhead() handles
	 * u_error.
	 */
	mp = alloc_mhead(uap->msgflg);
	if (!mp)
		goto out;

	/*
	 * Now actually get the queue.  We got our memory first to minimize
	 * the nesting of locks, and to allow the copyin()'s to happen without
	 * any locks held.
	 */
	if ((qp = msgconv(uap->msqid)) == NULL)
		goto out;
	if (ipcaccess(&qp->msg_perm, MSG_W)) {
		v_lock(&qp->mutex, SPL0);
		goto out;
	}
	lckd = 1;

retry:
	/*
	 * Be sure that q has not been removed.
	 */
	if (lckd == 0 && msgconv(uap->msqid) == NULL) {
		u.u_error = EIDRM;
		goto out;
	}

	/*
	 * If message won't currently fit on target queue, sleep
	 * and try again.
	 */
	if ((cnt + qp->msg_cbytes > qp->msg_qbytes)
			|| (qp->msg_qnum == MSGMAXONQ)) {

		if(uap->msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			v_lock(&qp->mutex, SPL0);
			goto out;
		}
		if (msgsleep(qp, &qp->msg_writers))
			goto out;

		/*
		 * msgsleep() returns with queue unlocked (it uses
		 * p_sema_v_lock), flag that we don't hold the lock now.
		 */
		lckd = 0;

		goto retry;
	}

	/*
	 * Put messge buffer on queue
	 */
	qp->msg_qnum++;
	qp->msg_cbytes += cnt;
	mp->msg_next = NULL;
	mp->msg_type = type;
	mp->msg_ts = cnt;
	mp->msg_spot = cnt ? spot : -1;
	if(qp->msg_last == NULL)
		qp->msg_first = qp->msg_last = mp;
	else {
		qp->msg_last->msg_next = mp;
		qp->msg_last = mp;
	}
	qp->msg_lspid = u.u_procp->p_pid;
	qp->msg_stime = (int)time.tv_sec;
	if (blocked_sema(&qp->msg_readers)) {
		vall_sema(&qp->msg_readers);
	}
	u.u_r.r_val1 = 0;
	v_lock(&qp->mutex, SPL0);

	/*
	 * These resources have successfully been sent, flag that we
	 * don't need to clean them up.
	 */
	spot = 0;
	mp = NULL;
out:
	/*
	 * If we have failed while holding a message buffer, return
	 * it to the free pool.  Same for message head.
	 */
	if (spot)
		free_msg(spot, (unsigned)cnt);
	if (mp)
		free_mhead(mp);
}

/*
 * msgsys()
 *	Actual system entry point for all message system calls
 */
msgsys()
{
	int		msgctl(),
			msgget(),
			msgrcv(),
			msgsnd();
	static int	(*calls[])() = { msgget, msgctl, msgrcv, msgsnd };
	register struct a {
		unsigned	id;	/* function code id */
		int		*ap;	/* arg pointer for recvmsg */
	}		*uap = (struct a *)u.u_ap;

	if (uap->id > 3) {
		u.u_error = EINVAL;
		return;
	}
	(*calls[uap->id])(&u.u_arg[1]);
}

/*
 * msgsleep()
 *	Common code to fall asleep on a message queue
 *
 * Entered with message queue mutex held; uses p_sema_v_lock to fall
 * asleep on the specified semaphore.  On return the lock is still
 * released; the call must then re-validate the message queue, as an
 * IPC_RMID could have been done to it.
 */
static
msgsleep(qp, wait_sema)
	struct msqid_ds *qp;
	sema_t	*wait_sema;
{
	if (setjmp(&u.u_qsave)) {
		msgqlog[MSG_RW]++;
		u.u_error = EINTR;
		return(1);
	}
	/*
	 * Transfer to semaphore to sleep
	 */
	msgrlog[MSG_RW]++;
	p_sema_v_lock(wait_sema, PMSG, &qp->mutex, SPL0);
	return(0);
}
#endif SVMESG
