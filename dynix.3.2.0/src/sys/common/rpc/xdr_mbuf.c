/* $Copyright:	$
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

#ifdef	NFS

#ifndef	lint
static	char	rcsid[] = "$Header: xdr_mbuf.c 1.8 91/03/11 $";
#endif	lint

/*
 * xdr_mbuf.c 
 *	XDR implementation on kernel mbufs.
 * 
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/* $Log:	xdr_mbuf.c,v $
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../netinet/in.h"
#include "../h/uio.h"

static bool_t	xdrmbuf_getlong(),  xdrmbuf_putlong();
static bool_t	xdrmbuf_getbytes(), xdrmbuf_putbytes();
static u_int	xdrmbuf_getpos();
static bool_t	xdrmbuf_setpos();
static long	*xdrmbuf_inline();
static void	xdrmbuf_destroy();

/*
 * Xdr on mbufs operations vector.
 */
static struct xdr_ops xdrmbuf_ops = {
	xdrmbuf_getlong,
	xdrmbuf_putlong,
	xdrmbuf_getbytes,
	xdrmbuf_putbytes,
	xdrmbuf_getpos,
	xdrmbuf_setpos,
	xdrmbuf_inline,
	xdrmbuf_destroy
};

/*
 * Initailize xdr stream.
 */
void
xdrmbuf_init(xdrs, m, op)
	register XDR		*xdrs;
	register struct mbuf	*m;
	enum xdr_op		op;
{

	xdrs->x_op = op;
	xdrs->x_ops = &xdrmbuf_ops;
	xdrs->x_base = (caddr_t)m;
	xdrs->x_private = mtod(m, caddr_t);
	xdrs->x_public = (caddr_t)0;
	xdrs->x_handy = m->m_len;
}

/* ARGSUSED */
static void
xdrmbuf_destroy(xdrs)
	XDR	*xdrs;
{
	/* do nothing */
}

static bool_t
xdrmbuf_getlong(xdrs, lp)
	register XDR	*xdrs;
	long		*lp;
{

	if ((xdrs->x_handy -= sizeof(long)) < 0) {
		if (xdrs->x_handy != -sizeof(long)) {
#ifdef	NFSDEBUG
			printf("xdr_mbuf: long crosses mbufs!\n");
#endif	NFSDEBUG
			xdrs->x_handy += sizeof(long);
			if (!xdrmbuf_getbytes(xdrs, (caddr_t)lp, sizeof(long)))
				return (FALSE);
			*lp = ntohl((u_long)(*lp));
			return (TRUE);
		}
		if (xdrs->x_base) {
			register struct mbuf *m =
			    ((struct mbuf *)xdrs->x_base)->m_next;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = mtod(m, caddr_t);
			xdrs->x_handy = m->m_len - sizeof(long);
		} else {
			return (FALSE);
		}
	}
	*lp = ntohl(*((u_long *)(xdrs->x_private)));
	xdrs->x_private += sizeof(long);
	return (TRUE);
}

static bool_t
xdrmbuf_putlong(xdrs, lp)
	register XDR	*xdrs;
	long		*lp;
{

	if ((xdrs->x_handy -= sizeof(long)) < 0) {
		if (xdrs->x_handy != -sizeof(long))
			printf("xdr_mbuf: putlong, long crosses mbufs!\n");
		if (xdrs->x_base) {
			register struct mbuf *m =
			    ((struct mbuf *)xdrs->x_base)->m_next;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = mtod(m, caddr_t);
			xdrs->x_handy = m->m_len - sizeof(long);
		} else {
			return (FALSE);
		}
	}
	*(long *)xdrs->x_private = htonl((u_long)(*lp));
	xdrs->x_private += sizeof(long);
	return (TRUE);
}

static bool_t
xdrmbuf_getbytes(xdrs, addr, len)
	register XDR	*xdrs;
	caddr_t		addr;
	register u_int	len;
{

	while ((xdrs->x_handy -= len) < 0) {
		if ((xdrs->x_handy += len) > 0) {
			bcopy(xdrs->x_private, addr, (u_int)xdrs->x_handy);
			addr += xdrs->x_handy;
			len -= xdrs->x_handy;
		}
		if (xdrs->x_base) {
			register struct mbuf *m =
			    ((struct mbuf *)xdrs->x_base)->m_next;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = mtod(m, caddr_t);
			xdrs->x_handy = m->m_len;
		} else {
			return (FALSE);
		}
	}
	bcopy(xdrs->x_private, addr, (u_int)len);
	xdrs->x_private += len;
	return (TRUE);
}

static bool_t
xdrmbuf_putbytes(xdrs, addr, len)
	register XDR	*xdrs;
	caddr_t		addr;
	register u_int	len;
{

	while ((xdrs->x_handy -= len) < 0) {
		if ((xdrs->x_handy += len) > 0) {
			bcopy(addr, xdrs->x_private, (u_int)xdrs->x_handy);
			addr += xdrs->x_handy;
			len -= xdrs->x_handy;
		}
		if (xdrs->x_base) {
			register struct mbuf *m =
			    ((struct mbuf *)xdrs->x_base)->m_next;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = mtod(m, caddr_t);
			xdrs->x_handy = m->m_len;
		} else {
			return (FALSE);
		}
	}
	bcopy(addr, xdrs->x_private, len);
	xdrs->x_private += len;
	return (TRUE);
}

/*
 * Like putbytes, only we avoid the copy by pointing a MCLT_KHEAP
 * mbuf at the buffer.  Not safe if the buffer goes away before
 * the mbuf chain is deallocated.
 */
bool_t
xdrmbuf_putbuf(xdrs, addr, len, func, arg)
	register XDR	*xdrs;
	caddr_t		addr;
	u_int		len;
	int		(*func)();
	int		arg;
{
	register struct mbuf *m;
	long llen = len;
	struct mbuf *mclgetx();

	if (len % BYTES_PER_XDR_UNIT) {
		/*
		 * Can't handle roundup problems. Since we need to pad
		 * with zeroes and cannot do in place.
		 */
		return (FALSE);
	}

	if (! xdrmbuf_putlong(xdrs, &llen)) {
		return (FALSE);
	}
	((struct mbuf *)xdrs->x_base)->m_len -= xdrs->x_handy;
	m = mclgetx(func, arg, addr, (int)len, M_DONTWAIT);
	if (m == NULL) {
		printf("xdrmbuf_putbuf: mclgetx failed\n");
		return (FALSE);
	}
	((struct mbuf *)xdrs->x_base)->m_next = m;
	xdrs->x_handy = 0;
	return (TRUE);
}

static u_int
xdrmbuf_getpos(xdrs)
	register XDR	*xdrs;
{

	return (
	   (u_int)xdrs->x_private - mtod(((struct mbuf *)xdrs->x_base), u_int));
}

static bool_t
xdrmbuf_setpos(xdrs, pos)
	register XDR	*xdrs;
	u_int		 pos;
{
	register caddr_t newaddr =
			mtod(((struct mbuf *)xdrs->x_base), caddr_t) + pos;
	register caddr_t lastaddr = xdrs->x_private + xdrs->x_handy;

	if ((int)newaddr > (int)lastaddr)
		return (FALSE);
	xdrs->x_private = newaddr;
	xdrs->x_handy = (int)lastaddr - (int)newaddr;
	return (TRUE);
}

static long *
xdrmbuf_inline(xdrs, len)
	register XDR	*xdrs;
	int		len;
{
	long *buf = 0;

	if (xdrs->x_handy >= len) {
		xdrs->x_handy -= len;
		buf = (long *) xdrs->x_private;
		xdrs->x_private += len;
	}
	return (buf);
}
#endif	NFS
