/* $Copyright:	$
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

#ifndef	lint
static	char	rcsid[] = "$Header: uipc_syscalls.c 2.17 91/03/11 $";
#endif

/*
 * upic_syscalls.c
 *	IPC syscalls
 */

/* $Log:	uipc_syscalls.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/uio.h"

/*
 * System call interface to the socket abstraction.
 *
 * socket(domain, socktype, protocol)
 * system call.  Its function is to acquire the resources required
 * for a socket data structure.  The socket data structure is
 * associated with a u.area the same way that file descriptors
 * are associated.  socket requires the Unix file system functions
 * to acquire file system items.  kernel functions called are:
 *
 * 	falloc() - to allocate a Unix struct file .
 * 		this references the struct socket.
 * 	ffree(fp) - to free the struct file if things go bad.
 *
 * When the function completes, a socket is created and the
 * combination of domain, socket type and and protocol have
 * had their chance to personalize the socket. (e.g. a protocol
 * specific control block can be attached, limits placed upon
 * data queues, various flags set for characterizing the socket
 * management, etc.
 *
 * TMP notes:
 * 	this routine is called at user level.  The file/socket
 * 	data structures are not yet active and so should not
 * 	be concurrently accessed anywhere.  falloc() manages
 * 	mutex on the kernel's struct file data base, including
 * 	exclusion of the same user process on the u.area.
 *
 * Signalling issues:
 * 	If the process is signalled to quit before completion
 * 	of socket creation, then the socket resources must
 * 	be returned.
 */

struct	file *getsock();
extern	struct fileops socketops;

socket()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
	} *uap = (struct a *)u.u_ap;
	struct socket *so;
	register struct file *fp;

	if ((fp = falloc()) == (struct file *)NULL) {
		return;
	}

	/*
	 * these should not be referenced until socket is bound 
 	 * no mutex required.
 	 * However, up until the socketops field is filled in,
 	 * there are no handles for clean up.
 	 */

	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_SOCKET;
	fp->f_ops = &socketops;

	/*
	 * struct file setup, tack on a struct socket, let the appropriate
	 * protocol engines characterize the struct via PRU_ATTACH, etc.
	 */

	u.u_error = socreate(uap->domain, &so, uap->type, uap->protocol);

	/*
	 * socreate returns the error code (no error == 0) and 
	 * acquires the resources to describe the socket.  A pointer
	 * to the struct socket is stored into so which is on the stack.
	 * This value is then stored into the struct file
	 */

	if (u.u_error)
		goto bad;

	/*
	 * link struct file to struct socket
	 */

	fp->f_data = (caddr_t)so;
	ofile_install(u.u_ofile_tab, u.u_r.r_val1, fp);
	return;
bad:
	ofile_install(u.u_ofile_tab, u.u_r.r_val1, (struct file *) NULL);

	/*
	 * ffree() is interface to mutex freeing the struct file
	 */

	ffree(fp);
	return;
}

/*
 * the bind system call activates the socket in the sense that
 * it sets up the identification of which domain item (Internet
 * uses a "port", Unix domain uses a special file).  This is a
 * local address in the sense that the appropriate protocol
 * engine understands how to find this address and map it to
 * the user's socket via PRU_BIND call.
 */

bind()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	namelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct mbuf *nam;

	fp = getsock(uap->s);
	if (fp == (struct file *)NULL)
		return;
	u.u_error = sockargs(&nam, uap->name, uap->namelen);
	if (u.u_error)
		return;
	u.u_error = sobind((struct socket *)fp->f_data, nam);
	m_freem(nam);
	return;
}

/*
 * listen() system call is used to establish a Server's willingness to
 * accept a Client's connect request to the bound address.  The listen
 * socket is used as a template to create accept sockets.
 */

listen()
{
	register struct a {
		int	s;
		int	backlog;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	fp = getsock(uap->s);
	if (fp == (struct file *)NULL)
		return;
	u.u_error = solisten((struct socket *)fp->f_data, uap->backlog);
	return;
}

/*
 * accept completes the connect/accept sequence of Server/Client
 * protocol - it returns a new socket
 * which gets its characteristics from the "listen" socket specified
 * in the accept system call.
 *
 * locking requirements for "parent" socket - guard against disconnect.
 */

accept()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	*anamelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct mbuf *nam;
	int namelen;
	register struct socket *so;
	struct socket_peer *saved_partial_sopp;
	struct socket_peer *saved_head_sopp;
	struct socket *aso;
	spl_t splevel;

	if (uap->name == (caddr_t)NULL)
		goto noname;
	u.u_error = copyin((caddr_t)uap->anamelen, (caddr_t)&namelen,
		sizeof (namelen));
	if (u.u_error)
		return;
	if (useracc((caddr_t)uap->name, (u_int)namelen, B_WRITE) == 0) {
		u.u_error = EFAULT;
		return;
	}
noname:
	fp = getsock(uap->s);
	if (fp == (struct file *)NULL)
		return;
	so = (struct socket *)fp->f_data;

	splevel = SOLOCK(so);
	saved_head_sopp = so->so_sopp;

	if ((so->so_options & SO_ACCEPTCONN) == 0) {
		u.u_error = EINVAL;
		SOUNLOCK(so, splevel);
		return;
	}	

	if ((so->so_state & SS_NBIO) && so->so_qlen == 0) {
		u.u_error = EWOULDBLOCK;
		SOUNLOCK(so, splevel);
		return;
	}

	/*
	 * user waits for v_sema on connect_sema and releases lock.  After
	 * lock released, state of socket is checked again.
	 *
	 * 	note - so_lock is *held*
	 *
	 * use sblock protocol to guard against multiple acceptors
	 */

	sblock(&so->so_snd);

	while ((so->so_qlen == 0) && (so->so_error == 0)) {

		/*
		 * while no connects to accept and no error
		 *
		 * release the connect semaphore and socket lock.
		 * When something happens that affects connection,
		 * semaphore gotten again.  Lock the socket again and see
		 * if there is an accept queued.
		 *
		 * Note that process can be signalled off of semaphore
		 * therefore let go of the sblock semaphore thereby allowing
		 * another acceptor in.  sblock/sbunlock allows only
		 * one acceptor looking at a particular connection coming
		 * in at a time in no particular order.
		 */

		/*
		 * if not able to receive -> ECONNABORTED
		 */

		if (so->so_state & SS_CANTRCVMORE ||
		    ((so->so_options & SO_ACCEPTCONN) == 0)) {
			so->so_error = ECONNABORTED;
			break;
		}

		sbunlock(&so->so_snd);

		p_sema_v_lock(so->so_conn_semap, PZERO+1,
				&(saved_head_sopp->sop_lock), splevel);

		splevel = p_lock(&(saved_head_sopp->sop_lock), SPLNET);
		sblock(&so->so_snd);
	}

	if (so->so_error) {
		u.u_error = so->so_error;
		sbunlock(&so->so_snd);
		v_lock(&(saved_head_sopp->sop_lock), splevel);
		return;
	}

	aso = so->so_q;
	saved_partial_sopp = aso->so_sopp;

	/*
	 * Now open a WINDOW in order to lock the remqueuee and
	 * check it out.  Since we still must have a reference to
	 * so, it cannot have disappeared, but its state could have
	 * changed.
	 */

	v_lock(&(saved_head_sopp->sop_lock), SPLNET);
	(void) p_lock(&saved_partial_sopp->sop_lock, SPLNET);
	(void) p_lock(&saved_head_sopp->sop_lock, SPLNET);

	/*
	 * sbunlock side of multiple accept protocol
	 * this allows another acceptor but OK since socket is locked
	 * until acceptee is remqueued.
	 */

	sbunlock(&so->so_snd);

	/*
	 * saved_partial_sopp must still be around since sockets
	 * are not removed from so_q by other processes.
	 * NOTE: refcnt already accounts for socket management
	 * reference to socket.
	 */

	nam = m_get(M_DONTWAIT, MT_SONAME);
	if (nam == (struct mbuf *)NULL) {
		v_lock(&(saved_partial_sopp->sop_lock), SPLNET);
		v_lock(&(saved_head_sopp->sop_lock), splevel);
		u.u_error = ENOBUFS;
		return;
	}

	/*
	 * create another socket for acceptee
	 */

	fp = falloc();
	if (fp == (struct file *)NULL) {
		v_lock(&(saved_partial_sopp->sop_lock), SPLNET);
		v_lock(&(saved_head_sopp->sop_lock), splevel);
		m_freem(nam);
		return;
	}

	/*
	 * dequeue the acceptee and return to accept() caller
	 * 
	 * panic here rather than sanity check because soqremque must be
	 * executed.
	 *
	 * MUTEX is via agreement that protocol engines will not
	 * detach from so_q.  They can disconnect however.
	 */
	  
	if (soqremque(aso, 1) == 0) {	/* so_locked, remove queued socket */
		panic("accept");
	}

	/*
	 * new socket is now on its own - unlock parent
	 */

	v_lock(&(saved_head_sopp->sop_lock), SPLNET);

	so = aso;			/* so now acceptee socket */

	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = FREAD|FWRITE;
	fp->f_ops = &socketops;
	fp->f_data = (caddr_t)so;

	u.u_error = soaccept(so, nam);

	if (u.u_error) {
		ffree(fp);
		m_freem(nam);
		so->so_state |= SS_NOFDREF;
		(void) soabort(so);
		splx(splevel);
		ofile_install(u.u_ofile_tab, u.u_r.r_val1, (struct file *)NULL);
		return;
	}

	v_lock(&(saved_partial_sopp->sop_lock), splevel);

	/*
	 * socket is accepted, copy soname into user buffer
	 */

	if (uap->name) {
		if (namelen > nam->m_len)
			namelen = nam->m_len;

		/*
		 * SHOULD COPY OUT A CHAIN HERE (sic 4.2bsd)
		 */

		(void) copyout(mtod(nam, caddr_t), (caddr_t)uap->name,
		    (u_int)namelen);
		(void) copyout((caddr_t)&namelen, (caddr_t)uap->anamelen,
		    sizeof (*uap->anamelen));
	}

	m_freem(nam);

	/*
	 * Finally, ok to install file-descriptor.
	 */
	ofile_install(u.u_ofile_tab, u.u_r.r_val1, fp);
}

/*
 * connect sends Client's connect request to the destination Server's
 * listen socket
 */

connect()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	namelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *nam;

	spl_t splevel;

	fp = getsock(uap->s);
	if (fp == (struct file *)NULL)
		return;
	so = (struct socket *)fp->f_data;

	/*
	 * sockargs copies the name argument to newly acquired mbufs, and
	 * points nam to them.
	 */

	u.u_error = sockargs(&nam, uap->name, uap->namelen);
	if (u.u_error)
		return;

	/*
	 * soconnect uses so_lock to mutex the socket
	 * soconnect also initiates a connect request on the appropriate
	 * network.
	 */

	u.u_error = soconnect(so, nam);
	if (u.u_error)
		goto bad;		/* so_lock *not* held */

	splevel = SOLOCK(so);		/* mutex socket struct */

	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
		u.u_error = EINPROGRESS;
		goto bad2;	/* so_lock held */
	}

	/*
	 * setup to recover if signalled out of connect request
	 */

	if (setjmp(&u.u_qsave)) {
		if (u.u_error == 0)
			u.u_error = EINTR;
		goto bad;	/* N.B. so_lock *not* held */
	}

	/*
	 * so_lock held for state check and p_sema replaces 4.2 sleep
	 */

	/*
	 * wait for connect to be complete (i.e. peer process completes
	 * connection perhaps after some network protocol).
	 */

	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0) {
		p_sema_v_lock(so->so_conn_semap, PZERO+1,
					&(so->so_sopp->sop_lock), splevel);
		(void) SOLOCK(so);
	}
	u.u_error = so->so_error;
	so->so_error = 0;
bad2:
	SOUNLOCK(so, splevel);
bad:
	m_freem(nam);
	return;
}

/*
 * create a related socketpair (like full duplex pipe)
 */

socketpair()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
		int	*rsv;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp1, *fp2;
	struct socket *so1, *so2;
	int sv[2];

	if (useracc((caddr_t)uap->rsv, 2 * sizeof (int), B_WRITE) == 0) {
		u.u_error = EFAULT;
		return;
	}
	u.u_error = socreate(uap->domain, &so1, uap->type, uap->protocol);
	if (u.u_error)
		return;
	u.u_error = socreate(uap->domain, &so2, uap->type, uap->protocol);
	if (u.u_error)
		goto free;
	fp1 = falloc();
	if (fp1 == (struct file *)NULL)
		goto free2;
	sv[0] = u.u_r.r_val1;
	fp1->f_flag = FREAD|FWRITE;
	fp1->f_type = DTYPE_SOCKET;
	fp1->f_ops = &socketops;
	fp1->f_data = (caddr_t)so1;
	fp2 = falloc();
	if (fp2 == (struct file *)NULL)
		goto free3;
	fp2->f_flag = FREAD|FWRITE;
	fp2->f_type = DTYPE_SOCKET;
	fp2->f_ops = &socketops;
	fp2->f_data = (caddr_t)so2;
	sv[1] = u.u_r.r_val1;
	u.u_error = soconnect2(so1, so2);
	if (u.u_error)
		goto free4;
	(void) copyout((caddr_t)sv, (caddr_t)uap->rsv, 2 * sizeof (int));
	ofile_install(u.u_ofile_tab, sv[0], fp1);
	ofile_install(u.u_ofile_tab, sv[1], fp2);
	return;
free4:
	ffree(fp2);
	ofile_install(u.u_ofile_tab, sv[1], (struct file *) NULL);
free3:
	ffree(fp1);
	ofile_install(u.u_ofile_tab, sv[0], (struct file *) NULL);
free2:
	(void) soclose(so2);
free:
	(void) soclose(so1);
	return;
}

/*
 * sendto() sends a datagram to a destination specified in the call
 * this means for example that a datagram socket can address a different
 * destination for every send operation on an unconnected socket.
 */

sendto()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
		caddr_t	to;
		int	tolen;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = uap->to;
	msg.msg_namelen = uap->tolen;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	sendit(uap->s, &msg, uap->flags);
	return;
}

/*
 * send sends data on a connected socket
 */

send()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	sendit(uap->s, &msg, uap->flags);
	return;
}

/*
 * sendmsg sends data that is specified in a message buffer
 */

sendmsg()
{
	register struct a {
		int	s;
		caddr_t	msg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[MSG_MAXIOVLEN];

	u.u_error = copyin(uap->msg, (caddr_t)&msg, sizeof (msg));
	if (u.u_error)
		return;
	if ((u_int)msg.msg_iovlen >= sizeof (aiov) / sizeof (aiov[0])) {
		u.u_error = EMSGSIZE;
		return;
	}
	u.u_error =
	    copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof (aiov[0])));
	if (u.u_error)
		return;
	msg.msg_iov = aiov;
	sendit(uap->s, &msg, uap->flags);
	return;
}

/*
 * sendit is a generic send routine called after send, sendmsg, sendto
 * have figured out where things go.
 */

sendit(s, mp, flags)
	int s;
	register struct msghdr *mp;
	int flags;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *to, *rights;
	int len;
	
	fp = getsock(s);
	if (fp == (struct file *)NULL)
		return;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = 0;
	auio.uio_offset = 0;
	auio.uio_resid = 0;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		if (iov->iov_len == 0)
			continue;
		if (useracc(iov->iov_base, (u_int)iov->iov_len, B_READ) == 0) {
			u.u_error = EFAULT;
			return;
		}
		auio.uio_resid += iov->iov_len;
		iov++;
	}
	if (mp->msg_name) {
		u.u_error =
		    sockargs(&to, mp->msg_name, mp->msg_namelen);
		if (u.u_error)
		{
			return;
		}
	} else
		to = 0;
	if (mp->msg_accrights) {
		u.u_error = EINVAL;

#ifdef vax4_2	/* ----------- rights not supported ----------- */
		u.u_error =
		    sockargs(&rights, mp->msg_accrights, mp->msg_accrightslen);
#endif vax4_2	/* ----------- end rights ifdef --------------- */

		if (u.u_error)
			goto bad;
	} else
		rights = (struct mbuf *)NULL;

	len = auio.uio_resid;

	u.u_error =
	    sosend((struct socket *)fp->f_data, to, &auio, flags, rights);

	u.u_r.r_val1 = len - auio.uio_resid;

	if (rights)
		m_freem(rights);
bad:
	if (to) {
		m_freem(to);
	}
	return;
}

/*
 * recvfrom receives datagrams into a user specified buffer and also
 * receives the address of the sender in a separate user specified
 * buffer.  This allows the receiver to determine the source of the data
 * gram so it can respond.
 */

recvfrom()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
		caddr_t	from;
		int	*fromlenaddr;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;
	int len;

	u.u_error = copyin((caddr_t)uap->fromlenaddr, (caddr_t)&len,
	   sizeof (len));
	if (u.u_error)
		return;
	msg.msg_name = uap->from;
	msg.msg_namelen = len;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	recvit(uap->s, &msg, uap->flags, (caddr_t)uap->fromlenaddr, (caddr_t)0);
	return;
}

/*
 * recv receives data into a user specified buffer, from a byte stream
 */

recv()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	recvit(uap->s, &msg, uap->flags, (caddr_t)0, (caddr_t)0);
	return;
}

/*
 * recvmsg receives data in message form into user specified space
 */

recvmsg()
{
	register struct a {
		int	s;
		struct	msghdr *msg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[MSG_MAXIOVLEN];

	u.u_error = copyin((caddr_t)uap->msg, (caddr_t)&msg, sizeof (msg));
	if (u.u_error)
		return;
	if ((u_int)msg.msg_iovlen >= sizeof (aiov) / sizeof (aiov[0])) {
		u.u_error = EMSGSIZE;
		return;
	}
	u.u_error =
	    copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof (aiov[0])));
	if (u.u_error)
		return;
	msg.msg_iov = aiov;
	if (msg.msg_accrights)
		if (useracc((caddr_t)msg.msg_accrights,
		    (unsigned)msg.msg_accrightslen, B_WRITE) == 0) {
			u.u_error = EFAULT;
			return;
		}
	recvit(uap->s, &msg, uap->flags,
	    (caddr_t)&uap->msg->msg_namelen,
	    (caddr_t)&uap->msg->msg_accrightslen);
	return;
}

/*
 * recvit is generic receive routine for recv, recvfrom, recvmsg to
 * call after they figure out what goes where.
 */

recvit(s, mp, flags, namelenp, rightslenp)
	int s;
	register struct msghdr *mp;
	int flags;
	caddr_t namelenp, rightslenp;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *from, *rights;
	int len;
	
	fp = getsock(s);
	if (fp == (struct file *)NULL)
		return;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = 0;
	auio.uio_offset = 0;
	auio.uio_resid = 0;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		if (iov->iov_len == 0)
			continue;
		if (useracc(iov->iov_base, (u_int)iov->iov_len, B_WRITE) == 0) {
			u.u_error = EFAULT;
			return;
		}
		auio.uio_resid += iov->iov_len;
		iov++;
	}
	len = auio.uio_resid;
	u.u_error =
	    soreceive((struct socket *)fp->f_data, &from, &auio,
		flags, &rights);
	u.u_r.r_val1 = len - auio.uio_resid;
	if (mp->msg_name) {
		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else {
			if (len > from->m_len)
				len = from->m_len;
			(void) copyout((caddr_t)mtod(from, caddr_t),
			    (caddr_t)mp->msg_name, (unsigned)len);
		}
		(void) copyout((caddr_t)&len, namelenp, sizeof (int));
	}

	/*
	 * note: passing rights is not supported, but go ahead and allow user
	 * to specify a *receive* buffer for access rights for possible
	 * compatibility issues.  The buffer should never be used.
	 */

	if (mp->msg_accrights) {
		len = mp->msg_accrightslen;
		if (len <= 0 || rights == 0)
			len = 0;
		else {
			if (len > rights->m_len)
				len = rights->m_len;
			(void) copyout((caddr_t)mtod(rights, caddr_t),
			    (caddr_t)mp->msg_accrights, (unsigned)len);
		}
		(void) copyout((caddr_t)&len, rightslenp, sizeof (int));
	}

	/*
	 * note: this is returning a rights mbuf *returned* by soreceive
	 */

	if (rights)
		m_freem(rights);
	if (from)
		m_freem(from);
	return;
}

/*
 * shutdown part or all of a full duplex connection
 */

shutdown()
{
	struct a {
		int	s;
		int	how;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;

	fp = getsock(uap->s);
	if (fp == (struct file *)NULL)
		return;
	u.u_error = soshutdown((struct socket *)fp->f_data, uap->how);
	return;
}

/*
 * setsockopt sets various socket options
 */

setsockopt()
{
	struct a {
		int	s;
		int	level;
		int	name;
		caddr_t	val;
		int	valsize;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	struct mbuf *m = (struct mbuf *)NULL;

	fp = getsock(uap->s);
	if (fp == (struct file *)NULL)
		return;
	/* Bugfix: bounds check args */
	if (uap->valsize < 0 || uap->valsize > MLEN) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->val) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if (m == (struct mbuf *)NULL) {
			u.u_error = ENOBUFS;
			return;
		}
		u.u_error =
		    copyin(uap->val, mtod(m, caddr_t), (u_int)uap->valsize);
		if (u.u_error)
			goto bad;
		m->m_len = uap->valsize;
	}
	u.u_error =
	    sosetopt((struct socket *)fp->f_data, uap->level, uap->name, m);
bad:
	if (m != (struct mbuf *)NULL)
		(void) m_free(m);
	return;
}

/*
 * getsockopt() places socket options into user specified buffer
 */

getsockopt()
{
	struct a {
		int	s;
		int	level;
		int	name;
		caddr_t	val;
		int	*avalsize;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	struct mbuf *m = (struct mbuf *)NULL;
	int valsize;

	fp = getsock(uap->s);
	if (fp == (struct file *)NULL)
		return;
	if (uap->val) {
		u.u_error = copyin((caddr_t)uap->avalsize, (caddr_t)&valsize,
			sizeof (valsize));
		if (u.u_error)
			return;
		/* Bugfix: add bounds check */
		if (valsize < 0 || valsize > MLEN) {
			u.u_error = EINVAL;
			return;
		}
		m = m_get(M_WAIT, MT_SOOPTS);
		if (m == (struct mbuf *)NULL) {
			u.u_error = ENOBUFS;
			return;
		}
		m->m_len = valsize;	/* need for 4.2 <-> 4.3 compat */
	}
	u.u_error =
	    sogetopt((struct socket *)fp->f_data, uap->level, uap->name, m);
	if (u.u_error)
		goto bad;
	if (uap->val) {
		if (valsize > m->m_len)
			valsize = m->m_len;
		u.u_error = copyout(mtod(m, caddr_t), uap->val, (u_int)valsize);
		if (u.u_error)
			goto bad;
		u.u_error = copyout((caddr_t)&valsize, (caddr_t)uap->avalsize,
		    sizeof (valsize));
	}
bad:
	if (m != (struct mbuf *)NULL)
		(void) m_free(m);
	return;
}

/*
 * pipe() - create a pipe from connected sockets
 */

pipe()
{
	register struct file *rf, *wf;
	struct socket *rso, *wso;
	int r;

	/*
	 * socreate creates a socket and initializes it - it is
	 * not yet "active" since nothing is bound to it.
	 *
	 * pipe() requires *two* sockets, one on which to send and
	 * one on which to receive.
	 */

	u.u_error = socreate(AF_UNIX, &rso, SOCK_STREAM, 0);
	if (u.u_error)
		return;
	u.u_error = socreate(AF_UNIX, &wso, SOCK_STREAM, 0);
	if (u.u_error)
		goto free;

	/*
	 * wise to create sockets before falloc()
	 * to avoid dangling references
	 */

	rf = falloc();
	if (rf == (struct file *)NULL)
		goto free2;
	r = u.u_r.r_val1;
	rf->f_flag = FREAD;
	rf->f_type = DTYPE_SOCKET;
	rf->f_ops = &socketops;
	rf->f_data = (caddr_t)rso;
	wf = falloc();
	if (wf == (struct file *)NULL)
		goto free3;
	wf->f_flag = FWRITE;
	wf->f_type = DTYPE_SOCKET;
	wf->f_ops = &socketops;
	wf->f_data = (caddr_t)wso;
	u.u_r.r_val2 = u.u_r.r_val1;
	u.u_r.r_val1 = r;

	/*
	 * note that piconnect modifies socket and manipulates so_locks
	 * locks are unlocked upon return
	 */

	(void) piconnect(wso, rso);

	/*
	 * Pipe is completely set up, so ok to install file-descriptors.
	 * Must wait until pipe is connected: if shared ofile table another
	 * process can do IO operations the instant after it's installed.
	 */
	ofile_install(u.u_ofile_tab, u.u_r.r_val1, rf);
	ofile_install(u.u_ofile_tab, u.u_r.r_val2, wf);
	return;

free3:
	ofile_install(u.u_ofile_tab, r, (struct file *) NULL);
	ffree(rf);
free2:
	(void) soclose(wso);
free:
	(void) soclose(rso);
	return;
}

/*
 * Get socket name.
 */

getsockname()
{
	register struct a {
		int	fdes;
		caddr_t	asa;
		int	*alen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *m;
	int len;

	spl_t splevel;

	fp = getsock(uap->fdes);
	if (fp == (struct file *)NULL)
		return;
	u.u_error = copyin((caddr_t)uap->alen, (caddr_t)&len, sizeof (len));
	if (u.u_error)
		return;
	/* Bugfix: add bounds check */
	if (len < 0 || len > MLEN) {
		u.u_error = EINVAL;
		return;
	}
	so = (struct socket *)fp->f_data;
	m = m_getclr(M_WAIT, MT_SONAME);
	if (m == (struct mbuf *)NULL) {
		u.u_error = ENOBUFS;
		return;
	}
	splevel = SOLOCK(so);
	u.u_error = (*so->so_proto->pr_usrreq)(so, PRU_SOCKADDR, 0, m, 0);
	SOUNLOCK(so, splevel);

	if (u.u_error)
		goto bad;
	if (len > m->m_len)
		len = m->m_len;

	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)uap->asa, (u_int)len);

	if (u.u_error)
		goto bad;

	u.u_error = copyout((caddr_t)&len, (caddr_t)uap->alen, sizeof (len));
bad:
	m_freem(m);
	return;
}

/*
 * Get name of peer for connected socket.
 */

getpeername()
{
	register struct a {
		int	fdes;
		caddr_t	asa;
		int	*alen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *m;
	int len;

	spl_t splevel;

	fp = getsock(uap->fdes);
	if (fp == (struct file *)NULL)
		return;

	u.u_error = copyin((caddr_t)uap->alen, (caddr_t)&len, sizeof (len));
	if (u.u_error) {
		return;
	}
	/* Bugfix: add bounds check */
	if (len < 0 || len > MLEN) {
		u.u_error = EINVAL;
		return;
	}

 	m = m_getclr(M_WAIT, MT_SONAME);
	if (m == (struct mbuf *)NULL) {
		u.u_error = ENOBUFS;
		return;
	}

	so = (struct socket *)fp->f_data;

	splevel = SOLOCK(so);
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		u.u_error = ENOTCONN;
		SOUNLOCK(so, splevel);
		(void) m_freem(m);
		return;
	}
	u.u_error = (*so->so_proto->pr_usrreq)(so, PRU_PEERADDR, 0, m, 0);
	SOUNLOCK(so, splevel);

	if (u.u_error)
		goto bad;
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)uap->asa, (u_int)len);
	if (u.u_error)
		goto bad;
	u.u_error = copyout((caddr_t)&len, (caddr_t)uap->alen, sizeof (len));
bad:
	m_freem(m);
	return;
}

sockargs(aname, name, namelen)
	struct mbuf **aname;
	caddr_t name;
	int namelen;
{
	register struct mbuf *m;
	int error;

	if (namelen < 0 || namelen > MLEN)
		return (EINVAL);
	m = m_get(M_WAIT, MT_SONAME);
	if (m == (struct mbuf *)NULL)
		return (ENOBUFS);
	m->m_len = namelen;
	error = copyin(name, mtod(m, caddr_t), (u_int)namelen);
	if (error)
		(void) m_free(m);
	else
		*aname = m;
	return (error);
}

struct file *
getsock(fdes)
	int fdes;
{
	register struct file *fp;

	fp = getf(fdes);
	if (fp == (struct file *)NULL)
		return ((struct file *)NULL);
	if (fp->f_type != DTYPE_SOCKET) {
		u.u_error = ENOTSOCK;
		return ((struct file *)NULL);
	}
	return (fp);
}

/*
 * The following are a couple of debugging aids to trace kernel calls
 * to [pv]_lock.  Not usable by the general public.
 */

#define	IPCTRACE
#undef	IPCTRACE
#ifdef	IPCTRACE

	struct pppp { 
		char pvc; 
		char proco;
		int ppc; 
		int pla;
		int bs;
	};

extern struct pppp cbuf[];


struct pppp *cbufe = &cbuf[9990], *cbufp = cbuf, cbuf[10000];

#include "../machine/vmparam.h"
#include "../h/vmmeter.h"
#include "../h/vmsystm.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"

#define cbuffit(pv, pc, la) { \
	register struct pppp * P; \
	P = cbufp++; \
	if (cbufp > cbufe) \
		cbufp = cbuf; \
	P->ppc = pc; \
	P->proco = l.me; \
	P->pvc = pv; \
	P->pla = la; \
}

spl_t 
trace_p_lock(lock, spl)
{
	cbuffit('P', (&lock)[-1], lock);
	return (p_lock(lock, spl));
}

trace_v_lock(lock, spl)

{
	cbuffit('V', (&lock)[-1], lock);
	v_lock(lock, spl);
}

trace_p_sema_v_lock(sema, prior, lock, spl)

{
	cbuffit('Q', (&lock)[-1], lock);
	p_sema_v_lock(sema, prior, lock, spl);
}
#endif IPCTRACE

/*
 * The following is an example of a macro-replacement that is made
 * in debug kernels to trace lock activity.  Typically placed at
 * beginning of a module.
 */

#ifdef IPCTRACE

#undef p_lock
#undef v_lock
#undef p_sema_v_lock

#define p_lock(lock, spl) trace_p_lock(lock, spl)
#define v_lock(lock, spl) trace_v_lock(lock, spl)
#define p_sema_v_lock(s, p, l, spl) trace_p_sema_v_lock(s, p, l, spl)

#endif IPCTRACE
