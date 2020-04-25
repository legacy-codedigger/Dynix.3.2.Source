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
static char rcsid[] = "$Header: smem.c 1.2 90/11/06 $";
#endif lint

/*
 * smem.c 
 *	Logical memory driver for system controllers.
 *	Provides a dynamic indirect interface to 
 *	the memory driver of either the SSM or SCED 
 *	connected to the front panel.
 */

/* $Log:	smem.c,v $
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../balance/cfg.h"

int smem_major = -1;		/* Major device number of the current 
				 * system controller. */
/*
 * smemboot()
 *	Confirm that there is a system controller 
 *	memory driver present in the system.
 */
smemboot()
{
#ifdef DEBUG
	printf("smemboot: memory driver major device is %d.\n", smem_major);
#endif DEBUG
	if (smem_major < 0)
		printf("WARNING - no memory driver configured for this system");
		/*
		 *+ The major device number of the controller memory driver
		 *+ was not initialized by the autoconfiguration code.
		 */
}

/*
 * smemopen()	
 *	Verify that the minor device number is
 *	the special control device value and then
 *	call the device dependent routine.
 */
smemopen(dev)
	dev_t	dev;
{
	register int unit = minor(dev);
	
	if (unit == CTLR_MINOR)
		return((*cdevsw[smem_major].d_open)(dev));
	else
		return(ENXIO);
}

/*
 * smemwrite()
 *	Invoke the device dependent write-to-RAM function.
 */
smemwrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	return((*cdevsw[smem_major].d_write)(dev, uio));
}

/*
 * smemread()
 *	Invoke the device dependent read-from-RAM function.
 */
smemread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	return((*cdevsw[smem_major].d_read)(dev, uio));
}

/*
 * smemioctl()
 *	Invoke the device dependent ioctl function.
 */
smemioctl(dev, cmd, addr)
	int	cmd;
	dev_t	dev;
	caddr_t	addr;
{
	return((*cdevsw[smem_major].d_ioctl)(dev, cmd, addr));
}
