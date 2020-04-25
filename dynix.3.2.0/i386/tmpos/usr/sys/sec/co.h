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
 * $Header: co.h 2.1 90/11/05 $
 */

/*
 * co.h
 *	Info passed between the SCSI/Ether console driver and the
 *	binary config file.
 *
 * binary config table for the console driver.  There is one entry in
 * the co_bin_config[] table for each controller in the system.
 */

/* $Log:	co.h,v $
 */

struct co_bin_config {
	gate_t	cobc_gate;
};

/*
 * Console state.
 * There is one such structure for each tty device (minor device) on
 * each SCSI/Ether controller.
 *
 * The local and remote console devices are always minor devices 0
 * and 1, regardless of where they appear on the bus.  This is
 * guaranteed at config time.
 * 
 * Mutex notes:
 * 	We could lock the input and output sides separately.  
 *	But, this would probably not be a big win.  So, use one
 * 	lock for all state information.
 */

/*
 * throw together a structure containing the size of the program queu
 * and it's state.
 */

struct sec_pq {
	struct sec_progq	*sq_progq;
	u_short			sq_size;
};

struct co_state {
	struct	tty	ss_tty;
	u_char	ss_alive;
	u_char	ss_initted;
						/* input state */
	int	is_status;
	struct	sec_dev	*is_sd;
	struct	sec_pq	is_reqq;
	struct	sec_pq	is_doneq;
	u_long	is_ovc;			/* count number of overflow errors */
	u_long	is_parc;		/* count number of parity errors */
	char	is_buffer[CBSIZE];
	u_char	is_restart_read;	/* restart read flag		 */
	u_char	is_initted;
						/* output state */
	int	os_status;
	struct	sec_dev *os_sd;
	struct	sec_pq	os_reqq;
	struct	sec_pq	os_doneq;
	struct	sec_smode	os_smode;
	sema_t	os_busy_wait;
	u_char	os_busy_flag;
	u_char	os_initted;
};

#define	CM_BAUD		sm_un.sm_cons.cm_baud
#define CM_FLAGS	sm_un.sm_cons.cm_flags

/*
 * The following macro converts a sd_unit into a minor console device number
 * Since there is a input and output side, c0 has sd_units of 0 and 1 while
 * c1 has sd_units of 2 and 3.
 */
#define UNITMINOR(x)	((x)/2)

#ifdef	KERNEL
extern struct 	co_bin_config co_bin_config[];
extern int	co_bin_config_count;
#ifdef	DEBUG
extern int	co_debug;
#endif	DEBUG
#endif	KERNEL

#ifdef DEBUG
extern char gc_last;
#define	DBGCHAR	('f'&037)
#endif	DEBUG
