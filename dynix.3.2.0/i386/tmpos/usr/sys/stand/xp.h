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

/* $Header: xp.h 2.0 86/01/28 $ */

/*
 * Xylogics 450 multibus SMD disk controller definitions
 */

struct	xpdevice {
	u_char	xplrel;		/* lsb of the iopb relocation */
	u_char	xpmrel;		/* msb of the iopb relocation */
	u_char	xplba;		/* least significant byte of the iopb address */
	u_char	xpmba;		/* most significant byte of the iopb address */
	u_char	xpcsr;		/* control and status register */
	u_char	xpreset;	/* reset and update register */
};

/*
 * xpcsr
 */

#define	XP_DRDY		0x01	/* selected drive is ready */
#define	XP_AACK		0x02	/* attention acknowledge */
#define	XP_AREQ		0x04	/* attention request */
#define	XP_ADDR24	0x08	/* 24 bit address mode when set */
#define	XP_IPND		0x10	/* interrupt pending */
#define	XP_DERR		0x20	/* bus error (double error) */
#define	XP_ERR		0x40	/* general error */
#define	XP_GO		0x80	/* execute an iopb (write only) */
#define	XP_BUSY		0x80	/* controller busy (read  only) */

/*
 * I/O parameter block
 */

struct	xp_iopb	{
	u_char	io_comm;	/* disk command */
	u_char	io_imode;	/* interrupt mode */
	u_char	io_status;	/* status */
	u_char	io_compcode;	/* completion code */
	u_char	io_throt;	/* throttle */
	u_char	io_drive;	/* drive type, unit select */
	u_char	io_head;	/* head address */
	u_char	io_sect;	/* sector address */
	u_short	io_cyl;		/* cylinder address */
	union	{
	    u_short  u_scnt;
	    u_char   u_dstat[2];
	} io_u;
#define	io_scnt	io_u.u_scnt	/* sector count */
#define	io_dstat io_u.u_dstat[0]/* drive status (read drive status command) */
	u_long	io_lbaddr;	/* memory address */
	u_char	io_hdoff;	/* head offset */
	u_char	io_reserved;	/* not used */
	u_short	io_niop;	/* next iopb address */
	u_short	io_eccm;	/* ECC mask pattern */
	u_short	io_ecca;	/* ECC bit address */
	u_short io_iscnt;	/* initial sector count - driver use only */
};

#define	NIOPB	8	/* iopb's allocated per controller */

#define	I_NORMAL	1	/* normal transfer */
#define	I_REVEC		2	/* revectoring complete */

/*
 * io_comm
 */

#define	XP_NOP		0x00	/* no operation */
#define	XP_WRITE	0x01	/* write */
#define	XP_READ		0x02	/* read */
#define	XP_WTRKHD	0x03	/* write track headers */
#define	XP_RTRKHD	0x04	/* read track headers */
#define	XP_SEEK		0x05	/* seek */
#define	XP_DRESET	0x06	/* drive reset */
#define	XP_FORMAT	0x07	/* write format */
#define	XP_XREAD	0x08	/* read header, data, and ECC */
#define	XP_DSTAT	0x09	/* read drive status */
#define	XP_XWRITE	0x0A	/* write header, data, anc ECC */
#define	XP_DSIZE	0x0B	/* set drive size */
#define	XP_STEST	0x0C	/* self test */
				/* 0x0D reserved */
#define	XP_MBLOAD	0x0E	/* maintenance buffer load */
#define	XP_MBDUMP	0x0F	/* maintenance buffer dump */

#define	XP_IEN		0x10	/* interrupt enable */
#define	XP_CHEN		0x20	/* command-chaining enable */
#define	XP_RELO		0x40	/* data relocation enable */
#define	XP_AUD		0x80	/* auto iopb update enable */

/*
 * io_imode: interrupt mode and function modification
 */

#define	XPM_ECC0	0x00	/* report but don't correct ECC errors */
#define	XPM_ECC1	0x01	/* neither report nor correct ECC errors */
#define	XPM_ECC2	0x02	/* automatic ECC correction */
#define	XPM_ECC3	0x03	/* report all ECC errors as hard errors */
#define	XPM_EEF		0x04	/* enable extended function */
#define	XPM_ASR		0x08	/* automatic seek retry */
#define	XPM_HDP		0x10	/* hold dual port */
#define	XPM_IERR	0x20	/* interrupt on error */
#define	XPM_IEI		0x40	/* interrupt on each iopb */
				/* 0x80 reserved */

/*
 * io_status
 */

#define	XPS_DONE	0x01	/* command done */
				/* 0x02 reserved */
#define	XPS_TYPE	0x1C	/* controller type */
#define	XPS_XP440	0x00	/* ... Xylogics 440 */
#define	XPS_XP450	0x04	/* ... Xylogics 450 */
#define	XPS_XP472	0x08	/* ... Xylogics 472 */
				/* 0x60 reserved */
#define	XPS_ERROR	0x80	/* error */

/*
 * io_throt: throttle, interleave, and transfer mode
 */

#define	XPT_T2		0x00	/*   2 transfers per bus access */
#define	XPT_T4		0x01	/*   4 transfers per bus access */
#define	XPT_T8		0x02	/*   8 transfers per bus access */
#define	XPT_T16		0x03	/*  16 transfers per bus access */
#define	XPT_T32		0x04	/*  32 transfers per bus access */
#define	XPT_T64		0x05	/*  64 transfers per bus access */
#define	XPT_T128	0x06	/* 128 transfers per bus access */
#define	XPT_T128x	0x07	/* 128 transfers per bus access */
#define	XPT_INTLV1	0x00	/* 1:1 interleaving (consecutive sectors) */
#define	XPT_INTLV2	0x08	/* 2:1 interleaving (alternate sectors) */
#define	XPT_INTLV3	0x10	/* 3:1 interleaving */
#define	XPT_INTLV4	0x18	/* 4:1 interleaving */
#define	XPT_INTLV5	0x20	/* 5:1 interleaving */
#define	XPT_INTLV	0x78	/* interleave factors */
#define	XPT_BYTE	0x80	/* byte mode for DMA */

#define	XPT_USE_INTLV	XPT_INTLV1	/* interleave factor to use */

/*
 * io_dstat: returned by Read Drive Status
 */

#define	XPS_DFLT	0x04	/* drive faulted */
#define	XPS_SKER	0x08	/* seek error */
#define	XPS_DPB		0x10	/* dual port busy */
#define	XPS_WRPT	0x20	/* drive write protected */
#define	XPS_DNRDY	0x40	/* drive not ready */
#define	XPS_OFFCYL	0x80	/* off cylinder */

struct	xp_softc {
	int	state;			/* current activity */
	struct	xp_iopb	*sc_new;	/* list of iopbs to be inserted */
	struct	xp_iopb	*sc_actf;	/* list of iopbs currently executing */
	struct	xp_iopb	*sc_base;	/* base address of iopb array */
	struct	xp_iopb	*sc_done;	/* list of iopbs to be removed */
	struct	xp_iopb	*sc_free;	/* free iopbs */
	long	sc_addr;		/* multibus address of iopb's */
};

/*
 * completion codes
 */

#define	XPC_GOOD	0x00		/* successful completion */
#define	XPC_IPND	0x01		/* interrupt pending */
#define	XPC_CONFLCT	0x03		/* busy conflict */
#define	XPC_TIMO	0x04		/* operation timeout */
#define	XPC_HNF		0x05		/* header not found */
#define	XPC_HECC	0x06		/* hard ECC */
#define	XPC_ICA		0x07		/* illegal cylinder address */
#define	XPC_SSCE	0x09		/* sector slip command error */
#define	XPC_ISA		0x0A		/* illegal sector address */
#define	XPC_SMSEC	0x0D		/* last sector too small */
#define	XPC_SLACK	0x0E		/* slave ACK error (NXM) */
#define	XPC_HERR	0x12		/* cylinder/head header error */
#define	XPC_ASR		0x13		/* auto seek retry successful */
#define	XPC_WPE		0x14		/* write protect error */
#define	XPC_BADCMD	0x15		/* unimplemented command */
#define	XPC_DNR		0x16		/* drive not ready */
#define	XPC_SCZ		0x17		/* sector count zero */
#define	XPC_FAULT	0x18		/* drive faulted */
#define	XPC_ISS		0x19		/* illegal sector size */
#define	XPC_STA		0x1A		/* self test A */
#define	XPC_STB		0x1B		/* self test B */
#define	XPC_STC		0x1C		/* self test C */
#define	XPC_SECC	0x1E		/* soft ECC error */
#define	XPC_RECC	0x1F		/* recoverred ECC error */
#define	XPC_BADHEAD	0x20		/* illegal head */
#define	XPC_DSE		0x21		/* disk sequencer error */
#define	XPC_SEEKERR	0x25		/* seek error */

#define	XP_TNODISK	-1		/* illegal type */
#define	XP_T9766	0		/* CDC 9766 */
#define	XP_T9762	1		/* CDC 9762 */
#define	XP_TEAGLE	2		/* Fujitsu 2351 "Eagle" */

/*
 * ioctl argument structure
 */

struct	x450ioctl {
	char	cmd;		/* command */
	short	cyl;		/* cylinder */
	char	head;		/* head */
	char	sect;		/* sector */
	caddr_t	buf;		/* buffer address */
	u_short	scnt;		/* sector count */
};

/*
 * Multibus addressing macros
 */

#define	SETIOPBADDR(x, a) \
	x->xplba  = a;       \
	x->xpmba  = a >>  8; \
	x->xplrel = a >> 16; \
	x->xpmrel = a >> 24;

#define	GETIOPBADDR(a) \
	(a->xplba | (a->xpmba << 8) | (a->xplrel << 16) | (a->xpmrel << 24))
