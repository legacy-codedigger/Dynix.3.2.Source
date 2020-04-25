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
static char rcsid[] = "$Header: ssm_scsi.c 1.8 90/11/10 $";
#endif lint

/*
 * ssm_scsi.c
 *	routines for manipulating misc pieces of the SSM.
 */

/* $Log:	ssm_scsi.c,v $
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/scsi.h"
#include "../h/proc.h"
#include "../h/cmn_err.h"
#include "../h/vmmac.h"
#include "../h/vmmeter.h"
#include "../machine/pte.h"
#include "../machine/mftpr.h"
#include "../machine/intctl.h"
#include "../machine/vmparam.h"
#include "../machine/plocal.h"
#include "../balance/slic.h"
#include "../balance/cfg.h"
#include "../ssm/ioconf.h"
#include "../ssm/ssm_scsi.h"
#include "../ssm/ssm_misc.h"
#include "../ssm/ssm.h"

static struct scsi_cb *probe_cbs;	/* SCSI CBs recycled during PROBE */

/*
 * init_ssm_scsi_dev() 
 * 	Called while probing or booting a SCSI device 
 *	to provide initialization information about it 
 *	to the SSM board.  It returns an SSM specific 
 *	device id and the location of SCSI CBs to be 
 *	used to communicate with the device.  The device
 *	id less than zero indicates an error occurred.
 *	NOT to be called after booting is performed.
 *
 *	Initialization data and return values are passed
 *	via the scb_init structure, the address of which 
 *	is this function's argument.
 *
 *	This function provides initialization information
 * 	to the SSM by filling out an initialization CB 
 *	for the device and passing its address to the SSM
 *	via a sequence of 5 SLIC messages.  The address
 *	is also the location of the SCSI CBs to be used
 *	thereafter.
 *
 *	If the function argument's si_mode field is SCB_PROBE
 *	a static set of CBs is designated for use long enough
 *	to probe the device, and then are recycled.  Also a
 *	special SSM interrupt vector is used at the beginning
 *	of the SLIC sequence to allow the SSM to recycle its
 *	device i.d.  Otherwise, a new set of CBs is allocated
 *	for the device and a different vector is used. 
 */
void
init_ssm_scsi_dev(sinfo)
	register struct scb_init *sinfo;
{
	struct scsi_init_cb *icb;
	unsigned char *addr_bytes = (unsigned char *)&icb;
	u_int nbytes = sizeof(struct scsi_cb) * NCBPERSCSI;

	if (sinfo->si_mode == SCB_PROBE) {
		if (!probe_cbs) 
			probe_cbs = (struct scsi_cb *) 
			       ssm_alloc(nbytes, SSM_ALIGN_BASE, SSM_BAD_BOUND);
		icb = (struct scsi_init_cb *)probe_cbs;
	} else {
		ASSERT(sinfo->si_mode == SCB_BOOT, 
		       "ssm_scsi: invalid init mode.\n");
	       /*
		*+ init_ssm_scsi_dev() was called after device probing
		*+ was complete.  A device might have been
		*+ corrupted.
		*/

		icb = (struct scsi_init_cb *)
			ssm_alloc(nbytes, SSM_ALIGN_BASE, SSM_BAD_BOUND);
	}

	/*
	 * Fill in the initialization CB.
	 */
	bzero((char *)icb, SCB_SHSIZ);
	icb->icb_pagesize = CLBYTES;
	icb->icb_sdest = SL_GROUP | TMPOS_GROUP;
	icb->icb_svec = sinfo->si_host_basevec;
	icb->icb_scmd = SL_MINTR | sinfo->si_host_bin;
	icb->icb_scsi = sinfo->si_scsi;
	icb->icb_target = sinfo->si_target;
	icb->icb_lunit = sinfo->si_lunit;
	icb->icb_control = sinfo->si_control;
	icb->icb_flags = sinfo->si_flags;
	icb->icb_cmd = SCB_INITDEV;
	icb->icb_compcode = SCB_BUSY;

#ifdef	SSMDEBUG
	printf("init_ssm_scsi_dev: scsi_init_cb\n");
	ssm_dump(icb, sizeof(struct scsi_init_cb));
#endif	SSMDEBUG

	/* 
	 * Notify the SSM of the initialization CB 
	 * and CB table address.
	 */
	mIntr(sinfo->si_ssm_slic, SCSI_BIN, (unsigned char)
		((sinfo->si_mode == SCB_PROBE)? SCB_PROBEVEC : SCB_BOOTVEC));
	mIntr(sinfo->si_ssm_slic, SCSI_BIN, addr_bytes[0]); /* low byte first */
	mIntr(sinfo->si_ssm_slic, SCSI_BIN, addr_bytes[1]);
	mIntr(sinfo->si_ssm_slic, SCSI_BIN, addr_bytes[2]);
	mIntr(sinfo->si_ssm_slic, SCSI_BIN, addr_bytes[3]); /* high byte last */
	
	while (icb->icb_compcode == SCB_BUSY)	
		;			/* Poll for command completion */

	sinfo->si_cbs = (struct scsi_cb *)icb;	/* CB address to return */
	sinfo->si_id = icb->icb_id;	/* SSM device i.d. to return */
}

/*
 * scb_buf_iovec()
 *	Fill out a CB's indirect address table 
 *	for an I/O request.
 *
 * 	B_RAWIO, B_PTEIO, B_PTBIO cases must flush 
 *	TLB to avoid stale mappings thru Usrptmap[], 
 *	since this is callable from interrupt 
 *	procedures (SGS only).
 *
 * 	Returns vitual address of transfer.
 * 	Panics if bad pte found ("can't" happen).
 */
u_long
scb_buf_iovects(bp, iovstart)
	struct buf *bp;
	u_long *iovstart;
{
	register struct pte *pte;
	register int	count;
	register u_long	*iovp;
	u_long	retval;
	unsigned offset;
	extern struct pte *vtopte();

	/*
	 * Source/target pte's are found differently 
	 * based on type of IO operation.
	 */
	switch(bp->b_iotype) {
	case B_RAWIO:			/* RAW IO */
#ifdef	SSMDEBUG
		printf("scb_buf_iovects: case B_RAWIO\n");
#endif	SSMDEBUG
		/*
		 * In this case, must look into 
		 * alignment of physical memory, 
		 * since we can start on any 
		 * 16-byte boundary.
		 */
		FLUSH_TLB();
		pte = vtopte(bp->b_proc, clbase(btop(bp->b_un.b_addr)));
		count = (((int)bp->b_un.b_addr & CLOFSET) 
			  + bp->b_bcount + CLOFSET) / CLBYTES;
		retval = (u_long)bp->b_un.b_addr;
		break;
	case B_FILIO:			/* file-sys IO */
#ifdef	SSMDEBUG
		printf("scb_buf_iovects: case B_FILIO\n");
#endif	SSMDEBUG
		/*
		 * Filesys/buffer-cache IO.  These 
		 * are always cluster aligned both 
		 * physically and virtually.
		 * Note: also used when to kernel 
		 * memory acquired via wmemall().
		 * For example, channel configuration 
		 * buffer in zdget_chancfg().
		 */
		pte = &Sysmap[btop(bp->b_un.b_addr)];
		count = (bp->b_bcount + CLOFSET) / CLBYTES;
		retval = (u_long)bp->b_un.b_addr;
		break;
	case B_PTEIO:			/* swap/page IO */
#ifdef	SSMDEBUG
		printf("scb_buf_iovects: case B_PTEIO\n");
#endif	SSMDEBUG
		/*
		 * Pte-based IO - already know pte 
		 * of 1st page, which is cluster aligned, 
		 * and b_count is a multiple of CLBYTES.
		 */
		FLUSH_TLB();
		pte = bp->b_un.b_pte;
		count = (bp->b_bcount + CLOFSET) / CLBYTES;
		retval = PTETOPHYS(*pte);
		break;
	case B_PTBIO:			/* Page-Table IO */
#ifdef	SSMDEBUG
		printf("scb_buf_iovects: case B_PTBIO\n");
#endif	SSMDEBUG
		/*
		 * Page-Table IO: like B_PTEIO, but 
		 * can start/end with non-cluster aligned 
		 * memory (but is always HW page aligned). 
		 * Count is multiple of NBPG.
		 *
		 * Separate case for greater efficiency 
		 * in B_PTEIO.
		 */
		FLUSH_TLB();
		pte = bp->b_un.b_pte;
		retval = PTETOPHYS(*pte);
		offset = PTECLOFF(*pte);
		pte -= btop(offset);
		count = (offset + bp->b_bcount + CLOFSET) / CLBYTES;
		break;
	default:
		panic("scb_buf_iovects: bad b_iotype");
		/*
		 *+ The io type of a buffer passed to scb_buf_iovects() is
		 *+ unknown and cannot be processed correctly.
		 */
		/*NOTREACHED*/
	}

#ifdef	SSMDEBUG
	printf("scb_buf_iovects: iovstart = 0x%x, pte = 0x%x, count = 0x%x, retval = 0x%x\n",
		iovstart, pte, count, retval);
#endif	SSMDEBUG
	/*
	 * Now translate PTEs and fill-in iovectors.
	 */
	for (iovp = iovstart; count--; pte += CLSIZE) {
		*iovp++ = PTETOPHYS(*pte);
#ifdef	SSMDEBUG
		printf("scb_buf_iovects: iovp = 0x%x\n", *(iovp - 1));
#endif	SSMDEBUG
	}
	return (retval);
}

#ifdef notyet
/*
 * scb_num_iovects()
 *	Takes a "bp" (struct buf *) and returns	the worst
 *	case number of iovects needed for this transfer.
 *
 * 	Does *no* locking.
 */
scb_num_iovects(bp)
	register struct	buf *bp;	/* buffer header */
{
	return (howmany(bp->b_bcount, CLBYTES) + 1);
}
#endif notyet

#ifdef	SSMDEBUG
ssm_dump(ptr, len)
	int len;
	char * ptr;
{
	int	x;
	
	for (x = 0; x < len; x++) {
		printf("%x ", (u_char) ptr[x]);
		if ((x % 30 == 0) && (x != 0))
			printf("\n");
	}
	printf("\n");
}
#endif	SSMDEBUG



/*
 * ssm_scsi_status()
 * 	Translate the completion code and SCSI
 * 	termination status from an executed
 *	SSM SCSI command block into one of the
 *	generic SCSI command termination codes
 *	defined in 'sys/scsi.h'.  
 * 
 *	If the CB did not terminate with SCB_OK, 
 *	this function determine's the return
 *	code.  Otherwise, invoke a generic SCSI 
 *	function to translate based upon the
 *	SCSI command termination byte and/or 
 *	available sense data.
 */
static int
ssm_scsi_status(cb) 
	struct scsi_cb *cb;
{
	switch(cb->sh.cb_compcode) {
	case SCB_OK:
		return(scsi_status(cb->sh.cb_status, 
			(struct scrsense *)cb->sh.cb_sense));
	case SCB_NO_TARGET:
		return (SSTAT_NOTARGET);
	case SCB_SCSI_ERR:
		return (SSTAT_BUSERR);
	case SCB_BUSY:
		return (SSTAT_BUSYTARGET);
	case SCB_BAD_CB:
		panic("ssm_scsi_status: Invalid CB reported");
		/*
		 *+ The SSM firmware rejected a SCSI command
		 *+ block as being invalid and nonexecutable.
		 *+ The possible cause is defective SSM firmware
		 *+ or memory corruption from an unknown source.
		 */
		/*NOTREACHED*/
	}
		/*NOTREACHED*/
}

int
ssm_scsi_probe_cmd(cmd, cb, sp)
	u_char cmd;
	register struct scsi_cb *cb;
	register struct ssm_probe *sp;

{

	static struct scrsense *sensebuf = NULL;
	static struct scinq *inquiry = NULL;
	
#ifdef DEBUG
	ASSERT(cmd == SCSI_TEST || cmd == SCSI_INQUIRY, 
		"ssm_scsi_probe_cmd: Unexpected SCSI command issued.");
	ASSERT(cb, "ssm_scsi_probe_cmd: Unexpected NULL CB.");
	ASSERT(sp, "ssm_scsi_probe_cmd: Unexpected NULL probe descriptor.");
#endif DEBUG

	/*
	 * Allocate a reusable SCSI request-sense buffer 
	 * for probing, if not already done.
	 */
	if (!sensebuf)
		sensebuf = (struct scrsense *)ssm_alloc(sizeof(struct scrsense),
			SSM_ALIGN_XFER, SSM_BAD_BOUND);
	ASSERT(sensebuf, "ssm_scsi_probe_cmd: Request-sense buffer not allocated.\n");
	/*
	 *+ A request for memory to store request/sense info was not
	 *+ successful due to a lack of system memory.
	 */
		
	bzero((char *)cb, SCB_SHSIZ);
	cb->sh.cb_sense = (u_long) sensebuf;	/* Its virtaddr == physaddr */
	cb->sh.cb_slen =  sizeof(struct scrsense);
	cb->sh.cb_cmd = SCB_READ;
	cb->sh.cb_clen = SCSI_CMD6SZ; 
	cb->sh.cb_scmd[1] = sp->sp_unit << SCSI_LUNSHFT;

	if  ((cb->sh.cb_scmd[0] = cmd) == SCSI_INQUIRY) { 
		/*
		 * Fill in additional CB fields. 
		 *
	 	 * Allocate a reusable SCSI inquiry buffer 
	 	 * for probing, if not already done.
	 	 */
		if (!inquiry)
			inquiry = (struct scinq *) ssm_alloc(
				sizeof(struct scinq), SSM_ALIGN_XFER, 
				SSM_BAD_BOUND);
		ASSERT(inquiry, "ssm_scsi_probe_cmd: Inquiry buffer not allocated.\n");
		/*
	 	 *+ A request for memory to store inquiry info was not
	 	 *+ successful due to a lack of system memory.
	 	 */

		cb->sh.cb_addr = (u_long)inquiry; /* Its virtaddr == physaddr */
		cb->sh.cb_count = sizeof(struct scinq);
		cb->sh.cb_scmd[4] = sizeof(struct scinq);
	}

	/*
	 * Start the SSM processing the CB 
	 * and poll for completion.
	 */
	cb->sh.cb_compcode = SCB_BUSY;
	mIntr(sp->sp_desc->ssm_slicaddr, SCSI_BIN, cb->sw.cb_unit_index);
	while (cb->sh.cb_compcode == SCB_BUSY) 
		continue;
	/*
	 * Translate CB and SCSI termination
	 * status into a generic SCSI command 
	 * termination code, then return it.
	 */
	return (ssm_scsi_status(cb));
}
