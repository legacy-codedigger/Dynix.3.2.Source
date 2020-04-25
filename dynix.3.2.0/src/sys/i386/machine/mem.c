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

#ifndef	lint
static	char	rcsid[] = "$Header: mem.c 2.6 90/06/09 $";
#endif

/*
 * mem.c
 *	Memory special file.
 *
 * 80386 version.
 */

/* $Log:	mem.c,v $
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/uio.h"

#include "../machine/pte.h"
#include "../machine/hwparam.h"
#include "../machine/plocal.h"

#include "../mbad/mbad.h"

/*
 * Minor number mnemonics.
 */

#define	DEV_MEM		0
#define	DEV_KMEM	1
#define	DEV_NULL	2
#define	DEV_KMWMEM	3
#define	DEV_KMBMEM	4

mmread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (mmrw(dev, uio, UIO_READ));
}

mmwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (mmrw(dev, uio, UIO_WRITE));
}

static
mmrw(dev, uio, rw)
	dev_t dev;
	struct uio *uio;
	enum uio_rw rw;
{
	register u_int o;
	register u_int c;
	register struct iovec *iov;
	struct pte lpte;
	int	MBAd_idx;
	int	error = 0;
	extern	caddr_t maxkmem;
	extern	caddr_t topmem;
	extern	etext;

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0) {
				panic("mmrw");
                                /*
                                 *+ The memory driver was passed an invalid
                                 *+ uio structure describing the I/O operation
                                 *+ on a memory read/write.
                                 */
			}
			continue;
		}
		o = uio->uio_offset;
		c = iov->iov_len;
		switch (minor(dev)) {

		case DEV_MEM:				/* physical memory */
			/*
			 * Since might read/write over hole, do one page at a
			 * time.  Could do whole thing if there are no holes.
			 *
			 * If mapping of holes changes (ie, no Sysmap[] entry
			 * for holes; invalid at level 1), need to change this.
			 */
			if (o >= (u_int)topmem
			||  (o + c) > (u_int)topmem
			||  (o + c) < o			/* wrap-around */
			||  !Sysmap[btop(o)].pg_v)	/* hole or bad */
				goto fault;
			c = min(c, NBPG - (o & PGOFSET));
			error = uiomove((caddr_t)o, (int)c, rw, uio);
			continue;

		case DEV_KMEM:				/* kernel memory */
			/*
			 * Must grab static copy in case another processor
			 * causes Sysmap[] to change after we check the pte.
			 * 80386 doesn't allow page-tables to protect text,
			 * so protect by using &etext.
			 */
			if (o >= (u_int)maxkmem)
				goto fault;
			lpte = Sysmap[btop(o)];
			if (lpte.pg_v == 0
			||  (rw == UIO_WRITE && o < (u_int)&etext))
				goto fault;
			o &= PGOFSET;
			c = min(c, NBPG-o);
			error = uiomove((caddr_t)(PTETOPHYS(lpte)+o), (int)c, rw, uio);
			continue;

		case DEV_NULL:				/* EOF/RATHOLE */
			if (rw == UIO_READ)
				return (0);
			break;

		case DEV_KMWMEM:	/* multibus memory (access by shorts) */
		case DEV_KMBMEM:	/* multibus memory (access by chars) */
			/*
			 * History: ns32000 kernel insisted on read/write
			 * to MBAd use kernel virtual addresses (8Meg).
			 * We allow this, but also allow zero-origin.
			 */
			if (o >= OLD_VIRT_MBAD)
				o -= OLD_VIRT_MBAD;
			/*
			 * If the addresses are in the same (existing) MBAd,
			 * then pte's are valid and not ever remapped.
			 *
			 * Don't know here if the address being read/written
			 * actually exists in the multibus; if not, MBAd causes
			 * an NMI (access error) and the kernel panics.
			 */
			MBAd_idx = o / MBAD_ADDR_SPACE;
			if ((MBAdvec & (1 << MBAd_idx)) == 0
			||  c > MBAD_ADDR_SPACE
			||  ((o + c - 1) / MBAD_ADDR_SPACE) > MBAd_idx)
				goto fault;
			if (!useracc(iov->iov_base, c, rw == UIO_READ ? B_WRITE : B_READ))
				goto fault;
			o &= (MBAD_ADDR_SPACE-1);
			MBAdcpy((caddr_t)(VA_MBAd(MBAd_idx) + o),
				iov->iov_base, (int)c, rw, minor(dev));
			break;

		default:
			error = ENXIO;
			break;
		}
		if (error)
			break;
		iov->iov_base += c;
		iov->iov_len -= c;
		uio->uio_offset += c;
		uio->uio_resid -= c;
	}
	return (error);
fault:
	return (EFAULT);
}

/*
 * Multibus Address Space <--> User Space transfer
 */

static
MBAdcpy(mbad_add, usradd, n, rw, mindev)
	register caddr_t mbad_add;
	register caddr_t usradd;
	register int n;
	enum uio_rw rw;
	int mindev;
{
	union {
		short	svar;
		char	sbuf[2];
	} x;

	if (mindev == DEV_KMWMEM) {
		/*
		 * Reads and writes must be as shorts.
		 */
		if (rw == UIO_READ) {
			for (n >>= 1; n > 0; n--) {
				x.svar = *(short *)mbad_add;
				mbad_add += 2;
				(void) subyte(usradd++, x.sbuf[0]);
				(void) subyte(usradd++, x.sbuf[1]);
			}
		} else {
			for (n >>= 1; n > 0; n--) {
				x.sbuf[0] = fubyte(usradd++);
				x.sbuf[1] = fubyte(usradd++);
				*(short *)mbad_add = x.svar;
				mbad_add += 2;
			}
		}
	} else {
		/*
		 * Reads and writes must be as characters.
		 */
		if (rw == UIO_READ) {
			for (; n > 0; n--)
				(void) subyte(usradd++, *mbad_add++);
		} else {
			for (; n > 0; n--)
				*mbad_add++ = fubyte(usradd++);
		}
	}
}

/*
 * mmmmap()
 *	Perform mapping functions (neat name, huh?).
 */

static	char	mm_generic[CLBYTES+CLBYTES-1];	/* could calloc... */

/*ARGSUSED*/
mmmmap(dev, cmd, off, size, prot)
	dev_t		dev;
	int		cmd;
	u_long		off;			/* HW pages */
	int		size;			/* HW pages */
	int		prot;			/* PROT_READ|PROT_WRITE */
{
	register u_long	addr = (u_long)ptob(off);
	register u_long	max_addr = (u_long)ptob(off+size);
	int		MBAd_idx;
	struct	pte	lpte;
	extern		etext;

	switch(cmd) {

	case MM_MAP:
		/*
		 * All legit except /dev/null.  Check sizes.
		 * /dev/mem and /dev/kmem allow IO services,
		 * MBAd devices don't.
		 */

		switch(minor(dev)) {

		case DEV_MEM:
			/*
			 * Insure valid physical memory address.
			 */
			if (addr < (u_long)topmem
			&&  max_addr > addr
			&&  max_addr <= (u_long)topmem)
				return(MM_NPMEM);
			else
				return(ENXIO);

		case DEV_KMEM:
			/*
			 * Insure legal "kmem" range, and not trying to
			 * write on kernel text.
			 */
			if (addr < (u_long)maxkmem
			&&  max_addr <= (u_long)maxkmem
			&&  max_addr > addr
			&&  ((prot&PROT_WRITE) == 0 || addr >= (u_long)&etext))
				return(MM_NPMEM);
			else
				return(ENXIO);

		case DEV_KMWMEM:
		case DEV_KMBMEM:
			/*
			 * History: ns32000 kernel insisted on read/write
			 * to MBAd use kernel virtual addresses (8Meg).
			 * We allow this, but also allow zero-origin.
			 */
			if (addr >= OLD_VIRT_MBAD) {
				addr -= OLD_VIRT_MBAD;
				max_addr -= OLD_VIRT_MBAD;
			}
			/*
			 * Insure range fits within a single existing MBAd.
			 */
			MBAd_idx = addr / MBAD_ADDR_SPACE;
			if ((MBAdvec & (1 << MBAd_idx)) == 0
			||  size > btop(MBAD_ADDR_SPACE)
			||  (max_addr - 1) / MBAD_ADDR_SPACE > MBAd_idx)
				return(ENXIO);
			else
				return(MM_PHYS);

		default:
			return(ENXIO);
		}

	case MM_REFPG:

		switch(minor(dev)) {

		case DEV_MEM:
			/*
			 * If page exists, return its physical address.
			 * Else return generic page.
			 */
			if (Sysmap[off].pg_v)
				return((int)ptob(off));
			else
				return(((int)mm_generic + CLOFSET) & ~CLOFSET);

		case DEV_KMEM:
			/*
			 * If invalid part of kernel address space
			 * (transient in parts), return generic page.
			 * Problem is that MM_MAP can't accurately
			 * check what's valid, and part of kernel
			 * address space change dynamically.
			 */
			lpte = Sysmap[off];
			if (lpte.pg_v)
				return((int)PTETOPHYS(lpte));
			return(((int)mm_generic + CLOFSET) & ~CLOFSET);

		case DEV_KMWMEM:
		case DEV_KMBMEM:
			/*
			 * Same history...
			 */
			if (addr >= OLD_VIRT_MBAD)
				addr -= OLD_VIRT_MBAD;
			return(PA_MBAd(0) + addr);

		default:
			panic("mmmmap: ref minor");
                        /*
                         *+ The kernel attempted to reference a page
                         *+ mapped by the memory driver and passed the driver
                         *+ an invalid minor device number.
                         */
			/*NOTREACHED*/
		}

	case MM_UNMAP:
	case MM_SWPOUT:
	case MM_SWPIN:
		return(0);	/* NOP */

	default:
		printf("cmd=%d\n", cmd);
		panic("mmmmap: function");
                /*
                 *+ The mapping support function of the memory driver was
                 *+ asked to perform an invalid mapping operation.
                 */
		/*NOTREACHED*/
	}
}
