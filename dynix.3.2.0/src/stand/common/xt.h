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

/* $Header: xt.h 2.0 86/01/28 $ */

/*
 * Xylogics 472 multibus tape controller
 * registers and bits.
 */

struct xtdevice
{
	u_char	xtlrel;		/* lsb of the iopb relocation */
	u_char	xtmrel;		/* msb of the iopb relocation */
	u_char	xtlba;		/* least significant byte of the iopb address */
	u_char	xtmba;		/* most significant byte of the iopb address */
	u_char	xtcsr;		/* control and status register */
	u_char	xtreset;	/* reset and update register */
};

/* 
 * xtcsr
 */

#define	XT_DRDY		0x01	/* selected drive is ready */
#define	XT_AACK		0x02	/* attention acknowledge */
#define	XT_AREQ		0x04	/* attention request */
#define	XT_ADDR24	0x08	/* 24 bit address mode when set */
#define	XT_IPND		0x10	/* interrupt pending */
#define	XT_DERR		0x20	/* bus error (double error) */
#define	XT_ERR		0x40	/* general error */
#define	XT_GO		0x80	/* execute an iopb (write only) */
#define	XT_BUSY		0x80	/* controller busy (read only) */

struct xt_iopb
{
	u_short	io_scomm;	/* subfunction and tape command */
	u_char	io_status;	/* error flag, controller type, done flag */
	u_char	io_compcode;	/* completion code */
	u_char	io_dstat;	/* drive status */
	u_char	io_imode;	/* interrupt mode */
	u_char	io_throt;	/* throttle */
	u_char	io_drive;	/* unit select */
	u_short	io_cnt;		/* count */
	u_short	io_baddr;	/* low  order memory address */
	u_short	io_xbaddr;	/* high order memory address */
	u_short	io_niop;	/* next iopb address */
	u_short	io_acnt;	/* actual count */
};

#define	SETBADDR(ip, a) { \
	ip->io_baddr = a; \
	ip->io_xbaddr = (a) >> 16; \
}

/*
 * io_scomm: function codes
 */

#define	XT_NOP		0x00		/* no operation */
#define	XT_WRITE	0x01		/* write */
#define	XT_READ		0x02		/* read */
#define	XT_SEEK		0x05		/* position */
#define	XT_DRESET	0x06		/* drive reset */
#define	XT_FMARK	0x07		/* write file mark / erase */
#define	XT_DSTAT	0x09		/* read drive status */
#define	XT_SPARAM	0x0B		/* set parameters */
#define	XT_STEST	0x0C		/* self test */

#define	XT_IEN		0x10		/* interrupt enable */
#define	XT_CHEN		0x20		/* command-chaining enable */
#define	XT_RELO		0x40		/* data relocation enable */
#define	XT_AUD		0x80		/* auto iopb update enable */

/*
 * io_scomm: subfunction codes
 */

#define	XT_REC		(0x00 << 8)	/* search for record marks */
#define	XT_FILE		(0x01 << 8)	/* search for file marks */
#define	XT_REW		(0x02 << 8)	/* rewind */
#define	XT_UNLOAD	(0x03 << 8)	/* unload */

#define	XT_ERASE	(0x01 << 8)	/* erase, when used with FMARK */

#define	XT_PE		(0x00 << 8)	/* low density */
#define	XT_GCR		(0x01 << 8)	/* high density */
#define	XT_LOW		(0x02 << 8)	/* low speed */
#define	XT_HIGH		(0x03 << 8)	/* high speed */

#define	XT_REV		(0x20 << 8)	/* reverse */
#define	XT_RETY		(0x40 << 8)	/* enable retry */
#define	XT_SWAP		(0x80 << 8)	/* swap bytes */

/* 
 * io_status
 */

#define	XTS_ERROR	0x80		/* error */
#define	XTS_DONE	0x01		/* command done */

/*
 * io_compcode: completion codes
 */

#define	XTC_GOOD	0x00		/* successful completion */
#define	XTC_IPND	0x01		/* interrupt pending */
#define	XTC_CONFLCT	0x03		/* busy conflict */
#define	XTC_TIMO	0x04		/* operation timeout */
#define	XTC_UNCD	0x06		/* uncorrectable data */
#define	XTC_SLACK	0x0E		/* slave ACK error (NXM) */
#define	XTC_WPE		0x14		/* write protect error */
#define	XTC_BADCMD	0x15		/* unimplemented command */
#define	XTC_OFFL	0x16		/* drive off line */
#define	XTC_STA		0x1A		/* self test A */
#define	XTC_STB		0x1B		/* self test B */
#define	XTC_STC		0x1C		/* self test C */
#define	XTC_TMARK	0x1D		/* tapemark failure */
#define	XTC_TMREAD	0x1E		/* tape mark detected on read */
#define	XTC_CORR	0x1F		/* corrected data */
#define	XTC_SHORT	0x22		/* record length short */
#define	XTC_LONG	0x23		/* record length long */
#define	XTC_RBOT	0x30		/* reverse into BOT */
#define	XTC_EOT		0x31		/* EOT detected */

/*
 * io_dstat
 */

#define	XTS_FBY		0x01		/* formatter busy */
#define	XTS_DBY		0x02		/* data busy */
#define	XTS_RDY		0x04		/* drive ready */
#define	XTS_ONL		0x08		/* on line */
#define	XTS_REW		0x10		/* rewinding */
#define	XTS_FPT		0x20		/* write protected */
#define	XTS_BOT		0x40		/* beginning of tape */
#define	XTS_EOT		0x80		/* end of tape */

/*
 * io_imode
 */

#define	XTS_IEI		0x40		/* interrupt on each iopb */

/*
 * throttle bits
 */

#define	XTT_T2		0x00	/*  2 transfer throttle */
#define	XTT_T4		0x01	/*  4 transfer throttle */
#define	XTT_T8		0x02	/*  8 transfer throttle */
#define	XTT_T16		0x03	/* 16 transfer throttle */
#define	XTT_T32		0x04	/* 32 transfer throttle */
#define	XTT_T64		0x05	/* 64 transfer throttle */
#define	XTT_T64x	0x06	/* 64 transfer throttle */
#define	XTT_T64y	0x07	/* 64 transfer throttle */
#define	XTT_BYTE	0x80	/* byte mode for DMA */
