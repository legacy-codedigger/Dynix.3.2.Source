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
static	char	rcsid[] = "$Header: if_ether.c 2.16 1991/05/10 23:09:27 $";
#endif

/*
 * if_ether.c
 *	Ethernet address resolution protocol (ARP)
 */

/* $Log: if_ether.c,v $
 *
 *
 */

/*
 * Ethernet address resolution protocol.
 */

#define AT

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/errno.h"
#include "../h/ioctl.h"

#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/if_ether.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"

#ifdef AT
/*
 * The Kinetics implementation of Appletalk of Ethernet uses
 * a perverted form of Internet arp.  To map Appletalk node address  (1 byte) 
 * to Ethernet addresses, an arp request with net of zero and host of the
 * appletalk node.
 */
#include "../h/socketvar.h"
#include "../netat/atalk.h"
#include "../netat/katalk.h"
#define	ATF_ISAT	0x80	/* this is an appletalk segment */

extern	int kin_atalk_arp;	/* Kinetics Appletalk Arp kludge */
#define ISATIP(addr)	((ntohl(addr) & 0xffffff00) == 0)
#endif AT

#define		ARPTAB_BSIZ	arptab_bsiz
#define		ARPTAB_NB	arptab_nb
#define		ARPTAB_SIZE	(arptab_bsiz * arptab_nb)
extern	struct arptab arptab[];
extern	int arptab_size;
extern	int arptab_bsiz;
extern	int arptab_nb;

/*
 * ARP trailer negotiation.  Trailer protocol is not IP specific,
 * but ARP request/response use IP addresses.
 */

#define ETHERTYPE_IPTRAILERS ETHERTYPE_TRAIL

#define	ARPTAB_HASH(a) \
	((u_long)(a) % ARPTAB_NB)

#define	ARPTAB_LOOK(at,addr) { \
	register n; \
	at = &arptab[ARPTAB_HASH(addr) * ARPTAB_BSIZ]; \
	for (n = 0 ; n < ARPTAB_BSIZ ; n++,at++) \
		if (at->at_iaddr.s_addr == addr) \
			break; \
	if (n >= ARPTAB_BSIZ) \
		at = 0; }

#ifdef DEBUG
int	if_ether_debug = 0;	/* 0 == none, 1 == some, 2 == all */
#endif DEBUG

/*
 * timer values
 */

#define	ARPT_AGE	(60*1)	/* aging timer, 1 min. */
#define	ARPT_KILLC	20	/* kill completed entry in 20 mins. */
#define	ARPT_KILLI	3	/* kill incomplete entry in 3 minutes */

/*
 * lock used to gate all structures.
 */

lock_t	arp_lock;

#define ARP_LOCK()	(p_lock(&arp_lock, SPLIMP))
#define ARP_UNLOCK(s)	(v_lock(&arp_lock, s))


u_char	etherbroadcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
extern struct ifnet loif[];

/*
 * Initialize ARP data structures.
 * Presently, this just requires that we initialize
 * the global lock.
 */

arp_init()
{
	init_lock(&arp_lock, G_IFNET);
}

/*
 * Timeout routine.  Age arp_tab entries once a minute.
 */
arptimer()
{
	register struct arptab *at;
	register i;
	spl_t s;

	s = ARP_LOCK();
	timeout(arptimer, (caddr_t)0, ARPT_AGE * hz);
	at = &arptab[0];
	for (i = 0; i < ARPTAB_SIZE; i++, at++) {
		if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
				continue;
		if ((int)(++at->at_timer) < ((at->at_flags&ATF_COM) ?
		    ARPT_KILLC : ARPT_KILLI))
			continue;
		/* timer has expired, clear entry */
		arptfree(at);
	}
	ARP_UNLOCK(s);
}

/*
 * Broadcast an ARP packet, asking who has addr on interface ac.
 */
arpwhohas(ac, addr)
	register struct arpcom *ac;
	struct in_addr *addr;
{
	register struct mbuf *m;
	register struct ether_header *eh;
	register struct ether_arp *ea;
	struct sockaddr sa;

	if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	m->m_len = sizeof *ea;
	m->m_off = MMAXOFF - m->m_len;
	ea = mtod(m, struct ether_arp *);
	eh = (struct ether_header *)sa.sa_data;
	bzero((caddr_t)ea, sizeof (*ea));
	bcopy((caddr_t)etherbroadcastaddr, (caddr_t)eh->ether_dhost,
	   sizeof (etherbroadcastaddr));
	eh->ether_type = ETHERTYPE_ARP;		/* if_output will swap */
	ea->arp_hrd = htons((u_short)ARPHRD_ETHER);
	ea->arp_pro = htons((u_short)ETHERTYPE_IP);
	ea->arp_hln = sizeof(ea->arp_sha);	/* hardware address length */
	ea->arp_pln = sizeof(ea->arp_spa);	/* protocol address length */
	ea->arp_op = htons((u_short)ARPOP_REQUEST);
	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
	   sizeof (ea->arp_sha));
#ifdef AT
	/*
	 * if this an appletalk address, then munge the outgoing packet
	 */

	if (kin_atalk_arp && ISATIP(addr->s_addr)) {
		struct ifnet *ifp = &ac->ac_if;
		long spa = 0;

		bcopy((caddr_t)&spa, (caddr_t) ea->arp_spa,
				sizeof (ea->arp_spa));
		ea->arp_spa[3] = ifpgetnode(ifp);
	} else
#endif AT
	bcopy((caddr_t)&ac->ac_ipaddr,
	   (caddr_t)ea->arp_spa, sizeof (ea->arp_spa));
	bcopy((caddr_t)addr, (caddr_t)ea->arp_tpa, sizeof (ea->arp_tpa));
	sa.sa_family = AF_UNSPEC;
	(void) (*ac->ac_if.if_output)(&ac->ac_if, m, &sa);
}

/*
 * Resolve an IP address into an ethernet address.  If success, 
 * desten is filled in.  If there is no entry in arptab,
 * set one up and broadcast a request for the IP address.
 * Hold onto this mbuf and resend it once the address
 * is finally resolved.  A return value of 1 indicates
 * that desten has been filled in and the packet should be sent
 * normally; a 0 return indicates that the packet has been
 * taken over here, either now or for later transmission.
 *
 * We do some (conservative) locking here at splimp, since
 * arptab is also altered from input interrupt service (ecintr/ilintr
 * calls arpinput when ETHERTYPE_ARP packets come in).
 */
arpresolve(ac, m, destip, desten, usetrailers)
	register struct arpcom *ac;
	struct mbuf *m;
	register struct in_addr *destip;
	register u_char *desten;
	int *usetrailers;
{
	register struct arptab *at;
	register struct ifnet *ifp;
	struct sockaddr_in sin;
	int s, lna;

	*usetrailers = 0;
	if (in_broadcast(*destip)) {	/* broadcast address */
		bcopy((caddr_t)etherbroadcastaddr, (caddr_t)desten,
		   sizeof (etherbroadcastaddr));
		return (1);
	}
	lna = in_lnaof(*destip);
	ifp = &ac->ac_if;
	/* if for us, then use software loopback driver */
	if (destip->s_addr == ac->ac_ipaddr.s_addr) {
		if (loif[0].if_flags & IFF_UP) {
			sin.sin_family = AF_INET;
			sin.sin_addr = *destip;

			(void) looutput(&loif[0], m, (struct sockaddr *) & sin);
			/*
			 * The packet is sent and freed.
			 */
			return(0);
		} else {
			bcopy((caddr_t)ac->ac_enaddr, (caddr_t)desten,
			    sizeof(ac->ac_enaddr));
			return (1);
		}
	}
	s = ARP_LOCK();
	ARPTAB_LOOK(at, destip->s_addr);
	if(at == 0) {	/* not found */
		if (ifp->if_flags & IFF_NOARP) {
			bcopy((caddr_t)ac->ac_enaddr, (caddr_t)desten, 3);
			desten[3] = (lna >> 16) & 0x7f;
			desten[4] = (lna >> 8) & 0xff;
			desten[5] = lna & 0xff;
			ARP_UNLOCK(s);
			return (1);
		} else {
			at = arptnew(destip);
			at->at_hold = m;
#ifdef AT
			if (kin_atalk_arp && ISATIP(destip->s_addr)) 
				at->at_flags |= ATF_ISAT; /* this is an appletalk pkt */
#endif AT
			ARP_UNLOCK(s);
			arpwhohas(ac, destip);
			return (0);
		}
	}
	at->at_timer = 0;		/* restart the timer */
	if (at->at_flags & ATF_COM) {	/* entry IS complete */
		bcopy((caddr_t)at->at_enaddr, (caddr_t)desten, 
			sizeof(at->at_enaddr));
		if(at->at_flags & ATF_USETRAILERS)
			*usetrailers = 1;
		ARP_UNLOCK(s);
		return (1);
	}

	/*
	 * There is an arptab entry, but no ethernet address
	 * response yet.  Replace the held mbuf with this
	 * latest one.
	 */

	if (at->at_hold) {
		ifp->if_odiscards++;
		m_freem(at->at_hold);
	}
	at->at_hold = m;
	ARP_UNLOCK(s);
	arpwhohas(ac, destip);		/* ask again */
	return (0);
}

/*
 * Called from se_intr() when ether packet type ETHERTYPE_ARP
 * is received.  Algorithm is that given in RFC 826.
 * Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */

arpinput(ac, m)
	struct arpcom *ac;
	struct mbuf *m;
{
	register struct arphdr *ar;

	if (ac->ac_if.if_flags & IFF_NOARP)
		goto out;
	if (m->m_len < sizeof(struct arphdr))
		goto out;
	ar = mtod(m, struct arphdr *);
	if (ntohs(ar->ar_hrd) != ARPHRD_ETHER)
		goto out;
	if (m->m_len < sizeof(struct arphdr) + 2 * ar->ar_hln + 2 * ar->ar_pln)
		goto out;

	switch (ntohs(ar->ar_pro)) {

	case ETHERTYPE_IP:
	case ETHERTYPE_IPTRAILERS:
		in_arpinput(ac, m);
		return;

	default:
		break;
	}
out:
	m_freem(m);
}

/*
 * ARP for Internet protocols on 10 Mb/s Ethernet.
 * Algorithm is that given in RFC 826.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 * We also handle negotiations for use of trailer protocol:
 * ARP replies for protocol type ETHERTYPE_TRAIL are sent
 * along with IP replies if we want trailers sent to us,
 * and also send them in response to IP replies.
 * This allows either end to announce the desire to receive
 * trailer packets.
 * We reply to requests for ETHERTYPE_TRAIL protocol as well,
 * but don't normally send requests.
 */
in_arpinput(ac, m)
	register struct arpcom *ac;
	struct mbuf *m;
{
	register struct ether_arp *ea;
	struct ether_header *eh;
	register struct arptab *at;  /* same as "merge" flag */
	struct mbuf *mcopy = 0;
	struct sockaddr_in sin;
	struct sockaddr sa;
	struct in_addr isaddr, itaddr, myaddr;
	int proto, op;
	spl_t s;
#ifdef AT
	int	appletalk = 0;
#endif AT

	/*
	 * very conservative mutex.
	 */

	s = ARP_LOCK();

	myaddr = ac->ac_ipaddr;

	ea = mtod(m, struct ether_arp *);
	proto = ntohs(ea->arp_pro);
	op = ntohs(ea->arp_op);
	isaddr.s_addr = ((struct in_addr *)ea->arp_spa)->s_addr;
	itaddr.s_addr = ((struct in_addr *)ea->arp_tpa)->s_addr;
#ifdef AT

	/*
	 * high bytes of dest == 0's?  If so, use our appletalk addr.
	 */

	if (kin_atalk_arp && ISATIP(itaddr.s_addr)) {
		struct ifnet *ifp = &ac->ac_if;
		myaddr = itaddr;
		((u_char *)&myaddr)[3] = ifpgetnode(ifp);
		appletalk = 1;		/* its an appletalk ARP */
	}

#endif AT

	if (!bcmp((caddr_t)ea->arp_sha, (caddr_t)ac->ac_enaddr,
	  sizeof (ea->arp_sha)))
		goto out;	/* it's from me, ignore it. */
	if (!bcmp((caddr_t)ea->arp_sha, (caddr_t)etherbroadcastaddr,
	    sizeof (ea->arp_sha))) {
		printf("arp: ether address is broadcast for IP address %x!\n",
		    ntohl(isaddr.s_addr));
		goto out;
	}

	if (isaddr.s_addr == myaddr.s_addr) {

#ifdef AT
	   if(appletalk) 
		if (ntohs(ea->arp_op) == ARPOP_REQUEST)
			goto reply;
#endif AT

	   printf("duplicate IP address (%x)!! sent from ethernet address: ",
	            isaddr.s_addr);
	   printf("%x %x %x %x %x %x\n", ea->arp_sha[0], ea->arp_sha[1],
		    ea->arp_sha[2], ea->arp_sha[3],
		    ea->arp_sha[4], ea->arp_sha[5]);
		itaddr = myaddr;
		if (op == ARPOP_REQUEST)
			goto reply;
		goto out;
	}
	ARPTAB_LOOK(at, isaddr.s_addr);
	if (at) {
		bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
		    sizeof(ea->arp_sha));
		at->at_flags |= ATF_COM;
		if (at->at_hold) {
			struct mbuf *atheld = at->at_hold;
			at->at_hold = 0;
#ifdef AT
			if (at->at_flags & ATF_ISAT)
				sin.sin_family = AF_APPLETALK;
			else
#endif AT
				sin.sin_family = AF_INET;
			sin.sin_addr = isaddr;

			ARP_UNLOCK(s);

			(*ac->ac_if.if_output)(&ac->ac_if, 
			    atheld, (struct sockaddr *)&sin);

			s = ARP_LOCK();
		}
	}
	if (at == 0 && itaddr.s_addr == myaddr.s_addr) {
		/* ensure we have a table entry */
		at = arptnew(&isaddr);
		bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
		    sizeof(ea->arp_sha));
		at->at_flags |= ATF_COM;
	}
reply:
	switch (proto) {

	case ETHERTYPE_IPTRAILERS:
		/* partner says trailers are OK */
		if (at)
			at->at_flags |= ATF_USETRAILERS;
		/*
		 * Reply to request iff we want trailers.
		 */
		if (op != ARPOP_REQUEST || ac->ac_if.if_flags & IFF_NOTRAILERS)
			goto out;
		break;

	case ETHERTYPE_IP:
		/*
		 * Reply if this is an IP request, or if we want to send
		 * a trailer response.
		 */
		if (op != ARPOP_REQUEST && ac->ac_if.if_flags & IFF_NOTRAILERS)
			goto out;
	}
	if (itaddr.s_addr == myaddr.s_addr) {
		/* I am the target */
		bcopy((caddr_t)ea->arp_sha, (caddr_t)ea->arp_tha,
		    sizeof(ea->arp_sha));
		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
		    sizeof(ea->arp_sha));
	} else {
		ARPTAB_LOOK(at, itaddr.s_addr);
		if (at == NULL || (at->at_flags & ATF_PUBL) == 0)
			goto out;
		bcopy((caddr_t)ea->arp_sha, (caddr_t)ea->arp_tha,
		    sizeof(ea->arp_sha));
		bcopy((caddr_t)at->at_enaddr, (caddr_t)ea->arp_sha,
		    sizeof(ea->arp_sha));
	}

	bcopy((caddr_t)ea->arp_spa, (caddr_t)ea->arp_tpa,
	    sizeof(ea->arp_spa));
	bcopy((caddr_t)&itaddr, (caddr_t)ea->arp_spa,
	    sizeof(ea->arp_spa));
	ea->arp_op = htons(ARPOP_REPLY); 
	/*
	 * If incoming packet was an IP reply,
	 * we are sending a reply for type IPTRAILERS.
	 * If we are sending a reply for type IP
	 * and we want to receive trailers,
	 * send a trailer reply as well.
	 */
	if (op == ARPOP_REPLY)
		ea->arp_pro = htons(ETHERTYPE_IPTRAILERS);
	else if (proto == ETHERTYPE_IP &&
	    (ac->ac_if.if_flags & IFF_NOTRAILERS) == 0)
		mcopy = m_copy(m, 0, (int)M_COPYALL);
	eh = (struct ether_header *)sa.sa_data;
	bcopy((caddr_t)ea->arp_tha, (caddr_t)eh->ether_dhost,
	    sizeof(eh->ether_dhost));
	eh->ether_type = ETHERTYPE_ARP;
	sa.sa_family = AF_UNSPEC;

	ARP_UNLOCK(s);

	(*ac->ac_if.if_output)(&ac->ac_if, m, &sa);
	if (mcopy) {
		ea = mtod(mcopy, struct ether_arp *);
		ea->arp_pro = htons(ETHERTYPE_IPTRAILERS);
		(*ac->ac_if.if_output)(&ac->ac_if, mcopy, &sa);
	}
	return;
out:
	ARP_UNLOCK(s);
	m_freem(m);
	return;
}

/*
 * Free an arptab entry.
 *
 * Called while someone is holding the ARP lock.
 */
arptfree(at)
	register struct arptab *at;
{
	if (at->at_hold)
		m_freem(at->at_hold);
	at->at_hold = 0;
	at->at_timer = at->at_flags = 0;
	at->at_iaddr.s_addr = 0;
}

/*
 * Enter a new address in arptab, pushing out the oldest entry 
 * from the bucket if there is no room.
 * This always succeeds since no bucket can be completely filled
 * with permanent entries (except from arpioctl when testing whether
 * another permanent entry will fit).
 *
 * Called while someone is holding the ARP lock.
 */
struct arptab *
arptnew(addr)
	struct in_addr *addr;
{
	register n;
        int first_entry = 1;  
        int oldest;           
	register struct arptab *at, *ato = NULL;
	static int first = 1;

	if (first) {
		first = 0;
		timeout(arptimer, (caddr_t)0, hz);
	}

	at = &arptab[ARPTAB_HASH(addr->s_addr) * ARPTAB_BSIZ];
	for (n = 0 ; n < ARPTAB_BSIZ ; n++,at++) {
		if (at->at_flags == 0)
			goto out;	 /* found an empty entry */
		if (at->at_flags & ATF_PERM)
			continue;
                if (first_entry) {    /* initialize oldest */
                        first_entry = 0;
			oldest = at->at_timer;
			ato = at;
		}
                else {               /* compare oldest entry so far  */
		if ((int) at->at_timer > oldest) {
			oldest = at->at_timer;
			ato = at;
		      }
		}
	}
	if (ato == NULL)
		return(NULL);
	at = ato;
	arptfree(at);
out:
	at->at_iaddr = *addr;
	at->at_flags = ATF_INUSE;
	return (at);
}

arpioctl(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct arpreq *ar = (struct arpreq *)data;
	register struct arptab *at;
	register struct sockaddr_in *sin;
	int s;

	if (ar->arp_pa.sa_family != AF_INET ||
	    ar->arp_ha.sa_family != AF_UNSPEC)
		return (EAFNOSUPPORT);
	if (cmd == SIOCFARP) {
		arptflush();
		return(0);
	}
	sin = (struct sockaddr_in *)&ar->arp_pa;
	s = ARP_LOCK();
	ARPTAB_LOOK(at, sin->sin_addr.s_addr);
	if (at == NULL) {		/* not found */
		if (cmd != SIOCSARP) {
			splx(s);
			return (ENXIO);
		}
		if (ifa_ifwithnet(&ar->arp_pa) == NULL) {
			splx(s);
			return (ENETUNREACH);
		}
	}
	switch (cmd) {

	case SIOCSARP:		/* set entry */
		if (at == NULL) {
			at = arptnew(&sin->sin_addr);
			if (ar->arp_flags & ATF_PERM) {
			/* never make all entries in a bucket permanent */
				register struct arptab *tat;
				
				/* try to re-allocate */
				tat = arptnew(&sin->sin_addr);
				if (tat == NULL) {
					arptfree(at);
					splx(s);
					return (EADDRNOTAVAIL);
				}
				arptfree(tat);
			}
		}
		bcopy((caddr_t)ar->arp_ha.sa_data, (caddr_t)at->at_enaddr,
		    sizeof(at->at_enaddr));
		at->at_flags = ATF_COM | ATF_INUSE |
			(ar->arp_flags & (ATF_PERM|ATF_PUBL));
		at->at_timer = 0;
		break;

	case SIOCDARP:		/* delete entry */
		arptfree(at);
		break;

	case SIOCGARP:		/* get entry */
		bcopy((caddr_t)at->at_enaddr, (caddr_t)ar->arp_ha.sa_data,
		    sizeof(at->at_enaddr));
		ar->arp_flags = at->at_flags;
		break;
	}
	ARP_UNLOCK(s);
	return (0);
}

/*
 * walk arptab blowing away all entries
 */
arptflush()
{
	register struct arptab *at;
	register i;
	spl_t s;

	s = ARP_LOCK();	
	at = &arptab[0];
	for (i = 0; i < ARPTAB_SIZE; i++, at++) {
		if (at->at_flags != 0)		/* found an entry in use*/
			arptfree(at);
	}
	ARP_UNLOCK(s);	
}