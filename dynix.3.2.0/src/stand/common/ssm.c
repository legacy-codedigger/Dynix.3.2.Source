/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
*/

#ident	"$Header: ssm.c 1.4 1991/04/23 21:29:49 $"

/*
 * ssm.c
 *	Suport routines for SSM standalone device drivers.
 */

/* $Log: ssm.c,v $
 *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/reboot.h>
#include <machine/slic.h>
#include <machine/slicreg.h>
#include "ssm.h"
#include "scsi.h"
#include "ssm_scsi.h"
#include <machine/cfg.h>

#ifndef	SSMFW
#include "ssm_cons.h"
#include "ssm_misc.h"
#endif	/* SSMFW */


extern char *calloc();
#ifdef	SSMFW
extern	void allocCb();
#endif	/* SSMFW */

/*
 * ssm_get_devinfo()
 * 	Validates the description for the SCSI device 
 *	and fills in the info structure for it.  
 *
 *	A set of SCSI CBs and request/sense buffer are
 *	shared by all devices since device operations
 *	are atomic, it saves memory, and the SSM has a 
 * 	limited number of device identifiers.  The
 *	SSM device identifier is recycled when the SSM 
 *	notes the SLIC vector its given before the address
 *	of the initialization CB, which must occur during
 *	each successful invocation of this function.  
 *
 *	Returns 1 for success, 0 for failure.
 */
ssm_get_devinfo(boardno, devno, info, control_byte) 
	int boardno;
	register int devno;
	register struct ssm_sinfo *info;
	u_char control_byte;
{
	struct scsi_init_cb *icb;
	unsigned char *addr_bytes = (unsigned char *)&icb;
	struct ctlr_desc *cp;
	static struct scsi_cb *scsi_cbs; 	/* Recycled SCSI CBs */
	static char *scsi_sensebuf;		/* Recycled SCSI rsense buf */
#ifndef	SSMFW
	struct config_desc *cd = CD_LOC;
	int count, ssm_cnt, ssm2_cnt;
	/*
	 * get counts of SSM and SSM2 and total
	 */
	ssm_cnt = cd->c_toc[SLB_SSMBOARD].ct_count;
	ssm2_cnt = cd->c_toc[SLB_SSM2BOARD].ct_count;
	count = ssm_cnt + ssm2_cnt;
#endif SSMFW

	/* 
	 * Validate range of devno and boardno.
	 */
	 if (devno != SCSI_DEVNO(devno)) {
		printf("ssm: Bad SCSI address number %d\n", devno);
		return (0);
#ifndef	SSMFW
	} else if (boardno >= count) {
		printf("ssm: Bad board number %d\n", boardno);
		return (0);
	} else {
		if (boardno < ssm_cnt) {
			cp = cd->c_ctlrs + (cd->c_toc[SLB_SSMBOARD].ct_start +
					    boardno);
		} else {
			cp = cd->c_ctlrs + (cd->c_toc[SLB_SSM2BOARD].ct_start +
					    boardno - ssm_cnt);
		}
		if (cp->cd_diag_flag & (CFG_FAIL | CFG_DECONF)) {
			printf("ssm: Board failed diagnostics or ");
			printf("was deconfigured %d\n", boardno);
			return(0);
		}
#endif	/* SSMFW */
	}

	/*
	 * If the one-time allocation of SCSI CBs and
	 * request/sense buffer has not occurred
	 * then allocate them.  The buffer address
	 * must be stored in the first CB later
	 * since the initialization CB would clobber
	 * its value.  Only one buffer is allocated
 	 * which is later assigned to the first CB,
 	 * since it is the only one that should
	 * be used in the standalone environment.
	 */
	if (!scsi_cbs) {
		scsi_cbs = (struct scsi_cb *) 
		       ssm_alloc((sizeof(struct scsi_cb) << NCBSCSISHFT),
				SSM_ALIGN_XFER, SSM_BAD_BOUND);
		if (!scsi_cbs) {
			printf("ssm: Out of memeory for SCSI CBs.\n");
			return (0);
		}
		scsi_sensebuf = (char *)
			ssm_alloc(SCB_RSENSE_SZ, SSM_ALIGN_XFER, SSM_BAD_BOUND);
		if (!scsi_sensebuf) {
			printf("ssm: Out of memeory for SCSI sense buffer.\n");
			return (0);
		}
	} 

	/* 
	 * Fill in the initialization CB.
	 */
	icb = (struct scsi_init_cb *)scsi_cbs;
	bzero((char *)icb, sizeof(struct scsi_cb));
	icb->icb_pagesize = CLBYTES;
	icb->icb_scsi = 0;			/* Could change in the future */
	icb->icb_target = SCSI_TARGET(devno);
	icb->icb_lunit = SCSI_UNIT(devno);
	icb->icb_control = control_byte;
	icb->icb_cmd = SCB_INITDEV;
	icb->icb_compcode = SCB_BUSY;

	/* 
	 * Notify the SSM of the initialization CB 
	 * and device's CB table address.
	 */
#ifndef	SSMFW
	mIntr(cp->cd_slic, SCSI_BIN, (unsigned char) SCB_PROBEVEC);
	mIntr(cp->cd_slic, SCSI_BIN, addr_bytes[0]); /* low byte first */
	mIntr(cp->cd_slic, SCSI_BIN, addr_bytes[1]);
	mIntr(cp->cd_slic, SCSI_BIN, addr_bytes[2]);
	mIntr(cp->cd_slic, SCSI_BIN, addr_bytes[3]); /* high byte last */
	
#else	/* SSMFW */
	allocCb(SCB_PROBEVEC,1, icb);
#endif	/* SSMFW */
	while (icb->icb_compcode == SCB_BUSY)	
		continue;		/* Poll for command completion */

	if (icb->icb_id < 0) {
		printf("ssm: Invalid device i.d. returned by SSM;");
		printf(" an initialization error occurred.\n");
		return (0);
	}

	/*
	 * Now save request sense information
	 * since that would have been clobbered
	 * while filling out the initialization CB.
	 */
	scsi_cbs->sh.cb_sense = (u_long) scsi_sensebuf;
	scsi_cbs->sh.cb_slen = SCB_RSENSE_SZ;

	/* 
	 * Fill in the info structure passed to this function.
	 */
	info->si_cb = scsi_cbs;		/* Device CB address to return */
	info->si_id = icb->icb_id;	/* SSM device i.d. to return */
	info->si_unit = (u_char)devno;
#ifndef	SSMFW
	info->si_slic = cp->cd_slic;	/* SLIC address of its SSM */
	info->si_version = ((u_long)cp->cd_ssm_version[0]) << 16;
	info->si_version |= ((u_long)cp->cd_ssm_version[1]) << 8;
	info->si_version |= (u_long)cp->cd_ssm_version[2];
#endif	/* SSMFW */
	return (1);
}

/*
 * ssm_print_sense()
 *	Print out the current request sense information 
 *	that lives in the buffer.
 */
ssm_print_sense(cb)
	struct scsi_cb *cb;
{
	int i;
	u_char *cp = (u_char *)cb->sh.cb_sense;
	struct scrsense *rs = (struct scrsense *) cb->sh.cb_sense;

	/* 
	 * For extended error codes print as much data 
	 * as is available.  
	 * Otherwise, print only the standard data.
	 */
	i = ((rs->rs_error & RS_CLASS_EXTEND) == RS_CLASS_EXTEND) 
		? cb->sh.cb_slen : 8; 

	printf("Request sense bytes:");
	while (i--) 
		printf(" %x", *cp++);
	printf("\n");
}

/*
 * ssm_alloc() 
 *	Allocate a properly-aligned chunk of memory 
 *	that does not cross the specified boundary.
 *
 * 	'nbytes' is the number of bytes to allocate.
 * 	'align' is the byte multiple at which the 
 *	memory is to be aligned (e.g. 2 means align 
 *	to two-byte boundary).  'badbound' is a 
 *	boundary which cannot be crossed (usually one 
 *	megabyte for the SSM); it must be a power of 
 *	two and a multiple of 'align'.
 */
char *
ssm_alloc(nbytes, align, badbound)
	unsigned nbytes, align, badbound;
{
	long addr;
	caddr_t	calloc();

	callocrnd((int)align);
	addr = (long)calloc(0);
	if ((addr & ~(badbound - 1)) != (addr + nbytes & ~(badbound - 1))) {
		/*
		 * It would have crossed a 'badbound' boundary,
		 * so bump past this boundary.
		 */
		callocrnd((int)badbound);
	}
	return (calloc((int)nbytes));
}

#ifndef SSMFW

static struct ssm_misc SSM_misc;  /* Used by message passing functions */

/*
 * ssm_send_misc_addr() 
 *	Send address of ssm_misc structure to
 *	the SSM firmware.
 */
static void
ssm_send_misc_addr()
{
	struct ssm_misc *addr;
	u_char dest;
	register u_char *addr_bytes;
	static int misc_addr_sent = 0;

	if (misc_addr_sent)
		return;		/* Short circuit multiple calls */

	addr = &SSM_misc;
	dest = CD_LOC->c_cons->cd_slic;
	addr_bytes = (u_char *)&addr;

	mIntr(dest, SM_BIN, SM_ADDRVEC);
	mIntr(dest, SM_BIN, addr_bytes[0]);	/* low byte first */
	mIntr(dest, SM_BIN, addr_bytes[1]);
	mIntr(dest, SM_BIN, addr_bytes[2]);
	mIntr(dest, SM_BIN, addr_bytes[3]);	/* high byte last */
	misc_addr_sent = 1;
}

/*
 * ssm_gen_cmd()
 *	Send a generic command to the SSM
 *
 * 	'dest' is the SLIC id of the SSM (for when 
 *	there are multiple SSM's in a system).
 *	'who' is who on the SSM gets the message.
 * 	'cmd' is the command to send.
 * 	Assumes that the rest of the 'ssm_mc' has 
 *	been filled in.
 */
ssm_gen_cmd(ssm_mc, dest, who, cmd)
	struct ssm_misc *ssm_mc;
	u_char dest;
	int who, cmd;
{
	/* build command to ssm */
	ssm_mc->sm_who = who;
	ssm_mc->sm_cmd = cmd;
	ssm_mc->sm_stat = SM_BUSY;

	/* send it and wait for completion */
	mIntr(dest, SM_BIN, (u_char)who);
	while (ssm_mc->sm_stat == SM_BUSY)
		;
}

/*
 * ssm_get_fpst()
 *	Returns a bit-vector defining the 
 *	front-panel state.
 */
u_long
ssm_get_fpst()
{	
	register struct ssm_misc *ssm_mc = &SSM_misc;

	ssm_send_misc_addr();
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, CD_LOC->c_cons->cd_slic, SM_FP, SM_GET);
	return(ssm_mc->sm_fpst);
}

/*
 * ssm_reboot()
 *	Reboot with these flags and string.
 *
 * 	'flags' is the boot flags to reboot with.
 * 	'size' is the number of bytes in 'str'.
 * 	'str' is a character buffer with the boot string.
 */
static void
ssm_reboot(flags, size, str)
	uint size;
	ushort	flags;
	char *str;
{
	register struct ssm_misc *ssm_mc = &SSM_misc;

	ssm_send_misc_addr();
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_boot.boot_flags = flags;
	if (size)
		bcopy(str, ssm_mc->sm_boot.boot_str,
	      		(size > BNAMESIZ)? BNAMESIZ: size);
	ssm_gen_cmd(ssm_mc, CD_LOC->c_cons->cd_slic, SM_BOOT, SM_REBOOT);
}

/*
 * SSM board dependent return to firmware.
 */
void
ssm_rtofw()
{
	ssm_reboot(RB_HALT, 0, (char *)NULL);	
	for (;;)
		/* spin till shut down by firmware */;
}

/*
 * Fumctions/data structures for console i/o using the SSM.
 */

extern int _slscsi;			/* Slic addr for putchar's 
					 * defined in putchar.c */
static int _cbinit = 1;
static int cb_sent = 0;
static struct cons_cb cb[NCONSDEV*NCBPERCONS];

/* 
 * init_ssm_cons - Notify SSM about location of console CBs.
 *
 * Notify the SSM by sending the address of the console
 * CBs a byte at a time to it over the slic.  
 * Return the base CB address to the caller.
 * Assumes that mIntr() retries messages until they
 * succeed, that c_cons is pointing at a SSM controller 
 * descriptor, and that console CBs are 4-byte aligned.
 */
static
init_ssm_cons()
{
	register struct config_desc *cd = CD_LOC;
	u_char slic = cd->c_cons->cd_slic;
	uint addr = (uint)cb;

 	/* Notify the SSM of the CB'a location. */
	mIntr(slic, CONS_BIN, CONS_ADDRVEC);
	mIntr(slic, CONS_BIN, (u_char)(addr >>0));/* low byte first */
	mIntr(slic, CONS_BIN, (u_char)(addr >>8));
	mIntr(slic, CONS_BIN, (u_char)(addr >>16));
	mIntr(slic, CONS_BIN, (u_char)(addr >>24));/* high byte last */
}

#define CONS_BUFF_SIZE 32
static char input_buff[CONS_BUFF_SIZE];
static ushort buff_index;

int
ssm_getchar(wait)
	int wait;
{
	struct config_desc *cd = CD_LOC;
	u_char slic = cd->c_cons->cd_slic;
	int unit;
	static struct cons_rcb *rcb;

	if (_cbinit) {
		_cbinit = 0;
		init_ssm_cons();
	}

    	do {
		if (!cb_sent) {
			/* Determine which port is currently the console */
			unit = (ssm_get_fpst() & FPST_LOCAL) ? 
				CCB_LOCAL : CCB_REMOTE;
			rcb = (struct cons_rcb *)
				(CONS_BASE_CB(cb, unit) + CCB_RECV_CB);

			/* build and send console a request for data */
			rcb->rcb_addr = (u_long)input_buff;
			rcb->rcb_timeo = 30;	/* wait 30 msec */
			rcb->rcb_count = CONS_BUFF_SIZE;
			rcb->rcb_cmd = CCB_RECV;
			rcb->rcb_status = CCB_BUSY;	/* Transfer status */
			mIntr(slic, CONS_BIN, COVEC(unit,CCB_RECV_CB));
			cb_sent++;
			buff_index = 0;
		}
	
		if (rcb->rcb_status != CCB_BUSY) {
			if (&input_buff[buff_index] < (char *)rcb->rcb_addr)
				return((int)input_buff[buff_index++] & 0xff);
			else
				cb_sent = 0;	/* Need to fetch more data */
		}
    	} while (wait);
	return(-1);			/* Can't wait */
}

ssm_putchar(c)
	int c;
{
	if (c == '\n') ssm_putchar('\r');
	mIntr(_slscsi, 1, c);
}

#endif /* SSMFW */
