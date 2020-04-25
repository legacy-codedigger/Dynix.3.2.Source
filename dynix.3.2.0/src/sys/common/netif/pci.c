/*
 * $Copyright: $
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

#ifndef lint
static char rcsid[] = "$Header: pci.c 2.4 1991/05/13 18:36:54 $";
#endif

/*
 * FUNCTION: The role of this pseudo-driver (PCI driver) is to send &
 * receive 3Com packets.  PCI driver is bound to the S-E driver, receiving
 * 3Com packets from it and sending packets through it.  PCI-driver supports
 * a number of minor (ether) devices, or "ports".
 * 
 * INTERNAL DESIGN: Initially, PCI driver is given the ether address of the
 * station and method for sending packets.  When packets are received,
 * they are enqueued on a port-specific internal queue until the client
 * actually asks to read the packet.  Packets are sent immediately.
 *
 * All the EtherSeries PCI software supplied by 3com assumes that the
 * user-level code does the network->host conversion of data.  Since
 * the Ethernet driver does some of this for us, some of that work
 * needs to be undone on reads and writes.  That's why you will see
 * occasional ntohs() and htons() when you don't expect them.
 * 
 * DEVICE DATA:
 *      pci->pci_ifp		- packet send method
 *      pci->pci_ether_addr	- ether address of this station
 *      pci->pci_stats		- statistics
 *      pci_debug		- controls tracing
 *	pci_qsizes[]		- controls unit queue lengths
 * 
 * UNIT DATA:
 *	pcu_chain	- pending packets for this unit
 *	pcu_pending	- count of packets waiting in pcu_chain.
 *
 * CLIENT ENTRY POINTS:
 *      pciopen    - client opens a port
 *      pciclose   - client closes a port
 *      pciread    - client receives a packet
 *      pciwrite   - client sends a packet
 *      pciioctl   - client control channel
 * 
 * E DRIVER ENTRY POINTS:
 *      pciattach  - E driver tells PCI driver addr & send method
 *      pcirint    - E driver gives PCI driver ether hdr & mbuf
 * 
 * INTERNAL ROUTINES:
 *      pci_free_chain	- free all packets associated with a unit.
 *	pci_mbuf_move	- move data from/to user space to/from mbufs.
 *	pci_make_chain	- create a template mbuf chain for pci_mbuf_move.
 * 
 */

/*
 * $Log: pci.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/uio.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/if_ether.h"
#include "../netif/pci.h"

#include "../balance/slic.h"

#include "../machine/ioconf.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"

static struct pci_state *pci_state;

static struct mbuf *pci_make_chain();
static int pci_free_chain(), pci_mbuf_move();

int	pciattach();
int	pcirint();

int pciboot(ndevs)
{
	register struct pci_state *pci;
	register struct pci_unit *pcu;
	int i;

	if (ndevs == 0)
		return;
	if (ndevs < 8) {
		printf("pci: Warning: Not enough devices for pci operation\n");
		printf("pci:          Try using 'pseudo pci 8' in config.\n");
	}
	pci = pci_state = (struct pci_state *)calloc(sizeof(struct pci_state));
	if (pci == NULL) {
		printf("Not enough memory for pci\n");
		return;
	}
	pci->pci_nunits = ndevs;
	pci->pci_units = (struct pci_unit *)
				calloc(pci->pci_nunits*sizeof(struct pci_unit));
	if (pci->pci_units == NULL) {
		printf("Not enough memory for pci units\n");
		pci_state = NULL;
		return;
	}
	for (i = 0; i < pci->pci_nunits; ++i) {
		pcu = &pci->pci_units[i];
		init_lock(&pcu->pcu_lock, pci_gate);
		pcu->pcu_state = PCI_CLOSED;
		init_sema(&pcu->pcu_read, 0, 0, pci_gate);
		pcu->pcu_chain = (struct mbuf *)NULL;
		pcu->pcu_pending = 0;
		if (i < pci_nqsizes) pcu->pcu_maxpend = pci_qsizes[i];
		else pcu->pcu_maxpend = pci_qsizes[pci_nqsizes-1];
	}
	/*
	 * backpatch function pointers so se driver will call us when the
	 * time comes
	 */
	pciattach_fctn = pciattach;
	pcirint_fctn = pcirint;
}


/*
 * pci_attach: attach to an Ethernet driver.
 *
 * In the presence of multiple Ethernet interfaces,
 * attach to the first.
 */
pciattach(ifp, ecaddr)
	struct ifnet *ifp;
	u_char *ecaddr;
{
	register struct pci_state *pci = pci_state;

	if (pci != NULL && pci->pci_ifp == NULL) {
		pci->pci_ifp = ifp;
		bcopy((caddr_t)ecaddr, (caddr_t)pci->pci_ether_addr, 6);
	}
}


/*ARGSUSED*/
pciopen(dev, flag)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct pci_unit *pcu = &pci_state->pci_units[unit];
	spl_t ipl;

	if (pci_state == NULL || unit >= pci_state->pci_nunits)
		return (ENXIO);
	ipl = p_lock(&pcu->pcu_lock, SPLIMP);
	if (pcu->pcu_state == PCI_OPEN) {
		v_lock(&pcu->pcu_lock, ipl);
		return (EBUSY);
	}
	pcu->pcu_state = PCI_OPEN;
	v_lock(&pcu->pcu_lock, ipl);
	if (pci_debug)
		printf("pci%d: open\n", unit);
	return (0);
}

/*ARGSUSED*/
pciclose(dev, flag)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct pci_unit *pcu = &pci_state->pci_units[unit];
	int chainsize;
	spl_t ipl;

	if (unit >= pci_state->pci_nunits)
		return (ENXIO);
	ipl = p_lock(&pcu->pcu_lock, SPLIMP);
	if (pcu->pcu_state == PCI_CLOSED) {
		v_lock(&pcu->pcu_lock, ipl);
		return (0);
	}

	pcu->pcu_state = PCI_CLOSED;
	chainsize = pci_free_chain(pcu);
	pcu->pcu_pending -= chainsize;
	vall_sema(&pcu->pcu_read);
	v_lock(&pcu->pcu_lock, ipl);

	if (pci_debug) printf("pci%d: close\n", unit);
	return (0);
}


pciread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = minor(dev);
	register struct pci_unit *pcu = &pci_state->pci_units[unit];
	register struct mbuf *m;
	spl_t ipl;
	int error;

	if (pci_state == NULL || unit >= pci_state->pci_nunits)
		return (ENXIO);

	/*
	 * avoid the sys-call restart by tromping on the qsave
	 * set up by ino_rw.
	 */
	if (setjmp(&u.u_qsave))
		return (EINTR);

	ipl = p_lock(&pcu->pcu_lock, SPLIMP);

	while ((m = pcu->pcu_chain) == NULL) {
		p_sema_v_lock(&pcu->pcu_read, PCI_PRI, &pcu->pcu_lock, ipl);
		ipl = p_lock(&pcu->pcu_lock, SPLIMP);
	}
	pcu->pcu_chain = m->m_act;
	pcu->pcu_pending--;
	v_lock(&pcu->pcu_lock, ipl);

	error = pci_mbuf_move(m, uio, UIO_READ);
	m_freem(m);
	if (pci_debug) printf("pci%d: read %d left\n", unit, uio->uio_resid);
	return (error);
}



pciwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int unit = minor(dev), nbytes, error;
	register struct mbuf *m;
	register struct ether_header *eh;
	struct sockaddr dest;

	if (pci_debug) printf("pci%d: write %d\n", minor(dev), uio->uio_resid);

	if (unit >= pci_state->pci_nunits || pci_state->pci_ifp == NULL)
		return (ENXIO);

	nbytes = uio->uio_resid;
	if (nbytes < ETHERMIN || nbytes > ETHERMTU + SZ_EHEAD || (nbytes & 1))
	    	return (EINVAL);

	m = pci_make_chain(nbytes);
	if (m == NULL) {
		pci_state->pci_stats.pci_write_fails++;
		return (ENOBUFS);
	}
	error = pci_mbuf_move(m, uio, UIO_WRITE);
	if (error)
		return (error);

	eh = mtod(m, struct ether_header *);
	eh->ether_type = ntohs((u_short)eh->ether_type);
	m->m_off += SZ_EHEAD; m->m_len -= SZ_EHEAD;
	if (bcmp((caddr_t) eh, (caddr_t)pci_state->pci_ether_addr, 6) == 0) {
		pcirint(eh, m);
		return (0);
	}

	if (eh->ether_dhost[0] & 0x1) {
		struct mbuf *mcop = m_copy(m, 0, (int)M_COPYALL);

		if (mcop)
			pcirint(eh, mcop);
	}

	dest.sa_family = AF_UNSPEC;
	bcopy((caddr_t)eh, (caddr_t)dest.sa_data, SZ_EHEAD);
	return((*pci_state->pci_ifp->if_output)(pci_state->pci_ifp, m, &dest));
}


/*ARGSUSED*/
pciioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	int unit = minor(dev);
	register struct pci_unit *pcu = &pci_state->pci_units[unit];
	spl_t ipl;
	int chainsize;

	/* EtherSeries servers are inconsistent in use of ioctl numbers... */
	switch (cmd & 0xFF) {
	case EIOCGETA & 0xFF:
		bcopy((caddr_t)pci_state->pci_ether_addr, addr, 6);
		break;

	case EIOCFLSH & 0xFF:
		ipl = p_lock(&pcu->pcu_lock, SPLIMP);
		chainsize = pci_free_chain(pcu);
		pcu->pcu_pending -= chainsize;
		vall_sema(&pcu->pcu_read);
		v_lock(&pcu->pcu_lock, ipl);
		break;
	}
	return (0);
}


/*
 * pcirint - PCI interrupt routine, called from se_intr() and pciwrite().
 *
 * Takes the mbuf chain just read, and queues it at the appropriate unit.
 */
pcirint(ec, m)
	register struct ether_header *ec;
	register struct mbuf *m;
{
	register struct pci_unit *pcu;
	register struct pci_state *pci = pci_state;
	short punit;
	spl_t ipl;

	if (pci == NULL) {
		m_freem(m);
		return;
	}

	/* insert the Ethernet header at the front of the chain */
	if (m->m_off >= MMINOFF + SZ_EHEAD) {
		m->m_off -= SZ_EHEAD; m->m_len += SZ_EHEAD;
		bcopy((caddr_t)ec, mtod(m, caddr_t), SZ_EHEAD);
	} else {
		struct mbuf *m_new = m_getm(M_DONTWAIT, MT_HEADER, 1);

		if (m_new == 0) {
			m_freem(m);
			return;
		}
		m_new->m_next = m;
		m = m_new;
		m->m_off = MMINOFF; m->m_len = SZ_EHEAD;
		bcopy((caddr_t)ec, mtod(m, caddr_t), SZ_EHEAD);
	}
	ec = mtod(m, struct ether_header *);
	ec->ether_type = htons((u_short)ec->ether_type);

	/* Make sure that the port number is in the first mbuf */
	if (m->m_len < 40)
		if ((m = m_pullup(m, 40)) == NULL)
			return;

	punit = ntohs(mtod(m, u_short *)[15]) - PCI_BASE;
	if (pci_debug) printf("pci: received packet for unit %d\n", punit);

	if (punit < 0 || punit >= pci->pci_nunits)
		punit = PCI_MISC_UNIT;

	/* statistics are rough guesses only... */
	pci->pci_stats.pci_nreceived++;
	pcu = &pci->pci_units[punit];
	ipl = p_lock(&pcu->pcu_lock, SPLIMP);
	if (pcu->pcu_state == PCI_CLOSED || pcu->pcu_pending >= pcu->pcu_maxpend) {
		v_lock(&pcu->pcu_lock, ipl);
		m_freem(m);
		return;
	}
	pcu->pcu_pending++;

	if (pcu->pcu_chain == NULL) {
		pcu->pcu_chain = m;
	} else {
		register struct mbuf *mch = pcu->pcu_chain;

		while (mch->m_act)
			mch = mch->m_act;
		mch->m_act = m; m->m_act = NULL;
	}
	v_sema(&pcu->pcu_read);
	v_lock(&pcu->pcu_lock, ipl);
}


/*
 * pci_free_chain - free the units mbuf chain, returning the number
 *		   of packets freed.
 */
static int pci_free_chain(pcu)
	register struct pci_unit *pcu;
{
	register struct mbuf *m, *m_next;
	int nfreed = 0;

	for (m = pcu->pcu_chain; m != NULL; m = m_next) {
		m_next = m->m_act;
		m_freem(m);
		nfreed++;
	}
	pcu->pcu_chain = NULL;
	return (nfreed);
}



/*
 * pci_make_chain - build a template mbuf chain of the desired length.
 *
 * Used by pciwrite.
 */
static struct mbuf *pci_make_chain(length)
{
	int mcount, i;
	struct mbuf *m_chain;

	mcount = (length + MLEN-1)/MLEN;
	m_chain = NULL;
	for (i = 0; i < mcount; i++) {
		register struct mbuf *m_new = m_getm(M_DONTWAIT, MT_DATA, 1);

		if (m_new == NULL) {
			m_freem(m_chain);
			return (NULL);
		}
		m_new->m_off = MMINOFF;
		m_new->m_len = (i == 0)? length - (mcount-1)*MLEN : MLEN;
		m_new->m_next = m_chain;
		m_chain = m_new;
	}
	return (m_chain);
}



/*
 * pci_mbuf_move - copy data into or out of an mbuf chain.
 *
 * 'm' is the chain, 'uio' the description of the user buffer,
 * 'dir' is the direction: UIO_READ or UIO_WRITE.
 */
static int pci_mbuf_move(m, uio, dir)
	register struct mbuf *m;
	struct uio *uio;
{
	int error;

	for (; m != NULL; m = m->m_next) {
		if (error = uiomove(mtod(m, caddr_t), m->m_len, dir, uio))
			break;
	}
	return (error);
}
