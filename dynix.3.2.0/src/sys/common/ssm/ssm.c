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
static char rcsid[]="$Header: ssm.c 1.12 90/11/10 $";
#endif

/*
 * ssm.c
 *	Systems Services Module (SSM) driver code.
 */

/* $Log:	ssm.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/reboot.h"
#include "../h/vm.h"			  /* for VA_SLIC */
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/bic.h"
#include "../balance/cfg.h"
#include "../balance/clkarb.h"
#include "../machine/hwparam.h"
#include "../machine/intctl.h"
#include "../machine/param.h"
#include "../machine/pte.h"
#include "../ssm/ioconf.h"
#include "../ssm/sc.h"
#include "../ssm/sp.h"
#include "../h/scsi.h"
#include "../ssm/ssm_misc.h"
#include "../ssm/ssm_scsi.h"
#include "../ssm/ssm_vme.h"
#include "../ssm/ssm.h"
#include "../h/cmn_err.h"


/* 
 * Array of SSM descriptors alloc'd 
 * and filled by conf_ssm() and a
 * description of its elements.
 */
struct ssm_desc	*SSM_desc;		/* Array of SSM descriptors. */
struct ssm_desc	*SSM_cons;		/* SSM descriptor of console board. */
static int	NSSM;			/* # SSM's to map at boot time. */

static int	max_ssm_idx;		/* Max config index # for an SSM */

static u_int	SSM_vec;		/* Bit-vector of existing SSM's */
/* static u_char	SSM_basevec;	 * Base interrupt vector of SSM's */
static struct pte *ssm_vme_pt;		/* Address of second level page
					 * tables for VME memory */
static label_t	ssm_vme_nmi_jmp;	  /* for NMI recovery during probe */
static int ssm_probed;			  /* for error reporting */

static struct ssm_desc *probe_scsi_dev(); 	/* Forward Reference */
static struct ssm_desc *probe_other_dev();	/* Forward Reference */
static struct ssm_desc *probe_vme_dev();	/* Forward Reference */
static void ssm_pic_set();		       /* Forward Reference */
static int ssm_vme_nmi();		       /* Forward Reference */

/*
 * The static array 'scsi_found' is a table of 64-bit 
 * vectors, one per SSM.  A set bit in the 
 * [ssm,devno] bit position of the table means 
 * a device exits on that SSM at that devno (SCSI 
 * device number made up of target adapter and 
 * logical unit number; between 0 and 63).  For 
 * now, the macros SCSI_IN_USE, SET_SCSI_IN_USE,
 * SCSI_NOPROBE, and SET_SCSI_NOPROBE are used to
 * manipulate it.
 * It is allocated by and used by probe_scsi_dev().
 *
 * Note: this scheme will require modification if/when
 * multiple SCSI interfaces appear on a single SSM.
 */
static u_char *scsi_found;

#define	SCSI_IN_USE(ssm_index,target,unit) \
	(scsi_found[((ssm_index) << 3) + (target)] & 1 << (unit))

#define	SET_SCSI_IN_USE(ssm_index,target,unit) \
	(scsi_found[((ssm_index) << 3) + (target)] |= 1 << (unit));

#define SCSI_NOPROBE(ssm_index,target) \
	(scsi_found[((ssm_index) << 3) + (target)] == 0xff)

#define SET_SCSI_NOPROBE(ssm_index,target) \
	(scsi_found[((ssm_index) << 3) + (target)] = 0xff);

extern caddr_t calloc();

extern struct ssm_misc *ssm_misc_init();

/*
 * conf_ssm()
 *	Configure system services module.  
 *
 *	Assume that there may be more than 
 * 	one of these in the system at a time
 *	(only one is the console though). 
 */
conf_ssm()
{
	register struct ctlr_toc *toc1, *toc2;
	register struct	ctlr_desc *cd;
	register struct	ssm_desc *ssm;
	register struct ssm_conf *sc = ssm_conf;
	register int i;
	int ssm1_cnt, ssm2_cnt, ssm_cnt, board, bdcount;
	extern	int	light_show;
	extern	unsigned sec0eaddr;

	/*
	 * For SSM devices that have dual interrupts
	 * the config utility only reserves one vector.
	 * Increase the number of vectors reserved for
	 * these devices since they will be allocated
	 * and attempt made to boot them, even if they
	 * are not alive.
	 */
	for ( ; sc->ssm_driver; sc++) 
		if (sc->ssm_driver->sdr_cflags & SDR_DUALINT) 
			ivecres(sc->ssm_dev->sdv_bin, sc->ssm_nent);

	toc1 = CD_LOC->c_toc + SLB_SSMBOARD;
	toc2 = CD_LOC->c_toc + SLB_SSM2BOARD;

	ssm1_cnt = toc1->ct_count;
	ssm2_cnt = toc2->ct_count;

	ssm_cnt = ssm1_cnt + ssm2_cnt;
	max_ssm_idx = -1;

	if (ssm1_cnt < 1) {
		if (ssm2_cnt < 1) {
			CPRINTF("No SSMs\n");
			NSSM = 0;
			return;
		}
	}

	/*
	 * Allocate an array of descriptors, 
 	 * one per potential SSM. 
	 */
	ssm = SSM_desc = (struct ssm_desc *)
			calloc((int)ssm_cnt * sizeof(struct ssm_desc));
	/*
	 * Initialize each of the descriptors.
	 * Save the location of the master board
	 * for access by misc. functions.
	 */
	NSSM = 0;
	cd = CD_LOC->c_ctlrs + toc1->ct_start;
	board = SLB_SSMBOARD;
	bdcount = ssm1_cnt;

nextboard:
	CPRINTF("%d SSM%d controller%s %s", bdcount, 
			(board == SLB_SSMBOARD) ? 1 : 2,
			(bdcount == 1) ? "" : "s",
			(bdcount == 0) ? "" : "; slic"); 
			;
	for (i = 0; i < bdcount; i++, cd++, ssm++) {

		ssm->ssm_cd = cd;
		CPRINTF(" %d", ssm->ssm_slicaddr);
		if (ssm->ssm_is_cons) {
			/*
			 * We're in charge -set up to use 
			 * front-panel LED's and get system 
			 * ID from the SSM.
			 */
			SSM_cons = ssm;
			CPRINTF(" (system controller)");
			fp_lights = 1;		/* use front panel LEDs */
			if (light_show > 1)
				fp_lights = -1;	/* front-panel and proc LEDs */
			FP_IO_ONLINE;		/* ASSUME all drives online */
			sec0eaddr = cd->cd_ssm_sysid & 0x00FFFFFF;
		}
		/* turn off this board's VME memory accessibility via PIC */
		ssm_pic_set(ssm->ssm_slicaddr, 0L);

		/*
		 * Initialize the SSM's interfaces, unless 
		 * the board is deconfigured or failed diags.  
		 * 
		 * Note that a reported failure can only 
		 * occur if there are multiple SSM type 
		 * boards in the system.
		 */
		if ((cd->cd_diag_flag & (CFG_FAIL | CFG_DECONF)) == 0) {
			/*
			 * calc the pic flush register address here for
			 * use in the ssm_vme_dma_flush routine for
			 * speed reasons
			 */
			switch( cd->cd_type ) {
				case SLB_SSMBOARD:
					ssm->pic_flush_method =
							SSM_PIC_FLUSH_REG;
					ssm->pic_flush_reg =
					        PIC_REGISTER_ADDR( PA_SSMVME(i),
								PIC_BCR_NARROW);
					break;

				case SLB_SSM2BOARD:
					ssm->pic_flush_method =
							SSM_PIC_FLUSH_MEMORY;
					ssm->pic_flush_reg =
						PIC_REGISTER_ADDR( PA_SSMVME(i),
								PIC_BCR_WIDE);
					break;

				default:
					continue;
					/* something is wrong, don't use this
					 * board
					 */
			}
			/* 
			 * Allocate console and printer command blocks and 
			 * notify the SSM of their location.
			 */
			ssm->ssm_cons_cbs =  init_ssm_cons(ssm->ssm_slicaddr);
			ssm->ssm_prnt_cbs = init_ssm_prnt(ssm->ssm_slicaddr);
			ssm->ssm_mc = ssm_misc_init(ssm == SSM_cons, 
						    ssm->ssm_slicaddr);
			/* 
			 * setup first mapping value for this board, even
			 * if the VME is not present (we'll determine that
			 * later.) We still need the first map at
			 * which to start allocating maps later
			 * Set starting values for s2v and v2s mapping -
			 * the next values will be calc'd from these
			 */
			ssm->ssm_s2v_map = SSM_S2V_1ST_MAP;

			ssm->ssm_v2s_map[ SSM_VME_A16_MAPS ] = 0;
			ssm->ssm_v2s_map[ SSM_VME_A24_MAPS ] = 0;
			ssm->ssm_v2s_map[ SSM_VME_A32_LO_MAPS ] = 0;
			ssm->ssm_v2s_map[ SSM_VME_A32_HI_MAPS ] = 0;


			if( cd->cd_ssm_biff_type == CFG_SSM_BTYPE_VBIF ) {
    				/* IF there is a VBIF on this SSM, then...
		         	 * Initialize mapping of VME interrupts and set 
			 	 * phy address at which SSM should respond to. 
				 * Memory address is calc by
			 	 *  IOBASE + 32Mb + (i * 32Mb)    
				 *	[ i is the board index 0,1,2, etc]
		   	 	 * if the VME is not present
			 	 * these routines will return an error, 
				 * but we don't really care here. 
				 * If it is there, then set the numbers for 
				 * the board
				 * For SSM1, use old method for compatibility
				 * for SSM2, ask fw to do the programming so
				 * both kernel and fw know where its at.
		         	 */
		        	ssm_clr_vme_imap( ssm->ssm_mc, 
						ssm->ssm_slicaddr);

				/*
				 * depending on the SSM board in question
				 * set its physical address
				 */
				switch( cd->cd_type ) {
					case SLB_SSM2BOARD:
							ssm_set_vme_mem_window( 
							ssm->ssm_mc,
							ssm->ssm_slicaddr, 
							(u_char) i );
						break;

					case SLB_SSMBOARD:
						ssm_pic_set(ssm->ssm_slicaddr, 
							(long) PA_SSMVME(i) );
						break;
				
					default:
						CPRINTF("conf_ssm:bad board type");
						break;
				}
				/* 
				 * For wide PIC flush bug, we need to do memory
				 * type of flush. If PIC is update and the bug
				 * no longer exists, remove this
				 *
				 * need to setup a s2v and a v2s map for
				 * performing a PIC flush if the board
				 * requires memory based flushing
			         */
				 if(ssm->pic_flush_method==SSM_PIC_FLUSH_MEMORY)
					ssm_set_pic_flush_address(ssm);
						
			}

			NSSM++;
			SSM_vec |= 1 << i;
		}	
#ifdef DEBUG
		if (cd->cd_diag_flag)
			CPRINTF(" flags 0x%b", cd->cd_diag_flag);
#endif DEBUG
	}
	CPRINTF(".\n");
	
	/* After looking at all SSM1 boards, look at SSM2 boards. */
	if (board == SLB_SSMBOARD) {
		cd = CD_LOC->c_ctlrs + toc2->ct_start;
		board = SLB_SSM2BOARD;
		bdcount = ssm2_cnt;
		goto nextboard;
	}

	/* 
	 * Report any SSM's that were 
 	 * deconfigured or failed diags.
	 * Save the largest valid index into
	 * SSM_desc also.
	 */
	if (NSSM == ssm_cnt) 
		max_ssm_idx = (int) ssm_cnt - 1;
	else {
		CPRINTF("Not using System Services Modules:  slic");
		for (i = 0; i < ssm_cnt; i++) 
			if (SSM_EXISTS(i)) 
				max_ssm_idx = i;
			else
				CPRINTF(" %d", SSM_desc[i].ssm_slicaddr);
		CPRINTF(".\n");
	}
}

/*
 * probe_ssm_devices()
 *	Probe for devices attached to the SSM controllers.
 *
 * 	Probing gets tricky when ambiguous specifications 
 *	of devices are allowed.  
 * 	Wildcarding requires that we keep track of where 
 * 	devices were found to avoid looking for other 
 * 	devices there.  Wildcarding occurs in the 
 * 	interface-specific probe routine.
 *	Currently, only SCSI devices can wildcard their 
 *	location on that board.  
 *
 *	If a VME device is determined to be available,
 *	it must be noted so that SSM's VME memory is mapped
 *	into the kernel's virtual address space later on.
 */
probe_ssm_devices()
{
	register struct	ssm_conf *ssm;
	register struct	ssm_driver *driver;
	register struct	ssm_dev	*dev;
	register int i;
/*	register int j;							*/
	u_char driver_bin;
	int nbr_vme_windows = 0;
/*	u_char vecbase, vec;						*/

	/*
	 * Run through the ssm_conf array doing 
	 * the probes.  A driver's boot procedure 
	 * is invoked even if existing HW is not found.
	 * 
         * Probe each SSM driver for each possible 
 	 * device it may control.
	 */
	for (ssm = ssm_conf; driver = ssm->ssm_driver; ssm++) {
#ifdef DEBUG
		CPRINTF("probe_ssm_devices: %d probes for %s driver, 0x%x ",
			ssm->ssm_nent, driver->sdr_name, driver->sdr_cflags);
		CPRINTF("cflags, probe @ 0x%x, boot @ 0x%x, intr @ 0x%x.\n",
			(u_long)driver->sdr_probe, (u_long)driver->sdr_boot, 
			(u_long)driver->sdr_intr);
#endif DEBUG
		for (driver_bin = (u_char)((dev = ssm->ssm_dev)->sdv_bin),
		     i = 0;  i < ssm->ssm_nent;  i++, dev++) {

#ifdef DEBUG
			CPRINTF("probe_ssm_devices: probe %s%d.\n",
				driver->sdr_name, i);
#endif DEBUG

			/*
			 * Drivers assume contiguous interrupt 
			 * vectors in the same bin for all devices 
			 * they control.  Verify that all have been 
			 * configured in this manner. 
			 */
			if (dev->sdv_bin != driver_bin) {
				CPRINTF("probe_ssm: %s devices configured with ",
					driver->sdr_name);
				CPRINTF("different bin #s; reconfigure.\n.");
				panic("Different bins");
				/*
				 *+ The interrupt vectors for this bin are
				 *+ not contiguous.  This is required by the
				 *+ driver.
				 */
			} else if (driver->sdr_intr) {		
				/* 
				 * The device has an interrupt 
				 * procedure; allocate a vector 
				 * in the appropriate bin even 
				 * if the probe function is 
				 * undefined to ensure sequential 
			 	 * vectors for all occurences of 
				 * its device type.
			 	 */
				dev->sdv_sw_intvec = ivecall(dev->sdv_bin);
				if (driver->sdr_cflags & SDR_DUALINT) { 
					/* Allocate second vector for device */
					(void)ivecall(dev->sdv_bin);
				}
			}

			/*
			 * Determine where the device resides
			 * upon the SSM and invoke the appropriate 
			 * routine to handle any wildcarding and probing 
			 * for the device.  If the device is there, 
			 * fill out configuration fields of *dev. 
			 * Otherwise, skip this device and attempt 
			 * probing the next one.
			 */
			switch (driver->sdr_cflags & SDR_INTERFACE_TYPE) {
			case SDR_IS_SCSI:
				if (!probe_scsi_dev(driver, dev)) {
#ifdef DEBUG
					CPRINTF("\tprobe for %s%d failed.\n",
						driver->sdr_name, i);
#endif DEBUG
					continue;	/* Not found */
				}

				/* Tell the world it's alive.  */
				CPRINTF("%s%d found on SSM%d", 
					driver->sdr_name, i, dev->sdv_ssm_idx);
				CPRINTF(" bin %d target adapter %d unit %d", 
					dev->sdv_bin, dev->sdv_target, 
					dev->sdv_unit);

				/* Allocate requested mapping table entries */
				dev->sdv_maps = (caddr_t) 
					ssm_alloc(dev->sdv_maps_req *
 						  sizeof(long), SSM_ALIGN_XFER, 
						  SSM_BAD_BOUND);
				if (dev->sdv_maps)
					dev->sdv_maps_avail = dev->sdv_maps_req;
				break;
			case SDR_IS_VME:

				if (!probe_vme_dev(driver, dev))
					continue;	/* Not found */

				/* map interrupt vector */
				ssm_set_vme_imap( SSM_desc[i].ssm_mc,
						 SSM_desc[i].ssm_slicaddr,
						 dev->sdv_hw_level,
						 dev->sdv_hw_vector,
						 SL_GROUP | TMPOS_GROUP,
						 dev->sdv_sw_intvec,
						 SL_MINTR | dev->sdv_bin);

				/* Tell the world it's alive.  */
				CPRINTF("%s%d found on SSM%d ", 
				       driver->sdr_name, i, dev->sdv_ssm_idx);
				CPRINTF("csr 0x%lx(0x%x) level %d VMEvec 0x%x",
				       dev->sdv_vme_csr,
				       dev->sdv_csr_space,
				       dev->sdv_hw_level, dev->sdv_hw_vector);

				/* Note the need to map the SSM's VME window */
				if (!dev->sdv_desc->ssm_vmedevs++)
					nbr_vme_windows++;

				break;
			case SDR_OTHER:
				/*
				 * This case covers devices located
				 * on ports directly on the SSM, such
				 * as uarts and parallel ports.  
				 */
				if (!probe_other_dev(driver, dev))
					continue;	/* Not found */

				/* Tell the world it's alive.  */
				CPRINTF("%s%d found on SSM%d", 
					driver->sdr_name, i, dev->sdv_ssm_idx);
				CPRINTF(" bin %d unit %d", dev->sdv_bin,
					dev->sdv_unit);
				break;
			default:
				CPRINTF("probe_ssm: %s%d not configured;",
					driver->sdr_name, i);
				CPRINTF(" invalid driver interface type");
				CPRINTF(" (cflags 0x%x).\n", driver->sdr_cflags);
				continue;
			}

			/*
			 * Common code for all interface types.  
			 * At this point the device has been 
			 * found.  Finish initializing
			 * fields of its device table entry.
			 */
			dev->sdv_alive = 1;
			dev->sdv_hostslic = SL_GROUP | TMPOS_GROUP;
			if (driver->sdr_intr) {		/* At least one int. */
				/* Map ISR of base interrupt vector */
				CPRINTF(" bin %d vec %d", dev->sdv_bin,
				       dev->sdv_sw_intvec);
				ivecinit(dev->sdv_bin, dev->sdv_sw_intvec, 
					 driver->sdr_intr);
				if (driver->sdr_cflags & SDR_DUALINT) {
					/* Map ISR of second vector */
					CPRINTF(" (dual vectors)");
					ivecinit(dev->sdv_bin, 
						 dev->sdv_sw_intvec + 1, 
						 driver->sdr_intr);
				}
			}
			CPRINTF(".\n");
		}
	}

	/*
	 * Probes done, "boot" the drivers.
	 */
	for (ssm = ssm_conf; driver = ssm->ssm_driver; ssm++) {
		if (driver->sdr_boot) 	/* Has a boot routine. */
			(*driver->sdr_boot)(ssm->ssm_nent, ssm->ssm_dev);
	}

	if (nbr_vme_windows) {
		/*
		 * Allocate 2nd level page table space for
		 * virtual address mappings of VME memory
		 * of SSM's with VME devices alive.
		 */
		callocrnd(NBPG);
		ssm_vme_pt = (struct pte *) calloc(nbr_vme_windows * 
			NPTE_SSMVME * sizeof(struct pte));
		ASSERT(((u_long)ssm_vme_pt & NBPG - 1) == 0,
			"SSM/VME page table not on page boundary");
		/*
		 *+ When the kernel was allocating space for SSM/VME page table
	  	 *+ the space was not allocated on a page boundary.
		 */
	}
	
	/*
	 *  Initialize SSM error reporting vectors.
	 *  These NMI vectors must be sequential.
	 */
/*	SSM_basevec = ivecpeek(SSM_ERROR_BIN);				*/
/*	for (i = max_ssm_idx; i >= 0; i--) {				*/
/*		vecbase = ivecpeek(SSM_ERROR_BIN);			*/
/*		for (j = ERRST_NERRS; j; j--) {				*/
/*			vec = ivecall(SEC_ERROR_BIN);			*/
/*			ivecinit(SSM_ERROR_BIN, vec, SSM_error);	*/
/*		}							*/
/*		set_ssm_errst(1000, SL_GROUP|TMPOS_GROUP, vecbase, SL_NMINTR);*/
/*	}								*/
}

/*
 * probe_scsi_dev()
 *	Probe for a scsi device on an SSM.
 *
 * 	Handles wild-card SSM specification, 
 * 	finding an SSM with the device if it 
 *	exists.  We won't look where a device 
 *	has already been found.
 *
 * 	Returns a pointer to the SSM descriptor 
 *	the device lives on or NULL if not found.
 */
static struct ssm_desc *
probe_scsi_dev(driver, dev)
	struct ssm_driver *driver;
	register struct ssm_dev *dev;
{
	register int ssm_index, ssm_count;
	register struct ssm_desc *ssm;
	register int target, unit;
	int target_start, target_end, unit_min, unit_max;
	int pstatus;
	struct ssm_probe probe;

 	/*
	 * Allocate the 'scsi_found' bit-vectors if
	 * not already done.
 	 */
	if (!scsi_found)
		scsi_found = (u_char *) calloc((max_ssm_idx + 1) << 3);
	ASSERT(scsi_found, "SSM: No memory for configuration data.\n");
	/*
	 *+ Allocation of system memory for configuration data failed.
	 *+ System requires more memory for proper operation.
	 */

	if (!driver->sdr_probe) {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: Not driver->sdr_probe\n");
#endif	DEBUG
		/*
		 * The driver hasn't a probe routine. 
		 * Assume the device exists if its
		 * location is not being wildcarded
		 * and its ssm exists.
		 */
		if (SSM_SCSI_WILD(dev)) {
			CPRINTF("Illegal configuration: %s ",driver->sdr_name);
			CPRINTF("device can't be probed for wildcards.\n");
			return (NULL);
		} else if (dev->sdv_ssm_idx <= max_ssm_idx 
		  &&       SSM_EXISTS(dev->sdv_ssm_idx)) {
			dev->sdv_desc = SSM_desc + dev->sdv_ssm_idx;
			SET_SCSI_IN_USE(dev->sdv_ssm_idx,
				dev->sdv_target, dev->sdv_unit)
			return (SSM_desc + dev->sdv_ssm_idx);
		} else
			return (NULL);
	} else if (SSM_WILD(dev)) {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: SSM_WILD\n");
#endif	DEBUG
		/* Prepare to scan all SSM's if necessary */
		ssm_index = 0;
		ssm_count = max_ssm_idx + 1;
	} else {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: Not Wild\n");
#endif	DEBUG
		/* Only scan the specified SSM */
		if ((ssm_index = dev->sdv_ssm_idx) > max_ssm_idx) 
			return (NULL);	/* Its outside the valid range */
		ssm_count = 1;
	} 

	/*
	 * At this this point we know it has a
	 * probe routine and the SSM's to look
	 * for the device upon.   We must also
	 * determine which target adapters and
	 * logical unit numbers to try.
	 */
	if (SSM_WILD_TARG(dev)) {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: SSM_WILD_TARG\n");
#endif	DEBUG
		/* Wildcard target adapter numbers from low to high */
		target_start = 0; 
		target_end = 7; 
	} else {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: One adapter\n");
#endif	DEBUG
		/* Try only the specified target adapter number */
		target_start = target_end = dev->sdv_target;
	}

	if (SSM_WILD_LUNIT(dev)) {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: SSM_WILD_LUNIT\n");
#endif	DEBUG
		/* Wildcard through the unit numbers from low to high */
		unit_min = 0; 
		unit_max = 7;
	} else {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: One unit\n");
#endif	DEBUG
		/* Try only the specified unit number */
		unit_min = unit_max = dev->sdv_unit;
	}

	for ( ; ssm_count--; ssm_index++) {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: for ssm_count--\n");
#endif	DEBUG
		if (!SSM_EXISTS(ssm_index)) 
			continue; 	/* try next SSM location. */
		else
			ssm = SSM_desc + ssm_index;

		/*
		 * Valid SSM found ... for the rest
		 * of wildcarding we have to look at 
		 * all target adapters and unit numbers.
		 * But for non-wildcarded entries 
		 * target_start, etc., are set up just 
		 * to look at the right target/unit.
		 */
		for (target = target_start; target <= target_end; target++) {
#ifdef	DEBUG
			CPRINTF("probe_scsi_dev: for target ;start, end\n");
#endif	DEBUG
			if (target == ssm->ssm_target_no 
			    || SCSI_NOPROBE(ssm_index,target)) {

#ifdef	DEBUG
			CPRINTF("probe_scsi_dev: SCSI_NOPROBE\n");
			CPRINTF("probe_scsi_dev: target = 0x%x, ssm->ssm_target_no = 0x%xssm_index = 0x%x\n", target, ssm->ssm_target_no, ssm_index);
			CPRINTF("probe_scsi_dev: scsi_found[x] = 0x%x\n",
				scsi_found[ssm_index << 3 + target]);
		
#endif	DEBUG
				/*
			 	 * Loop short-circuit:  Skip the host
				 * target adapter and ones that have 
				 * either timed out or are embedded with
				 * a device already located.
				 */
				continue;
			}

			for (unit = unit_min; unit <= unit_max; unit++) {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: for unit; min, max\n");
#endif	DEBUG
				/*
				 * Probe at this combination of SSM#,
				 * target#, and unit# for the device,
				 * if a device has not already been 
				 * located there.
				 */
				if (SCSI_IN_USE(ssm_index, target, unit)) {
#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: SCSI_IN_USE\n");
#endif	DEBUG
					continue;
				}

				probe.sp_desc = ssm;
				probe.sp_flags = dev->sdv_flags;
				probe.sp_busno = dev->sdv_busno;
				probe.sp_target = target;
				probe.sp_unit = unit;

#ifdef	DEBUG
		CPRINTF("probe_scsi_dev: Probing device\n");
#endif	DEBUG
				pstatus = (*driver->sdr_probe)(&probe);
				if (pstatus & SCP_FOUND) {
					/* 
					 * Finish filling in the device
					 * description, mark this address
					 * as in-use, and return. 
					 */
					dev->sdv_ssm_idx = ssm_index;
					dev->sdv_target = target;
					dev->sdv_unit = unit;
					dev->sdv_desc = ssm;
					if (pstatus & SCP_ONELUN) {
						/* Embedded SCSI */
						SET_SCSI_NOPROBE(ssm_index,
								     target);
					} else {
						SET_SCSI_IN_USE(ssm_index, 
								target, unit);
					}
					return (ssm);
				} else if (pstatus & SCP_NOTARGET
				  &&  	  unit == 0 || pstatus & SCP_ONELUN) {
					SET_SCSI_NOPROBE(ssm_index, target);
					break; 		/* unit for-loop */
				}
			}		/* end unit for-loop */
		}			/* end target for-loop */
	}				/* end ssm-index for-loop */

	/*
	 * If at this point in the code then
	 * all valid possiblilities for SSM, 
	 * targets, and unit have been tried
	 * without locating the device.
	 */
	return (NULL);
}

/*
 * probe_vme_dev()
 * 	Probe the vme device located on the SSM.
 *
 * 	Handles wild-card SSM specification, 
 * 	finding an SSM with the device if it 
 *	exists.  
 *
 *	Returns a pointer to the SSM descriptor
 *	the device lives on or NULL if not found.
 */
static struct ssm_desc *
probe_vme_dev(driver, dev)
	struct ssm_driver *driver;
	register struct ssm_dev *dev;
{
	register int ssm_index, ssm_count;
	register struct ssm_desc *ssm;
	struct ssm_probe probe;
	extern int (*probe_nmi)();

#ifdef DEBUG
	CPRINTF("Probing vme driver %s  at ssm %d vme %lx(%x)...",
	       driver->sdr_name, dev->sdv_ssm_idx, dev->sdv_vme_csr,
	       dev->sdv_csr_space);
#endif DEBUG

	if (SSM_WILD(dev)) {
		/* Prepare to scan all SSM's if necessary */
		if (!driver->sdr_probe) {
			CPRINTF("Illegal configuration: %s ",driver->sdr_name);
			CPRINTF("device can't be probed for wildcards.\n");
			return (NULL);
		}
		ssm_index = 0;
		ssm_count = max_ssm_idx + 1;
	} else {
		/* Only scan the specified SSM */
		ssm_index = dev->sdv_ssm_idx;
		ssm_count = 1;

		/* sdv_ssm_idx is raw input--needs to be sanity checked */
		if (ssm_index>max_ssm_idx || !SSM_EXISTS(ssm_index))
			return (NULL);
	} 

	/*
	 * Start scanning.  We take over NMI vector temporarily here, to handle
	 * absent devices (see machine/locore.s for NMI handling).
	 */

	probe_nmi = ssm_vme_nmi;

	for (ssm = SSM_desc + ssm_index; ssm_count--; ssm_index++, ssm++) {
		/* If this ssm has no VMEBUS interface, skip it */
		if (ssm->ssm_biff_type != CFG_SSM_BTYPE_VBIF)
			continue;
		if (!SSM_EXISTS(ssm_index)) 
			continue; 	/* try next SSM location. */
		
		/* Valid SSM found ...  */
		ssm_probed = ssm_index;
		probe.sp_desc = ssm;
		probe.sp_flags = dev->sdv_flags;
		probe.sp_busno = dev->sdv_busno;

		/*
		 * Map a csr for probing.  We assume a csr block never crosses
		 * a SSM_S2V_MAP_SIZE (defined in ssm_vme.h) boundary,
		 * currently, 16384.  Check to be sure we've got a map before
		 * we go probing.  Because of the number of maps available, we
		 * make no attempt to share maps.
		 * NOTE:  we still do the set up even if there is no probe
		 * routine.  WE still need the map set up, etc.
		 */

		if (ssm->ssm_s2v_map >= SSM_S2V_NMAPS) {
#define fmtstr "probe_vme_dev: Out of S to VME maps on ssm%d, probe failed.\n"
			CPRINTF(fmtstr, ssm_index);
#undef fmtstr
			continue;
		}

		probe.sp_csr = ssm_s2v_map( ssm_index, (int)ssm->ssm_s2v_map,
					   dev->sdv_vme_csr,
					   dev->sdv_csr_space ); 

#ifdef DEBUG
		CPRINTF( "sequent address = %lx, map = %d\n", probe.sp_csr,
		       (int)ssm->ssm_s2v_map);
#endif DEBUG

		/*
		 * setjmp will return true if the following probe NMI's,
		 * meaning the device isn't there.
		 */
		if (setjmp(&ssm_vme_nmi_jmp))
			continue;

		if ( !driver->sdr_probe
		    || (*driver->sdr_probe)(&probe) ) {
			/* 
			 * Success! Finish filling in the device
			 * description, and return. 
			 */
			probe_nmi = NULL;
			dev->sdv_ssm_idx = ssm_index;
			dev->sdv_desc = ssm;

			/* Make temporary CSR mapping permanent */
			dev->sdv_csr = ssm_s2v_map( ssm_index,
						   (int) ssm->ssm_s2v_map,
						   dev->sdv_vme_csr,
						   dev->sdv_csr_space ); 
			ssm->ssm_s2v_map++; 

			/* Now, allocate DMA maps */
			if ( ! ssm_allocate_maps( dev) )
				return NULL;
			else
				return ssm;
		}
	}				/* end ssm-index for-loop */

	/*
	 * If at this point in the code then
	 * all valid possiblilities for SSM
	 * have been tried without locating the device.
	 */
	probe_nmi = NULL;
	return NULL;
}

/*
 * ssm_vme_nmi()
 *	NMI handler while probing VME devices.
 *
 * We clear the access error from the processor and longjmp (returning
 * to probe_vme_dev, above).
 *
 * This is entered directly from HW vector table; however, since
 * entry is from drivers 'probe' procedure, probe_vme_dev() already
 * considers scratch registers as volitile (thus, no need to save/restore
 * them).
 */

static
ssm_vme_nmi()
{
#if	defined(ns32000) || defined(KXX)
	int timeout;
	u_char	regval =  rdslave(va_slic->sl_procid, SL_G_ACCERR);

	/*
	 * Writing anything to the processor's access-error register
	 * clears it.
	 */

	wrslave(va_slic->sl_procid, SL_G_ACCERR, 0xbb);
	/*
	 * Need a "clearnmi" for 32K.
	 */
	timeout = ((~regval & SLB_ATMSK) == SLB_AETIMOUT);
#else	SGS HW
	/*
	 * SGS and SGS2 HW
	 */
	int 	timeout = clearnmi();
#endif	KXX

	/*
	 * Determine why we got an NMI.  Hopefully it was NOT
	 * due to a Sequent Bus Timeout.
	 */

	if (timeout) { 
		/*
		 * We COULD deconfigure the board here, and keep on running.
		 * But, we panic instead, just to play it safe, and protect
		 * naive system administrators.
		 */
		CPRINTF("ssm%d: VMEbus is NOT FUNCTIONING, SQT BUS TIMEOUT\n",
		         ssm_probed);
		panic("ssm_vme_nmi: ssm nonfunctional");
		/*
		 *+ The VMEbus is not functioning correctly.  Corrective 
		 *+ action:  contact Sequent Service.
		 */
	}

	/*
	 * "return" to probe_vme_dev.
	 */

	longjmp(&ssm_vme_nmi_jmp);
}

/*
 * probe_other_dev()
 * 	Probe for a device located on the SSM.
 * 	Wildcarding is not allowed.
 *
 *	Returns a pointer to the SSM descriptor
 *	the device lives on or NULL if not found.
 */
static struct ssm_desc *
probe_other_dev(driver, dev)
	struct ssm_driver *driver;
	register struct ssm_dev *dev;
{
	struct ssm_probe probe;

	/*
	 * The device's location must be 
	 * explicit and its SSM must exist.
	 * Note: wildcarding is note valid.
	 */
	if (SSM_WILD(dev) || SSM_WILD_BUS(dev)) {
		CPRINTF("Illegal configuration: %s ",driver->sdr_name);
		CPRINTF("device can't be probed for wildcards.\n");
		return (NULL);
	} else if (dev->sdv_ssm_idx > max_ssm_idx 
	||	   !SSM_EXISTS(dev->sdv_ssm_idx)) 
		return (NULL);		/* Non-existent SSM */

	dev->sdv_desc = SSM_desc + dev->sdv_ssm_idx;

	if (!driver->sdr_probe) 
		return (dev->sdv_desc);		/* Assume device is alive. */
	else {
		/* 
		 * Fill out a probe structure and 
		 * go probe the device.
		 */
		probe.sp_desc = dev->sdv_desc;
		probe.sp_flags = dev->sdv_flags;
		probe.sp_busno = dev->sdv_busno;
		probe.sp_unit = dev->sdv_unit;
		return(((*driver->sdr_probe)(&probe)) ? dev->sdv_desc : NULL);
	}
}

/*
 * SSM_error()
 *	Error interrupt from an SSM board.
 *
 * 	SSM has detected a hard error.  
 * 	Determine the source SSM board, 
 *	report the error, and panic.
 */
/* static								*/
/* SSM_error(vector)							*/
/* 	u_char	vector;							*/
/* {									*/
/* 	register int board = (vector -= SSM_basevec) / ERRST_NERRS;	*/
/* 	register int error = vector % ERRST_NERRS;			*/
/* 	switch (error) {						*/
/* 	case ERRST_BUS_LOCK:						*/
/* 		CPRINTF("SSM%d detects system bus lock.\n", board);	*/
/*		panic("bus lock"); 						*/
/* 	case ERRST_DEVFLT:						*/
/* 		CPRINTF("SSM%d detects host processor or ", board);	*/
/*		CPRINTF("controller device fault.\n"); 			*/
/*		panic("device fault"); 						*/
/* 	case ERRST_SSMFLT:						*/
/* 		CPRINTF("SSM%d reset after fatal error detection.\n", board); */
/*		panic("fatal error detected"); 						*/
/* 	case ERRST_VME_BERR:						*/
/* 		CPRINTF("SSM%d detects VME bus error.\n", board);	*/
/*		panic("VME bus error"); 						*/
/* 	}								*/
/* }									*/

/*
 * ssm_map()
 *	Map VME memory being used into the
 *	kernel's virtual address space.
 *
 *	probe_ssm_devs() determined the number of 
 *	windows to map and allocated enough level
 *	two page table space for them.
 */
ssm_map(kl1pt) 
	struct pte *kl1pt;
{
	register struct pte *pte;
	register u_long paddr, endpaddr;
	int i, k, l1idx;
	register struct ssm_desc *ssm;

	if (!ssm_vme_pt) 
		return;			/* No windows to map */

	for (pte = ssm_vme_pt, i = 0; i < max_ssm_idx; i++) {
		ssm = &SSM_desc[i];

		/* If the SSM does not exist, skip it */
		if (!SSM_EXISTS(i))
			continue;

		/* If the SSM does not have a VME bus, skip it */	
		if (ssm->ssm_biff_type != CFG_SSM_BTYPE_VBIF)
			continue;
		
		/* If no VME devices exist on the bus, skip it */
		if (ssm->ssm_vmedevs == 0)
			continue;

		/* 
		 * Map the SSM's VME memory into the kernel's virtual 
		 * address space.  Each level1 page table entry 
		 * maps NPTEPG of level2 page table entries.
		 */
		for (l1idx = L1IDX(VA_SSMVME(i)), paddr = PA_SSMVME(i),
		     endpaddr = paddr + SSMVME_ADDR_SPACE; 
		     paddr < endpaddr; l1idx++) {
			/* Fill in a level1 page table entry */
			*(long *)(kl1pt + l1idx) = (long)pte 
				| PG_V | PG_R | PG_M | PG_KW;

			/* Fill in the corresponding level 2 entries */ 
			for (k = NPTEPG; k--; paddr += NBPG) 
				*(long *)(pte++) = PHYSTOPTE(paddr)
					| PG_V | PG_R | PG_M | PG_KW;
		}
		/*
		 * Notify the SSM that its VME device
		 * initialization is complete.  And that
		 * it may enable VME bus monitoring.
		 */
		ssm_vme_imap_ready(ssm->ssm_mc, ssm->ssm_slicaddr);
	}
}

/*
 * ssm_pic_set( slic, window )
 * sets the PIC of the ssm whose slic is given to respond to the 32 megabyte
 * window which starts at (window & 0xff000000).  If the most significant bit
 * of window is 0, the PIC is turned off.
 */
static void
ssm_pic_set( slic, window )
	u_char slic;
	long window;
{
	u_char pic_addr = (window & SSM_VME_PIC_MASK)>>PIC_IO_ADDR_SHIFT;

	wrSubslave( slic, SSM_IO_PIC_ADDR, PIC_IO_ADDR_REG, pic_addr );
	
}

/*
 * ssm_boot()
 *	SSM dependent reboot the machine.
 * 	Sets up a return of control to the SSM firmware.  
 * 	If called by panic it returns specifying that the 
 *	alternate boot name is to be booted (normally the 
 *	Memory dumper).  Prior to returning to the 
 *	firmware it attempts to sync the disks.
 * 	Only ONE engine is alive at this point.
 */

static u_int reboot_flags = RB_HALT;

ssm_boot(paniced, howto)
	int paniced;
	register int howto;
{
	spl_t s_ipl;
	extern bool_t dblpanic;
	extern reboot_sync();
	extern int wdtreset();
	extern short upyet;

	if ((!upyet) || ((paniced == RB_PANIC) && dblpanic))
		return_fw();

	/*
	 * Now tell FW how to reboot
	 */
	if (paniced == RB_PANIC) {
		/*
		 * Setup reboot to perform a system dump
		 * and then turn on the error light.
		 */
		reboot_flags = RB_AUXBOOT;
		s_ipl = splhi();
		ssm_set_fpst(ssm_get_fpst(SM_NOLOCK) | FPST_ERR, SM_NOLOCK);
		splx(s_ipl);
	} else {
		/*
		 * Setup reboot to use 'howto' flags passed in.
		 * Then disable the watchdog timer to prevent
  		 * the error light from changing state.
		 */ 
		ASSERT(paniced == RB_BOOT, "ssm_boot: invalid boot specifier");
		/*
		 *+ The ssm_boot routine was called to halt the system.
	         *+ Neither the RB_PANIC or the RB_BOOT bit was set in the
		 *+ parameter that tells ssm_boot what action to take.
		 */

		reboot_flags = howto;
		untimeout(wdtreset, (caddr_t)0);	/* Stop wdt reset */
		s_ipl = splhi();
		ssm_init_wdt((u_long)(60 * 1000));	/* Disable wdt on SSM */
		splx(s_ipl);
	}

	reboot_sync(howto);		/* Sync the disks */
	return_fw();
}

/*
 * ssm_return_fw()
 *	Return control to the SSM firmware.
 *	Prior to doing so, invoke custom
 *	panic handlers (if panicing) and
 *	turn off the light show.
 */
ssm_return_fw()
{
	register int	i;
	extern char *panicstr;
	extern int (*cust_panics[])();
	extern shutdown_light_show(); 
	extern short upyet;

	if (upyet)
		(void) splhi();

	/*
	 * If a panic, call custom panic handlers.
	 */
	if (panicstr != NULL)
		for (i = 0; cust_panics[i] != NULL; i++)
			(*cust_panics[i])();

	shutdown_light_show();		/* Turn off proc lights, etc. */

	ssm_reboot(reboot_flags, 0, (char *)NULL);
	for (;;); 			/* SSM will take control.  */
	/*NOTREACHED*/
}
