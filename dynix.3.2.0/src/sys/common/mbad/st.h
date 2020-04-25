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

/* $Header: st.h 2.2 87/01/22 $ */

/* $Log:	st.h,v $
 */

/*
 * Per board data structure.
 */
struct stinfo {
	struct stdevice *st_addr;	/* board address */
	lock_t	st_lock;		/* board lock */
	u_long	st_cflags;		/* configuration flags */
	int	st_size;		/* number of lines */
	int	st_vector;		/* interrupt vector */
	int	st_mblevel;		/* MULTIBUS interrupt level */
	struct mb_desc *st_mbdesc; 	/* MULTIBUS descriptor */
	u_char	st_response[8];		/* current response buffer */
	int	st_response_index;	/* index into st_response */
	int	st_response_length;	/* length of response in st_response */
	int st_chars[16];		/* char input count in last interval */
	int st_rate[16];		/* smoothed input count */
	int st_mode[16];		/* imput mode: FAST or SLOW */
	struct tty st_tty[16];		/* tty structures */
	struct cblock *st_cblocks[16];	/* block input buffers */
};

/* Block input Mode low/high water */
struct stlh {
	u_short	st_low;			/* low water mark */
	u_short	st_high;		/* high water mark */
};

/* Block Input Modes - st_mode */
#define	SLOW	0
#define	FAST	1

#define STLINE(x)	(minor((x))&0xf)
#define STBOARD(x)	(minor((x))>>4)

/*
 * Registers and bits for Systech MTI terminal interface.
 */

/*
 * I/O registers, read-only (RO) and write-only (WO).
 */
struct	stdevice {
	u_char	stcmd;			/* WO Command FIFO */
#define		stresp	stcmd		/* RO Response FIFO */
	u_char	stie;			/* WO Interrupt Enable bits */
#define		ststat	stie		/* RO Status */
	u_char	stgo;			/* WO Set Execute */
#define		stcra	stgo		/* RO Clear Response Available */
	u_char	streset;		/* WO Reset board */
#define		stclrtim streset	/* RO Clear timer */
	u_char	stres[4];		/* Reserved */
};

/* Status register */
#define ST_VD		0x80	/* Valid Data */
#define ST_TIMER	0x10	/* Timer Ticked */
#define ST_ERR		0x04	/* Command Error */
#define ST_RA		0x02	/* Response Available */
#define ST_READY	0x01	/* Ready to Accept Command */

/* Interrupt enable (same bits as status register) */
#define STI_TIMER	0x10	/* Timer Ticked */
#define STI_ERR		0x04	/* Command Error */
#define STI_RA		0x02	/* Response Available */
#define STI_READY	0x01	/* Ready to Accept Command */

/* Commands */
#define	ST_ESCI		0x00		/* Enable Single Character Input */
#define	ST_RSTAT	0x10		/* Read USART Status Register */
#define	ST_RERR		0x20		/* Read Error Code */
#define	ST_RBDATA	0x30		/* Return Buffered Data */
#define	ST_OUT		0x40		/* Single Character Output */
#define	ST_WCMD		0x50		/* Write USART Command Register */
#define	ST_DSCI		0x60		/* Disable Single Character Input */
#define	ST_CONFIG	0x70		/* Configuration Command */
#define	ST_BLKIN	0x80		/* Block Input */
#define	ST_BSCIN	0x90		/* BSC input */
#define	ST_ABORTIN	0xA0		/* Abort Input */
#define	ST_SUSPEND	0xB0		/* Suspend Output */
#define	ST_BLKOUT	0xC0		/* Block Output */
#define	ST_RESUME	0xD0		/* Resume Output */
#define	ST_ABORTOUT	0xE0		/* Abort Output */

/* Configure Commands */
#define	STC_ASYNC	0x00		/* Configure Asynchronous */
#define	STC_SYNC	0x10		/* Configure Synchronous */
#define	STC_MODES	0x20		/* Configure Modes */
#define STC_BSC		0x30		/* Configure BSC */
#define	STC_INPUT	0x40		/* Configure Input */
#define	STC_TMASK	0x50		/* Termination Mask */
#define	STC_OUTPUT	0x60		/* Configure Output */
#define STC_MODEM	0x70		/* Modem Status */
#define	STC_BUFSIZ	0x80		/* Buffer Sizes */
#define	STC_TIMER	0x90		/* Configure Timer */

/* Configure Async bytes */
#define CCA2		0x03
#define CCA3		0x70
#define CCA4		0x27

/* Flags */
#define STF_BI	  	0x01		/* block input allowed */

/* 2661 Programming Bytes */
#define	STB_MSTOP1	0x40
#define STB_MSTOP15	0x80
#define STB_MSTOP2	0xC0
#define STB_ODD_PARITY	0x10
#define STB_EVEN_PARITY	0x30
#define STB_NO_PARITY	0x00
#define STB_BITS5	0x00
#define STB_BITS6	0x04
#define STB_BITS7	0x08
#define STB_BITS8	0x0C
#define STB_M50		0x00
#define STB_M75		0x01
#define STB_M110	0x02
#define STB_M134	0x03
#define STB_M150	0x04
#define STB_M300	0x05
#define STB_M600	0x06
#define STB_M1200	0x07
#define STB_M1800	0x08
#define STB_M2000	0x09
#define STB_M2400	0x0A
#define STB_M3600	0x0B
#define STB_M4800	0x0C
#define STB_M7200	0x0D
#define STB_M9600	0x0E
#define STB_M19200	0x0F

/*
 * Timer constants.
 * NOTE: The values in the names are somewhat
 * rounded because the timer is not highly accurate.
 */
#define	STT_530		0x0		/* 530 msec */
#define	STT_353		0x1		/* 353 msec */
#define	STT_241		0x2		/* 241 msec */
#define	STT_197		0x3		/* 197 msec */
#define	STT_177		0x4		/* 177 msec */
#define	STT_88		0x5		/* 88 msec */
#define	STT_44		0x6		/* 44 msec */
#define	STT_22		0x7		/* 22 msec */
#define	STT_15		0x8		/* 15 msec */
#define	STT_13		0x9		/* 13 msec */
#define	STT_11		0xa		/* 11 msec */
#define	STT_7		0xb		/* 7 msec */
#define	STT_5		0xc		/* 5 msec */
#define	STT_4		0xd		/* 4 msec */
#define	STT_3		0xe		/* 3 msec */
#define	STT_1		0xf		/* 1 msec */

/*
 * Usart command/status register bits
 */
#define DSR		0x80		/* Status */
#define DCD		0x40		/* Status */
#define FE		0x20		/* Status */
#define OE		0x10		/* Status */
#define PE		0x08		/* Status */
#define RTS		0x20		/* Cmd */
#define SBRK		0x08		/* Cmd */
#define RXEN		0x04		/* Cmd */
#define DTR		0x02		/* Cmd */
#define TXEN		0x01		/* Cmd */

/* Error Codes */
#define STE_MEMERR	0x04	/* Memory Address Error */
