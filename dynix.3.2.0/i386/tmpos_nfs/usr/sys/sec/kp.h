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

/*
 * $Header: kp.h 1.1 86/10/07 $
 */

/* $Log:	kp.h,v $
 */

/*
 * kp.h
 *	Profiler driver definitions.
 */

/*
 * kernel profiling NMI msg
 * (this really belongs in machine/intctl.h but is here so can be shared w/
 *  SCED fw)
 */
#define NMI_PROF	0x80

#ifdef	KERNEL		/* Cause kp.h shared w/ sced fw */
/*
 * struct prof: at most ONE per system
 */
struct kp {
	struct sec_dev *kp_desc;	/* config structure */
	int kp_stat;			/* status from read and write coms */
	struct sec_dev_prog kp_dp;	/* device program */
	sema_t kp_sema;			/* semaphore for ioctl's */
};
#endif

/*
 * profiling status structure
 * this is what is passed to/from the user process
 */

struct kp_status {
	int kps_interval;	/* SCED timer interrupt interval in ms */
	int kps_reload;		/* SCED sends NMI every reload timer intrs */
	int kps_state;		/* state of profiling */
				/* Below are not user settable (read only) */
	int kps_binshift;	/* Log2(Size in bytes of a bin) */
	int kps_bins;		/* Number of bins used in profiling */
	int kps_engines;	/* number of engines profiled */
	unsigned kps_b_text;	/* beginning of kernel text */
	unsigned kps_e_text;	/* end of kernel text */
	unsigned long kps_sced_nmis;/* Number of nmi's sent by fw */
};

#define KP_ENABLED	0x01
#define KP_BINERROR	0x40000000
#define KP_DEBUG	0x80000000

/*
 * Profiler Ioctl's
 */

#define KP_GETSTATE	_IOR(p,0,struct kp_status)
#define KP_SETSTATE	_IOW(p,1,struct kp_status)
#define KP_RESET	_IO(p,2)
