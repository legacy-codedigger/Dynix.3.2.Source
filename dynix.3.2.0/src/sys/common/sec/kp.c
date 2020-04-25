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
static	char	rcsid[] = "$Header: kp.c 1.5 87/05/26 $";
#endif

/* $Log:	kp.c,v $
 */

/*
 * kp.c - kernel profiler device driver
 *
 * In order to configure in the kp driver and related code:
 *
 * Add the following lines the config file (e.g., DYNIX in conf/):
 *
 *	options	KERNEL_PROFILING
 *
 *	## SCSI kernel profiler driver on SEC ##
 *	device		kp0	at sec? req  3 doneq  3 bin 4 unit 0
 */

#include "../h/param.h"
#include "../h/vm.h"
#include "../h/mutex.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/ioctl.h"

#include "../balance/cfg.h"
#include "../balance/slic.h"

#include "../machine/ioconf.h"
#include "../machine/hwparam.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/exec.h"
#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/mftpr.h"

#include "../sec/sec.h"		/* scsi common data structures */
#include "../sec/kp.h"		/* driver local structures */

/*
 * Global data structures.
 */
struct kp *kp;
struct sec_dev sec_kp[1];	/* At most 1 of these in the system */
u_long *kp_nmis;		/* ptr to # of NMI's recv'd per processor */
struct pc_mode *kp_pc_m;	/* ptr to pc/mode pairs */
int *kp_in_nmi;			/* "Currently handling nmi" flags */
int *kp_ov_nmi;			/* Overlapped NMI count */


static unsigned kp_buf[512];
static int kp_state;		/* local copy of profiler state */
static int kp_c_size;		/* Size of counters data area */
static u_long *kp_counters;	/* ptr to profiling counters */
static int kp_bins;		/* number of bins used in profiling */
static unsigned kp_b_text;	/* ptr to 1st text position: */

extern int kp_binshift;

extern int kpprobe(),  kpboot(), kpintr();

extern unsigned Nengine;	/* # of engines -- used to allocate storage */

struct sec_driver kp_driver = {
/*	name	base	flags		probe		boot	intr */
	"kp",	0xa,	SED_TYPICAL,	kpprobe,	kpboot,	kpintr
};

#ifdef	ns32000
extern gate_t	kp_gate;	/* gate for this device driver */
#endif

/*
 * kpprobe() - probe procedure 
 *	struct sec_dev *sec_cfg;
 *
 * The device is present.
 */
kpprobe(probe)
struct sec_probe *probe;
{
	struct sec_gmode *tio;

	/*
	 * Probe by attempting to read the state of the profiler
	 */
	tio = (struct sec_gmode *) kp_buf;
	tio->gm_status = 0;

	SEC_startio(SINST_GETMODE, (int *)tio, 4, probe->secp_chan,
		&probe->secp_desc->sec_powerup->pu_cib,
		(u_char) probe->secp_desc->sec_slicaddr);

	if (tio->gm_status != SINST_INSDONE) {
		return(0);
	} else {
		return(1);
	}
}

/*
 * kpboot - initialize this device driver.
 *
 * Called once after all probing has been done.
 *
 * This procedure initializes and allocates all device driver data
 * structures based off of the configuration information passed in from
 * autoconfig() and from the device drivers binary configuration tables.
 */

/*ARGSUSED*/
kpboot(num_dev, sec_array)
int num_dev;
struct sec_dev sec_array[];
{
	extern etext;

#ifdef DEBUG
	printf("Kpboot: Number of Devices: %d\n",num_dev);
#endif

	kp = (struct kp *) calloc(sizeof(struct kp));

	/*
	 * initialize the kp structure
	 */
	kp->kp_stat = 0;

	/*
	 * Init semas ...
	 */
	init_sema(&kp->kp_sema, 1, 0, kp_gate);

	/*
	 * Allocate space
	 */
	/* calc # of bins: + 2 is for truncation in shift and for user mode */
	kp_nmis = (u_long *) calloc((int)(Nengine * sizeof(u_long)));
	kp_in_nmi = (int *) calloc((int)(Nengine * sizeof(u_long)));
	kp_ov_nmi = (int *) calloc((int)(Nengine * sizeof(u_long)));
	kp_b_text = sizeof(struct exec);
	kp_bins = (((unsigned) &etext - kp_b_text) >> kp_binshift) + 2;
	kp_pc_m = (struct pc_mode *) calloc((int)(Nengine * sizeof(struct pc_mode)));
	kp_c_size = sizeof(unsigned) * kp_bins;
	kp_counters = (u_long *) calloc(kp_c_size);
	kp->kp_desc = &sec_array[0];
#ifdef DEBUG
	printf("pc_m: 0x%x counters: 0x%x b_text: 0x%x etext: 0x%x bins: %d\n",
		kp_pc_m, kp_counters, kp_b_text, &etext, kp_bins);
#endif
	printf("Profiling data counters took %d KB's\n",
	    (kp_c_size + 1023) / 1024);
}

/*
 * kpopen	Open the device
 */
kpopen(dev)
dev_t	dev;
{
	register unit;

	unit = minor(dev);

	if ((1 << unit) & SECvec)
		return(0);

	return(ENXIO);
}

/*
 * kpread	Read procedure.
 */
/*ARGSUSED*/
kpread(dev, uio)
dev_t	dev;
struct uio *uio;
{
	return(uiomove((caddr_t)kp_counters, kp_c_size, UIO_READ, uio));
}


/*
 * kpintr	Interrupt routine
 */
kpintr()
{
	printf("Kpintr: kernel profiler interrupt??\n");
}


/*
 * kpioctl - asynchronous io control requests of the driver.
 *
 * tmp concerns - uses kp_sema to assure that no one else is doing
 * anything to the device.
 */
/*ARGSUSED*/
kpioctl(dev, cmd, addr)
int	cmd;
dev_t	dev;
caddr_t	addr;
{
	register struct kp_modes *mp;
	register struct kp_status *stat;
	struct sec_gmode *gmp;
	struct sec_smode *smp;
	int error;
	extern etext;

	error = 0;

	p_sema(&kp->kp_sema, PZERO-1);

	switch (cmd) {

	case KP_GETSTATE:
		stat = (struct kp_status *) addr;
		gmp = (struct sec_gmode *) kp_buf;
		gmp->gm_status = 0;
		mp = (struct kp_modes *) &gmp->gm_un.gm_kp;
		sec_startio(SINST_GETMODE, (int *)gmp, kp->kp_desc);
		if (gmp->gm_status != SINST_INSDONE) {
			printf("kp: Get modes failure: %x\n", gmp->gm_status);
			error = EIO;
			break;
		}
		stat->kps_interval = mp->kpm_interval;
		stat->kps_reload = mp->kpm_reload;
		stat->kps_state = mp->kpm_state;
		stat->kps_binshift = mp->kpm_binshift;
		stat->kps_bins = mp->kpm_bins;
		stat->kps_engines = mp->kpm_engines;
		stat->kps_b_text = kp_b_text;
		stat->kps_e_text = (unsigned) &etext;
		stat->kps_sced_nmis = mp->kpm_sced_nmis;
		break;

	case KP_SETSTATE:
		stat = (struct kp_status *) addr;
		if ((stat->kps_interval <= 0) || (stat->kps_reload < 0)) {
			error = EINVAL;
			break;
		}
		kp_state = stat->kps_state;
		smp = (struct sec_smode *) kp_buf;
		smp->sm_status = 0;
		mp = (struct kp_modes *) &smp->sm_un.sm_kp;

		mp->kpm_interval = stat->kps_interval;
		mp->kpm_reload = stat->kps_reload;
		mp->kpm_state = stat->kps_state;
		/* Take care of things the user can't */
		mp->kpm_b_text = kp_b_text;
		mp->kpm_binshift = kp_binshift;
		mp->kpm_bins = kp_bins;
		mp->kpm_engines = Nengine;
		mp->kpm_pc = kp_pc_m;
		mp->kpm_cntrs = kp_counters;
		sec_startio(SINST_SETMODE, (int *)smp, kp->kp_desc);
		if (smp->sm_status != SINST_INSDONE) {
			printf("kp: Set kp Modes Failure: %x",
				smp->sm_status);
			error = EIO;
			break;
		}
		break;

	case KP_RESET:
		/* zero the counters */
		bzero((caddr_t)kp_counters, (unsigned)kp_c_size);
		break;

	/*
	 * Unknown command.
	 */
	default:
		error = ENXIO;
		break;
	}

	v_sema(&kp->kp_sema);
	return(error);
}
