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

/* $Header: vlsi.h 1.1 89/08/24 $ */

/*
 * vlsi.h
 *	VLSI performance driver ioctl and misc driver parameters.
 */

/* $Log:	vlsi.h,v $
 */

/*
 * Driver info
 */

struct vlsiinfo {
	sema_t	vi_mutex;
	u_char	vi_state;		/* Current state (see defines below) */
	u_char	vi_ncmc;		/* total number of CMCs in system */
	u_char	vi_cmcrev;		/* CMC rev (minimum rev if mixed) */
	u_char	vi_nprocbic;		/* total number of processor BICs */
	u_char	*vi_proc_ncmc;		/* number of CMCs on each processor */
	u_char	*vi_procbic_slic;	/* SLIC id for each processor BIC */
	u_int	*vi_data;		/* data buffer */
};

/*
 * VLSI performance counter driver states
 */

#define VLSI_INUSE	0x1		/* Currently in use (open) */
#define VLSI_STARTED	0x2		/* counter are running */

/*
 * Number of counters on each chip
 */
#define VLSI_BIC_CNTRS	1		/* 1 counter on each BIC */
#define VLSI_CMC_CNTRS	2		/* 2 counters on each CMC */

/*
 * Information needed to setup up a BIC counter
 */
struct bic_cntr_setup {
	u_char	bs_bicnum;		/* which BIC in list of BICs */
	u_char	bs_select0;
	u_char	bs_mask0;
	u_char	bs_select1;
	u_char	bs_mask1;
};

/*
 * Information needed to setup up a CMC counter
 */
struct cmc_cntr_setup {
	u_char	cs_cntr;		/* which of the two CMC counters */
	u_char	cs_select;		/* select register setting */
	u_long	cs_mask;		/*   mask register    "  (bits 0:23) */
};

/*
 * VLSI perf config information (for return via ioctl)
 */
struct vlsi_cfg {
	u_char  vc_nprocbic;		/* number of processor BICs available */
	u_char	vc_ncmc;		/* total number of CMCs */
	u_char	vc_cmcrev;		/* "global" CMC rev */
};

/* 
 * IO controls.
 *
 * VLSIIOCGETCMCP passes in a ptr and the driver does its own copyout()
 */

#define	VLSIIOCSTART	_IO(V,0)			/* start counters */
#define	VLSIIOCSTOP	_IO(V,1)			/* stop  counters */
#define	VLSIIOCGETCFG	_IOR(V,2,struct vlsi_cfg)	/* get vlsi config */
#define	VLSIIOCGETCMCP	_IOW(V,3,u_char *)		/* get CMCs/proc */
#define	VLSIIOCSETCMC	_IOW(V,4,struct cmc_cntr_setup)	/* setup CMC counter */
#define	VLSIIOCSETPROCBIC _IOW(V,5,struct bic_cntr_setup) /* setup BIC cntr */
