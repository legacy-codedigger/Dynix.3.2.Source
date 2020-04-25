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

/*
 * $Header: xp.h 2.4 91/01/14 $
 *
 * Xylogics 450 multibus SMD disk controller definitions
 */

/* $Log:	xp.h,v $
 */

#define b_cylin b_resid		/* buffer field for job sorting */
#define b_ioctl b_proc		/* buffer field for ioctls */

#define	BADSCNT		/* controller returns incorrect sector count on done */
#define	QUEUE_ONE	/* force request queue to hold only one request */

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
	struct xpdiskaddr {
		u_char	io_head;	/* head address */
		u_char	io_sect;	/* sector address */
		u_short	io_cyl;		/* cylinder address */
	} io_diskaddr;
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
	/*
	 * the following fields are used only by the software
	 */
	struct	xp_iopb	*io_forw;	/* next iopb on free/new/done queue */
	struct	xp_iopb	*io_actf;	/* next active iopb */
	struct	xp_iopb	*io_actb;	/* previous active iopb */
	struct	buf	*io_bp;		/* the buffer header for this request */
	short	io_errcnt;		/* retry count */
	short	io_type;		/* type of operation */
	short	io_ctlr;		/* controller associated with iopb */
	u_char	io_basemap;		/* offset of maps to use */
	u_char	io_nmaps;		/* number of maps allocated */
	char	io_flag;		/* processed: disposition (see below) */
	/*
	 * state saved during sector revectoring
	 */
	struct xpdiskaddr io_sdiskaddr;	/* saved diskaddr */
	u_short	io_sscnt;		/* saved sector count */
	int	io_bbn;			/* bad block number to revector */
	/*
	 * initial state for error retries
	 */
	struct xpdiskaddr io_idiskaddr;	/* initial disk adress  */
	u_short	io_iscnt;		/* initial sector count */
	/* two byte hole */
	u_long	io_ilbaddr;		/* initial memory address */
};

#define FREE_IOPB	1		/* an iopb can be freed */
#define XP_COMMASK	0xf		/* disk command mask */

/*
 * dispositions
 */

#define	D_PROCESSED	0x01		/* processed */
#define	D_REQUEUE	0x02		/* requeue for error recovery */
#define	D_RESET		0x04		/* reset iopb to start of transfer */
#define	D_REVEC		0x08		/* revector bad sector */
#define	D_CONTINUE	0x10		/* continue after revectoring */

#ifdef QUEUE_ONE
#define	NIOPB	1
#else
#define	NIOPB	(MB_MRSIZE/sizeof (struct xp_iopb))	/* 1K of iopbs */
#endif QUEUE_ONE

#define	I_NORMAL	1	/* normal transfer */
#define	I_REVEC		2	/* revectoring complete */

#define XP_TIMER	3000000	/* timer for commands that do not generate
				 * interrupts
				 */
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
#define	XPT_INTLV	0x78	/* interleave factor */
#define	XPT_BYTE	0x80	/* byte mode for DMA */

/*
 * io_dstat: returned by Read Drive Status
 */

#define	XPS_DFLT	0x04	/* drive faulted */
#define	XPS_SKER	0x08	/* seek error */
#define	XPS_DPB		0x10	/* dual port busy */
#define	XPS_WRPT	0x20	/* drive write protected */
#define	XPS_DNRDY	0x40	/* drive not ready */
#define	XPS_OFFCYL	0x80	/* off cylinder */

/*
 * per controller state
 */

struct	xp_softc {
	u_short		 sc_nmaps;	/* number of maps for data */
	char		 sc_need;	/* interrupt needed: timer state */
	u_char		 sc_level;	/* interrupt level */
	short		 sc_dmap;	/* Multibus map base for data */
	short		 sc_ipmap;	/* Multibus map base for iopb's */
	struct	mb_desc	 *sc_desc;	/* multibus adaptor information */
	caddr_t		 sc_base;	/* base address of iopb array */
	struct	xp_iopb	 *sc_active;	/* list of iopbs currently executing */
	struct	xp_iopb	 *sc_done;	/* list of iopbs to be removed */
	struct	xp_iopb	 *sc_free;	/* free iopbs */
	struct	xp_iopb	 *sc_savechain;	/* saved iopb chain */
	struct	xp_iopb	 *sc_resiopb;	/* iopb used for drive reset command */
	long		 sc_mbaddr;	/* multibus address of iopb's */
	struct	xpdevice *sc_xpaddr;	/* device address */
	char		 *sc_malloc;	/* map allocation management */
	struct	xpheader *sc_header;	/* buffer for manipulating headers */
	struct  buf	 sc_bhead;	/* buffer header for i/o queue */
	lock_t		 sc_lock;	/* controller lock */
	char		 sc_alive;	/* controller present flag */
	char		 sc_opened;	/* controller has been opened */
	u_char		 sc_drives;	/* bit vector of present drives */
	u_char		 sc_newiopb;	/* iopb added to active chain*/
};

/*
 * states
 */

#define	SIDLE	0			/* controller idle */
#define	SACTIVE	1			/* transferring data */
#define	SIPND	2			/* attention request pending */

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
#define	XPC_RECC	0x1F		/* recovered ECC error */
#define	XPC_BADHEAD	0x20		/* illegal head */
#define	XPC_DSE		0x21		/* disk sequencer error */
#define	XPC_SEEKERR	0x25		/* seek error */

struct	xpst	{
	short	st_nsect;		/* number of sectors */
	short	st_ntrack;		/* number of tracks */
	short	st_nspc;		/* sectors per cylinder */
	short	st_ncyl;		/* number of cylinders */
	struct	cmptsize *st_size;	/* partition table */
	short	st_type;		/* drive type */
	char	*st_name;		/* drive name */
};

/* these should be moved to conf_xp.c */
#define	XP_TNODISK	-1 		/* illegal type */
#define	XP_T9766	0 		/* CDC 9766 */
#define	XP_T9762	1 		/* CDC 9762 */
#define	XP_TEAGLE	2 		/* Fujitsu 2351 "Eagle" */

struct	xp_unit {
	int		u_ctlr;		/* controller number */
	int		u_drive;	/* drive number on controller */
	struct xpst	*u_st;		/* size table */
	struct xp_softc	*u_sc;		/* controller information */
	struct buf	u_ctlbuf;	/* buffer header for ioctls */
	struct dkbad	*u_bad;		/* bad block info */
	struct dk	*u_dk;		/* pointer to dk stats */
	int		u_active;	/* number of active iopbs */
	int		u_lastcyl;	/* last cylinder we were on */
	struct timeval	u_starttime;	/* xfer start time */
	char		u_alive;	/* drive is present */
	char		u_allbusy;	/* Whole-disk partition is open */
					/* for writing */
	sema_t		u_xpsema;	/* open serialisation semaphore */
	struct vtoc	*u_xppart;	/* VTOC partition infomation */
	int		u_nopen;	/* drive open counter */
	unsigned short	*u_opens;	/* # opens per partition */
	unsigned int	*u_modes;	/* modes per partition */
	char		u_xpvtoc_read;	/* Do we need to read the VTOC? */
};

/*
 * Multibus addressing macros
 */

#define	IOPBMBADDR(ip, sc) ((sc)->sc_mbaddr + ((caddr_t)(ip) - (sc)->sc_base))

#define	SETIOPBADDR(x, a)    \
	(x)->xplba  = (a),       \
	(x)->xpmba  = (a) >>  8, \
	(x)->xplrel = (a) >> 16, \
	(x)->xpmrel = (a) >> 24

#define	GETIOPBADDR(x) \
	((x)->xplba | ((x)->xpmba<<8) | ((x)->xplrel<<16) | ((x)->xpmrel<<24))

/*
 * special function definitions
 */

struct	xpheader {
	long	header;				/* header */
	char	data[512];			/* data */
	char	ecc[4];				/* data ecc */
};

#define	BADHEADER	0xEEEEEEEE		/* invalid header field */

/*
 * binary configuration definitions
 */

#define	ANY	(-1)				/* accept any drive or ctlr */

#ifdef KERNEL
extern int		xpmaxunit;
extern short		xpmaxretry;
extern struct xp_unit	xpunits[];
extern struct xp_softc	*xp_softc;
extern gate_t		xpgate;
extern int		xpsrequests[];
extern u_char		xpthrottle;
#endif KERNEL
