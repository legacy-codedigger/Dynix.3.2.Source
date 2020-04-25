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

/* $Header: ssm_cons.h 1.1 90/07/06 $
 *
 * sc.h
 *	SSM standalone console input driver structure definitions.
 */

/* $Log:	ssm_cons.h,v $
 */

/*
 * Console unit definitions.
 */
#define	NCONSDEV	2		/* Units per SSM: local or remote */
#define	CCB_LOCAL	0		/* Local console port */
#define	CCB_REMOTE	1		/* Remote console port */
#define CONS_FLAG	0x40		/* Minor dev number of /dev/console */
/*
 * Console command block (CB) definitions.
 */
#define	NCBPERCONS	4		/* One transmit, receive, modem status, 
					 * and unused CB per unit */
#define	NCBCONSHFT	2		/* Log2(NCBPERCONS) */

/* Types of console CB's */
#define	CCB_RECV_CB	0		/* Receive CB */
#define	CCB_XMIT_CB	1		/* Transmit CB */
#define	CCB_MSG_CB	2		/* Message-passing CB */
#define	CCB_UNUSED_CB	3		/* Currently unused CB */

/* Shift value for processing SSM interrupt vector. */
#define CCB_INTRSHFT    1
 
 /* 
 * Compute address of cons_cbs[unit][0].
 */
#define	CONS_BASE_CB(cons_cbs,unit)	((cons_cbs) + ((unit) << NCBCONSHFT))

/*
 * Given a console unit number and a CB type
 * value compute its SLIC interrupt vector.
 */
#define	COVEC(unit,type)	((unit) << NCBCONSHFT | (type))

/*
 * Extract device unit number and CB type 
 * number from SLIC vector.
 */
#define	COVEC_DEV(vec)	((vec) >> NCBCONSHFT & NCONSDEV - 1)
#define	COVEC_CB(vec)	((vec) & NCBPERCONS - 1)

/*
 * SLIC interrupt bins for sending console 
 * commands to SSM.
 */
#define	CONS_BIN	0x04		/* Bin used to send CB I/O intrs */

#define CONS_CMD	0x80		/* msb set for unique console command */
/*
 * notify that an address vector sequence has started, stop the xmit CB for 
 * the unit, flush all I/O for the unit.
 */
#define C_CMD_MASK	0x70
#define C_ADDR		0x40
#define C_STOP		0x20
#define C_FLUSH 	0x10
#define	CONS_ADDRVEC	CONS_CMD  | C_ADDR
#define CONS_STOP(unit) (CONS_CMD | C_STOP | COVEC(unit, CCB_XMIT_CB))
/* FLUSH 'type' can be either CCB_XMIT_CB or CCB_RECV_CB */
#define CONS_FLUSH(unit, type) (CONS_CMD | C_FLUSH | COVEC(unit, type))

/*
 * extract command from SLIC vector
 */
#define COVEC_CMD(vec) ((vec)&(CONS_CMD) ? (vec)&(C_CMD_MASK) : 0)

/*
 * Important CB alignments.
 */
#define	CCB_SHSIZ	16		/* Size of shared portion */
#define	CCB_SWSIZ	16		/* Size of s/w-only portion */

/*
 * GENERIC console CB.
 */
struct cons_cb {
	u_long	cb_reserved;		/* Reserved for Sequent use */
	u_char	cb_fill[10];		/* Pad to CCB_SHSIZ bytes */
	u_char	cb_cmd;			/* Command byte */
	u_char	cb_status;		/* Transfer status */

	/* Start of sw-only part */
	u_long	cb_sw[CCB_SWSIZ/sizeof(u_long)];
};

/* cb.cb_cmd values */
#define	CCB_IENABLE	0x80		/* Enable interrupts */
#define	CCB_TERM_MS	0x40		/* Quit cmd on modem status change */
#define	CCB_CMD_MASK	0x0F		/* Command bits */
#define	    CCB_XMIT	0x00		/* Transmit data specified */
#define	    CCB_RECV	0x01		/* Receive bytes from console */
#define	    CCB_GTTY	0x02		/* Get tty settings */
#define	    CCB_STTY	0x03		/* Set tty settings */
#define	    CCB_INIT	0x04		/* Init interrupt generation */

/* cb.cb_status values */
#define	CCB_BUSY	0x00		/* Command in progress */
#define	CCB_OK		0x01		/* Completed OK */
#define	CCB_MS_CHG	0x02		/* Completed due to modem
					 * status change */

/*
 * RECEIVE console CB.
 */
struct cons_rcb {
	u_long	rcb_reserved;		/* Reserved for Sequent use */
	u_long	rcb_addr;		/* Physical addr of cblock */
	u_long	rcb_timeo;		/* Timeout request in this # ms */
	u_short	rcb_count;		/* Transfer count */
	u_char	rcb_cmd;		/* Command byte */
	u_char	rcb_status;		/* Transfer status */

	/*Start of sw-only part */
	u_long	rcb_sw[CCB_SWSIZ/sizeof(u_long)];
};

/* cb.rcb_status for received data */
#define	CCB_TIMEO	0x03		/* Input timed out after rcb_timeo ms */
#define	CCB_FERR	0x04		/* Input char got a framing error */
#define	CCB_PERR	0x05		/* Input char got a parity error */
#define	CCB_OVERR	0x06		/* Input got an overrun error */
#define	CCB_BREAK	0x07		/* Input char was BREAK */

/*
 * TRANSMIT console CB.
 */
struct cons_xcb {
	u_long	xcb_reserved;		/* Reserved for Sequent use */
	u_char	*xcb_addr;		/* Physical addr of cblock */
	u_long	xcb_fill;		/* Fill to CCB_SHSIZ bytes */
	u_short	xcb_count;		/* Transfer count */
	u_char	xcb_cmd;		/* Command byte */
	u_char	xcb_status;		/* Transfer status */

	/* Start of sw-only part */
	u_long	xcb_sw[CCB_SWSIZ/sizeof(u_long)];
};

/*
 * MESSAGE console CB: get/set tty parameters.
 */
struct cons_mcb {
	u_long	mcb_reserved;		/* Reserved for Sequent use */
	u_char	mcb_baud;		/* Baud rate */
	u_char	mcb_parsiz;		/* Parity and char size */
	u_char	mcb_flow;		/* Flow control flags */
	u_char	mcb_fill[5];		/* Pad to CCB_SHSIZ bytes */
	u_short	mcb_modem;		/* Modem status */
	u_char	mcb_cmd;		/* Command byte */
	u_char	mcb_status;		/* Transfer status */

	/* Start of sw-only part */
	u_long	mcb_sw[CCB_SWSIZ/sizeof(u_long)];
};

/* cb.mcb_parsiz */
#define	CCP_PMASK	0x07		/* Parity bits */
#define	    CCP_NONE	0x00		/* Generate no parity bit */
#define	    CCP_EVENP	0x01		/* Generate even parity */
#define	    CCP_ODDP	0x02		/* Generate odd parity */
#define	    CCP_SPACEP	0x03		/* Generate space parity */
#define	    CCP_MARKP	0x04		/* Generate mark parity */
#define	CCP_CSMASK	0x30		/* Character size bits */
#define	    CCP_CSIZ5	0x00		/* 5-bit characters */
#define	    CCP_CSIZ6	0x10		/* 6-bit characters */
#define	    CCP_CSIZ7	0x20		/* 7-bit characters */
#define	    CCP_CSIZ8	0x30		/* 8-bit characters */
#define	CCP_STMASK	0xC0		/* Stop-bits mask */
#define	    CCP_ST1	0x00		/* 1 stop bit */
#define	    CCP_ST1P5	0x40		/* 1.5 stop bits */
#define	    CCP_ST2	0xC0		/* 2 stop bits */

/* cb.mcb_flow values */
#define	CCF_NOFLOW	0x00		/* Don't do any flow control */
#define	CCF_XOFF	0x01		/* Generate XOFF/XON flow control */
#define	CCF_HWFLOW	0x02		/* Generate hardware flow control */

/* cb.mcb_modem values */
#define	CCM_OVFL	0x0001		/* Usart overflowed */
#define	CCM_PARITY	0x0002		/* Parity error received */
#define	CCM_FRAME	0x0004		/* Framing error received*/
#define	CCM_CTS		0x0008		/* Clear-to-send */
#define	CCM_DSR		0x0010		/* Data set ready */
#define	CCM_RI		0x0020		/* Ring indicator */
#define	CCM_DCD		0x0040		/* Data carrier detect */
#define	CCM_BREAK	0x0100		/* BREAK received, or gen BREAK */
#define	CCM_DTR		0x0200		/* Data terminal ready */
#define	CCM_RTS		0x0400		/* Request to send */

/*
 * Transient and setable bits in cb.mcb_modem.
 */
#define	CCM_TRANSIENT	(CCM_OVFL | CCM_PARITY | CCM_FRAME | CCM_CTS \
			 | CCM_DSR | CCM_RI | CCM_DCD)
#define	CCM_SETABLE	(CCM_BREAK | CCM_DTR | CCM_CTS | CCM_DSR | CCM_RTS)

/*
 * MESSAGE console CB: Interrupt generation.
 */
struct cons_icb {
	u_long	icb_reserved;		/* Reserved for Sequent use */
	u_char	icb_scmd;		/* SLIC command for interrupts */
	u_char	icb_dest;		/* SLIC dest for interrupts */
	u_char	icb_basevec;		/* SLIC base vector for intrs */
	u_char	icb_fill[7];		/* Pad to CCB_SHSIZ bytes */
	u_char	icb_cmd;		/* Command byte */
	u_char	icb_status;		/* Transfer status */

	/* Start of sw-only part */
	u_long	icb_sw[CCB_SWSIZ/sizeof(u_long)];
};

