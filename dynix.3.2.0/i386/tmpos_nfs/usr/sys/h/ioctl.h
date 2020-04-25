/* $Copyright: $
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
 * $Header: ioctl.h 2.18 1991/05/28 23:35:27 $
 */

/* $Log: ioctl.h,v $
 *
 *
 */

/*
 * Ioctl definitions
 */
#ifndef	_IOCTL_
#define	_IOCTL_
#ifdef KERNEL
#include "../h/ttychars.h"
#include "../h/ttydev.h"
#else
#include <sys/ttychars.h>
#include <sys/ttydev.h>
#endif

#include <sgtty.h>
struct tchars {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};
struct ltchars {
	char	t_suspc;	/* stop process signal */
	char	t_dsuspc;	/* delayed stop process signal */
	char	t_rprntc;	/* reprint line */
	char	t_flushc;	/* flush output (toggles) */
	char	t_werasc;	/* word erase */
	char	t_lnextc;	/* literal next character */
};

/*
 * Window size structure
 */
struct winsize {
	unsigned short	ws_row, ws_col;		/* character size of window */
	unsigned short	ws_xpixel, ws_ypixel;	/* pixel size of window */
};

#ifndef _IO
/*
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 3 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#define	IOCPARM_MASK	0x7f		/* parameters must be < 128 bytes */
#define IOCPARM_LEN(x)	(((x) >> 16) & IOCPARM_MASK)
#define	IOC_VOID	(int)0x20000000	/* no parameters */
#define	IOC_OUT		(int)0x40000000	/* copy out parameters */
#define	IOC_IN		(int)0x80000000	/* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)
/* the 0x20000000 is so we can distinguish new ioctl's from old */
#define	_IO(x,y)	(IOC_VOID|('x'<<8)|y)
#define	_IOR(x,y,t)	(IOC_OUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define	_IOW(x,y,t)	(IOC_IN|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
/* this should be _IORW, but stdio got there first */
#define	_IOWR(x,y,t)	(IOC_INOUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)

/* 
 * in 3.0.4 TIOCFLUSH became _IO(t, 16) and OLD_TIOCFLUSH _IOW(t, 16, int).
 * in 3.2.0 TIOCFLUSH became _IOW(t, 16, int) and OLD_TIOCFLUSH _IO(t, 16).
 * This means before 3.0.4 we were not 4.2 compatable.
 * after 3.0.4 we were 4.2 safe but 4.3 incompatable.
 * after 3.2 we are now 4.3 compatable but 4.2 unsafe.
 */

#define OLD_TIOCFLUSH   _IO(t, 16)                /* flush buffers */

/* This is for backward binary compatablity for v3.0.17 binaries */

#define _X_IOR(x,y,t)     (IOC_OUT|(((t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define _X_IOW(x,y,t)     (IOC_IN|(((t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define OLD_TIOCGETP    _X_IOR(t, 8, 6)
#define OLD_TIOCSETP    _X_IOW(t, 9, 6)
#define OLD_TIOCSETN    _X_IOW(t,10, 6)

#endif

/*
 * tty ioctl commands
 */
#define	TIOCGETD	_IOR(t, 0, int)		/* get line discipline */
#define	TIOCSETD	_IOW(t, 1, int)		/* set line discipline */
#define	TIOCHPCL	_IO(t, 2)		/* hang up on last close */
#define	TIOCMODG	_IOR(t, 3, int)		/* get modem control state */
#define	TIOCMODS	_IOW(t, 4, int)		/* set modem control state */
#define		TIOCM_LE	0001		/* line enable */
#define		TIOCM_DTR	0002		/* data terminal ready */
#define		TIOCM_RTS	0004		/* request to send */
#define		TIOCM_ST	0010		/* secondary transmit */
#define		TIOCM_SR	0020		/* secondary receive */
#define		TIOCM_CTS	0040		/* clear to send */
#define		TIOCM_CAR	0100		/* carrier detect */
#define		TIOCM_CD	TIOCM_CAR
#define		TIOCM_RNG	0200		/* ring */
#define		TIOCM_RI	TIOCM_RNG
#define		TIOCM_DSR	0400		/* data set ready */
#define	TIOCGETP	_IOR(t, 8,struct sgttyb)/* get parameters -- gtty */
#define	TIOCSETP	_IOW(t, 9,struct sgttyb)/* set parameters -- stty */
#define	TIOCSETN	_IOW(t,10,struct sgttyb)/* as above, but no flushtty */
#define	TIOCEXCL	_IO(t, 13)		/* set exclusive use of tty */
#define	TIOCNXCL	_IO(t, 14)		/* reset exclusive use of tty */
#define	TIOCFLUSH	_IOW(t, 16, int)	/* flush buffers */
#define	TIOCSETC	_IOW(t,17,struct tchars)/* set special characters */
#define	TIOCGETC	_IOR(t,18,struct tchars)/* get special characters */
#define		TANDEM		0x00000001	/* send stopc on out q full */
#define		CBREAK		0x00000002	/* half-cooked mode */
#define		LCASE		0x00000004	/* simulate lower case */
#define		ECHO		0x00000008	/* echo input */
#define		CRMOD		0x00000010	/* map \r to \r\n on output */
#define		RAW		0x00000020	/* no i/o processing */
#define		ODDP		0x00000040	/* get/send odd parity */
#define		EVENP		0x00000080	/* get/send even parity */
#define		ANYP		0x000000c0	/* get any parity/send none */
#define		NLDELAY		0x00000300	/* \n delay */
#define			NL0	0x00000000
#define			NL1	0x00000100	/* tty 37 */
#define			NL2	0x00000200	/* vt05 */
#define			NL3	0x00000300
#define		TBDELAY		0x00000c00	/* horizontal tab delay */
#define			TAB0	0x00000000
#define			TAB1	0x00000400	/* tty 37 */
#define			TAB2	0x00000800
#define		XTABS		0x00000c00	/* expand tabs on output */
#define		CRDELAY		0x00003000	/* \r delay */
#define			CR0	0x00000000
#define			CR1	0x00001000	/* tn 300 */
#define			CR2	0x00002000	/* tty 37 */
#define			CR3	0x00003000	/* concept 100 */
#define		VTDELAY		0x00004000	/* vertical tab delay */
#define			FF0	0x00000000
#define			FF1	0x00004000	/* tty 37 */
#define		BSDELAY		0x00008000	/* \b delay */
#define			BS0	0x00000000
#define			BS1	0x00008000
#define 	ALLDELAY	(NLDELAY|TBDELAY|CRDELAY|VTDELAY|BSDELAY)
#define		CRTBS		0x00010000	/* do backspacing for crt */
#define		PRTERA		0x00020000	/* \ ... / erase */
#define		CRTERA		0x00040000	/* " \b " to wipe out char */
#define		TILDE		0x00080000	/* hazeltine tilde kludge */
#define		MDMBUF		0x00100000	/* start/stop output on carrier intr */
#define		LITOUT		0x00200000	/* literal output */
#define		TOSTOP		0x00400000	/* SIGSTOP on background output */
#define		FLUSHO		0x00800000	/* flush output to terminal */
#define		NOHANG		0x01000000	/* no SIGHUP on carrier drop */
#define		L001000		0x02000000
#define		CRTKIL		0x04000000	/* kill line with " \b " */
#define		PASS8		0x08000000	/* pass all 8 bits in any mode */
#define		CTLECH		0x10000000	/* echo control chars as ^X */
#define		PENDIN		0x20000000	/* tp->t_rawq needs reread */
#define		DECCTQ		0x40000000	/* only ^Q starts after ^S */
#define		NOFLSH		0x80000000	/* no output flush on signal */
/* locals, from 127 down */
#define	TIOCLBIS	_IOW(t, 127, int)	/* bis local mode bits */
#define	TIOCLBIC	_IOW(t, 126, int)	/* bic local mode bits */
#define	TIOCLSET	_IOW(t, 125, int)	/* set entire local mode word */
#define	TIOCLGET	_IOR(t, 124, int)	/* get local modes */
#define		LCRTBS		(CRTBS>>16)
#define		LPRTERA		(PRTERA>>16)
#define		LCRTERA		(CRTERA>>16)
#define		LTILDE		(TILDE>>16)
#define		LMDMBUF		(MDMBUF>>16)
#define		LLITOUT		(LITOUT>>16)
#define		LTOSTOP		(TOSTOP>>16)
#define		LFLUSHO		(FLUSHO>>16)
#define		LNOHANG		(NOHANG>>16)
#define		LCRTKIL		(CRTKIL>>16)
#define		LPASS8		(PASS8>>16)
#define		LCTLECH		(CTLECH>>16)
#define		LPENDIN		(PENDIN>>16)
#define		LDECCTQ		(DECCTQ>>16)
#define		LNOFLSH		(NOFLSH>>16)
#define	TIOCSBRK	_IO(t, 123)		/* set break bit */
#define	TIOCCBRK	_IO(t, 122)		/* clear break bit */
#define	TIOCSDTR	_IO(t, 121)		/* set data terminal ready */
#define	TIOCCDTR	_IO(t, 120)		/* clear data terminal ready */
#define	TIOCGPGRP	_IOR(t, 119, int)	/* get pgrp of tty */
#define	TIOCSPGRP	_IOW(t, 118, int)	/* set pgrp of tty */
#define	TIOCSLTC	_IOW(t,117,struct ltchars)/* set local special chars */
#define	TIOCGLTC	_IOR(t,116,struct ltchars)/* get local special chars */
#define	TIOCOUTQ	_IOR(t, 115, int)	/* output queue size */
#define	TIOCSTI		_IOW(t, 114, char)	/* simulate terminal input */
#define	TIOCNOTTY	_IO(t, 113)		/* void tty association */
#define	TIOCPKT		_IOW(t, 112, int)	/* pty: set/clear packet mode */
#define		TIOCPKT_DATA		0x00	/* data packet */
#define		TIOCPKT_FLUSHREAD	0x01	/* flush packet */
#define		TIOCPKT_FLUSHWRITE	0x02	/* flush packet */
#define		TIOCPKT_STOP		0x04	/* stop output */
#define		TIOCPKT_START		0x08	/* start output */
#define		TIOCPKT_NOSTOP		0x10	/* no more ^S, ^Q */
#define		TIOCPKT_DOSTOP		0x20	/* now do ^S ^Q */
#define	TIOCSTOP	_IO(t, 111)		/* stop output, like ^S */
#define	TIOCSTART	_IO(t, 110)		/* start output, like ^Q */
#define	TIOCMSET	_IOW(t, 109, int)	/* set all modem bits */
#define	TIOCMBIS	_IOW(t, 108, int)	/* bis modem bits */
#define	TIOCMBIC	_IOW(t, 107, int)	/* bic modem bits */
#define	TIOCMGET	_IOR(t, 106, int)	/* get all modem bits */
#define	TIOCREMOTE	_IO(t, 105)		/* remote input editing */
#define	TIOCGWINSZ	_IOR(t, 104, struct winsize)	/* get window size */
#define	TIOCSWINSZ	_IOW(t, 103, struct winsize)	/* set window size */
#define	TIOCUCNTL	_IOW(t, 102, int)	/* pty: set/clr usr cntl mode */
#define	TIOCSMON	_IO(t,100)	/* set tty monitor (superuser only) */
#define	TIOCNMON	_IO(t,99)	/* clear tty monitor */
#define	TIOCGETN	_IOWR(t, 98, int)	/* hint at next pty */

#define	OTTYDISC	0		/* old, v7 std tty driver */
#define	NTTYDISC	1		/* new tty discipline */

#define	FIOCLEX		_IO(f, 1)		/* set exclusive use on fd */
#define	FIONCLEX	_IO(f, 2)		/* remove exclusive use */
/* another local */
#define	FIONREAD	_IOR(f, 127, int)	/* get # bytes to read */
#define	FIONBIO		_IOW(f, 126, int)	/* set/clear non-blocking i/o */
#define	FIOASYNC	_IOW(f, 125, int)	/* set/clear async i/o */
#define	FIOSETOWN	_IOW(f, 124, int)	/* set owner */
#define	FIOGETOWN	_IOR(f, 123, int)	/* get owner */

/* socket i/o controls */
#define	SIOCSHIWAT	_IOW(s,  0, int)		/* set high watermark */
#define	SIOCGHIWAT	_IOR(s,  1, int)		/* get high watermark */
#define	SIOCSLOWAT	_IOW(s,  2, int)		/* set low watermark */
#define	SIOCGLOWAT	_IOR(s,  3, int)		/* get low watermark */
#define	SIOCATMARK	_IOR(s,  7, int)		/* at oob mark? */
#define	SIOCSPGRP	_IOW(s,  8, int)		/* set process group */
#define	SIOCGPGRP	_IOR(s,  9, int)		/* get process group */

#define	SIOCADDRT	_IOW(r, 10, struct rtentry)	/* add route */
#define	SIOCDELRT	_IOW(r, 11, struct rtentry)	/* delete route */

#define	SIOCSIFADDR	_IOW(i, 12, struct ifreq)	/* set ifnet address */
#define	SIOCGIFADDR	_IOWR(i,13, struct ifreq)	/* get ifnet address */
#define	SIOCSIFDSTADDR	_IOW(i, 14, struct ifreq)	/* set p-p address */
#define	SIOCGIFDSTADDR	_IOWR(i,15, struct ifreq)	/* get p-p address */
#define	SIOCSIFFLAGS	_IOW(i, 16, struct ifreq)	/* set ifnet flags */
#define	SIOCGIFFLAGS	_IOWR(i,17, struct ifreq)	/* get ifnet flags */
#define	SIOCGIFBRDADDR	_IOWR(i,18, struct ifreq)	/* get broadcast addr */
#define	SIOCSIFBRDADDR	_IOW(i, 19, struct ifreq)	/* set broadcast addr */
#define	SIOCGIFCONF	_IOWR(i,20, struct ifconf)	/* get ifnet list */
#define	SIOCSIFMTU	_IOWR(i,21, struct ifreq)	/* set ifnet mtu */
#define	SIOCGIFMTU	_IOWR(i,22, struct ifreq)	/* get ifnet mtu */

#define	SIOCIFPRON	_IOW(i,23, struct ifreq)	/* PROMISCUOUS ON */
#define	SIOCIFPROFF	_IOW(i,24, struct ifreq)	/* PROMISCUOUS OFF */
#define	SIOCIFPRMON	_IOW(i,25, struct ifreq)	/* MONITOR NET */
#define	SIOCIFPRMOFF	_IOW(i,26, struct ifreq)	/* MONITOR OFF */

#define	SIOCGIFNETMASK	_IOWR(i,27, struct ifreq)	/* get net addr mask */
#define	SIOCSIFNETMASK	_IOW(i, 28, struct ifreq)	/* set net addr mask */
#define	SIOCGIFMETRIC	_IOWR(i,29, struct ifreq)	/* get IF metric */
#define	SIOCSIFMETRIC	_IOW(i, 30, struct ifreq)	/* set IF metric */

#define	SIOCSARP	_IOW(i, 31, struct arpreq)	/* set arp entry */
#define	SIOCGARP	_IOWR(i,32, struct arpreq)	/* get arp entry */
#define	SIOCDARP	_IOW(i, 33, struct arpreq)	/* delete arp entry */
#define	SIOCFARP	_IOW(i, 34, struct arpreq)	/* flush arp entries */

/* SCED Memory driver ioctls */
#define	SMIOSTATS		_IOR(m,0,struct sm_stats) /* smem stats */
#define	SMIOGETREBOOT0		_IOWR(m,1,struct ioctl_reboot) /* get reboot str */
#define SMIOSETREBOOT0		_IOW(m,2,struct ioctl_reboot) /* set reboot str */
#define	SMIOGETREBOOT1		_IOWR(m,3,struct ioctl_reboot) /* get reboot str */
#define SMIOSETREBOOT1		_IOW(m,4,struct ioctl_reboot) /* set reboot str */
#define	SMIOGETLOG		_IOR(m,5,struct sec_mem)  /* get mem log */
#define SMIOSETLOG		_IOW(m,6,struct sec_mem)  /* set mem log */

/* ZDC disk driver ioctls */
#define ZIOCBCMD		_IOWR(z, 0, struct cb)	/* do zdc cmd in cb */

/* general device ioctl */
#define RIOC		'R'
#define RIOFIRSTSECT	_IOR(R, 0, int )	/* get first sector */
#define	RIODRIVER	_IO(R, 1)		/* Real driver ? */
#define RIOPATROL	_IO(R, 2)		/* patrol on */
#define RIONPATROL	_IO(R, 3)		/* patrol off */

/* genral disk ioctl's */
#define VTIOC		'V'
#define V_READ		_IO(V, 0)	/* get VTOC information */
#define V_WRITE		_IO(V, 1) 	/* write VTOC information */
#define V_PART  	_IO(V, 20) 	/* get just partition information */
#endif
