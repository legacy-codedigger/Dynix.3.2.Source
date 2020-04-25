
/*
 * $Copyright:	$
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
 * custA device configuration structure(s).
 * These are structures which are generated at least in part by 
 * the config utility.  Below are sample structures.  The names of
 * the structures are fixed.  But, the names of the structure 
 * members can be customer defined.   Likewise, the fields defined
 * below are those initialized by config.   These fields must
 * be present.  But, the structures can be extended by adding
 * members at the end of the definition.  These extended fields will
 * not be initialized by config.  The are left to the customer
 * developed software to initialize during boot time or later.
 */

/*
 * $Log:	ioconf.h,v $
 */

/*
 * An array of custA_conf's constitutes the configuration of custA
 * drivers and devices.
 */

struct	custA_conf {
	struct	custA_driver *mc_driver;	/* -> per-driver data */
	int		custA_nent;		/* # entries in mbad_dev[] */
	struct	custA_dev *mc_dev;		/* array describing related HW */
};

/*
 * Each driver includes one or more custA_driver struct's in its source
 * to define its characteristics.
 */

struct	custA_driver {
	char		*custA_name;		/* name, eg "xb" (no digit) */
	int		custA_cflags;		/* per-driver flags (below) */
	int		(*custA_probe)();		/* probe procedure */
	int		(*custA_boot)();		/* boot procedure */
	int		(*custA_intr)();		/* intr procedure */
};

/*
 * mbad_conf entry points at array of mbad_dev's; each mbad_dev structure
 * describes a single MBAd board.
 *
 * After probing, driver boot procedure is called with a pointer to an
 * array of these.
 */

struct	custA_dev {
	int		custA_idx;		/* mbad index; -1 == wildcard */
	u_char		custA_bin;		/* interrupt bin # */
	u_long		custA_vala;		/* customer value 0 */
	u_long		custA_valb;		/* customer value 1 */
	u_long		custA_valc;		/* customer value 2 */
	u_long		custA_vald;		/* customer value 3 */
	u_long		custA_vale;		/* customer value 4 */
	u_long		custA_valf;		/* customer value 5 */
	u_long		custA_valg;		/* customer value 6 */
};

/*
 * Configuration data declarations.
 *
 * These are generated in ioconf.c by configuration utilities.
 */

extern	struct	custA_conf	custA_conf[];	/* configuration */
