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

/* $Header: ssm_misc.h 1.1 90/07/06 $ */

/*
 * ssm_misc.h
 * 	Miscellaneous definitions for the SSM 
 *	firmware command interfaces.
 */

/* $Log:	ssm_misc.h,v $
 */

#define	BNAMESIZ	80		/* 80 bytes of boot string allowed */

struct ssm_misc {

	u_char	sm_who;			/* Who the sm_cmd is for */
	u_char	sm_cmd;			/* Command to be executed */
	lock_t	sm_lock;		/* Lock for access */

	union ssm_misc_un {
		u_long	smu_max[40];	/* Max-sized entry in sm_un */

		u_long	smu_fpst;	/* State of frontpanel lights */

		struct vme_imap {	/* Map a VME interrupt vector */
			u_char	vi_vflags;	/* see flag defines */
			u_char	vi_vlev;	/* VME interrupt level */
			u_char	vi_vvec;	/* VME interrupt vector */
			u_char	vi_dest;	/* SLIC dest for this vec */
			u_char	vi_svec;	/* SLIC vector to use */
			u_char	vi_cmd;		/* SLIC command to send */
		}	smu_vme_imap;

		struct vme_genintr {	/* Generate an interrupt on VME */
			u_char	vg_irq;		/* VME IRQ number (1-6) */
			u_char	vg_vvec;	/* VME intr vector */
		}	smu_vme_gen;

		struct wdtst {		/* Watchdog timer state */
			u_long	wdt_addr;	/* Addr of word that changes */
			u_long	wdt_intvl;	/* Interval in milliseconds */
		}	smu_wdtst;

		u_long	smu_tod;	/* Current time-of-day */

		struct todfreq {	/* Time-of-day state */
			u_long	tod_freq;	/* Clock frequency */
			u_char	tod_dest;	/* SLIC dest for TOD intrs */
			u_char	tod_vec;	/* SLIC vector for TOD intrs */
			u_char	tod_cmd;	/* SLIC cmd for tod intrs */
		}	smu_todfreq;

		struct errst {		/* Error-reporting state */
			u_long	err_time;	/* Milliseconds between polls */
			u_char	err_dest;	/* Dest to report errors to */
			u_char	err_vecbase;	/* Base SLIC vector */
			u_char	err_cmd;	/* Cmd (e.g. mIntr) to report */
		}	smu_errst;

		struct powerst { 	/* Power supply state */
			u_char	pow_dest;	/* Dest to report errors to */
			u_char	pow_vec;	/* SLIC vector to report errors */
			u_char	pow_cmd;	/* Cmd (e.g. mIntr) to report */
			u_long	pow_flags;	/* Power supply status flags */
		}	smu_powerst;	

		struct ssm_boot {
			u_char	boot_which;	/* Which boot string */
			u_short	boot_size;	/* Size of boot info */
			u_short	boot_flags;	/* Boot flags */
			char	boot_str[BNAMESIZ];	/* Boot string */
		}	smu_boot;

		struct ssm_log {
			u_short	log_size;	/* Size of log buffer */
			char	*log_buf;	/* Buffer to extract log */
		}	smu_log;

		struct ssm_nvram {
			u_short	nv_size;	/* Size of nvram buffer */
			char	*nv_buf;	/* Buffer to extract nvram */
		}	smu_nvram;

	}	sm_un;

	u_long	sm_sw[6];		/* Reserved for s/w use */
	u_char	sm_pad2[3];
	u_char	sm_stat;		/* Return value */
};

/* Abbreviations to union entries */
#define	sm_fpst		sm_un.smu_fpst
#define	sm_vme_imap	sm_un.smu_vme_imap
#define	sm_vme_gen	sm_un.smu_vme_gen
#define	sm_wdtst	sm_un.smu_wdtst
#define	sm_todval	sm_un.smu_tod
#define	sm_todfreq	sm_un.smu_todfreq
#define	sm_errst	sm_un.smu_errst
#define	sm_powerst	sm_un.smu_powerst
#define	sm_boot		sm_un.smu_boot
#define	sm_log		sm_un.smu_log
#define	sm_nvram	sm_un.smu_nvram

/*
 * mIntr bin for notifying SSM.
 */
#define	SM_BIN	2			/* Bin for SLIC interrupts to SSM */

/*
 * Destinations (for sm_who).
 */
#define	SM_ADDRVEC	0x0		/* mIntrs being sent for address */
#define	SM_FP		0x1		/* Cmd being sent to front panel */
#define	SM_VME		0x2		/* Cmd being sent to VME intr mapper */
#define	SM_WDT		0x3		/* Cmd being sent to watchdog timer */
#define	SM_ERR		0x4		/* Cmd being sent to err handling */
#define	SM_TOD		0x5		/* Cmd being sent to real-time clock */
#define	SM_POWER	0x6		/* Cmd being sent to power supply */
#define	SM_BOOT		0x7		/* Cmd being sent to boot info */
#define	SM_LOG		0x8		/* Cmd being sent to console log */
#define	SM_NVRAM	0x9		/* Cmd being sent to non-volatile RAM */

/*
 * Commands (sm_cmd).
 */
#define	SM_SET		0x01		/* Set associated data */
#define	SM_GET		0x02		/* Get associated data */
#define	SM_DS_BASE	0x10		/* Base of device-specific commands */

/*
 * Common status definitions (sm_stat).
 */
#define	SM_BUSY		0x00		/* No status yet */
#define	SM_OK		0x01		/* Command completed OK */
#define	SM_BAD_ARGS	0xFF		/* Command rejected: bad args */

/*
 * Front panel messages.
 */
/*
 * Front panel state data (sm_fpst).
 * Obviously, not all of these are writable,
 * as they correspond to physical switches
 * on the front panel.
 */
#define	FPST_REM_ENA	0x01		/* REMOTE is enabled (ro) */
#define	FPST_SECURE	0x02		/* Power switch is in SECURE (ro) */
#define	FPST_AUTO	0x02		/* AUTO light is on (rw) */
#define	FPST_LOCAL	0x04		/* Console is LOCAL port (rw) */
#define	FPST_ERR	0x08		/* ERROR light is on (rw) */
#define	FPST_DCOK	0x10		/* DC OK light is on (ro) */

/*
 * VME interrupt mapping (sm_vme_imap).
 */
/* Special VME commands (for sm_cmd) */
#define	SM_CLR_MAP	(SM_DS_BASE)	/* Clear all VME interrupt mapping */
#define	SM_GEN_VME	(SM_DS_BASE+1)	/* Generate a VME interrupt */

/* Defines for vi_vflags */
#define SM_VME_NO_IACK	0x01		/* do not generrate interrupt 
					   acknowlege cycles */

/*
 * Time-of-Day Clock (sm_tod).
 */
/* special TOD commands (for sm_cmd) */
#define	SM_SET_FREQ	(SM_DS_BASE)	/* Get frequency of intrs in Hz */

/*
 * Error reporting state data (sm_errst).
 */
/* err_time */
#define	ERRST_OFF	0x00		/* Bus polling is on */

/* err_cmd */
#define	ERRST_REP_OFF	0x00		/* Error reporting is off */

/* err_vecbase must account for ERRST_NERRS vectors */
#define	ERRST_NERRS	4		/* Max number of possible errors */
#define	ERRST_BUS_LOCK	0		/* SSM believes the bus is locked up */
#define	ERRST_DEVFLT	1		/* SSM believes fatal h/w err exists,
					 * e.g. cache parity error on proc */
#define	ERRST_SSMFLT	2		/* SSM itself has fatal h/w err */
#define	ERRST_VME_BERR	3		/* SSM detected VMEbus Bus Error */

/*
 * Power Supply state (sm_powerst).
 */
/* state flags (sm_powerst.pow_flags) */
#define	PSST_P5_FAULT	0x0001		/* Fault on +5 from power supply */
#define	PSST_M5_FAULT	0x0002		/* Fault on -5 from power supply */
#define	PSST_12V_FAULT	0x0004		/* Fault on +12/-12 from power supply */
#define	PSST_LOAD_FAULT	0x0010		/* Load alarm from power supply */
#define	PSST_AC_FAIL	0x0020		/* AC power failure */
#define	PSST_UPS_ON	0x0100		/* UPS is on */

/*
 * Boot info (sm_bootinfo).
 */
/* Special boot info commands */
#define	SM_GET_DFT	(SM_DS_BASE)	/* Get default boot info */
#define	SM_SET_DFT	(SM_DS_BASE+1)	/* Set default boot info */
#define	SM_REBOOT	(SM_DS_BASE+2)	/* Reboot with this info */

/* boot_which values */
#define	SM_DYNIX	0x00		/* Dynix boot string */
#define	SM_DUMPER	0x01		/* Default dumper boot string */

/*
 * Console log info (sm_log).
 */
/* special log info commands */
#define	SM_GET_LOGSIZ	(SM_DS_BASE)	/* Get log size */

/*
 * Non-volatile RAM info (sm_nvram).
 */
/* special log info commands */
#define	SM_GET_NVRAMSIZ	(SM_DS_BASE)	/* Get NVRAM size */
