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
 * $Header: sp.h 1.1 90/06/21 $
 *
 * sp.h
 *	SSM-based parallel printer port command block and definitions.
 *
 * There are two CBs used for each parallel port:
 *    transmit and message-passing.
 */

/* $Log:	sp.h,v $
 */

#ifdef SPDEBUG
#define dprintf		printf
#else SPDEBUG
#define dprintf
#endif SPDEBUG

struct sp_printer {
	int	sp_width;
	int	sp_ops;
	u_char	sp_interface;		/* flag for Centronics or DataProducts */
};

/* 
 * Printer port control structure.  One allocated per SSM printer port. 
 */

struct	sp_info {
	struct	ssm_dev	  *sp_dev;	/* Device descriptor from auto-conf */
	short	sp_devno;		/* device number */
	spl_t	sp_spl;		/* saved interrupt level */
	u_char	sp_state;		/* state flags below */
	u_char	sp_ops;		/* character mapping mode */
	u_int	sp_width;		/* page width */
	lock_t	sp_lock;		/* lock access */
	sema_t	sp_wait;		/* wait on events */ 
	sema_t	sp_bufsema;		/* synchronize transfer buf access */ 
	char	*sp_curp;		/* ptr to current loc in buffer */
	int	sp_count;		/* count of chars in buffer */
	int	sp_xcount;		/* transmit count */
	char	*sp_xptr;		/* transmit pointer */
	u_int	sp_col;		/* phys. print column */
	u_int	sp_lcol;		/* logical print column */
	u_int	sp_line;		/* vertical page position */
	char	*sp_buffer;		/* pointer to transmit buffer */
};

/*
 * State flags
 */
#define	SP_OPEN		0x01		/* device open */
#define SP_WOPEN	0x02		/* open in progress */ 
#define	SP_BUSY		0x04		/* device busy */
#define SP_READY	0x08		/* device ready */
#define SP_ERROR	0x10		/* printer error detected */
#define SP_DATAOUT	0x20		/* asynchronous I/O outstanding */

/*
 * Values for sp_ops 
 */
#define SPDEFAULT	0x00		/* standard mapping */
#define SPCAPS		0x01		/* caps only */
#define SPRAW		0x02		/* no mapping */

/*
 * Miscellaneous
 */
#define FORMFEED	'\f'		/* form feed char */
#define SP_SPL		4		/* interrupt level of sp */ 
#define SPRBSIZE	100		/* size of staging buffer (on stack)*/
#define SP_BUFSIZE	4096		/* sp buffer size in bytes */
#define SPLONGLINE	1000		/* maximum line length */

/*
 * Hardware related defines
 */
#define	NPRINTDEV	1		/* One port per SSM */
#define	PCB_PORT0	0		/* only 1 printer port */

/*
 * Vectors for interrupts to SSM
 */
#define PCB_ADDRVEC	0		/* CB addresses */
#define	PCB_XMIT_V	1		/* xmit */
#define	PCB_MSG_V	2		/* message */
#define PCB_FLUSH_V	3		/* flush */
/* Number of interrupt vectors per unit for outgoing interrupts */
#define NVEC_TO_SSM	4	
#define NVECSHIFT	2		/* log2(NVEC_TO_SSM) */

/*
 * Vectors for interrupts from SSM
 */
#define PCB_XMIT_COMPLETE	0	/* xmit complete */
#define PCB_READY		1	/* lp ready */
/* Number of interrupt vectors per unit for incoming interrupts */
#define NVEC_FROM_SSM		2

/*
 * Command block indices
 */
#define PCB_XMIT_CB	0		/* xmit */
#define PCB_MSG_CB	1		/* message */
/* Number of command blocks per unit */
#define NCBPERPRINT	2

#define PRINT_BASE_CB(base,dev)  ((base) + ((dev) * NCBPERPRINT))

/*
 * Given vector type 
 * compute SLIC interrupt vector
 */
#define	PRVEC(cbfunc)	(((PCB_PORT0) << NVECSHIFT) | (cbfunc))

/*
 * SLIC interrupt bins for sending printer 
 * port commands to SSM.
 */
#define	PRINT_BIN	0x05		/* bin used to send CB I/O intrs */

/*
 * Important CB sizes.
 */
#define	PCB_SHSIZ	16		/* size of shared portion */
#define	PCB_SWSIZ	16		/* size of s/w-only portion */

/*
 * Generic printer port CB
 */
struct print_cb {
	u_long	cb_reserved;		/* reserved for Sequent use */
	u_char	cb_fill[10];		/* pad to PCB_SHSIZ bytes */
	u_char	cb_cmd;			/* command byte */
	u_char	cb_status;		/* xfer status */

	/* start of sw-only part */
	u_long	cb_sw[PCB_SWSIZ/sizeof(u_long)];
};

/* 
 * cb_cmd values 
 */
#define	PCB_IENABLE	0x80		/* enable interrupts */
#define	PCB_CMD_MASK	0x0F		/* command bits */
#define	    PCB_XMIT	0x00		/* transmit data specified */
#define	    PCB_GSTAT	0x01		/* get interface line status */
#define	    PCB_INIT	0x02		/* init interrupt generation */
#define     PCB_FLUSH	0x04		/* flush current data */

/* 
 * cb_status values 
 */
#define	PCB_BUSY	0x00		/* working on command */
#define	PCB_OK		0x01		/* completed OK */
#define	PCB_ERR		0x02		/* completed due to ERROR line */
					/*    asserted by printer.     */
/*
 * Transmit CB 
 */
struct print_xcb {
	u_long	xcb_reserved;		/* reserved for Sequent use */
	u_long	xcb_addr;		/* physical addr of cblock */
	u_char	xcb_fill[3];		/* fill to PCB_SHSIZ bytes */
	u_char  xcb_pstatus;		/* printer status */
	u_short	xcb_count;		/* transfer count */
	u_char	xcb_cmd;		/* command byte */
	u_char	xcb_status;		/* xfer status */

	/* start of sw-only part */
	u_long	xcb_sw[PCB_SWSIZ/sizeof(u_long)];
};

/*
 * Message CB : get printer interface line status
 */
struct print_mcb {
	u_long	mcb_reserved;		/* reserved for Sequent use */
	u_char	mcb_fill[8];		/* pad to PCB_SHSIZ bytes */
	u_short	mcb_iface;		/* interface line status */
	u_char	mcb_cmd;		/* command byte */
	u_char	mcb_status;		/* command status */

	/* start of sw-only part */
	u_long	mcb_sw[PCB_SWSIZ/sizeof(u_long)];
};

/* 
 * mcb_iface values 
 */
#define	PCM_PE    	0x0001		/* Printer detected a PAPER ERROR */
#define	PCM_OFFLINE	0x0002		/* Printer is OFFLINE */ 
#define	PCM_ERR		0x0004		/* Printer detected an ERROR */

#define PCM_READY	0x0000		/* NOT(any of above bit fields) */

/*
 * Message CB : interrupt generation
 */
struct print_icb {
	u_long	icb_reserved;		/* reserved for Sequent use */
	u_char	icb_scmd;		/* SLIC command for interrupts */
	u_char	icb_dest;		/* SLIC dest for interrupts */
	u_char	icb_basevec;		/* SLIC base vector for intrs */
	u_char	icb_interface;		/* type of printer interface */
	u_char	icb_fill[6];		/* pad to PCB_SHSIZ bytes */
	u_char	icb_cmd;		/* command byte */
	u_char	icb_status;		/* xfer status */

	/* start of sw-only part */
	u_long	icb_sw[PCB_SWSIZ/sizeof(u_long)];
};

/*
 * icb_interface values
 */
#define ICB_CENTRON	0				/* Centronics interface */
#define ICB_DATAP	1				/* Data Products interface */
