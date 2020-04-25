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

#ifndef lint
static char rcsid[] = "$Header: cons.c 1.3 90/11/06 $";
#endif lint

/*
 * cons.c 
 *	Logical console driver for system controllers.
 *	Provides a dynamic indirect interface to the 
 *	console/tty driver of either the SSM or SCED 
 *	connected to the front panel.  
 *
 *	This implementation was chosen for its
 *	flexibility with regard to future enhancements.
 *	It was felt that the overhead would be small
 *	relative to the rest of the call sequence.
 *	
 */

/* $Log:	cons.c,v $
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../balance/cfg.h"

int cons_major = -1;		/* Major device number of the current 
				 * system controller console/tty driver. */
/*
 * consboot()
 *	Confirm that there is a system console driver 
 *	present in the system.
 */
consboot()
{
#ifdef DEBUG
	printf("consboot: memory driver major device is %d.\n", cons_major);
#endif DEBUG
	if (cons_major < 0)
		panic("Can't locate the console driver");
		/*
		 *+ The major device number for /dev/console was not
		 *+ initialized by autoconfiguration code.
		 */
}

/*
 * consopen()	
 *	Verify that the minor device number is
 *	the special control device value and then
 *	call the device dependent open routine.
 */
/*ARGSUSED*/
consopen(dev, flag)
	dev_t	dev;
	int flag;
{
	register int unit = minor(dev);
	if (unit == CTLR_MINOR)
		return((*cdevsw[cons_major].d_open)
			(makedev(cons_major, minor(dev))));
	else
		return(ENXIO);
}

/*
 * consclose()	
 *	Invoke the device dependent close routine.
 */
/*ARGSUSED*/
consclose(dev, flag)
	dev_t	dev;
	int flag;
{
	return((*cdevsw[cons_major].d_close)(makedev(cons_major, minor(dev))));
}

/*
 * conswrite()
 *	Invoke the device dependent console output function.
 */
conswrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	int xx;
	xx = ((*cdevsw[cons_major].d_write)
		(makedev(cons_major, minor(dev)), uio));
	return(xx);
}

/*
 * consread()
 *	Invoke the device dependent console input function.
 */
consread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	return((*cdevsw[cons_major].d_read)
		(makedev(cons_major, minor(dev)), uio));
}

/*
 * consioctl()
 *	Invoke the device dependent console ioctl function.
 */
consioctl(dev, cmd, addr, flag)
	int	cmd;
	dev_t	dev;
	caddr_t	addr;
	int flag;
{
	return((*cdevsw[cons_major].d_ioctl)
		(makedev(cons_major, minor(dev)), cmd, addr, flag));
}

/*
 * consselect()
 *	Invoke the device dependent console select function.
 */
consselect(dev, rw)
	dev_t	dev;
	int rw;
{
	return((*cdevsw[cons_major].d_select)
		(makedev(cons_major, minor(dev)), rw));
}

/*
 * consstop()
 *	Abort transfer with the device dependent console stop function.
 */
consstop(tp, flag)
	register struct tty *tp;
	int flag;
{
	return((*cdevsw[cons_major].d_stop)(tp, flag));
}
