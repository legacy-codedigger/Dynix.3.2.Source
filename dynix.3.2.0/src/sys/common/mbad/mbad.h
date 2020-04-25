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

/*
 * $Header: mbad.h 2.3 87/02/06 $
 *
 * mbad.h
 *	Data-structures representing Multi-Bus Adapter/Interface.
 */

/* $Log:	mbad.h,v $
 */

#ifdef	KERNEL
/*
 * Multi-bus Interface (software) descriptor.
 * One of these exists per MBAd.
 * All pointers are kernel virtual addresses, unless specified
 * otherwise.
 *
 * Note that mb_dmabase is typically 512K == 0x80000.
 */

struct	mb_desc	{
	caddr_t		mb_mem;		/* Multi-bus memory */
	struct	mb_ios	*mb_ios;	/* MBAd IO space/etc */
	unsigned	mb_dmabase;	/* DMA base, from MB side */
	unsigned	mb_clist;	/* DMA base for C-list */
	u_short		mb_dmareg;	/* next free DMA register */
	u_char		mb_slicaddr;	/* SLIC address of MBAd */
	u_char		mb_intrs;	/* bit ==> intr level in use */
};

/*
 * CLTOMB gives the DMA address to use for `addr' in the c-list thru
 * the given MBAd.
 */

#define	CLTOMB(mb,addr)	((mb)->mb_clist + (((unsigned)addr) - (unsigned)cfree))
#endif	KERNEL

/*
 * Multi-bus IO Space.  This is the SQL-bus view of the 256K DMA space
 * of the MBAd.  See hardware functional spec for details.
 * This is never "allocated" per-se, rather used as a template to
 * talk to an MBAd.
 *
 * Note that mb_map[] are actually u_shorts on u_int boundaries.
 */

#define	K	1024

#define	MB_MAPS		256		/* # mapping registers */
#define	MB_MRSIZE	1024		/* size mapped by one register */
#define	MB_MRSIZEL2	10		/* Log2(MB_MRSIZE) */

struct	mb_ios	{
	u_char	mb_io	[64*K];		/* Multi-bus IO addresses */
	u_char	mb_alm	[64*K];		/* MBAd Atomic Lock Memory */
	u_int	mb_map	[MB_MAPS];	/* DMA Mapping registers */
	u_char	mb_res2	[(64*K)-1024];	/* Reserved */
	u_char	mb_ctl	[1];		/* MBAd control register */
	u_char	mb_res3	[64*K-1];	/* Reserved */
};

#undef	K

/*
 * MBADMAP accesses given map register in given MBAd.
 * PHYSTOMB turns a physical address into the value to put in an MBAd map,
 *	ignoring offset.
 */

#define	MBADMAP(mb,i)	(*(u_short*)(&((mb)->mb_ios->mb_map[i])))
#define	PHYSTOMB(paddr)	(((unsigned)(paddr))>>MB_MRSIZEL2)

#ifdef	ns32000
/*
 * PTEMBOFF returns offset in MBAd map register of memory pointed at by pte.
 */
#define	PTEMBOFF(pte)	((unsigned)((*(int*)(&(pte))) & ((~(NBPG-1))&(MB_MRSIZE-1))))
#endif	ns32000

/*
 * Bit definitions for mb_ctl register.
 */

#define	MBC_MAPENAB	0x01		/* enable mapping */
#define	MBC_DIAG	0x02		/* diagnostics support */
#define	MBC_LOCK	0x04		/* assert /LOCK on multibus */
#define	MBC_ADDRSP	0xC0		/* address space (always 0) */

/*
 * Multibus adapter firmware defines, etc.
 * (from MBAD Firmware Spec, 1/19/84).
 */

/*
 * Commands from MBAD -> Unix
 */

#define	MCMD_VERSION_IS		(2 << 3)
#define	MCMD_CHECK_IS		(3 << 3)
#define	MCMD_CHECK_IS_OK	(MCMD_CHECK_IS | 1)
#define	MCMD_CHECK_IS_NOT_OK	(MCMD_CHECK_IS | 0)
#define	MCMD_INTR_IS		(4 << 3)
#define	MCMD_INTR_IS_OK		(MCMD_INTR_IS | 1)
#define	MCMD_INTR_IS_NOT_OK	(MCMD_INTR_IS | 0)
#define	MCMD_BIN0_IS		(5 << 3)

/*
 * Commands from Unix -> MBAD.
 */

#define	MCMD_PRINT_LINE		(0 << 3)
#define	MCMD_MBAD_RESET		(1 << 3)
#define	MCMD_INIT_MESSAGES	(2 << 3)
#define	MCMD_INIT_ERRORS	(3 << 3)
#define	MCMD_INIT_LINE		(4 << 3)
#define	MCMD_REPORT_VERSION	(5 << 3)
#define	MCMD_REPORT_CHECKSUM	(6 << 3)
#define	MCMD_SEND_INTERRUPT	(7 << 3)
#define	MCMD_STOP_INTERRUPT	(8 << 3)
#define	MCMD_SEND_EDGE_INTR	(9 << 3)
#define	MCMD_REENABLE_LINE	(10 << 3)
#define	MCMD_DISABLE_MSGS	(11 << 3)
#define	MCMD_CHECK_INTR_LINE	(12 << 3)
#define	MCMD_ENABLE_HW_ERRS	(13 << 3)
#define	MCMD_DISABLE_HW_ERRS	(14 << 3)
#define	MCMD_TEST_BIN0_INTR	(15 << 3)
#define	MCMD_SPODE_INIT		(31 << 3)

/*
 * MBAD error values returned in MBAD_ERROR.
 */

#define	MBAD_HARD_ERROR  	0

#ifdef	KERNEL
/*
 * MBAd error handling definitions...
 * This assumes maskable interrupt is the command at the MBAd; if this
 * changes to NMI, this changes as does MBAd_init(), configure().
 * MBAd error vectors must start at 0 mod MBAD_ERR_ALIGN.
 */

#define	MBAD_ERROR_BIN	7			/* bin to get errors */
#define	MBAD_ERR_ALIGN	8			/* Error Vectors alignment */

/*
 * Maximum amount of physical memory the MBAd can address.  All transfers
 * must be to physical addresses below this address.
 */

#define	MAX_MBAD_ADDR_MEM	(64 * 1024 * 1024)		/* 64Meg */

/*
 * MBAD_CMD() sends command to given MBAd.  No mutex/spl done here.
 * This uses bin 7 to get the MBAd's attention.
 *
 * mbad_reenable() re-enables interrupt on given line for given MBAd.
 * The splhi() is to mutex local use of SLIC in mIntr().
 */

#define	MBAD_CMD(mb,cmd)	mIntr((mb)->mb_slicaddr, 7, (u_char)(cmd))

#define	mbad_reenable(mb,level)	\
	{ \
		spl_t	mb_spl = splhi(); \
		sendsoft((mb)->mb_slicaddr, (u_char)(1<<(level))); \
		splx(mb_spl); \
	}

/*
 * Kernel data-structures to represent MBAd's.
 */

extern	struct	mb_desc	*MBAd;		/* base of array */
extern	u_int	 	MBAdvec;	/* bit-vector of existant MBAd's */
extern	unsigned	mbad_physmap();	/* set up phys DMA for boot/probe */
extern	unsigned	mbad_setup();	/* set up DMA for `bp' */
#endif	KERNEL
