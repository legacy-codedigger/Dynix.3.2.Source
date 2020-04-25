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
static char rcsid[] = "$Header: ss.c 1.4 90/07/10 $";
#endif lint

/*
 * ss.c 
 *	SSM internal device driver.
 */

/* $Log:	ss.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/vm.h"

#include "../balance/cfg.h"
#include "../balance/engine.h"
#include "../balance/slic.h"

#include "../ssm/ioconf.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

#include "../ssm/ssm.h"	
#include "../ssm/ssm_misc.h"
#include "../ssm/ss.h"			/* driver local structures */

/*
 * Global data structures.
 */
static struct ss SS;
static int ss_max_idx = -1;
static int ssboot();
static int ss_cons;

/*
 * Driver structure for configuring SSM internal 
 * driver.  There is no need for a probe function, 
 * since the configuration code will determine if
 * an SSM board is present and label device as
 * found if we don't specify one.  There is not a
 * interrupt handler since messages are polled
 * for completion.
 */
struct ssm_driver ss_driver = {
/*	name  flags      probe     boot    intr */
	"ss", SDR_OTHER, NULL, ssboot, NULL
};

#ifdef ns32000
static gate_t g_ssmem = 59;		/* Could be defined in conf/conf_ss.c */
#endif ns32000

/*
 * ssboot()
 *	Initialize each ss-device's info structure.
 *
 * 	Allocates and initializes all device driver data
 * 	structures based off of the configuration information.
 * 	Also notes which of its boards are driving the 
 *	console/front panel for special operations.
 */
static
ssboot(ndevs, dev_array)
	int ndevs;
	register struct ssm_dev dev_array[];
{
	register struct	ss_info *infop;
	register struct ssm_dev *sdv;
	int dev;

#ifdef DEBUG
	printf("ssboot: Number of Boards: %d SS @ 0x%x\n", ndevs, &SS);
#endif
	/*
	 * Initialize the SS structure.
	 */
	init_sema(&SS.ss_sync, 1, 0, g_ssmem);

	/* 
 	 * Initialize each configured ss-device.
	 */
	for (sdv = dev_array, dev = 0; dev < ndevs; dev++, sdv++) {
		if (!sdv->sdv_alive)
			continue;

		SS.ss_info[dev] = infop = (struct ss_info *) 
			calloc(sizeof(struct ss_info));
		infop->ss_desc = sdv;
		if (sdv->sdv_desc->ssm_is_cons) {
			SS.ss_cons = infop;
			ss_cons = dev;
		}
		ss_max_idx = dev;
	}
}

/*
 * ssopen()	
 *	Open the device.  It must specify
 *	either the console/front panel unit
 *	or a unit that was found.
 */
ssopen(dev)
	dev_t	dev;
{
	register int unit = minor(dev);
	
	if (unit == CTLR_MINOR && SS.ss_cons 
	||  unit <= ss_max_idx && SS.ss_info[unit]) 
		return(0);
	else 
		return(ENXIO);
}

/*
 * sswrite()
 *	Write the SSM's RAM.
 */
sswrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	return(ss_startio(dev, UIO_WRITE, uio));
}

/*
 * ssread()
 *	Read the SSM's RAM.
 */
ssread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	return(ss_startio(dev, UIO_READ, uio));
}

/*
 * ss_startio()
 *	Start an I/O request to the SSM's
 * 	RAM.  Synchronize with other accesses
 *	since we use driver's buffer.
 */
static
ss_startio(dev, rw, uio)
	dev_t	dev;
	enum uio_rw rw;
	register struct uio *uio;
{
	register struct ss_info *infop;
	int unit = minor(dev);
	int count;
	char *ram_loc;

	if (unit == CTLR_MINOR) {
		/* Get info ptr and real unit # of console device */
		infop = SS.ss_cons;
		unit = ss_cons;
	} else 
		infop = SS.ss_info[unit];

	p_sema(&SS.ss_sync, PZERO-1);

	while (uio->uio_resid) {
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;	/* Invalid transfer count */
			break;
		}

		/*
		 * Determine where the data is on the SSM
		 * and how big a chunk of it to copy; must
		 * fragment it if our buffer is too small.
		 */
		ram_loc = (char *)uio->uio_offset;
		count = (uio->uio_resid < SSBUFSZ) ? uio->uio_resid : SSBUFSZ;

		/*
		 * Perform the transfer...
		 */
		if (rw == UIO_WRITE) {
			if (u.u_error = uiomove((caddr_t)&SS.ss_buf[0], 
						count, UIO_WRITE, uio)) 
				break;	/* A problem occurred */
			ssm_set_ram(unit, count, (char *)&SS.ss_buf[0], 
				ram_loc);
		} else {
			ssm_get_ram(unit, count, (char *)&SS.ss_buf[0], 
				ram_loc);
			if (u.u_error = uiomove((caddr_t)&SS.ss_buf[0], 
						count, UIO_READ, uio)) 
				break;	/* A problem occurred */
		}

		/* 
		 * Collect stats and go back for more...
		 */
		infop->ss_stats.ss_stats_xfers++;
	}

	v_sema(&SS.ss_sync);
	return(u.u_error);
}

/*
 * ssioctl()
 *	Special I/O requests of the driver.
 */
ssioctl(dev, cmd, addr)
	int	cmd;
	dev_t	dev;
	caddr_t	addr;
{
	register int unit = minor(dev);
	register struct ss_info *infop;
	struct ioctl_reboot *arb;
	int error = 0;
	u_short flags;
	struct ssmlog *mlog;

#ifdef DEBUG
		printf("ssioctl: dev=%x, cmd=%x, address=%x\n", dev, cmd, addr);
#endif
	if (unit == CTLR_MINOR) {
		/* Get info ptr and real unit # of console device */
		infop = SS.ss_cons;
		unit = ss_cons;
	} else 
		infop = SS.ss_info[unit];

	p_sema(&SS.ss_sync, PZERO-1);

	switch (cmd) {
	case SMIOSTATS:
		bcopy((caddr_t)&infop->ss_stats, addr, sizeof(struct sm_stats));
		infop->ss_stats.ss_stats_ioctls++;
		infop->ss_stats.ss_stats_cmd++;
		break;
	case SMIOGETREBOOT0:
	case SMIOGETREBOOT1:
		arb = (struct ioctl_reboot *)addr;
		arb->re_cfg_addr = (u_char *)NULL;
		if (infop != SS.ss_cons) {
			/*
			 * This operation applies only to the 
			 * console/front panel unit.
			 */
			error = EBADF;
			break;
		}
                ssm_get_boot((cmd == SMIOGETREBOOT0) ? SM_DYNIX : SM_DUMPER,
			     (arb->re_powerup == RE_PERM_FLAG) ? 
			     SM_GET_DFT : SM_GET, (u_short *)&flags, BNAMESIZE,
			     (char *)&arb->re_boot_name[0]);
		arb->re_boot_flag = (u_int)flags;
		infop->ss_stats.ss_stats_ioctls++;
		infop->ss_stats.ss_stats_cmd++;
		break;
	case SMIOSETREBOOT0:
	case SMIOSETREBOOT1:
		arb = (struct ioctl_reboot *)addr;
		if (infop != SS.ss_cons) {
			/*
			 * This operation applies only to the 
			 * console/front panel unit.
			 */
			error = EBADF;
			break;
		}
		ssm_set_boot((cmd == SMIOSETREBOOT0) ? SM_DYNIX : SM_DUMPER,
			    (arb->re_powerup == RE_PERM_FLAG) ?
			    SM_SET_DFT : SM_SET, (u_short)arb->re_boot_flag,
			    BNAMESIZE, (char *)&arb->re_boot_name[0]);
		infop->ss_stats.ss_stats_ioctls++;
		infop->ss_stats.ss_stats_cmd++;
		break;
	case SMIOGETLOG:
		if (infop != SS.ss_cons) {
			/*
		 	 * This operation applies only to the 
		 	 * console/front panel unit.
		 	 */
			error = EBADF;
			break;
		}
		mlog = (struct ssmlog *)addr;
		ssm_get_log_info(unit, &mlog->ml_buffer, 
			&mlog->ml_nextchar, &mlog->ml_size);
		break;
	default:
		/* Unknown/unsupported command. */
		error = ENXIO;
		break;
	}

	v_sema(&SS.ss_sync);
	return(error);
}
