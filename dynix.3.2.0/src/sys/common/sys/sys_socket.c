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

#ifndef	lint
static	char	rcsid[] = "$Header: sys_socket.c 2.5 90/12/13 $";
#endif

/*
 * sys_socket.c
 *	Socket IO.
 */

/* $Log:	sys_socket.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/stat.h"

#include "../net/if.h"
#include "../net/route.h"

int	soo_rw(), soo_ioctl(), soo_select(), soo_close();
struct	fileops socketops =
    { soo_rw, soo_ioctl, soo_select, soo_close };

soo_rw(fp, rw, uio)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uio;
{
	int soreceive(), sosend();

	/* Note: sosend/soreceive do their own mutex */

	return (
	    (*(rw==UIO_READ?soreceive:sosend))
	      ((struct socket *)fp->f_data, 0, uio, 0, 0));
}

soo_ioctl(fp, cmd, data)
	struct file *fp;
	int cmd;
	register caddr_t data;
{
	register struct socket *so = (struct socket *)fp->f_data;
	int error;
	spl_t	splevel;

	splevel = SOLOCK(so);

	switch (cmd) {

	case FIONBIO:
		if (*(int *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		SOUNLOCK(so,splevel);
		return (0);

	case FIOASYNC:
		if (*(int *)data)
			so->so_state |= SS_ASYNC;
		else
			so->so_state &= ~SS_ASYNC;
		SOUNLOCK(so,splevel);
		return (0);

	case FIONREAD:
		*(int *)data = so->so_rcv.sb_cc;
		SOUNLOCK(so,splevel);
		return (0);

	case SIOCSPGRP:
		so->so_pgrp = *(int *)data;
		SOUNLOCK(so,splevel);
		return (0);

	case SIOCGPGRP:
		*(int *)data = so->so_pgrp;
		SOUNLOCK(so,splevel);
		return (0);

	case SIOCATMARK:
		*(int *)data = (so->so_state&SS_RCVATMARK) != 0;
		SOUNLOCK(so,splevel);
		return (0);
	}
	/*
	 * Interface/routing/protocol specific ioctls:
	 * interface and routing ioctls should have a
	 * different entry since a socket's unnecessary
	 */
#define	cmdbyte(x)	(((x) >> 8) & 0xff)
	if (cmdbyte(cmd) == 'i') {
		SOUNLOCK(so,splevel);
		return (ifioctl(so, cmd, data));
	}
	if (cmdbyte(cmd) == 'r'){
		SOUNLOCK(so,splevel);
		return (rtioctl(cmd, data));
	}
	error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL, 
	    (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));
	SOUNLOCK(so,splevel);
	return(error);
}

soo_select(fp, which)
	struct file *fp;
	int which;
{
	register struct socket *so = (struct socket *)fp->f_data;
	spl_t splevel;

	splevel = SOLOCK(so);
	switch (which) {

	case FREAD:
		if (soreadable(so)) {
			SOUNLOCK(so, splevel);
			return (1);
		}
		sbselqueue(&so->so_rcv);
		break;

	case FWRITE:
		if (sowriteable(so)) {
			SOUNLOCK(so, splevel);
			return (1);
		}
		sbselqueue(&so->so_snd);
		break;
	case 0:
		if (so->so_oobmark ||
		    (so->so_state & SS_RCVATMARK)) {
			SOUNLOCK(so, splevel);
			return(1);
		}
		sbselqueue(&so->so_rcv);
		break;
	}

	SOUNLOCK(so, splevel);
	return (0);
}

/*ARGSUSED*/
soo_stat(so, ub)
	register struct socket *so;
	register struct stat *ub;
{
#ifndef lint
	spl_t splevel;
#endif
	int errno = 0;

#ifdef	lint
	so = so;
#endif

	bzero((caddr_t)ub, sizeof (*ub));

#ifdef	notdef
	splevel = SOLOCK(so);

	errno = (*so->so_proto->pr_usrreq)(so, PRU_SENSE,
	    (struct mbuf *)ub, (struct mbuf *)0, (struct mbuf *)0);

	SOUNLOCK(so, splevel);
#endif
	return(errno);

}

soo_close(fp)
	struct file *fp;
{
	caddr_t	data;
	int	error = 0;
	
	data = fp->f_data;
	fp->f_data = 0;		/***N.B. socket no longer exists! **/
	ffree(fp);

	/* Note: soclose does mutexing */

	if (data)
		error = soclose((struct socket *)data);
	return (error);
}
