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
static	char	rcsid[] = "$Header: tty_tty.c 2.4 90/06/06 $";
#endif

/*
 * tty_tty.c
 * 	Indirect driver for controlling tty.
 */

/* $Log:	tty_tty.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/vnode.h"
#include "../h/file.h"

/*ARGSUSED*/
syopen(dev, flag)
	dev_t dev;
	int flag;
{

	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_open)(u.u_ttyd, flag));
}

/*ARGSUSED*/
syclose(dev, flag)
	dev_t dev;
	int flag;
{

	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_close)(u.u_ttyd, flag));
}

/*ARGSUSED*/
syread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_read)(u.u_ttyd, uio));
}

/*ARGSUSED*/
sywrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_write)(u.u_ttyd, uio));
}

/*ARGSUSED*/
syioctl(dev, cmd, addr, flag)
	dev_t	dev;
	int	cmd;
	caddr_t	addr;
	int	flag;
{
	register int i;
	register struct	file *fp;
	register struct	vnode *vp;
	label_t	lqsave;

	/*
	 * To disassociate the process from the control tty, close
	 * all file descriptors that the process has to /dev/tty and
	 * clear ttyp, ttyd and clear process group. This will assure
	 * that terminal device driver receives closes for each 
	 * corresponding open.
	 */

	if (cmd == TIOCNOTTY) {
		/*
		 * Catch signals so that if the process blocks on *last*
		 * close and is signalled, this disassociation still occurs.
		 */
		lqsave = u.u_qsave;
		if (setjmp(&u.u_qsave)) {
			u.u_qsave = lqsave;
			u.u_ttyp = 0;
			u.u_ttyd = 0;
			u.u_procp->p_pgrp = 0;
			longjmp(&u.u_qsave);
		}
		/*
		 * Close all open occurrences of "/dev/tty".
		 * If shared ofile table, this is somewhat heuristic but
		 * that's ok (if application is racing, may leave some
		 * unclosed; low probability).
		 *
		 * Inefficient implementation shouldn't matter; this is a low-runner.
		 */
		for (i = 0; i <= OFILE_LASTFILE(u.u_ofile_tab); i++) {
			fp = getf(i);
			if (fp && fp->f_type == DTYPE_VNODE) {
				vp = (struct vnode *) fp->f_data;
				if (vp->v_type == VCHR && vp->v_rdev == dev)
					closef(ofile_close(u.u_ofile_tab, i));
			}
			if (u.u_fpref) {
				closef(u.u_fpref);
				u.u_fpref = NULL;
			}
		}
		u.u_error = 0;

		u.u_qsave = lqsave;
		u.u_ttyp = 0;
		u.u_ttyd = 0;
		u.u_procp->p_pgrp = 0;
		return (0);
	}

	if (u.u_ttyp == NULL)
		return (ENXIO);

	/*
	 * Disallow - security hole.
	 */

	if (cmd == TIOCSTI)
		return(ENOTTY);

	return ((*cdevsw[major(u.u_ttyd)].d_ioctl)(u.u_ttyd, cmd, addr, flag));
}

/*ARGSUSED*/
syselect(dev, flag)
	dev_t dev;
	int flag;
{

	if (u.u_ttyp == NULL) {
		u.u_error = ENXIO;
		return (0);
	}
	return ((*cdevsw[major(u.u_ttyd)].d_select)(u.u_ttyd, flag));
}
