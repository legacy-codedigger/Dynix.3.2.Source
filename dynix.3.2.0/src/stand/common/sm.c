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

#ifdef RCS
static char rcsid[]= "$Header: sm.c 1.2 1991/06/03 19:57:15 $";
#endif RCS

/*
 * SCSI/SSM memory device driver (stand alone).
 *
 */

#include <sys/types.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>
#include <machine/slic.h>
#include <sys/param.h>
#include "sec.h"
#include <ssm/ssm_misc.h>"
#include <sec/sm.h>


#define SEC_ANYBIN	4	/* Bin to interrupt sec on */
#if !defined(BOOTXX)

/*
 * Data structures for Sced and SSM.
 */
static struct sec_dev_prog	sm_dp;
static struct ssm_misc		ssm_misc;

static void ssm_send_addr();

/*ARGSUSED*/
int
smgetlog(buffer)
	caddr_t buffer;
{

	register struct sec_powerup	*iq;
	register struct ssm_misc	*ssm_mc;
	struct sec_gmode tio;
	struct sec_mem	sm;
	struct ssm_log	ssm_log;
	int slic;
	int offset;
	int nchar;
	int len;

	slic = CD_LOC->c_cons->cd_slic;

	if (CD_LOC->c_cons->cd_type == SLB_SCSIBOARD) {
		/*
		 *  Sced based SCSI memory.
		 */
		iq = (struct sec_powerup *) CD_LOC->c_cons->cd_sc_init_queue;
		tio.gm_status = 0;
#ifdef DEBUG
		printf("SEC_startio(0x%x, 0x%x, %d, 0x%x, 0x%x, %d)\n", 
			SINST_GETMODE,
			&tio, SEC_ANYBIN, SDEV_MEM, &iq->pu_cib, slic);
#endif
		SEC_startio(SINST_GETMODE, (int *)&tio, SEC_ANYBIN, 
			SDEV_MEM, &iq->pu_cib, slic);
		if (tio.gm_status != SINST_INSDONE) {
			printf("smem:Get Log Modes Sync. Failure: %x\n",
						tio.gm_status);
			return (-1);
		} 
		sm = *(struct sec_mem *)&tio.gm_un.gm_mem;

		/*
		 * Now get the data into buffer.
		 * unwind the circular buffer.
		 */

		offset = (int)sm.mm_nextchar;
		len  = sm.mm_size - (offset - (int)sm.mm_buffer);
		nchar = len;

		if (SEC_do_cmd(slic, &tio, &sm_dp, iq, offset, len, buffer))
			return(0);

		buffer += len;
		offset = (int)sm.mm_buffer;
		len  = (int)sm.mm_nextchar - offset;

		if (SEC_do_cmd(slic, &tio, &sm_dp, iq, offset, len, buffer))
			return(nchar);
		nchar += len;
	} else {
		/*
		 * Read the SSM's console log.
		 */
		ssm_mc = &ssm_misc;
		ssm_send_addr(slic,ssm_mc);
		ssm_gen_cmd(ssm_mc, slic, SM_LOG, SM_GET);
		ssm_log = ssm_misc.sm_log;
		/*
		 * Now get the data into buffer.
		 * unwind the circular buffer.
		 */

		offset = (int)ssm_log.log_next;
		len    = ssm_log.log_size - (offset - (int)ssm_log.log_buf);

		ssm_mc->sm_ram.ram_buf = buffer;
		ssm_mc->sm_ram.ram_loc = (char *)offset;
		ssm_mc->sm_ram.ram_size = len;
		nchar = len;
		ssm_gen_cmd(ssm_mc, slic, SM_RAM, SM_GET);
	
		buffer += len;
		offset = (int)ssm_log.log_buf;
		len    = (int)ssm_log.log_next - offset;

		ssm_mc->sm_ram.ram_buf = buffer;
		ssm_mc->sm_ram.ram_loc = (char *)offset;
		ssm_mc->sm_ram.ram_size = len;
		nchar += len;
		ssm_gen_cmd(ssm_mc, slic, SM_RAM, SM_GET);
	}
	/*
	 * return an index into buffer.
	 */
	return (nchar);
}


static
SEC_do_cmd(slic, tio, dp, iq, offset, len, buffer)
	int slic;
	register struct sec_gmode 	*tio;
	register struct sec_dev_prog	*dp;
	register struct sec_powerup	*iq;
	int offset;
	int len;
	unsigned char *buffer;
{
	int head;
	register struct sec_progq	*diq;

	dp->dp_next = 0;
	dp->dp_un.dp_data = (u_char *)buffer;
	dp->dp_cmd_len = 10;
	dp->dp_data_len = len;
	dp->dp_cmd[0] = SMC_READ;
	dp->dp_cmd[1] = 0;
	dp->dp_cmd[2] = 0;
	dp->dp_cmd[3] = len >> 8;
	dp->dp_cmd[4] = len & 0xff;
	dp->dp_cmd[5] = 0;
	dp->dp_cmd[6] = offset >> 16;
	dp->dp_cmd[7] = offset >> 8;
	dp->dp_cmd[8] = offset & 0xff;
	dp->dp_cmd[9] = 0;

	diq = &iq->pu_requestq;
	head = diq->pq_head;
	diq->pq_un.pq_progs[head] = dp;
	diq->pq_head = (head+1)% SEC_POWERUP_QUEUE_SIZE;

	/*
	 * now do the read.
	 */
	tio->gm_status = 0;
#ifdef DEBUG
	printf("SEC_startio(0x%x, 0x%x, %d, 0x%x, 0x%x, %d)\n", 
		SINST_STARTIO,
		tio, SEC_ANYBIN, SDEV_MEM, &iq->pu_cib, slic);
#endif
	SEC_startio(SINST_STARTIO, (int *)tio, SEC_ANYBIN, 
		SDEV_MEM, &iq->pu_cib, slic);
	if (tio->gm_status != SINST_INSDONE) {
		printf("smem:read SCSI memory failed: %x\n", tio->gm_status);
		iq->pu_doneq.pq_tail = iq->pu_doneq.pq_head = head;
		return (-1);
	} 
	while(iq->pu_doneq.pq_tail == iq->pu_doneq.pq_head)
		continue;	
	iq->pu_doneq.pq_tail = iq->pu_doneq.pq_head;
	return (0);
}

/*
 * ssm_send_addr() 
 *	Send address of ssm_misc structure to
 *	the SSM firmware.
 */
static void
ssm_send_addr(dest,misc_ptr)
	u_char dest;
	struct ssm_misc *misc_ptr;
{
	struct ssm_misc *addr;
	register u_char *addr_bytes;

	addr = misc_ptr;
	addr_bytes = (u_char *)&addr;

	mIntr(dest, SM_BIN, SM_ADDRVEC);
	mIntr(dest, SM_BIN, addr_bytes[0]);	/* low byte first */
	mIntr(dest, SM_BIN, addr_bytes[1]);
	mIntr(dest, SM_BIN, addr_bytes[2]);
	mIntr(dest, SM_BIN, addr_bytes[3]);	/* high byte last */
}
#endif
