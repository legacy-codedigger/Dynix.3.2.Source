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

#ifndef	lint
static	char	rcsid[] = "$Header: promisc.c 2.18 1991/05/14 16:44:19 $";
#endif

/*
 * promisc.c 
 *	promiscuous queue handler
 */

/* $Log: promisc.c,v $
 *
 */

#undef	RAW_ETHER	/* no raw-ether kernel */
#define	RAW_ETHER	/* raw-ether kernel */

#define AT		/* APPLETALK */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/mbuf.h"
#include "../h/vmmac.h"
#include "../h/vm.h"
#include "../h/conf.h"

#include "../net/if.h"
#include "../net/af.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/if_ether.h"
#include "../netif/pci.h"
#include "../net/promisc.h"

#ifdef RAW_ETHER
#include "../net/raw_cb.h"
#endif RAW_ETHER

#ifdef AT
#include "../netat/atalk.h"
#include "../netat/katalk.h"
#endif AT

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"

#include "../sec/sec.h"

#include "../netif/if_se.h"

extern struct se_state *se_state;	/* pointer to array of soft states */

struct mbuf * se_reorder_trailer_packet();

#define wbufp (pp->promisc_wbufp)
#define wbuf_begin (pp->promisc_wbuf_begin)
#define wbuf_end (pp->promisc_wbuf_end)

extern int	ifqmaxlen;

struct	ifqueue	promiscq;		/* promiscuous packet input queue */
short promiscon = 0;			/* promiscuous mode off/on */
dev_t promiscdev = 0;
sema_t promisc_sema;
struct promiscstat *promiscstatp = 0;
int promiscbufs;
int promiscstatpref = 0;
int wbsize;
short recognized;

u_char SequentEaddr[3] = {0x08, 0x00, 0x47};	/* Sequent Eaddr's */

/*
 * a recognizazble, nonpalindrone, byte-order sensitive pattern
 */

u_char feedface[8] = {0xFE, 0xED, 0xFA, 0xCE, 0xDE, 0xAD, 0xBE, 0xEF};

n_time promisc_time();

extern struct custom_client custom_clients[];

#define	OFF	0
#define ON	1

/*
 * allow PROMISCRACE packets to be received in promiscuous mode after
 * turning it off - this avoids receiving unexpected packets and after
 * this many packets, avoid overhead of ether address compare on every
 * packet.
 */

short	PROMISCRACE = 300;
short	promiscrace = 0;

promiscread()
{
	register struct promiscstat *pp = promiscstatp;
	struct mbuf		*m;
	struct ether_header	*hp;
	struct se_state		*softp;
	struct ifnet		*ifp;
	struct mbuf 		*mnew;
	struct mbuf 		*n;
	struct ifqueue		*inq;
	struct promiscif 	*mp;
	u_char			*cp;
	int			int_to_sched;
	spl_t			splevel;
	short 			packet_len;
	short			pflag;
#ifdef RAW_ETHER
	struct raw_header 	*rh;
	struct mbuf 		*mrh;
#endif RAW_ETHER
	int			trailer = 0;
	int			ci;

	/*
	 * Get next datagram off input queue.
	 */

	splevel = IF_LOCK(&promiscq);

	promiscstatpref++;

	if (promiscq.ifq_busy) {	/* another processor is doing queue */
		promiscstatpref--;
		IF_UNLOCK(&promiscq, splevel);
		return;	
	}
	promiscq.ifq_busy = 1;
	goto skiplock;
next:
	splevel = IF_LOCK(&promiscq);
skiplock:

	/*
	 * if queue is empty, IF_DEQUEUE unbusies queue and m == null
	 */

	IF_DEQUEUE(&promiscq, m);
	if (m == (struct mbuf *) NULL){ 	/* no more in input queue */
		promiscstatpref--;
		IF_UNLOCK(&promiscq, splevel);
		return;
	}
	if (promiscon == 0) {		/* race between driver and user */
		m_freem(m);
		goto skiplock;
	}
	IF_UNLOCK(&promiscq, splevel);

	/*
	 * softp -> se_state which is a struct based on interface
	 */

	mp = mtod(m, struct promiscif *);
	softp = (struct se_state *) mp->promiscif_ifnet;
	ifp = &softp->ss_arp.ac_if;
	pflag = mp->promiscif_flag;
	m = m_free(m);	 /* can release the mbuf right away */
	hp = mtod(m, struct ether_header *);
	pp->promisc_packets++;

	/*
	 * insert the packet into a circular buffer.
	 * (i.e. before adjusting data pointers past header and byte
	 * swapping the type).
	 *
	 * copy even if not kept to compute packet_len and filter
	 */

	packet_len = 0;
	for (cp = (u_char *) wbufp, n = m; n != 0;  n = n->m_next) {
		bcopy(mtod(n, caddr_t), (caddr_t)cp, (u_int)n->m_len);
		cp += n->m_len;
		packet_len += n->m_len;
	}

	/*
	 * move feedface to end of kept packet to be recognized
	 */

	bcopy((caddr_t)feedface, (caddr_t)cp, (u_int) 8);

	wbufp->wbuf_ifnet = (caddr_t) softp;
	wbufp->wbuf_unit = softp->ss_arp.ac_if.if_unit;
	wbufp->wbuf_len = packet_len;
	wbufp->wbuf_time = promisc_time();

	if(promisc_filter((struct e_packet *) wbufp) != 0){ /* keep? */

		/*
		 * keep by incrementing wbufp
		 */

		if(++wbufp >= (struct wbuf *) wbuf_end) wbufp = wbuf_begin;
		pp->promisc_kept++;
		pp->promisc_bytes_kept += packet_len;
	}

	/*
	 * byte swap type field
	 */

	hp->ether_type = ntohs((u_short)hp->ether_type);
	
	/*
	 * strip ether_header from chain
	 */

	m->m_off += sizeof(struct ether_header);
	m->m_len -= sizeof(struct ether_header);

	if(bcmp((char *)SequentEaddr, (char *)hp->ether_shost, 3) == 0)
		pp->promisc_sequentE++;

	/*
	 * maintain "bucket" counts of packet lengths
	 */

	if(packet_len <= 64) pp->buck[0]++;
	else if(packet_len <= 128) pp->buck[1]++;
	else if(packet_len <= 256) pp->buck[2]++;
	else if(packet_len <= 512) pp->buck[3]++;
	else if(packet_len <= 1024) pp->buck[4]++;
	else pp->buck[5]++;

	pp->promisc_bytes += packet_len;
	if(pp->promisc_bytes > 1000000000) {
		pp->promisc_beelions++;
		pp->promisc_bytes -= 1000000000;
	}

	/*
	 * determine if packet is really intended for this station
	 */

	if (bcmp((char *)etherbroadcastaddr, (char *)hp->ether_dhost, 6) == 0)
		pp->promisc_bcast++;
	else if(bcmp((char *)softp->ss_arp.ac_enaddr,
		(char *)hp->ether_dhost, 6) == 0)
		pp->promisc_station++;
	else if(bcmp((char *)loopbackfakeaddr,
		(char *)hp->ether_dhost, 6) == 0)
		   pp->promisc_loopback++;
	else {		 /* not for me */
		m_freem(m);
		pp->promisc_reject++;
		goto next;
	}

	if(pflag == PROMISC_XMIT){ /* packet from transmitter? */
		m_freem(m);
		goto next;
	}

	/*
	 * packet is for me -
	 *
	 * check for 4.2bsd's trailer protocol and if trailer, unravel
	 */

	if (hp->ether_type >= ETHERPUP_TRAIL
	    && hp->ether_type < ETHERPUP_TRAIL + ETHERPUP_NTRAILER) {
		mnew = se_reorder_trailer_packet(hp, m);
		if (mnew == (struct mbuf *)0) {
			m_freem(m);
			return;
		}
		m = mnew;
		trailer++;
	}

	switch (hp->ether_type) {
	case ETHERPUP_IPTYPE:
		int_to_sched = NETISR_IP;
		inq = &ipintrq;
		break;

	case ETHERPUP_ARPTYPE:
		arpinput(&softp->ss_arp, m); /* arpinput handles m */
		goto next;

	case PCI_TYPE:
		if (pcirint_fctn)
			(*pcirint_fctn)(hp, m);
		else
			m_freem(m);
		goto next;

#ifdef AT
	case ETHERPUP_ATALKTYPE:
		int_to_sched = NETISR_DDP;
		inq = &ddpintq;
		break;
#endif AT

	default:

		pp->promisc_unk++;

		/*
		 * do not queue reordered trailer to rawif or custom
		 */

		if(trailer) {
			m_freem(m);
			goto next;
		}

		/*
		 * allow for custom ether_read device drivers
		 */

		for(ci = 0; ci < 4; ci++) {
			if(custom_clients[ci].custom_devno
			   && custom_clients[ci].custom_type == hp->ether_type)

			{

	    ASSERT(cdevsw[major(custom_clients[ci].custom_devno)].d_read,
			"no custom_client cdevsw.d_read!");

		        (*cdevsw[major(custom_clients[ci].custom_devno)].d_read)
				(hp, m, (caddr_t) ifp);

		  	custom_clients[ci].custom_count++;
		  	goto next;
		 	}
		}

#ifdef RAW_ETHER

		int_to_sched = NETISR_RAW;
		inq = &rawif.if_snd;
		mrh = m_getclrm(M_DONTWAIT, MT_DATA, 1);
		if(mrh == (struct mbuf *) NULL) {
			m_freem(m);
			goto next;
		}

		/*
		 * reput the ether header into the lead data buffer
		 * *and* copy a Unix4.2 raw_header for compatibility
		 */
	
		m->m_off -= sizeof(struct ether_header);
		m->m_len += sizeof(struct ether_header);
	
		/*
		 * link the raw_header into the ether packet for 4.2
		 * compatibility (?)
		 *
		 * set up raw header, using type as sa_data for bind.
		 * 	- for now assign NULL for protocol
		 * 	- copy AF_RAWE and ether_type in for dst addr
		 */

		/*
		 * make room for ifnet* - ifp passing
		 */

		mrh->m_off += sizeof(struct ifnet*);

		mrh->m_next = m;
		mrh->m_len = sizeof(struct raw_header);
		m = mrh;
		rh = mtod(mrh, struct raw_header*);
		rh->raw_proto.sp_family = AF_RAWE;
		rh->raw_proto.sp_protocol = AF_UNSPEC;
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

		bcopy((caddr_t)&softp->ss_arp.ac_if.if_unit,
			(caddr_t)&rh->raw_src.sa_data[2], sizeof(short));

#else		/* not RAW_ETHER */

		m_freem(m);
		goto next;

#endif RAW_ETHER

	}	/* end switch */

	/*
	 * packet is for me (either station or broadcast) && it is a
	 * recognized type 
	 * inq points to the appropriate protocol engine queue (viz. &ipintrq),
	 * and int_to_sched is the appropriate NETISR_ (viz. NETISR_IP)
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
			softp->ss_arp.ac_if.if_ierrors++;
			m_freem(m);
			goto next;
		}
		mm->m_off = MMINOFF;
		mm->m_len = sizeof(struct ifnet *);
		mm->m_next = m;
		m = mm;
	}

	*(mtod(m, struct ifnet **)) = ifp;

	splevel = IF_LOCK(inq);
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		IF_UNLOCK(inq, splevel);
		m_freem(m);
		goto next;
	}
	IF_ENQUEUE(inq, m);
	if (!inq->ifq_busy) {
		schednetisr(int_to_sched);
	}
	IF_UNLOCK(inq, splevel);
	goto next;
}

promiscboot()	
{
	if(promiscdev)
		return;
	promiscdev = makedev(PROMISC_DEV, 0);
	promiscq.ifq_maxlen = ifqmaxlen;
	init_lock(&promiscq.ifq_lock, G_IFNET);

	/*
	 * semaphore prevents simultaneous ioctl's
	 */

	init_sema(&promisc_sema, 1, 0, G_SOCK);
	return;
}

promisc_filter(wptr)
	register struct e_packet * wptr;
{
	int keepit = promiscstatp->promisc_keepall;
	u_short type;

	/*
	 * filter packets - return(1) if accept (0) if not
	 * these routines also gather stats
	 * keepit = number of keeps
	 */

	recognized = 0;			/* don't know yet */
	type = ntohs(wptr->ether_type);	/* in *net* order */
	if(type >= ETHERPUP_TRAIL
	    && type < ETHERPUP_TRAIL + ETHERPUP_NTRAILER) {

		int trail_off;

		trail_off = (type - ETHERPUP_TRAIL)*512;
		if (trail_off == 512 || trail_off == 1024) {
			promiscstatp->promisc_trailers++;
			keepit += promiscstatp->promisc_keeptrail;
		}
	}
	keepit += filter_trap((struct e_packet *) wptr);
	recognized += keepit;		/* if kept then recognized */

	switch(type) {
	case ETHERPUP_ARPTYPE:
		recognized++;
		promiscstatp->promisc_arp++;
		keepit += promiscstatp->promisc_keeparp;
		break;
	case ETHERPUP_IPTYPE:
		recognized++;
		promiscstatp->promisc_ip++;
		if(wptr->ip_p == IPPROTO_UDP)
			promiscstatp->promisc_udp++;
		else if (wptr->ip_p == IPPROTO_TCP)
			promiscstatp->promisc_tcp++;
		else if (wptr->ip_p == IPPROTO_ICMP)
			promiscstatp->promisc_icmp++;
		break;
	case XNS_TYPE:
		recognized++;
		promiscstatp->promisc_xns++;
		if(promiscstatp->promisc_keepxns) keepit++;
		break;
	case ETHERPUP_ATALKTYPE:
		recognized++;
		promiscstatp->promisc_at++;
		if(promiscstatp->promisc_keepat) keepit++;
		break;
	default:
		promiscstatp->promisc_unk++;
		keepit += promiscstatp->promisc_keepbogus;
		break;
	}
	if (bcmp((char *)etherbroadcastaddr,
		(char *)wptr->ether_dhost, 6) == 0) {
		keepit += promiscstatp->promisc_bkeep;
		if(ntohs(wptr->uh_sport) == RWHOPORT) {
			promiscstatp->promisc_rwho++;
		}
	}
	return(keepit);
}

filter_trap(wptr)
	register struct e_packet * wptr;
{
	u_char dhost4;
	u_char dhost5;
	u_char shost4;
	u_char shost5;
	u_char phost4;
	u_char phost5;
	int i;

	if(!promiscstatp->promisc_trapping)
		return(0);

	dhost5 = wptr->ether_dhost[5];
	dhost4 = wptr->ether_dhost[4];
	shost5 = wptr->ether_shost[5];
	shost4 = wptr->ether_shost[4];

	for(i = 0; i < promiscstatp->promisc_trapx && i < TRAPEES; i++) {
		phost5 = promiscstatp->promisc_trapees[i].eaddr[5];
		phost4 = promiscstatp->promisc_trapees[i].eaddr[4];
		if((dhost5 == phost5 || shost5 == phost5) &&
			(dhost4 == phost4 || shost4 == phost4))
			return(1);
	}
	return(0);
}

n_time
promisc_time()
{
	u_long t;

	/*
	 * NOTE: reading time is not gated
	 */

	t = (time.tv_sec % (24*60*60)) * 1000 + time.tv_usec / 1000;
	return (htonl(t));
}

/*
 * [OS]S_LOCK/UNLOCK yanked from ../netif/if_se.c
 */

/*
 * lock the controller state
 */

#define	SS_LOCK(softp)	(p_lock(&(softp)->ss_lock, SPLIMP))
#define	SS_UNLOCK(softp, sipl)	(v_lock(&(softp)->ss_lock, sipl))

/*
 * lock the output state
 */

#define	OS_LOCK(softp)	(p_lock(&(softp)->os_lock, SPLIMP))
#define	OS_UNLOCK(softp, sipl)	(v_lock(&(softp)->os_lock, sipl))

promiscioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifreq *ifr = (struct ifreq *)data;
	register struct se_state *softp = &se_state[ifp->if_unit];
	int error = 0;

	if (!suser())
		return (u.u_error);
	if(!promiscdev)
		return(EINVAL);
	ifr = (struct ifreq *)data;

	p_sema(&promisc_sema, PZERO);

	switch (cmd) {

	case SIOCIFPRON:

		/*
		 * SIOCIFPRON -> getwbuf if available, and increment
		 * monitoring flag. set interface into promisc.
		 * 	- setup wraparound buffer
		 *	- set modes of specified interface
		 */

		error = promisc_getwbuf(ifr);
		if(!error)
			(void) promisc_setmodes(softp, ON);
		break;

	case SIOCIFPROFF:

		/*
		 * SIOCIFPROFF -> dereference wraparound and monitoring.
		 */

		(void) promisc_setmodes(softp, OFF);
		(void) promisc_retwbuf();
		break;

	case SIOCIFPRMON:

		/*
		 * SIOCIFPRMON -> start monitoring, get wbufs
		 */

		error = promisc_getwbuf(ifr);
		break;

	case SIOCIFPRMOFF:

		/*
		 * SIOCIFPROFF -> dereference wbufs and decrement monitor.
		 */

		(void) promisc_retwbuf();
		break;

	default:
		error = EINVAL;
	}
	v_sema(&promisc_sema);
	return(error);
}

promisc_getwbuf(ifr)
	struct ifreq * ifr;
{
	register struct promiscstat * pp;

	/*
	 * can set up wraparound buffers once without
	 * reseting, specify number of wbufs in ioctl,
	 * memory allocated in clicks, all accepted packets
	 * go into same wraparound, need to do an ioctl for
	 * each interface to be promiscuous.
	 */

	if (!promiscstatp) {	 	/* do not have a wraparound yet */
		promiscbufs = WNUMBUFS;	/* default */
		if (ifr->ifr_wbsize)	/* #wbufs specified in ioctl */
			promiscbufs = ifr->ifr_wbsize;
		wbsize = promiscbufs * (sizeof(struct wbuf)) + NBPG;
		promiscstatp = (struct promiscstat *)wmemall(wbsize, 0);
		if (!promiscstatp) {
			printf("no memory for promiscuous\n");
			return(ENOBUFS);
		}

		/*
		 * start wbuf on the next page
		 *
		 * NOTE wbuf_... expands to pp->promisc_wbuf_..
		 */

		pp = promiscstatp;
		bzero((caddr_t)pp, NBPG);
		wbuf_begin = (struct wbuf *) ((int) pp + NBPG);
		wbuf_end = (struct wbuf *)((int) wbuf_begin
			+ (promiscbufs * (sizeof(struct wbuf))));
		wbufp = wbuf_begin;
		printf("PROMISCUOUS queue initialized %x at 0x%x\n",
			wbsize, promiscstatp);
	}

	promiscon++;
	promiscstatpref++;
	return(0);
}

promisc_setmodes(softp, flag)
	struct se_state * softp;
	int flag;
{
	int ss_flag = 0;
	spl_t sipl;

	switch(flag) {

	case ON:

		/*
		 * turn promiscuous mode on.  set up SEC to accept
		 * promiscuous packets.  This results in all packets
		 * being received and IP packets demultiplexed by
		 * promiscintr.  It is possible to monitor
		 * without being promiscuous.
		 */

		ss_flag = SETHER_PROMISCUOUS;
		break;

	case OFF:

		/*
		 * turn promiscuous mode off.
		 * race with promiscintr is experimentally solved by allowing
		 * PROMISCRACE packets to be received during which every
		 * packet is checked for appropriate ether_dhost -
		 * afterwards we don't bother for performance reasons.
		 */

		ss_flag = SETHER_S_AND_B;
		promiscrace = PROMISCRACE;
		break;

	default:
		return(EINVAL);
	}

	sipl = OS_LOCK(softp);
	(void) SS_LOCK(softp);

	if(softp->ss_ether_flags != ss_flag) {
		softp->ss_ether_flags = ss_flag;
		se_set_modes(softp);
	}

	SS_UNLOCK(softp, SPLIMP);
	OS_UNLOCK(softp, sipl);
	return (0);
}

promisc_retwbuf()
{
	/*
	 * promiscon is flag used to detect monitoring state.
	 * What about multiple
	 * SCEDs, where one is turned off, but not the other?
	 * Possible solution is to have if_se check particular
	 * interface for promiscuous mode rather than flag.
	 * now has implications for nonpromiscuous monitoring
	 */

	if(promiscon)
		promiscon--;

	if(promiscstatpref && --promiscstatpref == 0) {

		/*
		 * no other references to the wbufs so can return the
		 * memory.
		 */

		if(promiscstatp)
			wmemfree((caddr_t) promiscstatp, wbsize);
		promiscstatp = 0;
		wbsize = 0;
		promiscbufs = 0;
		printf("no longer promiscuous\n");
	}
}

promiscintr()
{
	if (promiscdev)
		(*cdevsw[major(promiscdev)].d_read)();
	else
		printf("promiscuous not configured\n");
}
