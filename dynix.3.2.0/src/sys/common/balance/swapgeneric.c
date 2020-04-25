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

#ifndef	lint
static	char	rcsid[] = "$Header: swapgeneric.c 2.7 90/12/13 $";
#endif

/*
 * swapgeneric.c
 *	GENERIC swap/root configuration.
 */

/* $Log:	swapgeneric.c,v $
 */

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/systm.h"
#include "../h/reboot.h"
#include "../h/vtoc.h"
#include "../h/cmn_err.h"

#include "../balance/cfg.h"
#include "../balance/slicreg.h"

#include "../sec/sec.h"

#define	MINARGLEN	5

extern	dev_t rootdev;
extern	struct	swdevt swdevt[];
extern  struct genericconf {
	char	*gc_name;
	dev_t	gc_root;
} genericconf[];
struct genericconf *setconf_dev();

/*
 * The generic rootdev and swapdev are passed into the kernel as the
 * 1st argument in the boot name. The argument specifies the device and
 * unit number to use for the rootdev and swapdev. The swapdev is typically
 * rootdev+1. The actual partitions to be used for a given device is 
 * specified in ../conf/conf_generic.c. An asterix at the end of the string
 * denotes that the root partition is the same as the swap partition.
 * The argument string is in the form:
 *		XXddd[*]		(eg. sd0*)
 *
 *	where	XX is the device type (e.g. sd for scsi disk),
 *		ddd  is the Dynix device unit number,
 *		*  yes '*' is the flag denoting swapdev==rootdev.
 */

setconf()
{
	register struct genericconf *gc;
	char *p;
	dev_t	swapdev;
	extern	u_char	cons_scsi;	/* slic address of console scsi */
	extern struct sec_cib *cbdcib;	/* address of Console board device */
	extern unsigned boothowto;

	boothowto |= RB_SINGLE;
	swapdev = -1;

	/*
	 * Find Generic rootdev string.
	 *
	 * Skip past bootname, then till argument string.
	 * Error if string not found.
	 */

	for (p = CD_LOC->c_boot_name; *p != '\0'; p++)
		continue;

	/*
	 * look for an opening paren.
         * {dev}({unit},{part}){kern} rootdev[*] [-r {rootdev} -s {swp_dev} ... ]
         */
        gc = (struct genericconf *)NULL;
        /*
         * now look for arguments.
         */
        for (; p < &CD_LOC->c_boot_name[BNAMESIZE]; p++) {
                if (*p == '-') {
                        p++;
                        switch (*p++) {
			case 's':
				/* 
				 * swap device 
				 */
				while ((*p == ' ') || (*p == '\0'))
					p++;
				if (gc = setconf_dev(&p)) {
					swapdev = gc->gc_root;
				}
				break;
			case 'r':
				/* 
				 * root device 
				 */
				while ((*p == ' ') || (*p == '\0'))
					p++;
				if (gc = setconf_dev(&p)) {
					rootdev = gc->gc_root;
				}
				break;
			}
		} else if (*p != '\0') {
			if (gc = setconf_dev(&p)) {
				rootdev = gc->gc_root;
				/*
				 * '*' means rootdev=swapdev
				 */
				if (*p == '*') {
					swapdev = rootdev;
					p++;
				}
			}
		}
	}

        if (gc == (struct genericconf *)NULL) {
		CPRINTF("Generic boot device ");
		if (*p == NULL)
			CPRINTF("not specified.\n");
		else
			CPRINTF("\"%s\" is incorrect.\n", p);
		CPRINTF("Generic devices:");
		for (gc = genericconf; gc->gc_name; gc++)
			printf(" %s", gc->gc_name);
		CPRINTF(".\n");

		printf("Returning to Firmware\n");
 		/*
                 *+ One of the devices specified in the boot string
                 *+ is not available as a bootable device.
                 *+ A list of available devices follows this message.
                 */
		if ((CD_LOC->c_cons->cd_type == SLB_SSMBOARD) ||
		   (CD_LOC->c_cons->cd_type == SLB_SSM2BOARD))
			ssm_return_fw();
		else {
			cbdcib->cib_inst = SINST_RETTODIAG;
			cbdcib->cib_status = SRD_BREAK;		/* Halt */
			mIntr(cons_scsi, 7, SDEV_SCSIBOARD);
			for(;;);			/*SCED takes control */
		}
	}
	if ( swapdev == -1) {
		swapdev = makedev(major(rootdev), minor(rootdev)+1);
	}
	swdevt[0].sw_dev = swapdev;

	CPRINTF("Generic; root on %s%d%c, swap on %s%d%c.\n",
		gc->gc_name, VUNIT(rootdev), 'a'+VPART(rootdev),
		gc->gc_name, VUNIT(swapdev), 'a'+VPART(swapdev));
	/* swap size set during autoconfigure */
}


/*
 * parse an option for a device specifier.
 */
struct genericconf *
setconf_dev(name)
	char	**name;
{
	register struct genericconf *gc;
	register char	*p;
	int	unit;
	int	part;
	int	error;

	p = *name;
	for (gc = genericconf; gc->gc_name; gc++) {
		/*
		 * Match the first 2 characters for a disk device.
		 */
		if (gc->gc_name[0] != p[0] || gc->gc_name[1] != p[1])
			continue;

		p += 2;
		if (*p == '(') {
			/*
			 * xx(nnn,mmm) spec xx = disk type
			 *               nnn = unit
			 *               mmm = partition
			 */
			/*
			 * Determine unit number.
			 */
			error = 1;	/* assume an error */
			unit = 0;
			while (*(++p) != ',') {
				if (*p >= '0' && *p <= '9') {
					unit = (unit*10) + (*p - '0');
					error = 0;
				} else {
					error = 1;
				}
			}
			if (error || unit > 256 ) {
				printf("WARNING: bad/missing unit number\n");
				/*
				 *+ A device specified in the
				 *+ boot string had a bad or
				 *+ missing unit number.
				 */
				break;
			}
			/*
			 * Determine partition number.
			 */
			error = 1;	/* assume an error */
			part = 0;
			while (*(++p) != ')') {
				if (*p >= '0' && *p <= '9') {
					part = (part*10) + (*p - '0');
					error = 0;
				} else {
					error = 1;
				}
			}
			if (error || part > 256 ) {
				printf("WARNING: bad/missing partition number\n");
				/*
				 *+ A device specified in the
				 *+ boot string had a bad or
				 *+ missing partition number.
				 */
				break;
			}
			/*
			 * Now find the minor.  Note that we cannot
			 * correctly determine the minor number
			 * if the unit number found is non-zero.
			 * This is due to the wierd standalone unit
			 * numbers used for SCSI disks (sd and wd) 
			 * where the first disk unit # is 0, the 
			 * second is 8, the third is 16, and so on.
			 * Since there are 256 minor numbers per unit
			 * we must multiply the unit number by 256
			 * and add this to the partition to get the
			 * minor number.  Rather than hard coding
			 * standalone device ID information into
			 * the kernel, we're going to assume here
			 * that the unit number is zero.  The
			 * correct way to boot when this is not
			 * the case is to use the "-r" and "-s"
			 * boot string options.  If the unit number
			 * found is non-zero, then we will insist
			 * that the "-r" option is present (naming
			 * the kernel's notion of the unit number).
			 */
			gc->gc_root = makedev(major(gc->gc_root),part);
			*name = p;
			return gc;
		}

		/*
		 * xxna spec xx = disk device
		 *             n = unit  (up to 3 digits)
		 *             a = partition (0-8 for now)
		 */

		/*
		 * parse up to 3 decimal digits for unit
		 */
		if (*p >= '0' && *p <= '9') {
			unit = *p++ - '0';
			if (*p >= '0' && *p <= '9') {
				unit = (unit*10) + (*p++ - '0');
				if (*p >= '0' && *p <= '9') {
					unit = (unit*10) + (*p++ - '0');
				}
			}
		} else {
			printf("WARNING: bad/missing unit number %s\n",
						*name);
			/*
			 *+ A device specified in the boot string
			 *+ had a bad or missing unit number.
			 */
			break;
		}

		if (*p >= 'a' && *p <= 'h') {
			part = *p-'a';
			p++;
		} else if (*p == '*') {
			part = 1;
		} else {
			part = 0;
		}

		if ( unit > 256 || part > 256 ) {
			printf("WARNING: bad/missing unit number %s\n",
						*name);
			/*
			 *+ A device specified in the boot string
			 *+ had a bad or missing unit number.
			 */
			break;
		}
		gc->gc_root = makedev(major(gc->gc_root),
					(unit<<V_PARTSHIFT) |
					part&V_PART_MASK |
					(part<<V_NEWPARTSHIFT)&
					    V_NEWPARTMASK);
		*name = p;
		return (gc);
	}
	*name = p;
	return (struct genericconf *)NULL;
}
