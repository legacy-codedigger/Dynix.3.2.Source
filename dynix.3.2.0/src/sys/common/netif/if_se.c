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

#undef RAW_ETHER
#define RAW_ETHER
#undef	PROMISCUOUS	/* UNDO promiscuous kernel */
#define	PROMISCUOUS	/* promiscuous kernel */
#undef INET
#define INET		/* INET currently not optional */
#undef AT
#define AT		/* APPLETALK currently not optional */


#define	IFPMLEN	(MLEN-sizeof(struct ifnet*))	/* ifp passing */

#ifndef lint
static char rcsid[] = "$Header: if_se.c 2.41 1991/05/13 18:35:35 $";
#endif

/*
 * SCSI/Ether Ethernet Communications Controller interface
 *
 * ETHER OUTPUT NOTES:
 * -------------------
 * Only one output request is active at a time.  A simple array
 * of iats holds the addresses of the mbuf data that get written.
 * We copy transmits into a single buffer because the higher-level
 * network code can generate mbufs too small for the DMA's to handle
 * (the firmware doesn't have enough time to turn around and reload).
 *
 * As a matter of convention, all SEC Ether ioctls are done with the
 * output device and output locks.
 * 
 * ETHER INPUT NOTES:
 * ------------------
 * Device programs for Ether read contain a pointer to an iat
 * and the number of data blocks in that iat.  The iat dp_data
 * fields give the physical addresses of the m_data field of 
 * a parallel array of mbufs.  Ether packets read from the net
 * are placed into these data blocks (and hence right into the
 * mbufs).
 *
 * Each controller has a circular queue of pointers to mbufs and
 * a circular queue of iats that are continually filled by the
 * SEC firmware with input packets.
 * Our job is to replace the used mbufs as quickly as possible
 * at interrupt time, and refill the iats.  We then add another
 * device program (or two if we wrap around the end of the iat
 * ring) for ether input.
 * 
 * Important hints:
 *	- There is an iat queue and an mbuf pointer queue for each
 *	  controller.
 *	- The iat queue and the mbuf queue have the same number
 *	  of elements.
 *	- Except when refilling the read programs (at interrupt time),
 *	  the heads of the iat queue and the mbuf queue should be the same.
 *	  Even here, they should only be different during the actual
 *	  refilling of the iat and mbuf queues.
 *	- All hell breaks loose if we run out of input programs
 *	  to replace the iats.  We can't sleep and wait for more at
 *	  interrupt level.
 *	- Overspecifying the size of the Ether read request and
 *	  done queues in the binary config file is a very good idea.
 *	- Refilling the queues after reading short packets will cause
 *	  each packet to have a single new device program added to the
 *	  Ether read device request queue.
 *	- No attempt is made to optimize these programs, as there is
 *	  no synchronization with the SEC firmware:  I can't ask him
 *	  to stop for a second while I increase the number of iats in
 *	  that last device program.
 *
 *
 *
 * TMP AND LOCKING NOTES:
 * ----------------------
 *
 * There is very little locking or synchronization needed at this
 * level of the software.  Most of it really goes on above when necessary.
 *
 * In general, we try to lock only the portions of the controller state
 * that we have to.  When changing "important" information (like fields
 * int the arp and/or ifnet structures), we lock everything.
 *
 * To lock everything, it is safe to lock structure from the inside out.
 * That is, lock either the input or output segment of the controller
 * state, then lock the common structure.  With the macros defined
 * below, the order OS_LOCK, then SS_LOCK should be safe.
 * See how se_init does locking for an example.
 */

/* $Log: if_se.c,v $
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/vm.h"
#include "../h/conf.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/if_ether.h"

#include "../balance/cfg.h"
#include "../balance/engine.h"
#include "../balance/slic.h"

#include "../machine/ioconf.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"
#include "../machine/mftpr.h"

#include "../sec/sec.h"

#include "../netif/pci.h"
#include "../netif/if_se.h"

#ifdef PROMISCUOUS
#include "../net/promisc.h"
#endif PROMISCUOUS

#ifdef AT

#include "../h/time.h"
#include "../h/kernel.h"

#include "../netat/atalk.h"
#include "../netat/katalk.h"

u_short AFDEFAULTNET=0xffff;
u_char AFDEFAULTNODE=0;

#endif AT

#define	KVIRTTOPHYS(addr) \
	(PTETOPHYS(Sysmap[btop(addr)]) + ((int)(addr) & (NBPG-1)))

/*
 * MODINCR() - macro to do modular arithmetic on head or tail indices
 * note % aritmetic is inefficient since it involves a divide - also
 * if head or tail is shared with SCED firmware, there are possible
 * races when value is temporarily set for comparison with size
 */

#define MODINCR(headortail, size) \
	if (((headortail) + 1) >= (size)) \
		(headortail) = 0; \
	else \
		(headortail)++;


/*
 * All procedures are referenced either through the se_driver structure,
 * or via the procedure handles in the ifnet structure.
 * Hence, everything but the se_driver structure should be able to be static.
 */

static int se_probe(), se_boot(), se_intr(), se_watch();
static int se_init(), se_output(), se_ioctl(), se_reset();

struct mbuf *se_reorder_trailer_packet();

static int se_handle_read(), se_add_erbuf();
static int se_start();
int se_set_modes();

struct sec_driver se_driver = {
	/* name	chan	flags		probe		boot		intr */
	"se",	1,	SED_TYPICAL,	se_probe,	se_boot,	se_intr
};

int (* ge_intr_fctn)() = 0;
int (* pciattach_fctn)() = 0;
int (* pcirint_fctn)() = 0;

/*
 * SCSI-command template for Ether write.
 * These are placed in the write device programs,
 * and altered by the se_start routine just before
 * we write the packet.
 */

u_char se_scsi_cmd[10] = { SCSI_ETHER_WRITE, SCSI_ETHER_STATION };

#ifdef RAW_ETHER

#include "../net/raw_cb.h"

#endif RAW_ETHER

/*
 * sec_init_iatq()
 *	initialize a ring of iat entries.
 *
 * If locking is needed, it is presumed to be done elsewhere.
 */

static void
sec_init_iatq(iq, count)
	register struct sec_iatq *iq;
	unsigned count;
{
	iq->iq_iats = (struct sec_iat *)
			calloc((int)(count*sizeof(struct sec_iat)));
	iq->iq_size = count;
	iq->iq_head = 0;
}

struct mbuf *
se_copy_buf_to_mbufs(bufp, count)
	register u_char *bufp;
	register int count;
{
	register struct mbuf *m;
	struct mbuf *mhead;

	m = m_getm(M_DONTWAIT, MT_DATA,
			howmany((count+sizeof(struct ifnet*)), MLEN));

	if ((mhead = m) != (struct mbuf *)NULL) {

		/*
		 * ifp passing
		 * accommodate struct ifnet * in front of data
		 */

		mhead->m_off += sizeof(struct ifnet *);
		m->m_len = (count > IFPMLEN) ? IFPMLEN : count;
		bcopy((caddr_t)bufp, mtod(m, caddr_t),
					(unsigned)m->m_len);
		count -= m->m_len;
		bufp += m->m_len;
		m = m->m_next;

		while (count) {
			m->m_off = MMINOFF;
			m->m_len = (count > MLEN) ? MLEN : count;
			bcopy((caddr_t)bufp, mtod(m, caddr_t),
						(unsigned)m->m_len);
			count -= m->m_len;
			bufp += m->m_len;
			m = m->m_next;
		}
	}
	return(mhead);
}

/*
 * sec_start_prog - start a program on a SCSI/Ether device.
 *
 * Calls mIntr() to do the actual slic fussing to send the message.  Use
 * bin 3, since this helps avoid SLIC-bus saturation/lockup (since SCED
 * interrupts Dynix mostly on bins 4-7, using bin 3 to interrupt SCED gives
 * SCED -> Dynix priority over Dynix -> SCED, thus SCED won't deadlock
 * against Dynix).  Initialization-time mIntr()'s can use other bins since
 * SLIC-bus is not busy at that time.
 */

static int
sec_start_prog(cmd, cib, slic, vector)
	struct sec_cib *cib;
	u_char slic, vector;
{
	register int *stat = cib->cib_status;
	spl_t sipl;

	sipl = splhi();
	cib->cib_inst = cmd;
	*stat = 0;
	mIntr(slic, 3, vector);
	splx(sipl);

	while ((*stat & SINST_INSDONE) == 0)
		continue;
	return(*stat & ~SINST_INSDONE);
}

#ifdef notused
/*
 * mbuf_chain_size - Determine the number of data bytes in a chain of mbufs.
 */

static int
mbuf_chain_size(m)
	register struct mbuf *m;
{
	register int count;

	for (count = 0; m != (struct mbuf *)0; m = m->m_next)
		count += m->m_len;
	return(count);
}
#endif notused

/* lock the controller state */

#define	SS_LOCK(softp)	(p_lock(&(softp)->ss_lock, SPLIMP))
#define	SS_UNLOCK(softp, sipl)	(v_lock(&(softp)->ss_lock, sipl))

/* lock the output state */

#define	OS_LOCK(softp)	(p_lock(&(softp)->os_lock, SPLIMP))
#define	OS_UNLOCK(softp, sipl)	(v_lock(&(softp)->os_lock, sipl))

int se_max_unit = -1;		/* largest index of active ether controller */
u_char se_base_vec;		/* base interrupt vector */
struct se_state *se_state;	/* pointer to array of soft states */

/*
 * Probe an SEC for existence of Ether controller.
 *
 * There's some debate about what this means: presently
 * if the controller is there, so is the Ether part.
 * This is expected to be changed in the future,
 * when the world of depopulated boards arrives.
 * So let's look at the diagnostics flags, and make
 * the decision based on that.
 */

static
se_probe(probe)
	struct sec_probe *probe;
{
	if (probe->secp_desc->sec_diag_flags & CFG_S_ETHER)
		return(0);
	return(1);
}

/*
 * se_boot_one()
 *	boot procedure for a single device.
 *
 * Allocate the non-mbuf data structures for the device.
 * We shouldn't really talk to the device now either.
 *
 * For both ether read and ether write, the request and done queues
 * were allocated by autoconfig code.  We record handles to these
 * queues and fill in the actual device programs.
 *
 * The done queues should not need anything done to them, as they
 * never need programs of their own.
 *
 * The status pointers for each cib are set to point to local data
 * in the state structures.
 *
 * Iat queues are also allocated, but can't be filled in  yet
 * (no mbufs to allocate yet).  For input, the parallel array
 * of mbuf pointers is allocated as well.
 *
 * No locking needs to be done here, as we are still running config
 * code single-processor.
 */

static void
se_boot_one(softp, sd)
	register struct se_state *softp;
	register struct sec_dev *sd;
{
	int i;

	/*
	 * Controller info:  Can do this with
	 * either the input device or output device.
	 * Either way, we just do it once.
	 */

	if (!softp->ss_initted) {
		register struct ifnet *ifp;

		ifp = &softp->ss_arp.ac_if;
		ifp->if_unit = softp-se_state;
		ifp->if_name = se_driver.sed_name;
		ifp->if_mtu = se_mtu;
		ifp->if_flags = IFF_BROADCAST;
		init_lock(&ifp->if_snd.ifq_lock, G_IFNET);
		ifp->if_init = se_init;
		ifp->if_output = se_output;
		ifp->if_ioctl = se_ioctl;
		ifp->if_reset = se_reset;
		init_lock(&softp->ss_lock, se_gate);
		bzero((caddr_t)&softp->ss_sum, sizeof(softp->ss_sum));
		softp->ss_scan_int = se_watch_interval;
		bcopy((caddr_t)sd->sd_desc->sec_ether_addr,
			(caddr_t)softp->ss_arp.ac_enaddr, 6);
                /*
		 * a miserable hack -- we save pointer to factory address at
		 * boot time so that we can get it back --  DECnet likes
		 * to be able to diddle the ethernet address 
		 */
	        softp->ss_pad = (int)sd->sd_desc->sec_ether_addr;
		softp->ss_slic = sd->sd_desc->sec_slicaddr;
		softp->ss_bin = se_bin;
		softp->ss_ether_flags = SETHER_S_AND_B;
		softp->ss_alive = 1;
		softp->ss_initted = 1;
		softp->ss_init_called = 0;
#ifdef AT
		ifpsetnet(ifp, AFDEFAULTNET);
		ifpsetnode(ifp, AFDEFAULTNODE);
#endif AT
		if_attach(ifp);
		if (pciattach_fctn)
			(*pciattach_fctn)(ifp, softp->ss_arp.ac_enaddr);
		if ((int)(&softp->os_gmode + 1) > 4*1024*1024) {
			printf("%s%d: data structures above 4Mb!\n",
				se_driver.sed_name, softp-se_state);
			printf("     Ethernet function is unpredictable.\n");
		}
	}

	if (sd->sd_chan == SDEV_ETHERREAD) {
		struct sec_iatq *iq;

		init_lock(&softp->is_lock, se_gate);
		softp->is_cib = sd->sd_cib;
		softp->is_cib->cib_status = &softp->is_status;
		softp->is_reqq.sq_progq = sd->sd_requestq;
		softp->is_reqq.sq_size = sd->sd_req_size;
		softp->is_doneq.sq_progq = sd->sd_doneq;
		softp->is_doneq.sq_size = sd->sd_doneq_size;
		SEC_fill_progq(softp->is_reqq.sq_progq,
			(int)softp->is_reqq.sq_size,
			(int)sizeof(struct sec_edev_prog));
		iq = &softp->is_iatq;
		sec_init_iatq(iq, softp->is_reqq.sq_size-3);

		softp->is_mbufq.mq_mbufp = (struct mbuf **)
		    calloc(((int)iq->iq_size) * sizeof(struct mbuf **));

		softp->is_mbufq.mq_tail = softp->is_mbufq.mq_head
						= softp->is_mbufq.mq_mbufp;

		softp->is_mbufq.mq_end = &softp->is_mbufq.mq_mbufp[iq->iq_size];
		softp->is_status = 0;
		softp->is_initted = 1;

	} else if (sd->sd_chan == SDEV_ETHERWRITE) {

		init_lock(&softp->os_lock, se_gate);
		softp->os_cib = sd->sd_cib;
		softp->os_cib->cib_status = &softp->os_status;
		softp->os_status = 0;
		softp->os_reqq.sq_progq = sd->sd_requestq;
		softp->os_reqq.sq_size = sd->sd_req_size;
		softp->os_doneq.sq_progq = sd->sd_doneq;
		softp->os_doneq.sq_size = sd->sd_doneq_size;
		SEC_fill_progq(softp->os_reqq.sq_progq,
			(int)softp->os_reqq.sq_size,
			(int)sizeof(struct sec_dev_prog));
		for (i = 0; i < softp->os_reqq.sq_size; ++i) {
			register struct sec_dev_prog *dp;

			dp = softp->os_reqq.sq_progq->pq_un.pq_progs[i];
			bcopy((caddr_t)se_scsi_cmd,
				(caddr_t)dp->dp_cmd, sizeof se_scsi_cmd);
			dp->dp_cmd_len = sizeof se_scsi_cmd;
		}

		/*
		 * the os_iatq isn't used since SCED
		 * transmits directly from command queue.
		 */

		softp->os_iatq.iq_iats = (struct sec_iat *) NULL;
		softp->os_iatq.iq_size = 0;
		softp->os_iatq.iq_head = 0;

		/*
		 * don't use these guys on output side
		 */

		softp->os_mbufq.mq_mbufp = (struct mbuf **) NULL;

		softp->os_xfree = (struct se_xbuf *) NULL;

		for (i = 0; i < se_write_iats; i++) {

			u_long curbrk;
			struct se_xbuf*	datap;

			curbrk = (u_long) calloc(0);
			if (((curbrk & 0xffff) + EBUF_SZ)  > 64 * 1024)
				callocrnd((int)64*1024);

			datap = (struct se_xbuf *) calloc(EBUF_SZ);
			datap->se_xnext = softp->os_xfree;
			softp->os_xfree = datap;
		}

		softp->os_npending = 0;
		softp->os_initted = 1;

	} else {

		printf("%s%d: invalid device chan %d (0x%x) in boot routine\n",
			se_driver.sed_name, softp-se_state,
			sd->sd_chan, sd->sd_chan);

		panic("se_bootone");
	}
}

/*
 * se_boot - allocate data structures, etc at beginning of time.
 *
 * Called with an array of configured devices and the number
 * of ether devices.
 * We allocate the necessary soft descriptions, and fill them
 * in with info from the devs[] array.
 * Due to the dual-device-channel nature of the SEC description,
 * each entry in the devs[] array describes either the Ether input
 * device or the Ether output device.  We combine this into a
 * single device state per controller.
 *
 * The program queues are allocated by the routine that calls us,
 * but the device programs themselves are not allocated til now.
 *
 * Work related to allocating mbufs is done later at se_init time.
 */

static
se_boot(ndevs, devs)
	struct sec_dev devs[];
{
	register int i;

	/*
	 * First, allocate soft descriptions.
	 */

	se_max_unit = ndevs/2;

	se_state = (struct se_state *)calloc((se_max_unit+1)
		* sizeof(struct se_state));

	se_base_vec = devs[0].sd_vector;

	/*
	 * Now, boot each configured device.
	 */

	for (i = 0; i < ndevs; ++i) {
		register struct sec_dev *devp;
		register int unit;

		devp = &devs[i];
		if (devp->sd_alive == 0)
			continue;
		unit = i/2;
		se_boot_one(&se_state[unit], devp);
	}
}

/*
 * Initialization of interface; clear recorded pending
 * operations, and reinitialize SCSI/Ether usage.
 */

static
se_init(unit)
	int unit;
{
	register struct se_state *softp = &se_state[unit];
	register struct ifnet *ifp;
	register struct sec_iat *iatp;
	struct sec_iatq *iq;
	struct mbuf *m;
	spl_t sipl;

	if (unit < 0 || unit > se_max_unit) {
		printf("%s%d: invalid unit in init\n",
			se_driver.sed_name, unit);
		return;
	}
	sipl = OS_LOCK(softp);
	(void) SS_LOCK(softp);

	if (!softp->ss_alive || !softp->ss_initted || !softp->is_initted
	    || !softp->os_initted)
		goto ret;

	ifp = &softp->ss_arp.ac_if;
	if (ifp->if_addrlist == (struct ifaddr *) NULL)
		goto ret;

	if (ifp->if_flags & IFF_RUNNING)
		goto justarp;

	if (softp->ss_init_called)
		goto justarp;

	ifp->if_watchdog = se_watch;
	ifp->if_timer = softp->ss_scan_int;

	/*
	 * Set the Ether modes before we add input programs,
	 * as the firmware will need to know the size of the
	 * input packets before we do the SINST_STARTIO.
	 */

	se_set_modes(softp);
	iq = &softp->is_iatq;
	for (iatp = iq->iq_iats; iatp < iq->iq_iats + iq->iq_size; iatp++) {
		m = m_getcl(M_DONTWAIT, MT_DATA);
		if (m == (struct mbuf *) NULL) {
			printf("%s%d se_init: not enough mclusters for ether\n",
				se_driver.sed_name, softp-se_state);
			printf("need %d more\n",
				iq->iq_iats+iq->iq_size-iatp);
			se_stop(softp);
			SS_UNLOCK(softp, SPLIMP);
			OS_UNLOCK(softp, sipl);
			return;
		}

		m->m_off += sizeof(struct ifnet *);	/* ifp passing */

		{
		  register struct sec_mbufq *mq = &softp->is_mbufq;

		  *(mq->mq_tail) = m;
		  if ((unsigned)++(mq->mq_tail) >= (unsigned)mq->mq_end)
			mq->mq_tail = mq->mq_mbufp;
		}

		/*
		 * NOTE BENE - mclusters are calloc() at startup soz
		 * virtual == physical address - required for sced or
		 * must convert e.g. 
		 * iatp->iat_data = (u_char *)KVIRTTOPHYS(mtod(m,u_char *));
		 */

		/*
		 * ifp passing
		 */

		iatp->iat_data = mtod(m, u_char *);
		iatp->iat_count = MCLEN;

		se_add_erbuf(softp);
	}

	/*
	 * Shouldn't have to restart output to the device,
	 * as nothing was reset.
	 */

	ifp->if_flags |= IFF_RUNNING;
	softp->ss_init_called = 1;

justarp:
	se_set_modes(softp);
	SS_UNLOCK(softp, SPLIMP);
	OS_UNLOCK(softp, sipl);

	return;
ret:
	SS_UNLOCK(softp, SPLIMP);
	OS_UNLOCK(softp, sipl);
}

/*
 * Ethernet interface interrupt routine.
 * Could be output or input interrupt.
 * We determine the source and call the appropriate routines
 * to decode the done queue programs.
 */

static
se_intr(vector)
	int vector;
{
	int unit = (vector - se_base_vec)/2;
	int is_read = !((vector - se_base_vec) & 0x01);
	register struct se_state *softp = &se_state[unit];
	register struct sec_pq *sq;
	spl_t sipl;

	if (unit < 0 || unit > se_max_unit) {
		printf("%s%d: invalid interrupt vector %d\n",
			se_driver.sed_name, unit, vector);
		return;
	}

	if (!(softp->ss_alive))	/* DEAD soz ignore interrupt */
		return;

	if (is_read) { 	/* Receiver interrupt.  */
		register struct sec_eprogq *epq;

		ASSERT_DEBUG(softp->ss_init_called, "se_intr: initted");
		sq = &softp->is_doneq;
		epq = (struct sec_eprogq *)sq->sq_progq;
		ASSERT_DEBUG(epq->epq_tail < sq->sq_size, "se_intr: tail");
		ASSERT_DEBUG(epq->epq_head < sq->sq_size, "se_intr: head");

		/*
		 * Lock the input state before we test for work.
		 * This keeps other processors out of the way once
		 * we commit to entering the loop and doing work.
		 * There is a race here between the decision to
		 * leave the loop and the v_lock that can cause an
		 * interrupt from the SCSI/Ether controller to be
		 * missed, but we say this is acceptable, as the
		 * net should always be busy, and we will eventually
		 * see the packet on the next interrupt.
		 */

		sipl = cp_lock(&softp->is_lock, SPLIMP);
		if (sipl == CPLOCKFAIL)
			return;
		while (epq->epq_tail != epq->epq_head) { /* work to do */
			se_handle_read(softp, &epq->epq_status[epq->epq_tail]);
			MODINCR(epq->epq_tail, sq->sq_size);
		}
		v_lock(&softp->is_lock, sipl);

	} else { 	/* Transmitter interrupt. */

		register struct sec_progq *pq;
		register struct sec_dev_prog *dp;
		register struct se_xbuf	*datap;

		if (softp->os_intr)
			return;		/* already processing xmit intr */

		sipl = OS_LOCK(softp);
		if (!softp->ss_alive || !softp->ss_init_called) {
			OS_UNLOCK(softp, sipl);
			return;
		}
		softp->os_intr = 1;
		sq = &softp->os_doneq;
		pq = sq->sq_progq;

		ASSERT_DEBUG(pq->pq_tail < sq->sq_size, "se_intr: tail 2");
		ASSERT_DEBUG(pq->pq_head < sq->sq_size, "se_intr: head 2");

		if (softp->os_npending == 0) {	 /* spurious interrupt */
			softp->os_intr = 0;
			OS_UNLOCK(softp, sipl);
			return;
		}
		while (pq->pq_tail != pq->pq_head) {
			dp = pq->pq_un.pq_progs[pq->pq_tail];
			if (dp->dp_status1 != 0) {
				int status = sec_start_prog(SINST_RESTARTIO,
						   softp->os_cib,
						   softp->ss_slic,
						   SDEV_ETHERWRITE);
				if (status != SEC_ERR_NONE
				    && status != SEC_ERR_NO_MORE_IO) {
					printf("%s%d: se_intr: status 0x%x\n",
						se_driver.sed_name,
						softp-se_state,
						softp->os_status);
				}
			}

			MODINCR(pq->pq_tail, sq->sq_size);

			datap = (struct se_xbuf *) dp->dp_un.dp_data;

			datap->se_xnext = softp->os_xfree;
			softp->os_xfree = datap;

			ASSERT_DEBUG(softp->os_npending > 0,
					"se_intr: npending low");
			softp->os_npending--;
		}
		softp->os_intr = 0;
		OS_UNLOCK(softp, sipl);
		se_start(softp);
	}
}

/*
 * Handle read interrupt requests.
 * This includes recognizing trailer protocol,
 * and passing up to the higher level software.
 *
 * This is called with the input state locked.
 */

extern struct custom_client custom_clients[];

#ifdef AT
extern struct ifqueue ddpintq;
#endif AT

static
se_handle_read(softp, statp)
	register struct se_state *softp;
	struct sec_ether_status *statp;
{
	register struct mbuf *m;
	struct mbuf *mnew;
#ifdef PROMISCUOUS
	struct mbuf *mpromisc;
	struct promiscif * mp;
#endif PROMISCUOUS
	int len, int_to_sched;
	struct ether_header *hp;
	struct ifqueue *inq;
	spl_t sipl;
	struct ifnet *ifp = &softp->ss_arp.ac_if;
#ifdef RAW_ETHER
	struct raw_header * rh;
	struct mbuf * mrh;
#endif RAW_ETHER
	struct sec_iat *iatp;
	int	trailer;
	int	ci;

	len = statp->es_count;
	ifp->if_ibytes += len;

	if (len > ETHERMTU+sizeof(struct ether_header)) {

		/*
		 * packet is bigger than expected - handle it and return
		 */

		se_handle_biggee(softp, statp);
		return;
	}

	{ register struct sec_iatq *iq = &softp->is_iatq;

	  iatp = &iq->iq_iats[iq->iq_head];
	  if (iatp->iat_data != statp->es_data) {

		/*
		 * GADS! - the driver is out of sync with the firmware
		 */

		printf("%s%d: botch: statp 0x%x; es_data 0x%x\n",
			se_driver.sed_name, softp-se_state, statp,
			statp->es_data);
		printf("iatq 0x%x iq->iq_head %d iat 0x%x addr 0x%x count %d\n",
			iq, iq->iq_head, iatp, iatp->iat_data, iatp->iat_count);

		se_stop(softp);
		return;
	  }
	}
	{
	  register struct sec_mbufq *mq = &softp->is_mbufq;

	  m = *(mq->mq_head);
	  if ((unsigned)++(mq->mq_head) >= (unsigned)mq->mq_end)
		mq->mq_head = mq->mq_mbufp;
	}

	/*
	 * Check for trailer protocol
	 */

	hp = mtod(m, struct ether_header *);
	hp->ether_type = ntohs((u_short)hp->ether_type);
	trailer =  hp->ether_type >= ETHERTYPE_TRAIL
	    && hp->ether_type < ETHERTYPE_TRAIL + ETHERTYPE_NTRAILER;

	/*
	 * pass to protocol layer if 
	 * packet is big enough that there is a free buffer to replace
	 * it and it doesn't have a trailer
	 * ifp passing must accommodate IFPMLEN not MLEN
	 */

	if (len > IFPMLEN && !trailer &&
	    (mnew = m_getcl(M_DONTWAIT,MT_DATA)) != (struct mbuf *)NULL) {

		{
		  register struct sec_mbufq *mq = &softp->is_mbufq;

		  mnew->m_off += sizeof(struct ifnet*); /* ifp passing */
		  *(mq->mq_tail) = mnew;
		  if ((unsigned)++(mq->mq_tail) >= (unsigned)mq->mq_end)
			mq->mq_tail = mq->mq_mbufp;
		}

		/*
		 * Set iat to point to new buffer
		 * Give iat back to controller
		 *
		 * NOTE BENE: mclusters are calloc() at startup soz
		 * virtual == physical - this is required for SCED
		 * or must convert E.g.
		 * iatp->iat_data = (u_char *)KVIRTTOPHYS(mtod(mnew,u_char *));
		 *
		 * ifp passing allows for ifnet * in front
		 */

		iatp->iat_data = mtod(mnew, u_char *);

		se_add_erbuf(softp);
		m->m_len = len;

	} else {

		/*
		 * Copy data into mbuf chain
		 */

		mnew = m;
		m = se_copy_buf_to_mbufs(mtod(mnew,u_char *), len);

		/*
		 * Give iat back to controller
		 */

		{
		  register struct sec_mbufq *mq = &softp->is_mbufq;

		  *(mq->mq_tail) = mnew;
		  if ((unsigned)++(mq->mq_tail) >= (unsigned)mq->mq_end)
			mq->mq_tail = mq->mq_mbufp;
		}
		se_add_erbuf(softp);
		if (m == (struct mbuf *)NULL) {
			ifp->if_idiscards++;
			return;
		}

		/*
		 * Recalculate hp
		 */

		hp = mtod(m, struct ether_header *);

	}

	/*
	 * m now contains a packet from the interface.
	 */

	len -= sizeof(struct ether_header);
	if (len < ETHERMIN) {
		ifp->if_ierrors++;
		m_freem(m);
		return;
	}

#ifdef PROMISCUOUS

	if (promiscon) { /* promiscon => give promiscintr the packet */

		/*
		 * Promisc expects this to be in net order, so put it back.
		 */

		hp->ether_type = htons((u_short)hp->ether_type);

		/*
		 * get an mbuf for passing softp to promiscintr.
		 */

		mpromisc = m_getm(M_DONTWAIT, MT_DATA, 1);
		if (!mpromisc) {
			m_freem(m);
			return;
		}
		mpromisc->m_next = m;
		mpromisc->m_len = sizeof(struct promiscif);
		inq = &promiscq;
		int_to_sched = NETISR_PROMISC;
		mp = mtod(mpromisc, struct promiscif *);
		mp->promiscif_ifnet = (caddr_t)softp;
		mp->promiscif_flag = PROMISC_RCVD;
		sipl = IF_LOCK(inq);
		if (IF_QFULL(inq)) {
			IF_DROP(inq);
			IF_UNLOCK(inq, sipl);
			m_freem(mpromisc);
			return;
		}
		IF_ENQUEUE(inq, mpromisc);
		if (!inq->ifq_busy) 
			schednetisr(int_to_sched);
		IF_UNLOCK(inq, sipl);
		return;
	}

#endif PROMISCUOUS

	/*
	 * check for SETHER_PROMISCUOUS but !promiscon
	 */

#ifdef PROMISCUOUS

	/*
	 * avoid race with promiscuous mode which
	 * causes illegitimate packets to be received and rejected,
	 * icmp messages are then sent which result in Connection Resets
	 * all over the network
	 */

	if (promiscrace || softp->ss_ether_flags == SETHER_PROMISCUOUS)
#else
	if (softp->ss_ether_flags == SETHER_PROMISCUOUS)

#endif PROMISCUOUS
	{
		if (bcmp((char *)etherbroadcastaddr,
			 (char *)hp->ether_dhost, 6) != 0
		    && bcmp((char *)softp->ss_arp.ac_enaddr,
			    (char *)hp->ether_dhost, 6) != 0) {

#ifdef PROMISCUOUS
			if (promiscrace)
				promiscrace--;
#endif PROMISCUOUS
			m_freem(m); 	/* throw promiscuous packets away. */
			return;
		}
	}

	m->m_off += sizeof(struct ether_header);
	m->m_len -= sizeof(struct ether_header);
	if (trailer) {
		mnew = se_reorder_trailer_packet(hp, m);
		if (mnew == (struct mbuf *)0) {
			ifp->if_idiscards++;
			m_freem(m);
			return;
		}
		m = mnew;
	}
	if (bcmp((char *)etherbroadcastaddr,
		 (char *)hp->ether_dhost, 6) == 0) {
		m->m_flags = MF_BROADCAST;
		ifp->if_inunicast++;
	} else
		ifp->if_ipackets++;

	switch (hp->ether_type) {

#ifdef INET
	case ETHERTYPE_IP:
		int_to_sched = NETISR_IP;
		inq = &ipintrq;
		break;

	case ETHERTYPE_ARP:
		arpinput(&softp->ss_arp, m);
		return;
#endif INET

	case PCI_TYPE:
		if (pcirint_fctn)
			(*pcirint_fctn)(hp, m);
		else
			m_freem(m);
		return;

#ifdef AT
	case ETHERPUP_ATALKTYPE:
		{
			int_to_sched = NETISR_DDP;
			inq = &ddpintq;
			break;
		}
#endif AT

	default:

		/*
		 * do not queue reordered trailer to rawif or custom
		 */

		if (trailer) {
			m_freem(m);
			return;
		}

	 	/*
		 * allow for custom ether_read device drivers
		 */

		for (ci = 0; ci < 4; ci++) {
		  if (custom_clients[ci].custom_devno
			&& custom_clients[ci].custom_type == hp->ether_type)
		  {

	    ASSERT(cdevsw[major(custom_clients[ci].custom_devno)].d_read,
			"no custom_client cdevsw.d_read!");

		        (*cdevsw[major(custom_clients[ci].custom_devno)].d_read)
				(hp, m, (caddr_t) ifp);

		  	custom_clients[ci].custom_count++;
		  	return;
		  }
		}

#ifdef RAW_ETHER

		/*
		 * reput the ether header into the lead data buffer
		 * *and* copy a Unix4.2 raw_header for compatibility
		 */
	
		m->m_off -= sizeof(struct ether_header);
		m->m_len += sizeof(struct ether_header);
                /*
		 * give DECnet code a peek at the packet.
		 * It needs to contain the ether_header, but
		 * NOT the ifnet ptr
		 */
		 {
			int gecnt;
			if (ge_intr_fctn &&
			    (*ge_intr_fctn)(ifp, m, 0, &gecnt) == 0)
		    		if (gecnt) /* we processed the datagram */
					return;
		 }

		int_to_sched = NETISR_RAW;
		inq = &rawif.if_snd;
		mrh = m_getclrm(M_DONTWAIT, MT_DATA, 1);
		if (mrh == (struct mbuf *) NULL) {
			m_freem(m);
			return;
		}

		mrh->m_off += sizeof(struct ifnet*); /* ifp passing */
	
		/*
		 * set up raw header, using type as sa_data for bind.
		 * raw_input() could do this if static struct set up.
		 * 	- assign AF_UNSPEC for protocol
		 * link the raw_header into the ether packet
		 */

		mrh->m_len = sizeof(struct raw_header);
		mrh->m_next = m;
		m = mrh;
		rh = mtod(mrh, struct raw_header*);
		rh->raw_proto.sp_family = AF_RAWE;
		rh->raw_proto.sp_protocol = AF_UNSPEC;

		/*
		 * copy AF_RAWE and ether_type in for dst addr
		 */

		rh->raw_dst.sa_family = AF_RAWE;

		/*
		 * put type back into net order
		 */

		hp->ether_type = htons(hp->ether_type);

		bcopy((caddr_t)&hp->ether_type,
			(caddr_t)rh->raw_dst.sa_data, 2);
		bcopy((caddr_t)&hp->ether_type,
			(caddr_t)rh->raw_src.sa_data, 2);

		/*
		 * copy AF_RAWE and if_unit # in for src addr
		 */

		rh->raw_src.sa_family = AF_RAWE;
		bcopy((caddr_t)&ifp->if_unit,
			(caddr_t)&rh->raw_src.sa_data[2], sizeof(short));

#else		/* not RAW_ETHER */

		m_freem(m);
		return;

#endif RAW_ETHER

	}	/* end switch */

	/*
	 * m->packet ready to ENQ except for ifp passing
	 *
	 * note trailer packets can fail to have room for
	 * ifnet* since there can be exactly MLEN bytes moved into
	 * the 1st mbuf of the header that was trailed;
	 * therefore check for appropriate m_off 
	 * if not, get yet another mbuf for the ifnet
	 * (should only be necessary for trailer packets)
	 */

	/*
	 * ifp passing
	 * Place interface pointer before the data
	 * for the receiving protocol.
	 */

	if (m->m_off <= MMAXOFF &&
	    m->m_off >= MMINOFF + sizeof(struct ifnet *)) {

		m->m_off -= sizeof(struct ifnet *);
		m->m_len += sizeof(struct ifnet *);

	} else {
		struct mbuf *	mm;

		MGET(mm, M_DONTWAIT, MT_HEADER);
		if (mm == (struct mbuf *)0) {
			ifp->if_idiscards++;
			m_freem(m);
			return;
		}
		mm->m_off = MMINOFF;
		mm->m_len = sizeof(struct ifnet *);
		mm->m_next = m;
		m = mm;
	}

	*(mtod(m, struct ifnet **)) = ifp;

	sipl = IF_LOCK(inq);
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		IF_UNLOCK(inq, sipl);
		ifp->if_idiscards++;
		m_freem(m);
		return;
	}
	IF_ENQUEUE(inq, m);
	if (!inq->ifq_busy) {
		schednetisr(int_to_sched);
	}
	IF_UNLOCK(inq, sipl);
	return;
}

/*
 * se_handle_biggee()
 * a packet larger than ETHERMTU was received - need to special case
 * this since it is bogus and takes up more than a single ether read
 * buffer.  For now, queue it up to the rawq for analysis and do not
 * deliver to higher layers - Also synch up the iatq and mbufq
 */

int	se_bigEs = 0;
int	se_bigestE = 0;

se_handle_biggee(softp, statp)
	register struct se_state *softp;
	struct sec_ether_status *statp;
{
	register struct sec_iatq *iq = &softp->is_iatq;
	register struct mbuf *m;
	register struct sec_mbufq *mq = &softp->is_mbufq;
	struct sec_iat *iatp;
	int niat, i;

	softp->ss_arp.ac_if.if_oerrors++;
	se_bigEs++;
	niat = statp->es_count / MCLEN + 1;
	if (niat > se_bigestE)
		se_bigestE = niat;

	if (niat > iq->iq_size) {
		printf("%s%d WAY TOO BIG\n",
			se_driver.sed_name, softp-se_state);
		se_stop(softp);
		return;
	}

	/*
	 * simply ignore the packet and give the mclusters back
	 * to the iat list
	 */

	for (i = 0; i < niat; i++) {
		iatp = &iq->iq_iats[iq->iq_head];
		m = *(mq->mq_head);
		/*
		 * ifp passing - allow for ifnet* in front
		 */
		iatp->iat_data = mtod(m, u_char *);
		*(mq->mq_tail) = m;
		se_add_erbuf(softp);
		if ((unsigned)++(mq->mq_tail) >= (unsigned)mq->mq_end)
			mq->mq_tail = mq->mq_mbufp;
	 	if ((unsigned)++(mq->mq_head) >= (unsigned)mq->mq_end)
	      		mq->mq_head = mq->mq_mbufp;
	}
	return;
}

/*
 * se_reorder_trailer_packet - return a real mbuf chain after noticing trailer
 *	protocol is being used.
 *
 * Return the new chain, and modify the header to reflect the real type.
 * Return a null mbuf if we couldn't do it.
 * It is the responsibility of the caller to free the original, if necessary.
 */

struct trailer {
	u_short tl_type;
	u_short tl_count;
};

struct mbuf *se_reorder_trailer_packet(hp, m)
	struct ether_header *hp;
	register struct mbuf *m;
{
	register struct mbuf *mnew, *split;
	int trail_off;
	struct trailer *trailerp;

	/*
	 * find mbuf where we have to split things.
	 */

	trail_off = (hp->ether_type - ETHERTYPE_TRAIL)*512;
	if (trail_off != 512 && trail_off != 1024) {
		printf("%s: ignore trailer with type 0x%x\n",
			se_driver.sed_name, hp->ether_type);
		return((struct mbuf *)0);
	}
	split = m;
	for (; split != 0 && split->m_len <= trail_off; split = split->m_next)
		trail_off -= split->m_len;

	if (split == (struct mbuf *)0)
		return((struct mbuf *)0);

	/*
	 * trail_off has the index into 'split' of the trailer.
	 * Lots of potential boundary conditions here that should
	 * be checked, but since we know the size of data blocks
	 * in trailer-protocol packets  == 512 or 1024 and MLEN == 112,
	 * we are guaranteed that the trailer header is completely
	 * embedded in a single mbuf.
	 */

	trailerp = (struct trailer *)(mtod(split, int) + trail_off);
	if (trail_off + sizeof(struct trailer) > split->m_len)
		return((struct mbuf *)0);

	MGET(mnew, M_DONTWAIT, MT_DATA);
	if (mnew == 0)
		return((struct mbuf *)0);

	/*
	 * Know where to split, and have place for start of header.
	 * Build real header by copying and chaining.
	 */

	mnew->m_off = MMINOFF;
	mnew->m_len = split->m_len - trail_off - sizeof(struct trailer);
	mnew->m_next = split->m_next;
	bcopy((caddr_t)(trailerp+1), mtod(mnew, caddr_t), (u_int)mnew->m_len);
	hp->ether_type = ntohs(trailerp->tl_type);

	split->m_len = trail_off;
	split->m_next = 0;

	split = mnew;
	while (split->m_next)
		split = split->m_next;
	split->m_next = m;

	return(mnew);
}

/*
 * se_add_erbuf - Add a device programs to the request queue.
 */

static
se_add_erbuf(softp)
	register struct se_state *softp;
{
	register struct sec_iatq *iq;
	struct sec_pq *sq;
	struct sec_progq *pq;
	struct sec_edev_prog *dp;

	iq = &softp->is_iatq;
	sq = &softp->is_reqq;
	pq = sq->sq_progq;
	dp = pq->pq_un.pq_eprogs[pq->pq_head];
	dp->edp_iat_count = 1;
	dp->edp_iat = SEC_IATIFY((iq->iq_iats + iq->iq_head));

	MODINCR(iq->iq_head, iq->iq_size);
	MODINCR(pq->pq_head, sq->sq_size);

	if (sec_start_prog(SINST_STARTIO, softp->is_cib, softp->ss_slic,
			   SDEV_ETHERREAD)
	    != SEC_ERR_NONE) {
		printf("%s%d: can't initialize.\n",
			se_driver.sed_name, softp-se_state);
	}
}
/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * If this packet is a broadcast packet or is destined for
 * ourselves, we pass a copy of it through the loopback
 * interface, as the SEEQ chip is not capable of hearing
 * its own transmissions.
 */

static
se_output(ifp, m, dest)
	struct ifnet *ifp;
	register struct mbuf *m;
	struct sockaddr *dest;
{
	register struct se_state *softp = &se_state[ifp->if_unit];
	u_char edst[6];
	struct in_addr idst;
	register struct ether_header *header;
	int type;
	extern struct ifnet loif[];
	spl_t	sipl;
	int usetrailers;
#ifdef AT
	struct mbuf *mcopy = (struct mbuf *)0;
#endif AT

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		ifp->if_odiscards++;
		return (ENETDOWN);
	}

	switch (dest->sa_family) {

#ifdef INET
	case AF_INET:
	{
		register struct mbuf *m0 = m;
		int off;
		struct trailer *tl;

		idst = ((struct sockaddr_in *)dest)->sin_addr;
		if (!arpresolve(&softp->ss_arp, m, &idst, edst, &usetrailers))
			return(0);	/* Not yet resolved */
		if (in_broadcast(idst)) {
			struct mbuf *copy = (struct mbuf *)0;

			copy = m_copy(m, 0, (int)M_COPYALL);
			if (copy != (struct mbuf *)0)
				(void) looutput(&loif[0], copy, dest);
		}

		/*
		 * Generate trailer protocol?
		 */
#ifdef notyet
		/*
		 * this should merge with algorithm below  XXX
		 */

		off = ntohs((u_short)mtod(m, struct ip*)->ip_len) - m->m_len;
		if (usetrailers && off > 0 && (off & 0x1ff) == 0 &&
	 	    m->m_off >= MMINOFF + 2 * sizeof (u_short)) {
			type = ETHERTYPE_TRAIL + (off >> 9);
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = htons((u_short)ETHERTYPE_IP);
			*(mtod(m, u_short *) + 1) = htons((u_short)m->m_len);
			goto gottrailertype;	/* not included */
		}
#endif notyet

		off = ntohs((u_short)mtod(m, struct ip *)->ip_len) - m->m_len;
		if ((ifp->if_flags & IFF_NOTRAILERS) == 0
		    && off > 0 && (off & 0x1FF) == 0
		    && m->m_off >= MMINOFF + sizeof(struct trailer)) {
			type = ETHERTYPE_TRAIL + (off >> 9);
			m->m_off -= sizeof(struct trailer);
			m->m_len += sizeof(struct trailer);
			tl = mtod(m, struct trailer *);
			tl->tl_type = htons((u_short)ETHERTYPE_IP);
			tl->tl_count = htons((u_short)m->m_len);

			/*
			 * Move first packet (control information)
			 * to end of chain.
			 */

			while (m0->m_next)
				m0 = m0->m_next;
			m0->m_next = m;
			m0 = m->m_next;
			m->m_next = (struct mbuf *)0;
			m = m0;
		} else
			type = ETHERTYPE_IP;
		break;
	}
#endif INET

#ifdef AT
	case AF_APPLETALK:
	{
		struct sockaddr_at *dat = (struct sockaddr_at *)dest;
		int dnode;
		struct lap *lap;
		int ltype, golocal = 0;

		if (ifpgetnode(ifp) == 0) { /* don't know who we are */
			m_freem(m);
			return(ENETDOWN);	/* right error ? */
		}

		lap = mtod(m, struct lap *);
		lap->src = ifpgetnode(ifp);
		ltype = lap->type & 0xFF;
		if (ltype == LT_DDP) {
			/*
			 * If dest. net is this net send to the specified
			 * dest. node, else send to a bridge.
			 */
			if (ifpgetnet(ifp) == 0)
				lap->dst = dnode = dat->at_node;
			else
				lap->dst = dnode = (ifpgetbridge(ifp)) & 0xff;
		}
		else
			/* lap->dst was filled in by ddp code for short ddp */
			dnode = lap->dst & 0xFF;

		idst.s_addr = 0L;
		((char *)(&idst))[3] = dnode; /* to fake out arpresolve */

		if (dnode == 0xFF) {		   /* broadcast */

			/*
			 * note if m_copy fails, mcopy == NULL and no
			 * looutput occurs - i.e. bcast but missed self
			 */

			mcopy = m_copy(m, 0, (int) M_COPYALL);


			/*
			 * should/could be done by arp
			 */

			bcopy((caddr_t)etherbroadcastaddr,
				(caddr_t) edst, sizeof (edst));
		}
		else
		/* as of now, can't send long ddp's to self */
		if ((dnode == ifpgetnode(ifp)) && (ltype == LT_SHORTDDP)) {
			/* forme... should/could be done by arp */
			mcopy = m;
			golocal = 1;
		}
		else
		if (!arpresolve(&softp->ss_arp, m, &idst, edst, &usetrailers))
			return 0;

		if (golocal)
			goto gotlocal;
		type = ETHERPUP_ATALKTYPE;
		break;
	}

#endif AT

	case AF_UNSPEC:
		header = (struct ether_header *)dest->sa_data;
		bcopy((caddr_t)header->ether_dhost, (caddr_t)edst,
			sizeof(edst));
		type = header->ether_type;
		break;

	default:
		m_freem(m);
		ifp->if_oerrors++;
		return(EAFNOSUPPORT);
	}

	/*
	 * Add the local header.
	 */

	if (!M_HASCL(m) && m->m_off - MMINOFF >= sizeof(struct ether_header)) {
		/* first mbuf has room for ether header */
		m->m_off -= sizeof(struct ether_header);
		m->m_len += sizeof(struct ether_header);
	} else {
		register struct mbuf *m0;

		MGET(m0, M_DONTWAIT, MT_HEADER);
		if (m0 == (struct mbuf *)0) {
			m_freem(m);
			ifp->if_odiscards++;
			return(ENOBUFS);
		}
		m0->m_next = m;
		m0->m_off = MMINOFF;
		m0->m_len = sizeof(struct ether_header);
		m = m0;
	}

	header = mtod(m, struct ether_header *);
	header->ether_type = htons((u_short)type);
	bcopy((caddr_t)edst, (caddr_t)header->ether_dhost,
		sizeof(edst));
	bcopy((caddr_t)softp->ss_arp.ac_enaddr,
		(caddr_t)header->ether_shost, 6);

	/*
	 * Queue message on interface, and start output if interface not active.
	 */

	sipl = IF_LOCK(&ifp->if_snd);
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		IF_UNLOCK(&ifp->if_snd, sipl);
		m_freem(m);
		ifp->if_odiscards++;
		return(ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	IF_UNLOCK(&ifp->if_snd, sipl);
	if (softp->os_npending <= 0)
		se_start(softp);

#ifdef AT
gotlocal:
	return (mcopy ? looutput(&loif[0], mcopy, dest) : 0);
#else
	return(0);
#endif AT
}

/*
 * se_start - start output on the interface.
 *
 * First we make sure it is idle and that there is work to do.
 *
 * We spray the mbuf into the output iat queue,
 * build the device program and start the program running.
 */

int	se_toobig = 0;

#define FULLETHER	(ETHERMTU+sizeof(struct ether_header))

static
se_start(softp)
	register struct se_state *softp;
{
	struct ifqueue *ifq = &softp->ss_arp.ac_if.if_snd;
	struct sec_pq *sq = &softp->os_reqq;
	register struct sec_progq *pq = sq->sq_progq;
	struct mbuf *m;
	spl_t sipl;
	register struct se_xbuf *xbuf;
	int	len;

	if (softp-se_state > se_max_unit)
		return;

	if (softp->os_npending >= se_write_iats)
		return;

	/*
	 * Device not busy.  Process the send queue.  Because of the 
	 * possibility of NFS-provided virtual memory buffers, the
	 * following TLB flush operation is necessary to ensure TLB
	 * coherency.
	 */

	flush_tlb();

	for (;;) {
		sipl = OS_LOCK(softp);
		if (softp->os_npending >= se_write_iats)
			break;
		if (ifq->ifq_len == 0)
			break;

		(void) IF_LOCK(ifq);
		IF_DEQUEUE(ifq, m);
		IF_UNLOCK(ifq, SPLIMP);
		if (m == (struct mbuf *)0)
			break;

		/*
		 * copy the packet into a se_xmit buffer
		 * NOTE BENE: ox_xfree buffers must be calloc() at
		 * boot time in order to guarantee virtual == physical
		 * addresses.
		 */

		xbuf = softp->os_xfree;

		if (xbuf == (struct se_xbuf *) NULL) {

			/*
			 * there are as many xmit buffers as there
			 * are se_write_bufs - if out of transmit
			 * buffers, something is BROKEN?
			 */

			printf("%s%d out of xmit buffers?\n",
				se_driver.sed_name, softp-se_state);

			se_stop(softp);
			OS_UNLOCK(softp, sipl);
			return;
		}

		softp->os_xfree = xbuf->se_xnext;

		/*
		 * os_npending reflects number of packets
		 * committed to queue
		 */

		softp->os_npending++;

		OS_UNLOCK(softp, sipl);

		{
		  register struct mbuf *n;
		  register u_char *cp;

		  len = 0;

		  for (cp = (u_char *) xbuf, n = m; n;  n = n->m_next)
		  {
			if ((len += n->m_len) > FULLETHER)
				break;
			bcopy(mtod(n, caddr_t), (caddr_t)cp, (u_int)n->m_len);
			cp += n->m_len;
		  }

		  if (n) {

			/*
			 * packet too big - should not happen!
			 * throw it away
			 */

			se_toobig++;
			sipl = OS_LOCK(softp);
			softp->os_npending--;
			xbuf->se_xnext = softp->os_xfree;
			softp->os_xfree = xbuf;
			OS_UNLOCK(softp, sipl);
			m_freem(m);
			continue;
	          }

		  if (len < ETHERMIN+sizeof(struct ether_header))
			len = ETHERMIN+sizeof(struct ether_header);
		}

		sipl = OS_LOCK(softp);

		ASSERT_DEBUG((pq->pq_head + 1) % sq->sq_size != pq->pq_tail,
			"se_start: head+1");

		softp->ss_arp.ac_if.if_obytes += len;

		/*
		 * If the packet is a multicast or broadcast
		 * packet, place an indicator in the dp_cmd[]
		 * so that the firmware knows to turn off the
		 * receiver.  The SCSI/Ether firmware can't look
		 * at the packet itself, as the mbuf might not
		 * be within its 4MB window.
		 */

		{
		  register struct sec_dev_prog *dp;

		  dp = pq->pq_un.pq_progs[pq->pq_head];
		  dp->dp_un.dp_data = (u_char *)xbuf;
		  dp->dp_data_len = len;
		  dp->dp_cmd_len = 0;
		  dp->dp_next = (struct sec_dev_prog *)0;
		  dp->dp_cmd[0] = SCSI_ETHER_WRITE;
		  if ((((struct ether_header *)xbuf)->ether_dhost[0] & 0x01)
		   || (softp->ss_ether_flags == SETHER_PROMISCUOUS)) {
			dp->dp_cmd[1] = SCSI_ETHER_MULTICAST;
			softp->ss_arp.ac_if.if_onunicast++;
		  } else {
			dp->dp_cmd[1] = SCSI_ETHER_STATION;
			softp->ss_arp.ac_if.if_opackets++;
		  }
		}

		MODINCR(pq->pq_head, sq->sq_size);

		if (sec_start_prog(SINST_STARTIO, softp->os_cib,
				   softp->ss_slic, SDEV_ETHERWRITE)
		    != SEC_ERR_NONE) {
			printf("%s%d: se_start: status 0x%x\n",
				se_driver.sed_name, softp-se_state,
				softp->os_status);
		}

		OS_UNLOCK(softp, sipl);

#ifdef PROMISCUOUS

		if (promiscon) {
			struct mbuf * xm;
			struct promiscif * xpm;
			spl_t	splevel;

			/*
			 * manage monitor receipt of transmitted packets.
			 */

			xm = m_getm(M_DONTWAIT, MT_DATA, 1);
			splevel = IF_LOCK(&promiscq);
			if (!xm || IF_QFULL(&promiscq)) {
				IF_DROP(&promiscq);
				IF_UNLOCK(&promiscq, splevel);
				m_freem(m);
				if (xm) (void) m_free(xm);
			} else {
				xm->m_next = m;
				xm->m_len = sizeof(struct promiscif);
				xpm = mtod(xm, struct promiscif *);
				xpm->promiscif_ifnet = (caddr_t) softp;
				xpm->promiscif_flag = PROMISC_XMIT;

				IF_ENQUEUE(&promiscq, xm);
				if (!promiscq.ifq_busy)
					schednetisr(NETISR_PROMISC);
				IF_UNLOCK(&promiscq, splevel);
			}

		} else		/* not monitoring */

#endif PROMISCUOUS
			m_freem(m);
	}

	OS_UNLOCK(softp, sipl);
}

/*
 * se_ioctl
 */

static
se_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	register struct se_state *softp = &se_state[ifp->if_unit];
	spl_t sipl;

	switch (cmd) {

	case SIOCSIFADDR:
		se_init(ifp->if_unit);
		sipl = SS_LOCK(softp);
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr.sa_family) {
		case AF_INET:	
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			SS_UNLOCK(softp, sipl);
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
#ifdef AT
			segetatnode(ifp);
#endif AT
			break;
		default:
			SS_UNLOCK(softp, sipl);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP)
			se_init(ifp->if_unit);
		break;

	default:

#ifdef PROMISCUOUS
		if (promiscdev)
		   return((*cdevsw[major(promiscdev)].d_ioctl)(ifp, cmd, data));
		else
			return(EINVAL);
#else
			return(EINVAL);
#endif PROMISCUOUS
	}
	return(0);
}

/*
 * se_watch - watchdog routine, request statistics from board.
 *
 * The cib's status pointer must have an address that is physical == virtual,
 * and must reside within the SEC's 4MB window.
 */

static
se_watch(unit)
	int unit;
{
	register struct se_state *softp = &se_state[unit];
	register struct sec_cib *cib;
	struct ifnet *ifp = &softp->ss_arp.ac_if;
	int *saved_status;
	spl_t sipl;

	if (unit < 0 || unit > se_max_unit)
		return;

	sipl = OS_LOCK(softp);
	cib = softp->os_cib;
	saved_status = cib->cib_status;
	cib->cib_status = (int *)&softp->os_gmode;

	if (sec_start_prog(SINST_GETMODE, cib, softp->ss_slic, SDEV_ETHERWRITE)
	    != SEC_ERR_NONE) {
		printf("%s%d: se_watch: status 0x%x\n",
			se_driver.sed_name, softp-se_state,
			softp->os_gmode.gm_status);
	}

	cib->cib_status = saved_status;

	(void) SS_LOCK(softp);

#define INCR(field1, field2) \
	softp->ss_sum.field1 += softp->os_gmode.gm_un.gm_ether.field2

	INCR(ec_rx_ovfl, egm_rx_ovfl);
	INCR(ec_rx_crc, egm_rx_crc);
	INCR(ec_rx_dribbles, egm_rx_dribbles);
	INCR(ec_rx_short, egm_rx_short);
	INCR(ec_rx_good, egm_rx_good);

	INCR(ec_tx_unfl, egm_tx_unfl);
	INCR(ec_tx_coll, egm_tx_coll);
	INCR(ec_tx_16xcoll, egm_tx_16x_coll);
	INCR(ec_tx_good, egm_tx_good);
#undef INCR

	softp->ss_arp.ac_if.if_ierrors +=
		softp->os_gmode.gm_un.gm_ether.egm_rx_ovfl
		+ softp->os_gmode.gm_un.gm_ether.egm_rx_crc
		+ softp->os_gmode.gm_un.gm_ether.egm_rx_dribbles;
	softp->ss_arp.ac_if.if_oerrors +=
		softp->os_gmode.gm_un.gm_ether.egm_tx_unfl;

#define X softp->ss_sum
	softp->ss_arp.ac_if.if_collisions = X.ec_tx_coll;
#undef X

	ifp->if_timer = softp->ss_scan_int;
	SS_UNLOCK(softp, SPLIMP);
	OS_UNLOCK(softp, sipl);
}



/*
 * reset: not necessary on sequent hardware.
 */

static
se_reset()
{
	panic("se_reset");
}

/*
 * se_stop() - something detected very broken - stop this ether
 * rather than panic.
 */

se_stop(softp)
	register struct se_state *softp;
{
	printf("%s%d Ethernet stopped - reboot advised\n",
		se_driver.sed_name, softp-se_state);

	softp->ss_alive = 0;
	return;
}

/*
 * se_set_modes - set the Ethernet modes based upon the soft state.
 *
 * Called with all pieces of the state locked.
 *
 * When we do the SINST_SETMODE, we use the get_mode structure
 * in the output state.  This is fair as everyone else is locked
 * out and the first part of the get_mode structure is a set_mode
 * piece.
 */

se_set_modes(softp)
	register struct se_state *softp;
{
	register struct sec_ether_smodes *esm;
	register struct sec_cib *cib = softp->os_cib;
	int *saved_status = cib->cib_status;

	cib->cib_status = (int *)&softp->os_gmode;

	esm = &softp->os_gmode.gm_un.gm_ether.egm_sm;
	bcopy((caddr_t)softp->ss_arp.ac_enaddr, (caddr_t)esm->esm_addr, 6);
	esm->esm_flags = softp->ss_ether_flags;
	esm->esm_size = MCLEN;

	if (sec_start_prog(SINST_SETMODE, cib, softp->ss_slic, SDEV_ETHERWRITE)
	    != SEC_ERR_NONE) {
		printf("%s%d: se_set_mode: status 0x%x\n",
			se_driver.sed_name, softp-se_state,
			softp->os_gmode.gm_status);
	}
	cib->cib_status = saved_status;
}

#ifdef	AT

/*
 *  pick the host's AppleTalk node number dynamically.
 */

#define AT_NTRIES	 5
#define	AT_MAXNODES	254

segetatnode(ifp)
	register struct ifnet *ifp;
{
	u_char edest[6];
	static short trying = 2;
	static u_char current[4];
	static times = 0;
	int	usetrailers;

	if (!ifpgetnode(ifp)) {
again:
		current[3] = trying;

		/*
		 * careful - very ugly type casting - assumes struct
		 * ifnet is overlayed on struct arp_com which is is
		 * in struct se_state
		 */

		if (arpresolve((struct arpcom *)ifp,
				 (struct mbuf *) NULL,
				  (struct in_addr *) current,
				   (u_char *) edest, &usetrailers)) {

			if (trying++ > AT_MAXNODES)
				return;			/* no luck */
			times = 0;
			goto again;  /* someone is this node already */
		}
		if (++times >= AT_NTRIES) {   /* assume have an ok node number*/
			times = 0;
			ifpsetnode(ifp, current[3]);
		} else
			timeout(segetatnode, (caddr_t)ifp, hz);
	}
}
#endif AT
