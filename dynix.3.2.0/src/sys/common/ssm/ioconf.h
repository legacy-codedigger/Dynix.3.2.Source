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

/* $Header: ioconf.h 1.5 90/11/08 $ */

/*
 * ioconf.h 
 *	Configuration description of a device on a Systems 
 * 	Services Module (SSM).
 */

/* $Log:	ioconf.h,v $
 */

/*
 * An array of ssm_conf constitutes the 
 * configuration of SSM drivers and devices.  
 */
struct	ssm_conf {
	struct ssm_driver *ssm_driver;	/* Addr of per-driver data */
	int ssm_nent;			/* # entries in ssm_dev[] */
	struct ssm_dev *ssm_dev;	/* Related H/W descr. array */
};

/*
 * Each device includes one or more ssm_driver 
 * structures in its source to define its 
 * characteristics.
 * 
 * The sdr_cflags and sdr_intr fields indicate 
 * how many interrupts the device has; none if 
 * sdr_intr is NULL, otherwise there are two if 
 * SDR_DUALINT is set in sdr_cflags and one if 
 * not.
 * sdr_cflags also indicates the bus type the 
 * device is located upon.
 * The probe and boot procedures are called on 
 * a per-device basis, and are NULL if non-existent.
 * The interrupt procedure is used to initialize 
 * interupt vectors.  It is also NULL if non-existent.
 */
struct	ssm_driver {
	char	*sdr_name;		/* Name, i.e. "tm" (no digit) */
	int	sdr_cflags;		/* Per-driver flags (below) */
	int	(*sdr_probe)();		/* Probe procedure */
	int	(*sdr_boot)();		/* Boot procedure */
	int	(*sdr_intr)();		/* Interrupt service routine */
};

#define	SDR_IS_SCSI	0x00000001	/* Device on the SCSI bus */
#define	SDR_IS_VME	0x00000002	/* Device on the VME bus */
#define SDR_OTHER	0x00000004	/* None of the above!  For ports
					 * located directly on the SSM */
#define SDR_DUALINT	0x00000008	/* 2 interrupts per unit */
#define	SDR_INTERFACE_TYPE	(SDR_IS_SCSI | SDR_IS_VME | SDR_OTHER)
#define	SDR_TYPICAL_SCSI	(SDR_IS_SCSI | SDR_DUALINT)

#define SDR_TYPICAL_CONS	(SDR_OTHER | SDR_DUALINT) /* Console ports */ 
#define SDR_TYPICAL_PRNT	(SDR_OTHER | SDR_DUALINT) /* printer port */

/*
 * Each ssm_conf entry addresses an array 
 * of ssm_dev's; each ssm_dev structure
 * describes a single SSM device. 
 * After probing, the driver boot procedure 
 * is invoked with the address of an array 
 * of these and the number of entries 
 * in that array.
 */
struct	ssm_dev {
	short		sdv_ssm_idx;	/* SSM index; -1 == wildcard */
	u_char		sdv_bin;	/* Interrupt bin number */
	u_short		sdv_maps_req;	/* #dma mappings requested */
	/*
	 * The sdv_vme_csr, sdv_hw_level, sdv_hw_vector,
 	 * sdv_map_space, and sdv_csr_space fields
	 * are for VME bus devices. 
	 * The sdv_unit field is for console,
	 * printer port, and SCSI bus devices. 
	 * The sdv_target is for SCSI bus devices. 
	 */
	u_char		sdv_map_space;	/* VME space for dma maps */
	u_long		sdv_vme_csr;	/* VME device register location */
	u_char		sdv_csr_space;	/* VME space for device regs */
	u_char		sdv_hw_level;	/* VME interrupt level */
	u_char		sdv_hw_vector;	/* VME interrupt vector */
	short		sdv_target;	/* SCSI target number */
	short		sdv_unit;	/* console/printer/SCSI unit number */
	/*
 	 * The remaining fields are filled in when 
	 * the configure() routine runs.
	 * sdv_sw_intvec of first ssm_dev in a set 
	 * defines "base" vector.
	 * sdv_csr is the sequent bus address of vme devices.
 	 */
	short		sdv_busno;	/* Bus number of its type on the SSM 
					 * (for future development) */
	u_long		sdv_flags;	/* Flags (arbitrary) */
	u_short		sdv_maps_avail;	/* #dma mappings given */
	caddr_t		sdv_maps;	/* Baseaddr of allocated maps */
	struct ssm_desc	*sdv_desc;	/* Host controller descriptor */
	u_long		sdv_csr;        /* sqnt addr mapped to sdv_vme_csr */
	u_char		sdv_sw_intvec;	/* Host interrupt vector */
	u_char		sdv_hostslic;	/* Host id/group to interrupt */
	u_char		sdv_alive;	/* Is the device alive? */
};

/*
 * Structure passed to SSM device's probe 
 * routine.  
 * sp_csr is only relevent to address-bus 
 * type devices.
 * sp_busno is for bus-type devices, i.e.
 * SCSI and VME, but not for console.
 * sp_target is for SCSI devices only.
 * sp_unit is relevent for console, printer, 
 * and SCSI devices.
 */
struct	ssm_probe {
	struct ssm_desc	*sp_desc;	/* SSM controller descriptor address */
	u_long		sp_flags;	/* Copy of sdv_flags */
	u_long		sp_csr;		/* VME register address */
	short		sp_busno;	/* Bus number of its type on the SSM */
	short		sp_target;	/* SCSI target number */
	short		sp_unit;	/* console/printer/SCSI unit number */
};

/*
 * One of these exists per active SSM.
 * Filled in by conf_ssm().
 */
struct ssm_desc {
	struct ctlr_desc *ssm_cd;	/* Descriptor for this SSM */
	struct cons_cb	*ssm_cons_cbs;	/* Base console command block for SSM */
	struct print_cb *ssm_prnt_cbs;	/* Base printer command block for SSM */
	struct ssm_misc	*ssm_mc;	/* Low level message block for SSM */
	u_char 		ssm_vmedevs;	/* # devices alive on the VME bus */
	u_short		ssm_s2v_map;	/* next available s2v map */
	u_short		ssm_v2s_map[4]; /* next map in mapset */
	char        	pic_flush_method;/* method of PIC flushing */
	long        	*pic_flush_reg; /* address of the pic flush register */
	long            *pic_flush_memory;/* address of memory flush for PIC */
};

#define ssm_diag_flags  ssm_cd->cd_diag_flag
#define ssm_target_no   ssm_cd->cd_ssm_host_num
#define ssm_is_cons     ssm_cd->cd_ssm_cons
#define ssm_version     ssm_cd->cd_sc_version
#define ssm_slicaddr    ssm_cd->cd_slic
#define ssm_biff_type   ssm_cd->cd_ssm_biff_type
#define ssm_biff_flags  ssm_cd->cd_ssm_biff_flags


/*
 *  defines for pic_flush_method
 *      REG method is for SSM1
 *      MEMORY method is for SSM2 PIC chip bug
 */
#define SSM_PIC_FLUSH_REG       0x01
#define SSM_PIC_FLUSH_MEMORY    0x02


/*
 * The following macros determine if the 
 * SCSI device associated with a given
 * ssm_dev structure has a wildcarded address.
 */
#define	SSM_WILD(dev)		((dev)->sdv_ssm_idx == -1)
#define	SSM_WILD_BUS(dev)	((dev)->sdv_busno == -1)
#define	SSM_WILD_TARG(dev)	((dev)->sdv_target == -1)
#define	SSM_WILD_LUNIT(dev)	((dev)->sdv_unit == -1)

#define	SSM_SCSI_WILD(dev)	(SSM_WILD(dev) || SSM_WILD_BUS(dev) || \
				 SSM_WILD_TARG(dev) || SSM_WILD_LUNIT(dev))
