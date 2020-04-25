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
static char rcsid[] = "$Header: sm.c 2.12 90/12/13 $";
#endif lint

/*
 * sm.c 
 *	scsi memory device driver
 */

/* $Log:	sm.c,v $
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
#include "../h/cmn_err.h"

#include "../balance/cfg.h"
#include "../balance/engine.h"
#include "../balance/slic.h"

#include "../machine/ioconf.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

#include "../sec/sec.h"			/* scsi common data structures */
#include "../sec/sm.h"			/* driver local structures */

#ifdef DEBUG
int smdebug = 0;
#endif
/*
 * Global data structures.
 */
struct sm *sm;
struct sec_dev sec_sm[4];

int smprobe(),  smboot(),  smintr();

struct sec_driver sm_driver = {
/*	name	base	flags		probe		boot	intr */
	"sm",	0x8,	SED_TYPICAL,	smprobe,	smboot,	smintr
};

#ifdef	ns32000			/* should be in conf/conf_sm.c! */
gate_t	g_smem = 59;		/* gate for this device driver */
#endif	ns32000

/*
 * smprobe() - probe procedure 
 *	struct sec_dev *sec_cfg;
 *
 * note: THIS device is always present.
 */
/*ARGSUSED*/
smprobe(sec_cfg)
	struct sec_dev *sec_cfg;
{
#ifdef DEBUG
	if (smdebug)
		printf("SMPROBE: Device %x Found\n",sec_cfg->sd_target);
#endif
	return(SM_FOUND);
}

/*
 * smboot - initialize all channels of this device driver.
 *
 * Called once after all probing has been done.
 *
 * This procedure initializes and allocates all device driver data
 * structures based off of the configuration information passed in from
 * autoconfig() and from the device drivers binary configuration tables.
 * The boot procedure also maps interrupt levels to unit number by
 * placing the channels communications structure pointer into a
 * major/minor number mapped dynamically allocated array (sm_info[]).
 */
smboot(num_dev,sec_array)
	int num_dev;
	register struct sec_dev sec_array[];
{
	register struct	sm_info *ifd;
	register struct sec_dev *sed;
	int size, dev, base;

#ifdef DEBUG
	if (smdebug)
		printf("SMBOOT: Number of Boards: %d\n",num_dev);
#endif
	sm = (struct sm *)calloc(sizeof(struct sm));

#ifdef DEBUG
	if (smdebug > 1)
		printf("&sm: %x\n",sm);
#endif

	/*
	 * initialize the sm structure
	 */
	sm->sm_stat = 0;
	sm->sm_busy = SM_FALSE;

	/*
	 * Init semas ...
	 */
	init_sema(&sm->sm_sema, 1, 0, g_smem);
	init_sema(&sm->sm_done, 0, 0, g_smem);

	/*
	 * for each configured device
	 */
	for (dev = 0; dev < num_dev; dev++) {

		sed = &sec_array[dev];

		if (!sed->sd_alive) {
#ifdef DEBUG
			if (smdebug)
				printf("Board: %d, Not Configured\n",dev);
#endif
			continue;
		}

#ifdef DEBUG
		if (smdebug)
			printf("Board: %d, SEC: %x Configured\n",dev,sed);
#endif
			
		size = sizeof(struct sm_info);
		base = (int)calloc(size);

#ifdef DEBUG
		if (smdebug >1)
			printf("&sm_info: %x\n",base);
#endif

		sm->sm_info[dev] = (struct sm_info *)base;
		ifd = (struct sm_info *)base;

		ifd->sm_desc = sed;
		ifd->sm_stats.sm_stats_cmd = 0;
		ifd->sm_stats.sm_stats_ioctls = 0;
		ifd->sm_stats.sm_stats_xfers = 0;

		if (sed->sd_desc->sec_is_cons)
			sm->sm_cons = ifd;
	}
}

/*
 * smopen	Open the device
 */
smopen(dev)
	dev_t	dev;
{
	register unit;

#ifdef DEBUG
	if (smdebug > 2)
		printf("SMOPEN: Device %d\n",dev);
	else if (smdebug)
		printf("O");
#endif

	unit = minor(dev);
	if (unit & SM_CONS && sm->sm_cons)
		return(0);

#ifdef DEBUG
	if (smdebug > 2)
		printf("Unit: %d\n",unit);
#endif

	if ((1 << unit) & SECvec)
		return(0);

#ifdef DEBUG
	if (smdebug)
		printf("SMOPEN: Device NOT FOUND: SECvec: %x\n",SECvec);
#endif

	return(ENXIO);
}

/*
 * smwrite	Write procedure.
 */
smwrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{

#ifdef DEBUG
	if (smdebug >2)
		printf("SMWRITE: Device %d, UIO: %x\n",dev,uio);
	else if (smdebug)
		printf("W");
#endif

	return(smstartio(dev, UIO_WRITE, uio));
}


/*
 * smread	Read procedure.
 */
smread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{

#ifdef DEBUG
	if (smdebug >2)
		printf("SMREAD: Device %d, UIO: %x\n",dev,uio);
	else if (smdebug)
		printf("R");
#endif

	return(smstartio(dev, UIO_READ, uio));
}

/*
 * smstartio - start a request to a channel.
 *
 * tmp concerns - a semaphore (sm_sema) is used to lock out other requests
 *		- another semaphore (sm_done) is used to get completion
 *
 */
smstartio(dev, rw, uio)
	dev_t	dev;
	enum uio_rw rw;
	struct uio *uio;
{
	register struct sm_info *ifd;
	register struct sec_progq *diq, *doq;
	register struct sec_dev_prog *dp;
	register int offset;
	register unit;

	unit = minor(dev);
	if (unit & SM_CONS)
		ifd = sm->sm_cons;
	else
		ifd = sm->sm_info[unit];

	/*
	 * get the semaphore to lock out other requests
	 */
	p_sema(&sm->sm_sema,PZERO-1);

#ifdef DEBUG
	if (smdebug > 2)
		printf("Locked\n");
	else if (smdebug)
		printf("L");
#endif

	while (uio->uio_resid && !u.u_error) {
		
		if (uio->uio_resid <= 0) {
			u.u_error = EINVAL;
			break;
		}

		sm->sm_busy = SM_TRUE;
		
		/*
		 * fill out the device program
		 */
		dp = &sm->sm_dp; 
		dp->dp_status1 = 0;
		dp->dp_un.dp_data = &sm->sm_buf[0];
		dp->dp_next = NULL;
		if (uio->uio_resid < sizeof(sm->sm_buf))
			dp->dp_data_len = uio->uio_resid;
		else
			dp->dp_data_len = sizeof(sm->sm_buf);
		
		/*
		 * if this is a write, copy the data into a local
		 * buffer.
		 * save the current offset since uiomove changes it.
		 */
		offset = uio->uio_offset;
		if (rw == UIO_WRITE) {
			u.u_error = uiomove((caddr_t)&sm->sm_buf[0],sizeof(sm->sm_buf),UIO_WRITE,uio);
#ifdef DEBUG
		if (smdebug > 2)
			printf("%x %x %x %x %x\n",sm->sm_buf[0],sm->sm_buf[1],
				sm->sm_buf[2],sm->sm_buf[3],sm->sm_buf[4]);
#endif
		}

		dp->dp_cmd_len = 10;

		dp->dp_cmd[0] = ((rw == UIO_READ) ? SMC_READ : SMC_WRITE);
		dp->dp_cmd[1] = 0;
		dp->dp_cmd[2] = 0;
		dp->dp_cmd[3] = dp->dp_data_len >> 8;
		dp->dp_cmd[4] = dp->dp_data_len & 0xff;
		dp->dp_cmd[5] = 0;
		dp->dp_cmd[6] = offset >> 16;
		dp->dp_cmd[7] = offset >> 8;
		dp->dp_cmd[8] = offset & 0xff;
		dp->dp_cmd[9] = 0;

#ifdef DEBUG
		if (smdebug > 2) {
			printf("Device Program: %x\n",dp);
			printf("Data Length: %d, SCSI Addr: %x\n",
				dp->dp_data_len, uio->uio_offset);
			printf("Local address: %x\n",&sm->sm_buf[0]);
		}
#endif

		/*
		 * load the diq
		 */
		diq = ifd->sm_desc->sd_requestq;

		diq->pq_un.pq_progs[diq->pq_head] = dp;
		diq->pq_head = (diq->pq_head +1) % ifd->sm_desc->sd_req_size;
		
		ifd->sm_stats.sm_stats_cmd++;
		/*
		 * signal the board to start the transaction
		 */
		sec_startio(	SINST_STARTIO,
				&sm->sm_stat,
				ifd->sm_desc
			);

		if (sm->sm_stat != SINST_INSDONE)
			CPRINTF("smem %x: Device Not Responding\n",unit);

		p_sema(&sm->sm_done,PZERO-1);

		/*
		 * remove the device program from the output queue
		 * always returns good status
		 */
		doq = ifd->sm_desc->sd_doneq;
		doq->pq_head = (doq->pq_head +1) % ifd->sm_desc->sd_doneq_size;

		/*
		 * if this was a read, copy the data to the users
		 * space
		 */
		if (rw == UIO_READ) {
			u.u_error = uiomove((caddr_t)&sm->sm_buf[0],sizeof(sm->sm_buf),UIO_READ,uio);
#ifdef DEBUG
		if (smdebug > 2)
			printf("%x %x %x %x %x\n",sm->sm_buf[0],sm->sm_buf[1],
				sm->sm_buf[2],sm->sm_buf[3],sm->sm_buf[4]);
#endif
		}
		
		ifd->sm_stats.sm_stats_xfers++;
		sm->sm_busy = SM_FALSE;
	}

	/*
	 * let others in
	 */
	v_sema(&sm->sm_sema);

	return(u.u_error);
}

/*
 * smintr - interrupt routine.
 *
 * When there is an outstanding request that has completed
 * signal smstart
 */

smintr()
{

	if (sm->sm_busy) {
#ifdef DEBUG
		if (smdebug)
			printf("I");
#endif
		v_sema(&sm->sm_done);
	}
}

/*
 * smioctl - asynchronous io control requests of the driver.
 *
 * tmp concerns - uses sm_sema to assure that no one else is doing
 * anything to the device.
 */
smioctl(dev, cmd, addr)
	int	cmd;
	dev_t	dev;
	caddr_t	addr;
{
	register int unit;
	register struct sm_info *ifd;
	register struct sec_gmode *tio;
	register struct sec_smode *md;
	struct ioctl_reboot *arb;
	struct reboot *lrb;
	int error;

#ifdef DEBUG
	if (smdebug > 2)
		printf("IOCTL: dev: %x, cmd: %x, address: %x\n",dev,cmd,addr);
	else if (smdebug)
		printf("i");
#endif

	unit = minor(dev);
	if (unit & SM_CONS)
		ifd = sm->sm_cons;
	else
		ifd = sm->sm_info[unit];
	error = 0;

	p_sema(&sm->sm_sema,PZERO-1);

	switch (cmd) {
	case SMIOSTATS:
		bcopy((caddr_t)&ifd->sm_stats,addr,sizeof(struct sm_stats));
		ifd->sm_stats.sm_stats_ioctls++;
		ifd->sm_stats.sm_stats_cmd++;
		break;

	case SMIOGETREBOOT0:
	case SMIOSETREBOOT0:
	case SMIOGETREBOOT1:
	case SMIOSETREBOOT1:
		tio = (struct sec_gmode *)&sm->sm_buf[0];
		tio->gm_status = 0;
		tio->gm_un.gm_board.sec_reboot = (struct reboot *)&sm->sm_buf[256];
		arb = (struct ioctl_reboot *)addr;
		tio->gm_un.gm_board.sec_reboot->re_powerup = arb->re_powerup;
		ifd->sm_stats.sm_stats_ioctls++;
		ifd->sm_stats.sm_stats_cmd++;
		sm_sec_startio(	SINST_GETMODE,
				(int *)tio,
				ifd->sm_desc
			);
		if (tio->gm_status != SINST_INSDONE) {
			printf("smem: %x, Get Reboot Modes Sync. Failure: %x",tio->gm_status);
			/*
			 *+ The driver controlling SCED memory couldn't get the
			 *+ device to respond.
		   	 */
		}
		/*
		 * if the reboot structure changes this will have to change
		 */
		if ((cmd == SMIOGETREBOOT0)
		   || (cmd == SMIOGETREBOOT1)) {
			lrb = (struct reboot *)&sm->sm_buf[256];
			arb->re_powerup = lrb->re_powerup;
			arb->re_boot_flag = lrb->re_boot_flag;
			if (cmd == SMIOGETREBOOT0) {
				arb->re_cfg_addr = lrb->re_cfg_addr[0];
				bcopy((caddr_t)&lrb->re_boot_name[0][0],
				     (caddr_t)&arb->re_boot_name[0],
				     BNAMESIZE);
			}
			else {
				arb->re_cfg_addr = lrb->re_cfg_addr[1];
				bcopy((caddr_t)&lrb->re_boot_name[1][0],
				     (caddr_t)&arb->re_boot_name[0],
				     BNAMESIZE);
			}
			break;
		}
		else {
			md = (struct sec_smode *) &sm->sm_buf[0];
			md->sm_status = 0;
			md->sm_un.sm_board.sec_powerup = (struct sec_powerup *)0;
			md->sm_un.sm_board.sec_dopoll = 0;
			md->sm_un.sm_board.sec_errlight = 0;
			lrb = (struct reboot *)&sm->sm_buf[256];
			lrb->re_powerup = arb->re_powerup;
			lrb->re_boot_flag = arb->re_boot_flag;
			if (cmd == SMIOSETREBOOT0) {
				lrb->re_cfg_addr[0] = arb->re_cfg_addr;
				bcopy((caddr_t)&arb->re_boot_name[0],
				(caddr_t)&lrb->re_boot_name[0][0],
				BNAMESIZE);
			}
			else {
				lrb->re_cfg_addr[1] = arb->re_cfg_addr;
				bcopy((caddr_t)&arb->re_boot_name[0],
				(caddr_t)&lrb->re_boot_name[1][0],
				BNAMESIZE);
			}
			md->sm_un.sm_board.sec_reboot = lrb;
			ifd->sm_stats.sm_stats_ioctls++;
			ifd->sm_stats.sm_stats_cmd++;
			sm_sec_startio(	SINST_SETMODE,
					(int *)md,
					ifd->sm_desc
				);
			if (md->sm_status != SINST_INSDONE) {
				printf("smem: %x, Set Reboot Modes Sync. Failure: %x",tio->gm_status);
				/*
				 *+ The driver controlling SCED memory 
				 *+ couldn't get the reboot flags from the 
				 *+ device.
				 */
			}
			break;
		}
	case SMIOGETLOG:
		tio = (struct sec_gmode *)&sm->sm_buf[0];
		tio->gm_status = 0;
		ifd->sm_stats.sm_stats_ioctls++;
		ifd->sm_stats.sm_stats_cmd++;
		sec_startio(	SINST_GETMODE,
				(int *)tio,
				ifd->sm_desc
			);
		if (tio->gm_status != SINST_INSDONE) {
			printf("smem: %x, Get Log Modes Sync. Failure: %x",tio->gm_status);
                        /*
                         *+ The SCED memory driver couldn't obtain the system
			 *+ log from the device.
                         */
		}
		bcopy((caddr_t)&tio->gm_un.gm_mem,addr,sizeof(struct sec_mem));
		break;

	case SMIOSETLOG:
		md = (struct sec_smode *) &sm->sm_buf[0];
		md->sm_status = 0;
		bcopy(addr,(caddr_t)&md->sm_un.sm_mem,sizeof(struct sec_mem));
		ifd->sm_stats.sm_stats_ioctls++;
		ifd->sm_stats.sm_stats_cmd++;
		sec_startio(	SINST_SETMODE,
				(int *)md,
				ifd->sm_desc
			);
		if (md->sm_status != SINST_INSDONE) {
			printf("smem: %x, Set Log Modes Sync. Failure: %x",tio->gm_status);
                        /*
                         *+ The SCED memory driver couldn't obtain the system
			 *+ log from the device.
                         */
		}
		break;

	/*
	 * Unknown command.
	 */
	default:
		error = ENXIO;
		break;
	}

	v_sema(&sm->sm_sema);
	return(error);
}

/*
 * sm_sec_startio - start an operation by sending a command to the
 *		SCSIBOARD device via slic.
 *
 * This procedure is used once interrupts have been enabled
 * in the kernel and is a fake version of the sec_startio procedure.
 * It exists because sec_startio won't take a parameter saying which
 * device to send the command to.  The memory driver needs to send
 * commands to both the MEMORY device and the SCSIBOARD device.
 *
 * Calls mIntr() to do the actual slic fussing to send the message.  Use
 * bin 3, since this helps avoid SLIC-bus saturation/lockup (since SCED
 * interrupts Dynix mostly on bins 4-7, using bin 3 to interrupt SCED gives
 * SCED -> Dynix priority over Dynix -> SCED, thus SCED won't deadlock
 * against Dynix).
 *
 * NOTE: It's critical that the status pointer live below 4 Meg (physical)
 * because the SEC can't directly address above that address, hence all status
 * variable must *not* live on kernel stack.
 */

#define	DELAY_TIME	50000

sm_sec_startio(cmd, statptr, sd)
	int	 	cmd;
	register int	*statptr;
	register struct sec_dev *sd;
{
	register int	spin;
	struct sec_cib *cib;

	*statptr = 0;
	cib = sd->sd_cib;
	cib -= SDEV_MEM;
	cib->cib_inst = cmd;
	cib->cib_status = statptr;

	/*
	 * Must insure that the interrupt gets sent, so don't allow
	 * interrupts on this processor while sending the interrupt.
	 */

#ifdef	ns32000
	{ spl_t spl = splhi();
	mIntr(sd->sd_desc->sec_slicaddr, 3, SDEV_SCSIBOARD);
	splx(spl);
	}
#endif	ns32000

#ifdef	i386
	DISABLE();
	mIntr(sd->sd_desc->sec_slicaddr, 3, SDEV_SCSIBOARD);
	ENABLE();
#endif	i386

	spin = calc_delay(10 * DELAY_TIME);
	while((*statptr & SINST_INSDONE) == 0) {
		if (spin-- <= 0) {
			printf("sm_sec_startio: timeout\n");
                        /*
                         *+ The driver initiated an operation on the SCED
                         *+ and didn't receive a completion notification.
                         */
			break;
		}
	}
}
