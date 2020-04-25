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

#ifdef RCS
static char rcsid[]= "$Header: sec.c 2.7 87/08/05 $";
#endif RCS

/*
 * sec.c - scsi common commands for standalone
 *	mode operation.
 *
 * TODO:
 *	document the return values on errors and the method to 
 *	get the information bytes from this code.
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/reboot.h>
#include <machine/cfg.h>
#include <machine/slic.h>
#include <machine/slicreg.h>
#include "sec.h"
#include "sec_ctl.h"
#include "saio.h"
#include "scsi.h"

#ifdef DEBUG
int	sec_debug =0;
#endif DEBUG

#ifdef BOOTXX
typedef	struct cfg_scsi * c_scsi_t;
typedef struct cfg_ptr  * c_ptr_t;
#else
#ifdef notdef
int	xtend_sense;
#endif notdef
#endif

/*
 * binary config candidates...
 */
#define SECBUFSZ	32	/* Must be > 8 */
struct sec_req_sense	rq_ss;
static u_char rq_sensebuf[SECBUFSZ];
int	scsi_info_bytes;

u_char	*SEC_rqinit();

#if (SECBUFSZ<=24)
static	struct sec_iat	SEC_IATS[18];
#else	/* Must be < 256 for h/w */
static	struct sec_iat	SEC_IATS[19+(SECBUFSZ-24)/2];
#endif

static	u_char	*SEC_iat;

/*
 * SEC_set_dev - setup a struct sec_pup_info for a device
 * based off of the powerup system state.
 *
 * ASSUMES - can only be called by standalone programs while
 *		the system is at an initialized state. This
 *		means that only one processor can be operating
 *	i.e. NO TMP assumed and NO Mutual exclusion preformed.
 */
SEC_set_dev(boardno, devno, info)
	register	int	boardno, devno;
	register struct sec_pup_info *info;
{
#ifdef BOOTXX
	register struct cfg_scsi *cfg;
	
	cfg = &((c_scsi_t)((c_ptr_t)CFG_PTR)->head_cfg->b_cfg_scsi)[boardno];

	if(devno<SDEV_SCSISTART || devno>SDEV_SCSIEND 
	|| (boardno >= ((c_ptr_t)CFG_PTR)->head_cfg->b_num_scsi)) {
		printf("sec: Bad channel number %d\n", boardno);
		return(-1);
	}
#else BOOTXX
	register struct config_desc *cd = CD_LOC;
	register struct ctlr_desc *cp;

	if (devno < SDEV_SCSISTART || devno > SDEV_SCSIEND
	  || boardno >= cd->c_toc[SLB_SCSIBOARD].ct_count) {
		printf("sec: bad channel number %d\n", boardno);
		return -1;
	}
	cp = &cd->c_ctlrs[cd->c_toc[SLB_SCSIBOARD].ct_start + boardno];
#endif BOOTXX

#ifndef BOOTXX
	if ((cp->cd_diag_flag & (CFG_FAIL|CFG_DECONF))) {
		printf("sec: CFG_FAIL|CFG_DECONF flag set\n");
		return -1;
	}

	if (SEC_iat == 0)
		SEC_iat = SEC_rqinit(rq_sensebuf, SECBUFSZ, SEC_IATS);
#endif	BOOTXX

	info->sec_pup_status = 0;
#ifdef BOOTXX
	info->sec_pup_desc.sec_powerup = (struct sec_powerup *)cfg->sc_init_queue;
	info->sec_pup_q = info->sec_pup_desc.sec_powerup;
	info->sec_pup_desc.sec_slicaddr = cfg->sc_slic_addr;
	info->sec_pup_desc.sec_target_no = cfg->sc_host_num;
	info->sec_target = devno;
	info->sec_pup_desc.sec_is_cons = cfg->sc_is_console;
	info->sec_pup_desc.sec_version = cfg->sc_version;
	bcopy(cfg->sc_enet_addr, info->sec_pup_desc.sec_ether_addr, 6);
#else BOOTXX
	info->sec_pup_desc.sec_powerup = 
		(struct sec_powerup *)cp->cd_sc_init_queue;
	info->sec_pup_q = info->sec_pup_desc.sec_powerup;
	info->sec_pup_desc.sec_slicaddr = cp->cd_slic;
	info->sec_pup_desc.sec_target_no = cp->cd_sc_host_num;
	info->sec_target = devno;
	info->sec_pup_desc.sec_is_cons = cp->cd_sc_cons;
	info->sec_pup_desc.sec_version = cp->cd_sc_version;
	bcopy(cp->cd_sc_enet_addr, info->sec_pup_desc.sec_ether_addr, 6);
#endif BOOTXX
	return(0);
}
/*
 * SEC_cmd - execute an scsi command in polled mode.
 *
 * Doesn't handle restarting instructions.
 *
 * Assumes single processor running in a non-tmp needed mode.
 */
#define SEC_ANYBIN	4	/* Bin to interrupt sec on */
SEC_cmd(scsilen, data_addr, data_count, info, timeout)
	register int timeout;
	register struct	sec_pup_info *info;
	u_char	*data_addr;
	int	data_count, scsilen;
{
	register struct sec_powerup	*iq = info->sec_pup_q;
	register struct	sec_dev_prog	*dp = info->sec_pup_dev_prog;
	register head;
	int	timer;
	int	 i;
	extern int	cpuspeed;

	dp->dp_un.dp_data = data_addr;
	dp->dp_next = 0;
	dp->dp_count = 0;			/* Ether data count field */
	dp->dp_data_len = data_count;
	dp->dp_cmd_len = scsilen;
	dp->dp_status1 = 0;
	dp->dp_status2 = 0;

	head = iq->pu_requestq.pq_head;
#if !defined(BOOTXX)
	if(head != iq->pu_requestq.pq_tail)
		printf("SCED: Head mismatch in input Q h=0x%x t=0x%x\n", 
			head, iq->pu_requestq.pq_tail);
#endif
	
	scsi_info_bytes = 0;
#if !defined(BOOTXX)
	bzero(rq_sensebuf, SECBUFSZ);	/* Insure zero status */
#endif
	iq->pu_requestq.pq_un.pq_progs[head] = dp;
	iq->pu_requestq.pq_head = (head + 1) % SEC_POWERUP_QUEUE_SIZE;
	info->sec_pup_status = 0;
	timer = timeout;

	SEC_startio(	SINST_STARTIO,
			&info->sec_pup_status,
			SEC_ANYBIN,
			info->sec_target,
			&iq->pu_cib,
			info->sec_pup_desc.sec_slicaddr
	);

	if(info->sec_pup_status == SINST_INSDONE) {
		if(timer == -1) {	/* Do no timeouts, which can hang */
			while(iq->pu_doneq.pq_tail == iq->pu_doneq.pq_head)
				continue;	
		}else{
			timer *= cpuspeed;
			while( (iq->pu_doneq.pq_tail == iq->pu_doneq.pq_head) 
			&& timer--)
				continue;	
			if(timer<=0) {
				printf("SEC_cmd: Startio: Async timeout\n"); 
				iq->pu_requestq.pq_tail = iq->pu_requestq.pq_head = 0;
				return(dp->dp_status1 = SEC_HARDERR);
			}
		}
		iq->pu_doneq.pq_tail = iq->pu_doneq.pq_head;
	}else{
		printf("sec: Timeout|no-device status=0x%x\n",info->sec_pup_status);
		iq->pu_requestq.pq_tail = iq->pu_requestq.pq_head = head;
		return(SEC_HARDERR);
	}

	if(dp->dp_status1 == SSENSE_CHECK) { 
#if !defined(BOOTXX)
		/*
		 * Hack for the request sense stuff 
		 */
		rq_ss.rs_dev_prog.dp_un.dp_data = SEC_iat;
		rq_ss.rs_dev_prog.dp_next = 0;
		rq_ss.rs_dev_prog.dp_count = 0;			/* Ether data count field */
		rq_ss.rs_dev_prog.dp_data_len = SECBUFSZ;
		rq_ss.rs_dev_prog.dp_cmd_len = 6;
		rq_ss.rs_dev_prog.dp_status1 = 0;
		rq_ss.rs_dev_prog.dp_status2 = 0;
		rq_ss.rs_dev_prog.dp_cmd[0] = 3;			/* request sense cmd hack */
		rq_ss.rs_dev_prog.dp_cmd[1] = dp->dp_cmd[1] & 0xe0;	/* mask lun */
		rq_ss.rs_dev_prog.dp_cmd[4] = SECBUFSZ;		/* > 14 */

		SEC_startio(	SINST_REQUESTSENSE,
				&rq_ss,
				SEC_ANYBIN,
				info->sec_target,
				&iq->pu_cib,
				info->sec_pup_desc.sec_slicaddr
		);

		if(rq_ss.rs_status == SINST_INSDONE) {
			while(iq->pu_doneq.pq_tail == iq->pu_doneq.pq_head)
				continue;	
			scsi_info_bytes = (int)(*(u_char *)(&rq_sensebuf[3])<<24);
			scsi_info_bytes += (int)(*(u_char *)(&rq_sensebuf[4])<<16);
			scsi_info_bytes += (int)(*(u_char *)(&rq_sensebuf[5])<<8);
			scsi_info_bytes += (int)*(u_char *)(&rq_sensebuf[6]);
#ifdef DEBUG
			printf("Resid=%d 0x%x\n", scsi_info_bytes, scsi_info_bytes);
			printf("Request Sense bytes:");
			for(i=0; i<SECBUFSZ; i++)
				printf(" 0x%x", (unsigned char) rq_sensebuf[i]);
			printf("\n");
#endif DEBUG
		}else{
			printf("sec: NO STATUS AVAILABLE ERROR\n");
		}
		iq->pu_doneq.pq_tail = iq->pu_doneq.pq_head;
#ifdef DEBUG
		printf("Error'd SCSI command block:");
		for(i=0; i<scsilen; i++)
			printf(" 0x%x", (unsigned char) dp->dp_cmd[i]);
		printf("\n");
#endif DEBUG
#endif
		dp->dp_status1 = 0;
		info->sec_pup_status = 0;
		SEC_startio(	SINST_RESTARTIO,
				&info->sec_pup_status,
				SEC_ANYBIN,
				info->sec_target,
				&iq->pu_cib,
				info->sec_pup_desc.sec_slicaddr
		);
		iq->pu_doneq.pq_tail = iq->pu_doneq.pq_head;
#if !defined(BOOTXX)
		if(iq->pu_requestq.pq_head != iq->pu_requestq.pq_tail)
		    printf("SCED: RESHEAD mismatch in input Q h=0x%x t=0x%x\n",
			iq->pu_requestq.pq_head, iq->pu_requestq.pq_tail);
		if((rq_sensebuf[0] & 0x70) == 0x70) {
#ifdef notdef
			xtend_sense = 1;
#endif notdef
			return((int)rq_sensebuf[2]);	/* drivers sense key status */
		} else {
#ifdef notdef
			xtend_sense = 0;
#endif notdef
			return((int)rq_sensebuf[0]);	/* standard sense key */
		}
#else
		return(dp->dp_status1);		/* need some status to return */
#endif
	} else {
		if(dp->dp_status1 != SSENSE_NOSENSE) { 
			dp->dp_status1 = 0;
			info->sec_pup_status = 0;
			SEC_startio(	SINST_RESTARTIO,
					&info->sec_pup_status,
					SEC_ANYBIN,
					info->sec_target,
					&iq->pu_cib,
					info->sec_pup_desc.sec_slicaddr
			);
		}
	}
	iq->pu_doneq.pq_tail = iq->pu_doneq.pq_head;
	return((int)(dp->dp_status1));
}

#if !defined(BOOTXX)
/*
 * SEC_print_sense()	- print out the current request sense information that
 * lives in the buffer.
 */
SEC_print_sense()
{
	int	i;

	printf("Request sense bytes:");
	for(i=0; i<SECBUFSZ; i++) {
		printf(" %x", rq_sensebuf[i]);
		if(((rq_sensebuf[0]&0x70) != 0x70) && (i==3))
			break;		/* standard sense data is out */
	}
	printf("\n");
}
#endif

/*
 * SEC_startio - start an operation by sending a command to the
 *		sec board via slic.
 *
 * calls mIntr() to do the actual slic fussing to send the message.
 *
 * NOTE: It's critical that the status pointer live below 0x400000
 * because the sec can't talk above that address, hence all status
 * variable must *not* live on kernel stack.
 */

#define	DELAY_TIME	50000

SEC_startio(cmd, statptr, bin, mesg, q, slicid)
	register int *statptr;
	int	 cmd;
	unsigned char	bin,	mesg, slicid;
	register struct sec_cib	*q;
{
	register int	spin;
	extern	int	cpuspeed;
	
	*statptr = 0;
	q->cib_inst = cmd;
	q->cib_status = statptr;

	mIntr(slicid, bin, mesg);
	spin = cpuspeed * DELAY_TIME;
	while ((*statptr & SINST_INSDONE) == 0) {
		if (spin-- <= 0) {
			printf("SEC_startio: timeout\n");
			break;
		}
	}
}


#if !defined(BOOTXX)
/* 
 * u_char *
 * SEC_rqinit(rqbuf, rqbufsz)
 *	u_char *rqbuf;
 *
 * SEC_rqinit - init a request sense iat chain to
 *	allow the SEC dma hardware to transfer complete
 *	request sense data buffers which are *not* 8 byte
 *	aligned. This procedure returns an SEC iat pointer
 *	that is used to point at a request sense buffer
 *	(ie. device_prog.dp_datap = rqbuf can be replaced with
 *	device_prog.dp_datap = returned pointer). Max size is 255.
 *
 *	NOTE: the larger the buffer size the more memory
 *		this eats! 
 *	Memory eatten =
 *		sz<8	16 bytes.
 *		sz<24	16+((sz-8)*8) 		; max 144 bytes
 *		sz>24	144+((sz-24)/2)*8+8	; max 1072 bytes (for 255 buf)
 *
 *	This hardware limititation is bye-passed by
 *	creating a chain of iat's that point to the
 *	(passed in) buffer pointer in the following
 *	structure respectively:
 *	#iat's_used	length	bytes covered
 *		1	4	0-3	(standard)
 *		1	4	4-7	(extended)
 *		16	1	8-23	(additional)
 *		sz-24/2	2	24-sz	(additional)
 *	
 *	Note: this must be a generic buffer to handle
 *	target adapter interchange since the number of
 *	returned bytes from the target adapter will vary 
 *	based on vendor, error type and buffer size.
 *	Maximum buffer size allowed is 255.
 *	Assumes iat passed in can hold the total iat's.
 *	For target adapters that transfer greater than 24 bytes
 *	it's posible to loose the last byte on an odd transfer.
 *	Returns type char * to please lint with straight replacement
 *	in existing drivers. Enforces a minimum of eight bytes to
 *	ease the iat fill out process.
 *
 *	This conforms to ansi X3T9.2/82-2 revision 14 specification.
 *
 */
u_char	*
SEC_rqinit(rqbuf, rqbufsize, iat)
	register u_char	*rqbuf;
	register int	rqbufsize;
	register struct	sec_iat	*iat;		/* iat descriptor list ptr */
{
	register u_char	*ret_iatptr;
	register int	count;
	int	 	iat_count;
#ifdef DEBUG
	int		dcnt;
	u_char		*drqbuf = rqbuf;
#endif DEBUG

	/*
	 * calculate the iat chain size.
	 */
	rqbufsize = MAX(rqbufsize, 8);		/* enforce minimum of eight */
	count = rqbufsize-8;
	iat_count = 2;				/* first two */
	if(count>0) {
		iat_count += (count-16 > 0) ? 16 : count;	/* bytes 8-23 */
		count  -= 16;
	}
	if(count>0) {
		iat_count += count/2 + count%2;	/* bytes 24-rqbufsize */
		count	= 0;			/* for sanity */
	}
#ifdef DEBUG
	if (sec_debug>1)
		printf("SEC_rqinit: iat's needed=%d\n", iat_count);
	dcnt = iat_count;			/* debug */
#endif DEBUG
	
	/*
	 * allocate space for the iat's
	 * and save the callers reference pointer.
	 */
	ret_iatptr = (u_char *)SEC_IATIFY(iat);

	/*
	 * fill out iat chain.
	 */
	for(count=0; count<2; count++, iat++, rqbuf = (u_char *)((int)rqbuf+4)) {	/* assumes 2 */
		iat->iat_count = 4;
		iat->iat_data = rqbuf;
	}
	iat_count -=2;

	/*
	 * bytes 8-23
	 */
	for(count=0; count<16 && iat_count>0; count++, iat++, iat_count--, rqbuf++) {
		iat->iat_count = 1;
		iat->iat_data = rqbuf;
	}
	/*
	 * bytes 24-rqbufsize
	 */
	for(; iat_count>0; iat++, iat_count--, rqbuf++, rqbuf++) {
		iat->iat_count = 2;
		iat->iat_data = rqbuf;
	}

	/*
	 * Adjust the last iat if the count was odd, data ptr is ok.
	 */
	if(rqbufsize&1) {
		iat--;
		iat->iat_count = 1;
	}
#ifdef DEBUG
	if(sec_debug) {
		for(iat=(struct sec_iat *)((u_int)ret_iatptr & ~SEC_IAT_FLAG); dcnt; dcnt--, iat++)
			printf("buf 0x%x iat 0x%x count %d retiat 0x%x\n",
				drqbuf, iat->iat_data, iat->iat_count, ret_iatptr);
	}
#endif DEBUG
			
	return(ret_iatptr);
}
#endif BOOTXX
