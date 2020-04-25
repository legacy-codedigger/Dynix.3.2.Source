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
 * $Header: lp.h 1.5 89/08/16 $
 *
 * Common definitions for the MLP2000 Systech Line Printer driver.
 */

/* $Log:	lp.h,v $
 */


/*
 * Configuration data structures
 */
struct	lp_printer	{
	int	lp_width;
	int	lp_height;
	int	lp_ops;			/* same as lp_softc.lp_ops */
};

/*
 * controller registers
 */
struct lpdevice {
	u_char	lpd_addr0;		/* low byte for pr0 transfer */
	u_char	lpd_cnt0;		/* number of bytes to xfer for pr0 */
	u_char	lpd_addr1;		/* low byte for pr1 transfer */
	u_char	lpd_cnt1;		/* number of bytes to xfer for pr1 */
	int	lpd_pad1;
	u_char	lpd_cmdreg;		/* CSR for the DMA chip */
	u_char	lpd_req;		/* request register */
	u_char	lpd_bmask;		/* single bit mask */
	u_char	lpd_mode;
	u_char	lpd_clbp;		/* clear the byte-ordering flip-flop */
	u_char	lpd_clear;		/* master DMA clear */
	u_char	lpd_clmask;		/* clear mask register */
	u_char	lpd_mask;		/* write all mask register bits */
	u_char	lpd_csr;
	u_char	lpd_intr;
	u_char	lpd_hiaddr0;		/* high byte of pr0 transfer */
	u_char	lpd_hiaddr1;		/* high byte of pr1 transfer */
	u_char	lpd_pad2[3];
	u_char	lpd_reset;		/* reset controller, but not the 8237 */
	u_char	lpd_pad3[8];
};

/*
 * defines for lpdevice
 */

/*
 * 8237 command register (lpd_cmdreg)
 */
#define LPS_PRI		0x10		/* write */
#define LPS_REQ1	0x10		/* read */
#define LPS_REQ0	0x08		/* read */
#define LPS_DISABLE	0x04		/* write */
#define LPS_ADDRHOLD0	0x02		/* write */
#define LPS_TC1		0x02		/* read */
#define LPS_XTYPE	0x01		/* write */
#define LPS_TC0		0x01		/* read */

/*
 * Interrupt register status bits (lpd_intr)
 */
#define LPI_REQUEST	0x80
#define LPI_MEMERR	0x40
#define	LPI_PR1RQST	0x20		/* FYI: was CHECK */
#define LPI_PR0RQST	0x10
#define LPI_EOP1	0x02
#define LPI_EOP0	0x01

/*
 * Command status register bits (lpd_csr)
 */
#define LPC_RDY0	0x01
#define LPC_RDY1	0x02
#define LPC_SLCT0	0x04
#define LPC_SLCT1	0x08
#define LPC_INTE0	0x10
#define LPC_INTE1	0x20
#define LPC_PBSY0	0x40
#define LPC_PBSY1	0x80

/*
 * Write Mode register bits (lpd_mode)
 */
#define LPM_MBO		0x40
#define LPM_DECADDR	0x20
#define LPM_AUTOINIT	0x10
#define	LPM_RFM		0x08
#define LPM_CHAN	0x01

/*
 * Request register (lpd_req)
 */
#define LPR_SET		0x04		/* FYI: was SETRQ */
#define LPR_CHAN	0x01


/*
 * one per line printer
 */
struct lp_softc {
	u_char	lp_alive;			/* copy of md->md_alive */
	u_char	lp_state;
	u_char	lp_unit;			/* 0 or 1 */
	u_char	lp_ops;
	u_int	lp_count;
	u_int	lp_height;
	u_int	lp_width;
	u_int	lp_col;				/* physical printing column */
	u_int	lp_lcol;			/* logical printing column */
	u_int	lp_line;
	u_int	lp_bufsz;
	u_int	lp_maddr;			/* multibus address for dma */
	u_char	*lp_nbuf;			/* pointer into print buffer */
	u_char	*lp_startbuf;			/* beginning of print buffer */
	u_short	lp_basemap;			/* map reg base */
	u_short	lp_nmaps;			/* 1/2 md->md_nmaps */
	struct	lpdevice	*lp_base;	/* controller base address */
	u_int	lp_mblevel;			/* real multibus level */
	struct	mb_desc		*lp_mbdesc;	/* multibus descriptor */
	lock_t	*lp_lock;			/* lock - one per board */
	sema_t	lp_waitsema;			/* general semaphore */
};

/*
 * lp_state
 */
#define LP_BUSY		0x01		/* dma active */
#define LP_OPEN		0x02
#define LP_ONLINE	0x04		/* printer online */
#define LP_NO_REQ	0x08		/* no software request needed */
#define	LP_WOPEN	0x10		/* waiting for open */

/*
 * lp_ops flag bits.
 */
#define LPDEFAULT	0		/* standard char mappings */
#define	LPCAPS		1		/* caps only mappings included */
#define LPRAW		2		/* No mappings at all */

/*
 * misc useful defines
 *
 * LPSIZE is the size of a buffer that lpwrite() uses as a staging area from
 * the uio structs to the filesystem buffer used to DMA from.  It must be
 * small because the system call is running on the user's per-process kernel
 * stack, but it must be large enough so that function calling overhead does
 * not kill us.
 */
#define LPSIZE		100
#define LPLONGLINE	1000				/* max line length */
#define LPUNIT(x)	(minor(x))
#define LPBOARD(x)	(minor(x)>>1)
#define LPINTE		LPC_INTE0|LPC_INTE1
#define LPRDY0		LPC_RDY0|LPC_SLCT0		/* unit 0 ready */
#define LPRDY1		LPC_RDY1|LPC_SLCT1		/* unit 1 ready */
#define LPMODE		LPM_MBO|LPM_RFM
#define LPSTART(x)	x&=(~LPS_DISABLE)		/* controller enable */
#define LPSTOP(x)	x|=(LPS_DISABLE)		/* controller disable */
#define XX		0x1				/* don't care value */
#define LPOFFLINE(x)	((x->lp_base->lpd_csr & (x->lp_unit==0 ? LPRDY0 : LPRDY1))\
			!= (x->lp_unit==0 ? LPRDY0 : LPRDY1))
