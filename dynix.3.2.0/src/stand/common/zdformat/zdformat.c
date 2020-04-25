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

#ifdef RCS
static char rcsid[]= "@(#)$Header: zdformat.c 1.16 1991/08/06 21:40:26 $";
#endif

/*
 * zdformat
 *
 * Stand-alone program to implement the following:
 *	- format/reformat disk drives on ZDC.
 *	- verify formatted disk drives on ZDC.
 *	- add bad blocks to an already formatted disk drive on ZDC.
 *	- display manufacturer's defect list or display bad block list.
 *	- clone the current format (ie reformat identical to current format).
 *
 * zdformat queries the user for all options.
 *
 * The basic algorithm is to format the disk a cylinder at a time while
 * marching through the disk with a 2 cylinder window. Off-cylinder
 * bad block replacements are satisfied via adjacent cylinders.
 * If Off-cylinder replacements cannot be satisfied via adjacent cylinders,
 * another "pass" finds replacements for the bad blocks in the closest
 * possible cylinder to the one with the bad block.
 */

/* $Log: zdformat.c,v $
 *
 */

#define	FMTVERSION	3		/* Version # of formatter */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <sys/vtoc.h>
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#include "../zdc.h"
#include "../saio.h"
#include "zdformat.h"

struct cyl_hdr cyls[2];


struct snf_list snf_list[SNF_LIST_SIZE];
int	snftogo;

/*
 * format/verify parameters
 */
int	startcyl;		/* starting cylinder */
int	lastcyl;		/* last cylinder */
int	verbose;		/* verbose output? */
bool_t	gotcfg = FALSE;		/* Set channel cfg */
bool_t	doverify = FALSE;	/* Verify? */
bool_t	checkdata = FALSE;	/* check data? */
int	fullpasses;		/* no. of full passes (all cylinders) */
int	defectpasses;		/* no. of defect passes (tracks with spots) */
int     hdrpasses;              /* no. of sector hdr verification passes
                                 * (suspected tracks only) */

/*
 * Misc global variables...
 */
int	totspt;			/* total sectors per track (includes spare) */
struct	zdcdd	*chancfg;	/* Channel configuration - drive descriptor */
struct	zdbad	*bbl;		/* bad block list */
struct	zdbad	*newbbl;	/* new bad block list - built during format */
struct	bz_bad	*tobzp;		/* ptr to new list entry */
char	devstr[132];		/* Save disk device name for reopen */
bool_t	writevtoc = FALSE;	/* Write a single-partition VTOC */
struct	partition part;		/* minimal partition table entry */
int	partno = 1;		/* no. of partition to make */
int	fd;			/* file descripter for disk */
#ifdef	DEBUG
int	debug;			/* enable ZD_BSFDEBUG? */
#endif /* DEBUG */
union   headers *headers = NULL;/* header list for READ_HDRS ioctl */
int     n_hdrs;                 /* Number of headers per track w/ runts */
int	nspc;			/* number of sectors per cylinder */
struct hdr_suspect *suspect;    /* Array of tracks containing potential
                                 * header defects resulting in hdr-ecc
                                 * errors.  Verified during hdr-pass */
int max_suspects;               /* Static size of header suspect list */
int num_suspects;               /* Number of elements in suspect list */
caddr_t vwbp;                   /* Verify write buffer */
caddr_t vrbp;                   /* Verify read buffer */
int	errcode;		/* error code for last I/O operation */
int task;                       /* Program task */

extern	struct zdinfo zdinfo[];	/* zdinfo table - indexed by drive type */
extern	nzdtypes;		/* no of configured zdc drive types */
extern	struct iob iob[];	/* iob descripter */
extern	struct partition zd_proto[]; /* default minimal partition sizes */

bool_t	get_chancfg();

main()
{
	register char *cp;
	int display;			/* Display option */
	int list;			/* reformat from mfglist or badlist */
	int num;			/* no. sectors to addbad */
	u_char drvcfg;			/* drive configuration state */
	struct probe_cb pcb;		/* cb for probe */

	display = 0;

	printf("Disk format/verify/addbad program for ZDC\n");

	do {
		printf("Task Menu:\n");
		printf("\tFormat     (1)\n\tReformat   (2)\n\tVerify     (3)\n\tAddbad     (4)\n\tDisplay    (5)\n\tClone      (6)\n\tWrite VTOC (7)\n\tExit       (8)\n\n");
		task = atoi(prompt("task? "));
	} while (task < FORMAT || task > EXIT);

	if (task == EXIT)
		exit(0);

	verbose = FALSE;
	do {
		cp = prompt("Verbose output [y/n]? ");
		if (*cp == 'y') {
			verbose = TRUE;
			break;
		}
	} while (*cp != 'n');

	/*
	 * Get device and verify its existence.
	 */
	fd = getdevice();

#ifdef	DEBUG
	if (*prompt("Enable bad sector forwarding debugging [y/n]? ") == 'y') {
		debug = ZD_BSFDEBUG;
	}
	if (*prompt("Enable dump cb debugging [y/n]? ") == 'y') {
		debug |= ZD_DUMPCBDEBUG;
	}
	if (debug)
		ioctl(fd, SAIODEBUG, (char *)debug);
#endif /* DEBUG */

	/*
	 * Provide access to entire disk drive.
	 */
	ioctl(fd, SAIOZSETBASE, (char *)NULL);

	/*
	 * Is drive already formatted?
	 * If not and not formatting, then complain and exit.
	 */
	bzero((caddr_t)&pcb, sizeof(struct probe_cb));
	pcb.pcb_cmd = ZDC_PROBEDRIVE;
	if (ioctl(fd, SAIOZDCCMD, (char *)&pcb) < 0) {
		printf("Cannot probe device\n");
		exit(1);
	}

	drvcfg = pcb.pcb_drivecfg[ZDC_DRIVE(iob[fd - 3].i_unit)];
	if (task != FORMAT && ((drvcfg & (ZD_FORMATTED | ZD_MATCH))
				!= (ZD_FORMATTED | ZD_MATCH))) {
		printf("Drive not formatted or channel mismatch.\n");
		exit(1);
	}

	/*
	 * Get channel configuration
	 */
	gotcfg = get_chancfg();

	/*
	 * Do requested task.
	 */
	switch(task) {

	case FORMAT:
		printf("Format disk.\n");
		get_options(FORMAT);
		alloc_structs();
		if (get_mfglist() == FAIL)
			break;
		create_badlist();
		format(FORMAT);
		if (writevtoc)
			write_min_vtoc();
		printf("\n\nDone.\n");
		break;

	case REFORMAT:
		printf("Reformat disk.\n");
		for(;;) {
			list = atoi(prompt("Reformat from mfg defect list (1) or bad block list (2)? "));
			if (list == MFG_DEFECT || list == BAD_BLOCK)
				break;
		}
		if (list == MFG_DEFECT) {
			get_options(FORMAT);
			alloc_structs();
			if (read_mfglist() == FAIL) {
				printf("Could not read mfg defect list.\n");
				break;
			}
			create_badlist();
			format(FORMAT);	/* Just like FORMAT case */
		} else {
			get_options(REFORMAT);
			alloc_structs();
			if (read_mfglist() == FAIL) {
				printf("Could not read mfg defect list.\n");
				break;
			}
			if (read_badlist() == FAIL) {
				printf("Could not read bad block list.\n");
				break;
			}
			format(REFORMAT);
		}
		if (writevtoc)
			write_min_vtoc();
		printf("\n\nDone.\n");
		break;

	case CLONEFMT:
		/*
		 * Intended for internal debug use...
		 */
		printf("Cloning format from bad block list data\n");
		get_options(CLONEFMT);
		alloc_structs();
		if (read_mfglist() == FAIL) {
			printf("Could not read mfg defect list.\n");
			break;
		}
		if (read_badlist() == FAIL) {
			printf("Could not read bad block list.\n");
			break;
		}
		clonefmt();
		if (writevtoc)
			write_min_vtoc();
		printf("\n\nDone.\n");
		break;

	case VERIFY:
		printf("Verify disk (no format).\n");
		doverify = TRUE;
		get_options(VERIFY);
		alloc_structs();
		if (read_mfglist() == FAIL) {
			printf("Could not read mfg defect list.\n");
			break;
		}
		if (read_badlist() == FAIL) {
			printf("Could not read bad block list.\n");
			break;
		}
		do {
			cp = prompt("Last chance! Proceed [y/n]? ");
		} while (*cp != 'y' && *cp != 'n');
		if (*cp == 'n') {
			printf("No verify.\n");
			break;
		}
		verify();
		if (startcyl <= 1 && lastcyl >= 1) {
			printf("\nVTOC was probably clobbered.\n");
			printf("A new VTOC can be created using zdformat\n");
			printf("or the mkvtoc command\n");
		}
		printf("\n\nDone.\n");
		break;

	case ADDBAD:
		printf("Add bad block(s) to bad block list.\n");
		alloc_structs();
		startcyl = 0;
		lastcyl = chancfg->zdd_cyls - 1;
		if (read_mfglist() == FAIL) {
			printf("Could not read mfg defect list.\n");
			break;
		}
		if (read_badlist() == FAIL) {
			printf("Cannot addbad\n");
			break;
		}
		if ((num = get_addlist()) == 0) {
			printf("Nothing to addbad!\n");
			break;
		}
		do {
			cp = prompt("Last chance! Proceed [y/n]? ");
		} while (*cp != 'y' && *cp != 'n');
		if (*cp == 'n') {
			printf("Addbad aborted.\n");
			break;
		}
		addbad(num, ADDBAD);
		printf("\n\nDone.\n");
		break;

	case DISPLAY:
		/*
		 * Display bad block list, mfg defect list, or both.
		 */
		printf("Print mfg defect list and/or bad block list\n");
		alloc_structs();
		do {
			cp = prompt("Mfg defect list [y/n]? ");
			if (*cp == 'y') {
				display |= MFG_DEFECT;
				break;
			}
		} while (*cp != 'n');

		do {
			cp = prompt("bad block list [y/n]? ");
			if (*cp == 'y') {
				display |= BAD_BLOCK;
				break;
			}
		} while (*cp != 'n');

		if (display & MFG_DEFECT) {
			if (read_mfglist() == FAIL)
				printf("Cannot read mfg defect list.\n");
			else
				print_mfglist();
		}
		if (display & BAD_BLOCK) {
			if (read_badlist() == FAIL)
				printf("Cannot read bad block list.\n");
			else
				print_badlist();
		}
		break;

	case WRITEVTOC:
		/*
		 * Write a new Volume Table of Contents
		 */
		printf("Write a new minimal VTOC\n");
		get_options(WRITEVTOC);
		if (!writevtoc) {
			printf("Not writing VTOC\n");
		} else {
			do {
				cp = prompt("Last chance! Proceed [y/n]? ");
			} while (*cp != 'y' && *cp != 'n');
			if (*cp == 'n') {
				printf("Write VTOC aborted.\n");
				break;
			}
			write_min_vtoc();
		}
		break;
	} /* end of switch */

	close(fd);
}

/*
 * Prompt and verify a device name from the user.  The partition number
 * (second parameter of device name) is replaced with V_NUMPAR, which
 * means to the disk driver "ignore the VTOC and just give me the whole
 * disk", which is what the formatter wants to deal with anyway.  Care
 * is taken to make this code independent of the value of V_NUMPAR.
 */

getdevice()
{
	register char *cp;
	register char *dp;
	register char *pp;
	register int lfd;
	char	partbuf[20];
	int n = V_NUMPAR;

	/*
	 * Load up partbuf with the character translation of V_NUMPAR.
	 */
	bzero(partbuf, 20);
	pp = partbuf + sizeof(partbuf) - 1;
	*pp-- = '\0';
	do {
		*pp-- = "0123456789"[n%10];
		n /= 10;
	} while (n);
	pp++;

	for (;;) {
		cp = prompt("ZDC device? ");
		if (cp[0] != 'z' || cp[1] != 'd') {
			printf("Example for ZDC 1, drive 0: zd(16,0)\n");
			continue;
		}

		/*
		 * Save for reopen.
		 */
		dp = devstr;
		while (*dp++ = *cp++)
			continue;
		/*
		 * Replace the partition number with V_NUMPAR
		 */
		dp = index(devstr, ',');
		dp++;
		strcpy(dp, pp);
		dp = devstr + strlen(devstr);
		strcpy(dp, ")");

		if ((lfd = open(devstr, 2)) < 0) {
			printf("Open failed. Check input, cables and drive switch settings.\n");
			printf("Example for ZDC 1, drive 0: zd(16,0)\n");
			continue;
		}
		printf("Drive zd%d on zdc %d: ", ZDC_DRIVE(iob[lfd - 3].i_unit),
				ZDC_CTRLR(iob[lfd - 3].i_unit));
		do {
			cp = prompt("Ok [y/n]? ");
			if (*cp == 'y')
				return (lfd);
		} while (*cp != 'n');
		close(lfd);
	}
}

/*
 * get_options
 *	get various options/parameters from user.
 *
 * Options resolved: start cyl, last cyl, do verify, write VTOC.
 * If doverify: checkdata, no of full passes, no of defect passes.
 * If writevtoc: partition size and start
 */
get_options(task)
	int	task;		/* FORMAT, REFORMAT, CLONEFMT, VERIFY or WRITEVTOC */
{
	register char	*cp;
	register int	type;		/* drive type */
	bool_t	use_default;		/* Use default */
	bool_t	fulldrive;		/* format entire drive */

	fulldrive = FALSE;
	use_default = FALSE;

	/*
	 * First report drive characteristics.
	 */
	if (gotcfg == TRUE) {
		print_chancfg(chancfg);

		/*
		 * If formatting, allow user to override current configuration.
		 */
		if (task == FORMAT) {
			do {
				cp = prompt("Configuration ok [y/n]? ");
				if (*cp == 'n') {
					gotcfg = FALSE;
					break;
				}
			} while (*cp != 'y');
		}
	}

	if (task == WRITEVTOC)
		goto checkforvtoc;

	if (gotcfg == FALSE) {
		/*
		 * Ask user for configuration.
		 */
		printf("Provide drive type.\n");
		do {
			printf("Available types are:\n	");
			for(type = 0; type < nzdtypes; type++) {
				/*
				 * Avoid printing Reserved type placeholders
				 */
				if (zdinfo[type].zi_zdcdd.zdd_magic != 0)
					printf("%s (%d), ",
						zdinfo[type].zi_name, type);
			}
			printf("\n");
			type = atoi(prompt("New drive type? "));
		} while (type < 0 || type >= nzdtypes ||
				zdinfo[type].zi_zdcdd.zdd_magic == 0);

		/*
		 * Set new configuration.
		 */
		set_chancfg(&zdinfo[type].zi_zdcdd);
		print_chancfg(chancfg);
		gotcfg = TRUE;
	}

	/*
	 * Get options.
	 */
	printf("Default options are:\n	");
	if (task == FORMAT || task == REFORMAT || task == CLONEFMT)
		printf("Format and ");
	printf("Verify entire disk without data checking.\n	");
        printf("%d Header passes, %d Full passes, %d Defect passes,\n",
                NHDRPASS, NFULLPASS, NDEFECTPASS);
	printf("	and write a single-partition VTOC\n");

	do {
		cp = prompt("Use default options [y/n]? ");
		if (*cp == 'y') {
			use_default = TRUE;
			break;
		}
	} while (*cp != 'n');

	if (use_default) {
		startcyl = 0;
		lastcyl = chancfg->zdd_cyls - 1;
		doverify = TRUE;
		checkdata = FALSE;
		hdrpasses = NHDRPASS;
		fullpasses = NFULLPASS;
		defectpasses = NDEFECTPASS;
		writevtoc = TRUE;
		partno = 1;
		part = zd_proto[chancfg->zdd_drive_type];
	} else {
		/*
		 * prompt for options
		 */
		do {
			cp = prompt("Use entire drive [y/n]? ");
			if (*cp == 'y') {
				fulldrive = TRUE;
				break;
			}
		} while (*cp != 'n');

		if (fulldrive) {
			startcyl = 0;
			lastcyl = chancfg->zdd_cyls - 1;
			printf("start cyl = 0, last cyl = %d\n", lastcyl);
		} else {
			/*
			 * Get starting cylinder and last cylinder
			 * to format.
			 */
			if (task == CLONEFMT || task == VERIFY) {
				do {
					startcyl = atoi(prompt("Start cyl? "));
				} while (startcyl < 0 || startcyl >= chancfg->zdd_cyls);
			} else
				startcyl = 0;		/* FORMAT, REFORMAT */
			do {
				lastcyl = atoi(prompt("Last cyl? "));
			} while (lastcyl < 0 || lastcyl < startcyl ||
						lastcyl >= chancfg->zdd_cyls);
			if ((task == FORMAT || task == REFORMAT) &&
						lastcyl < chancfg->zdd_cyls - 1)
				printf("WARNING: short format\n");
		}

		/*
		 * Verify?
		 */
		if (doverify == FALSE) {
			do {
				cp = prompt("Verify [y/n]? ");
				if (*cp == 'n') {
                                        if (task == FORMAT
                                        ||  task == REFORMAT
                                        || task == CLONEFMT) {
                                                /* Must do one header pass */
                                                checkdata = FALSE;
                                                hdrpasses = 1;
                                                fullpasses = 0;
                                                defectpasses = 0;
                                        }
					goto checkforvtoc;
				}
			} while (*cp != 'y');
			doverify = TRUE;
		}

		/*
		 * Check data?
		 */
		do {
			cp = prompt("Checkdata [y/n]? ");
			if (*cp == 'y') {
				checkdata = TRUE;
				break;
			}
		} while (*cp != 'n');

		/*
		 * get number of verify passes?
		 */
                hdrpasses = atoi(prompt("Number of header passes? "));
                if (hdrpasses < 1)
                        hdrpasses = 1;

		fullpasses = atoi(prompt("Number of full passes? "));
		if (fullpasses < 0)
			fullpasses = 0;
		do
			defectpasses = atoi(prompt("Number of defect passes? "));
		while (defectpasses < 0);
                printf("%d Header passes, %d Full passes, %d Defect passes.\n",                        hdrpasses, fullpasses, defectpasses);
		
		/*
		 * Write VTOC?
		 */
checkforvtoc:
		if (task == VERIFY)
			return;

		if (task == WRITEVTOC)
			writevtoc = TRUE;

		if (writevtoc == FALSE) {
			do {
				cp = prompt("Write minimal VTOC [y/n]? ");
				if (*cp == 'n')
					return;
			} while (*cp != 'y');
			writevtoc = TRUE;
		}
		if (gotcfg == FALSE) {
			printf("Cannot write VTOC to unformatted disk.\n");
			writevtoc = FALSE;
			return;
		}

		partno = 1;
		part = zd_proto[chancfg->zdd_drive_type];

		printf("Standard minimal VTOC for this disk:\n\n	");
		do {
			printf("Partition: %d\n	", partno);
			printf("Offset in sectors: %d\n	", part.p_start);
			printf("Size in sectors: %d\n", part.p_size);
			cp = prompt("Use this layout [y/n]? ");
			if (*cp != 'y') {
				partno = atoi(prompt("Partition number (0-254)? "));
				part.p_start = atoi(prompt("Offset (sectors)? "));
				part.p_size = atoi(prompt("Size (sectors)? "));
			}
		} while (*cp != 'y');
	}
}

/*
 * alloc_structs
 *	allocate memory for data structures sized by channel configuration.
 */
alloc_structs()
{
	register int i;
	int	tracksize;

	alloc_cyls();
	/*
	 * allocate memory for mfg defect list and bad block lists
	 */
	callocrnd(DEV_BSIZE);
	i = ZDMAXBAD(chancfg);
	mfg = (struct bad_mfg *)calloc(i);
	bbl = (struct zdbad *)calloc(i);
	newbbl = (struct zdbad *)calloc(i);

        max_suspects = (i - (sizeof(struct zdbad) - sizeof(struct bz_bad))) /
                sizeof(struct bz_bad);
        suspect = (struct hdr_suspect *)
                calloc(max_suspects * sizeof(struct hdr_suspect));
        num_suspects = 0;

        callocrnd(DEV_BSIZE);
        n_hdrs = totspt + ((chancfg->zdd_runt) ? 1 : 0);
        i = ROUNDUP(n_hdrs * sizeof(union headers), CNTMULT);
        headers = (union headers *)calloc(i);

	tracksize = chancfg->zdd_sectors << DEV_BSHIFT;
	callocrnd(DEV_BSIZE);
	vwbp = calloc(tracksize);
	callocrnd(DEV_BSIZE);
	vrbp = calloc(tracksize);
	nspc = chancfg->zdd_tracks * chancfg->zdd_sectors;
}

/*
 * alloc_cyls
 *	Allocate memory for cylinder maps.
 */
alloc_cyls()
{
	register int	size;
	register int	i;

	/*
	 * allocate cylinder tables.
	 */
	cyls[0].c_trk = (struct track_hdr *)calloc(chancfg->zdd_tracks
						* sizeof (struct track_hdr));
	cyls[1].c_trk = (struct track_hdr *)calloc(chancfg->zdd_tracks
						* sizeof (struct track_hdr));
	callocrnd(DEV_BSIZE);
	size = totspt;
	if (chancfg->zdd_runt)
		size++;
	size = ROUNDUP(sizeof(struct hdr) * size, CNTMULT);
	for (i = 0; i < chancfg->zdd_tracks; i++)
		cyls[0].c_trk[i].t_hdr = (struct hdr *)calloc(size);
	for (i = 0; i < chancfg->zdd_tracks; i++)
		cyls[1].c_trk[i].t_hdr = (struct hdr *)calloc(size);
}

/*
 * format
 *	Format a disk for the ZDC.
 *
 * Note: bad block list has been created.
 */
format(flag)
	int flag;				/* FORMAT or REFORMAT */
{
	register int i;				/* Cylinder index */
	register struct bz_bad	*fbzp;		/* ptr to bad list entry */
	register int prev;			/* previous cylinder */
	register int last;			/* last cylinder */
	char *cp;
	struct zdbad *tbbl;

	do {
		cp = prompt("Last chance! Proceed with format [y/n]? ");
		if (*cp == 'n') {
			printf("No Format.\n");
			exit(0);
		}
	} while (*cp != 'y');

	/*
	 * set pointers into bad block lists
	 */
	tobzp = newbbl->bz_bad;
	fbzp = bbl->bz_bad;

	/*
	 * Always start format with cylinder 0.
	 */
	setup_ddcyl(&cyls[ZDD_DDCYL & 1], fbzp);
	map_ddcyl(&cyls[ZDD_DDCYL & 1]);
	format_dd(&cyls[ZDD_DDCYL & 1]);
	while (fbzp < &bbl->bz_bad[bbl->bz_nelem] && fbzp->bz_cyl == ZDD_DDCYL)
		++fbzp;		/* skip past this cylinder */

	last = lastcyl;
	if (lastcyl >= (chancfg->zdd_cyls - ZDD_NDGNCYL))
		last = chancfg->zdd_cyls - ZDD_NDGNCYL - 1;
	/*
	 * setup 1st cylinder.
	 */
	i = 1;
	if (i <= last) {
		setup_cyl(i, &cyls[i & 1], fbzp, flag);
		map_cyl(i, &cyls[i & 1]);
		while (fbzp < &bbl->bz_bad[bbl->bz_nelem] && fbzp->bz_cyl == i)
			++fbzp;		/* skip past this cylinder */
		prev = i;
		++i;
	}
	/*
	 * Now do rest.
	 */
	for (; i <= last; i++, prev++) {
		setup_cyl(i, &cyls[i & 1], fbzp, flag);
		map_cyl(i, &cyls[i & 1]);
		while (fbzp < &bbl->bz_bad[bbl->bz_nelem] && fbzp->bz_cyl == i)
			++fbzp;		/* skip past this cylinder */
		/*
		 * Check if this cylinder needs help from the previous
		 * cylinder.
		 */
		if (cyls[i&1].c_bad) {
			/* See if previous can help */
			if (cyls[prev&1].c_free) {
				do_snf(&cyls[prev&1], &cyls[i&1]);
			}
		}
		/*
		 * Check to see if this cylinder can help the
		 * previous cylinder (if it indeed needs help).
		 */
		if (cyls[prev&1].c_bad) {
			if (cyls[i & 1].c_free)
				do_snf(&cyls[i&1], &cyls[prev&1]);
			if (cyls[prev & 1].c_bad)
				save_snf_pass(&cyls[prev & 1]);
		}
		/*
		 * Format (on disk) the previous cylinder.
                 * Run a quick check of the hdrs and note
                 * any tracks needing corrections for the
                 * hdr validation/correction pass.
                 */
                format_cyl(prev, &cyls[prev & 1]);
                check_hdrs(prev, &cyls[prev & 1], FALSE);
        }

	/*
	 * Write the last cylinder out to disk
	 */
	if (last != ZDD_DDCYL) {
		if (cyls[last&1].c_bad)
			save_snf_pass(&cyls[prev & 1]);
		format_cyl(last, &cyls[last & 1]);
		check_hdrs(last, &cyls[last & 1], FALSE);
	}

	/*
	 * Any dgn cylinders to format?
	 */
	if (last < lastcyl) {
		/*
		 * Yes.
		 */
		for (i = last + 1; i <= lastcyl; i++) {
			setup_dgncyl(i, &cyls[i & 1], fbzp);
			map_dgncyl(i, &cyls[i & 1]);
			while (fbzp < &bbl->bz_bad[bbl->bz_nelem] &&
							fbzp->bz_cyl == i)
				++fbzp;		/* skip past this cylinder */
			format_cyl(i, &cyls[i & 1]);
		}
	}

	/*
	 * Copy any remaining bad blocks after lastcyl.
	 */
	while (fbzp < &bbl->bz_bad[bbl->bz_nelem]) {
		if (fbzp->bz_rtype == BZ_SNF)
			newbbl->bz_nsnf++;
		newbbl->bz_nelem++;
		*tobzp++ = *fbzp++;
	}
	/*
	 * Sort list.
	 */
	qsort(newbbl->bz_bad, newbbl->bz_nelem, sizeof(struct bz_bad), bblcomp);
	tbbl = bbl;
	bbl = newbbl;
	newbbl = tbbl;
        bbl->bz_csn = getsum((long *)bbl->bz_bad,
                (int)((bbl->bz_nelem * sizeof(struct bz_bad)) / sizeof(long)),
                (long)(bbl->bz_nelem ^ bbl->bz_nsnf));
	ioctl(fd, SAIOSETBBL, (char *)bbl);

	/*
	 * If snftogo > 0, then do_snf_pass() to find homes for
	 * bad blocks which could not be resolved by the first pass
	 * through the disk.
	 */
	if (snftogo > 0)
		do_snf_pass(2);

	/*
	 * Now close, reopen device, and do pertinent ioctls.
	 * Driver now sees a formatted disk.
	 */
	close(fd);
	fd = open(devstr, 2);
#ifdef	DEBUG
	ioctl(fd, SAIODEBUG, (char *)debug);
#endif /* DEBUG */
	ioctl(fd, SAIOZSETBASE, (char *)NULL);
	write_badlist();
	write_mfglist();

        /*
         * Note that this at least one header
         * verify pass is manditory.  Other
         * passes will be skipped accordingly.
         */
	verify();		/* verify may write data on disk */
}

/*
 * do_snf_pass
 *	take a second pass across the disk to find replacements for the
 *	elements in the snf_list not resolved in adjacent cylinders.
 */
do_snf_pass(cyloff)
	int cyloff;
{
	register int i;
	register int offset;
	register struct bz_bad *bzp;
	int	where;
	int	cyl;
	struct	bz_bad *find_snf_rplcmnt();
	int limit;

	for (i = 0; i < snftogo; i++) {
		offset = cyloff;
		cyl = snf_list[i].snf_addr.da_cyl;
		limit = cyl - 1;
		if (lastcyl - cyl > limit)
			limit = lastcyl - cyl;
		for (;;) {
			bzp = find_snf_rplcmnt(&snf_list[i], cyl + offset,
						&cyls[(cyl + offset) & 1]);
			if (bzp != (struct bz_bad *)NULL)
				break;
			bzp = find_snf_rplcmnt(&snf_list[i], cyl - offset,
						&cyls[(cyl - offset) & 1]);
			if (bzp != (struct bz_bad *)NULL)
				break;
			offset++;
			if (offset > limit) {
				/*
				 * Should never happen as typically there
				 * are many more spares than even the bad
				 * block list can hold.
				 */
				printf("do_snf_pass: Cannot find replacement sector\n");
				exit(14);
			}
		}
                /*
                 * Calculate a new bad block list checksum,
                 * then update the drivers bad block list with
                 * the new SNF resolution before attempting
                 * to write the recovered data.
                 */
                bbl->bz_csn = getsum((long *)bbl->bz_bad,
                        (int)((bbl->bz_nelem * sizeof(struct bz_bad)) / sizeof(long)),
                        (long)(bbl->bz_nelem ^ bbl->bz_nsnf));
		ioctl(fd, SAIOSETBBL, (char *)bbl);

		/*
		 * write data in sector - if any saved.
		 */
		if (snf_list[i].snf_data != (caddr_t)NULL) {
			where = (cyl * chancfg->zdd_sectors * chancfg->zdd_tracks)
				+ (snf_list[i].snf_addr.da_head * chancfg->zdd_sectors)
				+ snf_list[i].snf_addr.da_sect;
			where <<= DEV_BSHIFT;
			lseek(fd, where, L_SET);
			if (write(fd, snf_list[i].snf_data, DEV_BSIZE) != DEV_BSIZE) {
				printf("Warning: cannot write replacement sector (%d, %d, %d).\n",
					bzp->bz_rpladdr.da_cyl,
					bzp->bz_rpladdr.da_head,
					bzp->bz_rpladdr.da_sect);
			}
		}
	}
	snftogo = 0;		/* No more */
}

/*
 * clonefmt
 *	Reformat cylinders from bad block list data. No new bad block list
 *	will be built. Clonefmt merely duplicates the existing setup.
 *	Note: no special case for DGN cylinders needed. The bad block list
 *	for these cylinders will contain only BZ_PHYS entries (remark_spots
 *	is sufficient to recreate format).
 */
clonefmt()
{
	register int i;
	register char *cp;

	do {
		cp = prompt("Last chance! Proceed with reformat [y/n]? ");
		if (*cp == 'n') {
			printf("No Format.\n");
			exit(0);
		}
	} while (*cp != 'y');

	i = startcyl;
	if (i == ZDD_DDCYL) {
		/*
		 * Fake new bad block list - but ignore anything put there.
		 * map_ddcyl understands about special sectors.
		 */
		tobzp = newbbl->bz_bad;
		setup_ddcyl(&cyls[ZDD_DDCYL & 1], bbl->bz_bad);
		map_ddcyl(&cyls[ZDD_DDCYL & 1]);
		format_dd(&cyls[ZDD_DDCYL & 1]);
		++i;
	}

	/*
	 * Now do rest.
	 */
	for (; i <= lastcyl; i++) {
		remap_cyl(i, &cyls[i & 1]);
		format_cyl(i, &cyls[i & 1]);
		check_hdrs(i, &cyls[i & 1], FALSE);
	}

	write_badlist();
	write_mfglist();
	verify();		/* verify will write data on disk */
}

/*
 * getskew
 *	Calculate skew for sector 0 on given cylinder/head.
 */
int
getskew(cyl, track)
	register int	cyl;
	int	track;
{

	/*
	 * Special case for cyl ZDD_DDCYL, and dgn cylinders.
	 */
	if (cyl == ZDD_DDCYL)
		return((track * ZDD_NDDSECTORS) % totspt);

	if (cyl >= (chancfg->zdd_cyls - ZDD_NDGNCYL))
		return(0);

	if (chancfg->zdd_tskew == 1)
		return((((chancfg->zdd_tracks - 1 + chancfg->zdd_cskew) * cyl)
				+ track) % totspt);
	/*
	 * track skew != 1
	 */
	return(( (( (chancfg->zdd_tskew * (chancfg->zdd_tracks - 1))
			+ chancfg->zdd_cskew) * cyl)
			+ (chancfg->zdd_tskew * track)) % totspt);
}

/*
 * setskew
 *	Calculate skew for this cylinder. And initialize headers.
 *	Assumes all sectors are good. Headers for spare sectors are
 *	also initialized.
 */
setskew(cyl, chp)
	int	cyl;		/* cylinder number */
	struct	cyl_hdr *chp;
{
	register struct hdr *hp;
	register int ls;		/* logical sector num */
	register int ps;		/* physical sector num */
	register int trk;

	for(trk = 0; trk < chancfg->zdd_tracks; trk++) {
		ps = getskew(cyl, trk);
		for (ls = 0; ls < chancfg->zdd_sectors; ls++) {
			hp = &chp->c_trk[trk].t_hdr[ps];
			hp->h_type = ZD_GOODSECT;
			hp->h_cyl = cyl;
			hp->h_head = trk;
			hp->h_sect = ls;
		 	ps = (ps + 1) % totspt;
		} 

		for (ls = chancfg->zdd_sectors; ls < totspt; ls++) {
			hp = &chp->c_trk[trk].t_hdr[ps];
			hp->h_type = ZD_GOODSPARE;
			hp->h_cyl = cyl;
			hp->h_head = trk;
			hp->h_sect = ls;
		 	ps = (ps + 1) % totspt;
		} 
	}
}

/*
 * mark_spots
 *	Mark sectors bad using bad block list data.
 *	Called only during FORMAT. Only BZ_PHYS entries will
 *	exist in bad block list.
 *
 *	REFORMAT, ADDBAD, and CLONEFMT typically use remark_spots.
 */
mark_spots(cyl, chp, bzp)
	int	cyl;			/* cylinder number */
	register struct cyl_hdr *chp;
	register struct bz_bad	*bzp;	/* bad block list entries */
{

	while (bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl) {
		chp->c_trk[bzp->bz_head].t_hdr[bzp->bz_sect].h_flag =
						bzp->bz_ftype | ZD_TORESOLVE;
		chp->c_trk[bzp->bz_head].t_bad++;
		chp->c_bad++;
		bzp++;
	}
}

/*
 * remark_spots
 *	Mark sectors bad using bad block list data.
 *
 *	REFORMAT and ADDBAD use remark_spots.
 */
remark_spots(cyl, chp, abp, flag)
	int	cyl;		/* cylinder number */
	struct cyl_hdr *chp;
	struct	bz_bad *abp;	/* cylinder data in bad block list */
	int	flag;		/* REFORMAT, ADDBAD, or CLONEFMT */
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	register struct bz_bad *bzp;
	struct hdr *rhp;

	if (abp->bz_cyl != cyl || abp == &bbl->bz_bad[bbl->bz_nelem])
		return;		/* no spots */

	bzp = abp;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl; bzp++) {
		if (bzp->bz_rtype != BZ_PHYS)
			continue;
		/*
		 * Mark all BZ_PHYS entries. These will be slipped.
		 */
		hp = &chp->c_trk[bzp->bz_head].t_hdr[bzp->bz_sect];
		hp->h_type = ZD_BADUNUSED;
		if (flag != CLONEFMT)
			hp->h_flag = ZD_TORESOLVE | bzp->bz_ftype;
		else
			hp->h_flag = 0;
		hp->h_cyl = ZD_BUCYL;
		hp->h_head = ZD_BUHEAD;
		hp->h_sect = ZD_INVALSECT;
		chp->c_trk[bzp->bz_head].t_bad++;
		chp->c_bad++;
	}

	/*
	 * Slip all BZ_PHYS entries.
	 */
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad)
			slip_track(cyl, (tp - chp->c_trk), chp);
	}

	/*
	 * Now look through list for auto-revector sectors.
	 */
	bzp = abp;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl; bzp++) {
		if (bzp->bz_rtype != BZ_AUTOREVECT)
			continue;

		hp = chp->c_trk[bzp->bz_head].t_hdr;
		for (; hp < &chp->c_trk[bzp->bz_head].t_hdr[totspt]; hp++) {
			if (hp->h_sect == bzp->bz_sect)
				break;
		}
		hp->h_type = ZD_BADREVECT;
		if (flag != CLONEFMT)
			hp->h_flag = ZD_TORESOLVE | bzp->bz_ftype;
		else
			hp->h_flag = 0;
		chp->c_trk[bzp->bz_head].t_bad++;
		chp->c_bad++;

		/*
		 * Now slip replacement track so that any SNF entries may
		 * be marked correctly.
		 */
		rhp = &chp->c_trk[bzp->bz_rpladdr.da_head].t_hdr[bzp->bz_rpladdr.da_sect];
		rhp->h_type = ZD_GOODRPL;
		rhp->h_cyl  = hp->h_cyl;
		rhp->h_head = hp->h_head;
		rhp->h_sect = hp->h_sect | ZD_AUTOBIT;
		slip_track(cyl, (int)bzp->bz_rpladdr.da_head, chp);
	}

	/*
	 * Now set SNF bad blocks
	 */
	bzp = abp;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl; bzp++) {
		if (bzp->bz_rtype != BZ_SNF)
			continue;

		/*
		 * Find physical sector corresponding to logical disk address.
		 */
		hp = chp->c_trk[bzp->bz_head].t_hdr;
		for (; hp < &chp->c_trk[bzp->bz_head].t_hdr[totspt]; hp++) {
			if (hp->h_sect == bzp->bz_sect)
				break;
		}
		if (hp->h_sect != bzp->bz_sect) {
			/*
			 * special case: some boot part blocks may have a
			 * BZ_SNF as well as a BZ_PHYS entry. Ignore BZ_SNF
			 * entry as already marked via BZ_PHYS.
			 */
			if (cyl == 1 && bzp->bz_head == 0)
				continue;
			printf("remark_spots: Could not find SNF sector (%d, %d, %d).\n",
				cyl, bzp->bz_head, bzp->bz_sect);
			exit(4);
		}
		hp->h_type = ZD_BADUNUSED;
		if (flag != CLONEFMT)
			hp->h_flag = ZD_TORESOLVE | bzp->bz_ftype;
		else
			hp->h_flag = 0;
		hp->h_cyl = ZD_BUCYL;
		hp->h_head = ZD_BUHEAD;
		hp->h_sect = ZD_INVALSECT;
		chp->c_trk[bzp->bz_head].t_bad++;
		chp->c_bad++;
	}
}

/*
 * newbad
 *	Add bad block to new bad block list being built.
 */
struct bz_bad *
newbad(hp, rtype, rhp)
	register struct hdr *hp;	/* Bad block */
	int	rtype;			/* replacement type */
	register struct hdr *rhp;	/* Replacement block - if germane */
{
	register int max;
	struct	bz_bad	*retval;

	max = ((chancfg->zdd_sectors - ZDD_NDDSECTORS) / 2) << DEV_BSHIFT;
	max -= (sizeof(struct zdbad) - sizeof(struct bz_bad)); 
	max /= sizeof(struct bz_bad);

	newbbl->bz_nelem++;
	if (newbbl->bz_nelem > max) {
		printf("Cannot add (%d, %d, %d) to bad block list.\n",
			hp->h_cyl, hp->h_head, hp->h_sect);
		printf("Ruptured disk - too many bad sectors (%d).\n",
			newbbl->bz_nelem);
		exit(7);
	}

	/*
	 * Add bad block to new bad block list.
	 */
	tobzp->bz_sect = hp->h_sect;
	tobzp->bz_head = hp->h_head;
	tobzp->bz_cyl = hp->h_cyl;
	tobzp->bz_rtype = rtype;
	tobzp->bz_ftype = hp->h_flag & ZD_ERRTYPE;
	hp->h_flag = 0;
	if (rtype != BZ_PHYS) {
		tobzp->bz_rpladdr.da_sect = rhp->h_sect;
		tobzp->bz_rpladdr.da_head = rhp->h_head;
		tobzp->bz_rpladdr.da_cyl = rhp->h_cyl;
		if (rtype == BZ_SNF)
			newbbl->bz_nsnf++;
	}
	retval = tobzp++;
	return(retval);
}

/*
 * slip_track
 *	slip the headers to skip past "Bad Unused" headers.
 *	Assumes NO other replacements besides slipped are set.
 */
slip_track(cyl, track, chp)
	int cyl;
	int track;
	struct	cyl_hdr *chp;
{
	register struct hdr *hp;
	register int	ps;		/* physical sector num */
	register int	ls;		/* logical sector num */
	int startps;

	ls = 0;
	ps = getskew(cyl, track);
	startps = ps;
	do {
		hp = &chp->c_trk[track].t_hdr[ps];

		/*
		 * Look for ZD_GOODSECT or ZD_GOODSPARE.
		 * Skip past other header types.
		 */
		if (hp->h_type == ZD_GOODSECT) {
			hp->h_sect = ls;
			ls++;
		} else if (hp->h_type == ZD_GOODSPARE) {
			/*
			 * Slipped into spares - format as ZD_GOODUSED.
			 */
			hp->h_type = ZD_GOODSECT;
			hp->h_sect = ls;
			ls++;
		}
		ps = (ps + 1) % totspt;
	} while (ls < chancfg->zdd_sectors && ps != startps);
}

/*
 * do_slippage
 *	Replace bad blocks via slippage on same track.
 *	The bad sectors will be marked with a "Bad Unused" header.
 */
do_slippage(cyl, chp)
	int	cyl;			/* cylinder number */
	struct	cyl_hdr *chp;
{
	register struct track_hdr *tp;
	register struct hdr *hp;
	register int trk;		/* Track number */

	for (trk = 0; trk < chancfg->zdd_tracks; trk++) {
		tp = &chp->c_trk[trk];
		if (tp->t_bad == 0)		/* no spots */
			continue;

		/*
		 * If enuff free to cover bad spots, then replace via slippage.
		 */
		if (tp->t_free >= tp->t_bad) {
			for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
				if ((hp->h_flag & ZD_TORESOLVE) == 0)
					continue;
				hp->h_sect = hp - tp->t_hdr;
				(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
				hp->h_type = ZD_BADUNUSED;
				hp->h_cyl = ZD_BUCYL;
				hp->h_head = ZD_BUHEAD;
				hp->h_sect = ZD_INVALSECT;
				if (verbose)
					printf("Slipping past bad sector at physical (%d, %d, %d).\n",
						cyl, trk, (hp - tp->t_hdr));
			}
			slip_track(cyl, trk, chp);
			tp->t_free -= tp->t_bad;
			chp->c_free -= tp->t_bad;
			chp->c_bad -= tp->t_bad;
			tp->t_bad = 0;
			continue;
		}

		/*
		 * Place t_free on sectors with bad headers first. Do
		 * bad data sectors, if replacement sectors available.
		 */
		for (hp = tp->t_hdr; tp->t_free && hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			if ((hp->h_flag & ZD_ERRTYPE) == BZ_BADDATA)
				continue;
			/*
			 * Got header error.
			 */
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (verbose)
				printf("Slipping past bad sector at physical (%d, %d, %d).\n",
					cyl, trk, (hp - tp->t_hdr));
			tp->t_bad--;
			tp->t_free--;
			chp->c_free--;
			chp->c_bad--;
		}

		for (hp = tp->t_hdr; tp->t_free && hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			/*
			 * Got error in data.
			 */
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (verbose)
				printf("Slipping past bad sector at physical (%d, %d, %d).\n",
					cyl, trk, (hp - tp->t_hdr));
			tp->t_bad--;
			tp->t_free--;
			chp->c_free--;
			chp->c_bad--;
		}
		slip_track(cyl, trk, chp);
	}
}

/*
 * do_autorevect
 *	Replace bad blocks via revectoring to next track.
 *	To autorevector the bad block MUST have a good header.
 *	The bad block will have a "Bad Revector" header. The replacement
 *	sector will get a "Good Replacement" header.
 */
do_autorevect(cyl, chp)
	int	cyl;			/* cylinder number */
	struct	cyl_hdr *chp;
{
	register int trk;		/* Track number */
	register struct hdr *hp;	/* sector header */
	register struct track_hdr *tp;
	register int sect;
	struct	track_hdr *next;
	int	rsect;			/* Replacement sector */

	for (trk = 0; trk < chancfg->zdd_tracks; trk++) {
		tp = &chp->c_trk[trk];
		if (tp->t_bad == 0)		/* no spots */
			continue;
		/*
		 * Have bad sector - check if next track can provide
		 * a replacement sector.
		 */
		next = &chp->c_trk[(trk + 1) % chancfg->zdd_tracks];
		for (sect = 0; next->t_free && sect < totspt; sect++) {
			/*
			 * Bad sector must have good header. If all
			 * bad sectors on track have bad headers, then
			 * cannot autorevector.
			 */
			hp = &tp->t_hdr[sect];
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			if ((hp->h_flag & ZD_ERRTYPE) == BZ_BADDATA) {
				hp->h_type = ZD_BADREVECT;
				tp->t_bad--;
				chp->c_bad--;

				/*
				 * set up replacement sector.
				 */
				if (chancfg->zdd_tskew > 1)
					rsect = sect + chancfg->zdd_tskew - 1;
				else
					rsect = sect + 1;
				rsect %= totspt;
				/*
				 * Find 1st good sector starting at "rsect".
				 */
				while ((next->t_hdr[rsect].h_flag & ZD_TORESOLVE)
				      || (next->t_hdr[rsect].h_type == ZD_BADUNUSED)
				      || (next->t_hdr[rsect].h_type == ZD_GOODRPL))
					rsect = (rsect + 1) % totspt;
				/*
				 * Init replacement sector
				 * Since newbad wants physical address
				 * for BZ_AUTOREVECT replacement, set sector
				 * in header to its physical address.
				 */
				next->t_hdr[rsect].h_sect = rsect;
				(void)newbad(hp, BZ_AUTOREVECT, &next->t_hdr[rsect]);
				next->t_hdr[rsect] = *hp;
				next->t_hdr[rsect].h_type = ZD_GOODRPL;
				next->t_hdr[rsect].h_sect |= ZD_AUTOBIT;
				next->t_free--;
				slip_track(cyl, next - chp->c_trk, chp);
				chp->c_free--;
				if (verbose)
					printf("Auto-Revector (%d, %d, %d) to physical (%d, %d, %d).\n",
						cyl, trk, hp->h_sect,
						cyl, next - chp->c_trk, rsect);
			}
		}
	}
}

/*
 * get_replace
 *	get replacement sector for SNF type replacements
 *
 * Must be called with pointer to cylinder with free spare sectors.
 * Return ptr to sector header.
 */
struct hdr *
get_replace(chp)
	register struct cyl_hdr *chp;
{
	register struct track_hdr *tp;
	register struct hdr *hp;

	for(tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_free == 0)
			continue;
		/*
		 * Find free replacement sector for SNF spare.
		 */
		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
			if (hp->h_type == ZD_GOODSPARE) {
				hp->h_type = ZD_GOODSECT;
				tp->t_free--;
				chp->c_free--;
				return(hp);
			}
		}
	}
	printf("get_replace: no free replacements.\n");
	exit(13);
	/* NOTREACHED */
}

/*
 * do_snf
 *	Mark bad blocks as "Bad Unused". Find replacement in "from"
 *	cylinder. Address of replacement entered in bad block list. And
 *	replacement cylinder will be marked as "Good Used" with its
 *	logical disk address instead of "Good Spare".
 */
do_snf(from, to)
	struct cyl_hdr *from;
	register struct cyl_hdr *to;
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	struct	hdr	*rpl;		/* pointer to replacement header */

	tp = to->c_trk;
	for (; from->c_free && tp < &to->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad == 0)		/* no spots */
			continue;
		hp = tp->t_hdr;
		for (; from->c_free && tp->t_bad && hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			/*
			 * find replacement in the cylinder. Mark sector
			 * as "Bad Unused".
			 */
			rpl = get_replace(from);
			(void)newbad(hp, BZ_SNF, rpl);
			if (verbose)
				printf("Revectoring (%d, %d, %d) to (%d, %d, %d).\n",
					hp->h_cyl, hp->h_head, hp->h_sect,
					rpl->h_cyl, rpl->h_head, rpl->h_sect);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			tp->t_bad--;
			to->c_bad--;
		}
	}
}

/*
 * save_snf_pass
 *	save to-be-resolved BZ_SNF bad blocks to be resolved by a second
 *	pass across the disk.
 */
save_snf_pass(chp)
	register struct cyl_hdr	*chp;
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	register struct bz_bad *bzp;

	tp = chp->c_trk;
	for (; chp->c_bad && tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad == 0)		/* no spots */
			continue;
		for (hp=tp->t_hdr; tp->t_bad && hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;

			if (snftogo == SNF_LIST_SIZE) {
				printf("save_snf_pass: snf_list overflow. Increase SNF_LIST_SIZE.\n");

				exit(5);
			}
			bzp = newbad(hp, BZ_SNF, hp);
			bzp->bz_rpladdr.da_cyl  = 0;	/* avoid confusion */
			bzp->bz_rpladdr.da_head = 0;
			bzp->bz_rpladdr.da_sect = 0;
			snf_list[snftogo].snf_addr.da_cyl = hp->h_cyl;
			snf_list[snftogo].snf_addr.da_head = hp->h_head;
			snf_list[snftogo].snf_addr.da_sect = hp->h_sect;
			snf_list[snftogo].snf_data = (caddr_t)NULL;
			snftogo++;
			if (verbose)
				printf("Save for pass2 (%d, %d, %d).\n",
					hp->h_cyl, hp->h_head, hp->h_sect);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			tp->t_bad--;
			chp->c_bad--;
		}
	}
}

/*
 * find_snf_rplcmnt
 *	find replacement cylinder for BZ_SNF bad block replacement.
 *	Used to resolve BZ_SNF bad blocks where 1st pass failed. That is,
 *	resolve outside 2 cylinder window used in initial pass.
 *
 * RETURN:
 *	Found: pointer to bad block list BZ_SNF entry.
 *	Not found: NULL pointer.
 */
struct bz_bad *
find_snf_rplcmnt(snf, cyl, chp)
	struct	snf_list *snf;
	int	cyl;
	register struct cyl_hdr	*chp;
{
	register struct	bz_bad	*bzp;
	register struct hdr *hp;

	/*
	 * Is requested cyl in range?
	 */
	if (cyl < 1 || cyl > lastcyl || cyl >= chancfg->zdd_cyls - ZDD_NDGNCYL)
		return((struct bz_bad *)NULL);		/* No spares */

	/*
	 * Got a real cylinder, check to see if spares exist.
	 */
	remap_cyl(cyl, chp);
	/*
	 * Compensate for BZ_SNF entries internal to the cylinder as
	 * this increments the c_bad count as well as decrements the
	 * c_free count in remap_cyl.
	 */
	bzp = bbl->bz_bad;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl < cyl; bzp++)
		continue;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl; bzp++)
		if (bzp->bz_rtype == BZ_SNF && bzp->bz_rpladdr.da_cyl == cyl)
			++chp->c_free;

	if (chp->c_bad >= chp->c_free)
		return((struct bz_bad *)NULL);		/* No spares */

	bzp = find_bbl_entry(bbl, (int)snf->snf_addr.da_cyl,
				snf->snf_addr.da_head, snf->snf_addr.da_sect);
	/*
	 * Spares exist. Now find one.
	 */
	hp = get_replace(chp);
	bzp->bz_rpladdr.da_sect = hp->h_sect;
	bzp->bz_rpladdr.da_head = hp->h_head;
	bzp->bz_rpladdr.da_cyl  = hp->h_cyl;
	if (verbose)
		printf("Revectoring (%d, %d, %d) to (%d, %d, %d).\n",
			bzp->bz_cyl, bzp->bz_head, bzp->bz_sect,
			hp->h_cyl, hp->h_head, hp->h_sect);
	return(bzp);
}

/*
 * init_cylmap
 *	init cyls structure for new cylinder.
 */
init_cylmap(cyl, chp)
	int cyl;
	register struct cyl_hdr	*chp;
{
	register struct hdr	*hp;
	register struct track_hdr *tp;

	chp->c_free = chancfg->zdd_spare * chancfg->zdd_tracks;
	chp->c_bad = 0;

	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		tp->t_free = chancfg->zdd_spare;
		tp->t_bad = 0;
	}
	/*
	 * Initialize runts - if any.
	 */
	if (chancfg->zdd_runt) {
		for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
			hp = &tp->t_hdr[totspt];
			hp->h_type = ZD_BADUNUSED;
			hp->h_flag = 0;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
		}
	}
	setskew(cyl, chp);
}

/*
 * setup_cyl
 *	Build in-core representation of cylinder's headers and
 *	mark defective sectors.
 */
setup_cyl(cyl, chp, bzp, flag)
	register int	cyl;		/* cylinder number */
	register struct cyl_hdr	*chp;
	struct	bz_bad	*bzp;		/* bad block list entries */
	int	flag;			/* FORMAT or REFORMAT */
{
	/*
	 * If format, set-up cylinder from simple (only BZ_PHYS) bad block list.
	 * If reformat, all 3 replacement entries possible.
	 */
	init_cylmap(cyl, chp);
	if (flag == FORMAT)
		mark_spots(cyl, chp, bzp);		/* New format */
	else {
		remark_spots(cyl, chp, bzp, flag);
		setskew(cyl, chp);
	}
}

/*
 * map_cyl
 *	Called by format to set up cylinder headers taking into account
 *	defects only within the cylinder.
 */
map_cyl(cyl, chp)
	register int	cyl;		/* cylinder number */
	register struct cyl_hdr	*chp;
{
	register struct hdr *hp;
	int	bootbad;		/* # bad after slipping boot track */
	int	bootfree;		/* # free after slipping boot track */
	u_char	savflag;

	if (chp->c_bad == 0 || chp->c_free == 0)
		return;
	do_slippage(cyl, chp);		/* Resolve via slippage */

	if (chp->c_bad == 0 || chp->c_free == 0)
		return;
	if (cyl == 1) {
		/*
		 * Don't allow autorevect into track 0.
		 */
		bootfree = chp->c_trk[0].t_free;
		chp->c_trk[0].t_free = 0;

		bootbad = 0;
		if (chp->c_trk[0].t_bad) {
			/*
			 * If additional errors are in the first track, then
			 * do NOT autorevector from this track.
			 * If no errors are in first track, then
			 * layout normally.
			 *
			 * Save count. SNF revectoring will be done from
			 * this track. Clear t_bad so that do_autorevect
			 * will ignore the track.
			 */
			bootbad = chp->c_trk[0].t_bad;
			if (bootbad > (chancfg->zdd_sectors - (BBSIZE >> DEV_BSHIFT))) {
				printf("Ruptured disk - too many errors (%d) in bootstrap track.\n",
						bootbad + chancfg->zdd_spare);
				exit(11);
			}
			chp->c_trk[0].t_bad = 0;
		}
	}

	do_autorevect(cyl, chp);	/* Resolve with auto-revectoring */
	if (chp->c_bad == 0 || chp->c_free == 0)
		return;

	if (cyl == 1) {
		chp->c_trk[0].t_free = bootfree;

		if (bootbad) {
			chp->c_trk[0].t_bad = bootbad;
			bootbad = 0;
			/*
			 * Find bad blocks on track.
			 * slip the track to get BBSIZE/DEV_BSIZE logically
			 * contiguous GOODSECT sectors.
			 */
			hp = chp->c_trk[0].t_hdr;
			for (; hp < &chp->c_trk[0].t_hdr[totspt]; hp++) {
				if ((hp->h_flag & ZD_TORESOLVE) == 0)
					continue;
				hp->h_type = ZD_BADUNUSED;
				savflag = hp->h_flag;
				hp->h_sect = hp - chp->c_trk[0].t_hdr;
				(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
				hp->h_flag = savflag;
				if (verbose)
					printf("Boot track revector (1, 0, %d).\n",
						hp->h_sect);
				bootbad++;
			}
			slip_track(1, 0, chp);
			/*
			 * Now mark these ZD_BADUNUSED sectors to
			 * the logical disk addresses that were just slipped
			 * off the end of the track. Then do_snf.
			 */
			hp = chp->c_trk[0].t_hdr;
			for (; bootbad && hp < &chp->c_trk[0].t_hdr[totspt]; hp++) {
				if ((hp->h_flag & ZD_TORESOLVE) == 0)
					continue;
				hp->h_type = ZD_GOODSECT;
				hp->h_sect = chancfg->zdd_sectors - bootbad;
				--bootbad;
			}
		}
	}
	do_snf(chp, chp);		/* Resolve with driver revectoring */
}

/*
 * remap_cyl
 *	Build in-core representation of cylinder's headers.
 *	Called by clonefmt to set up cylinder headers taking into account
 *	defects from within the cylinder and from adjacent cylinders.
 *
 *	Note: a new bad block list is not produced.
 */
remap_cyl(cyl, chp)
	register int	cyl;		/* cylinder number */
	register struct cyl_hdr	*chp;
{
	register struct hdr	*hp;
	register struct bz_bad	*bzp;

	/*
	 * First setup taking into account defects from within cylinder.
	 * Then take into account bad blocks of SNF from current and
	 * surrounding cylinders which revector here.
	 */
	init_cylmap(cyl, chp);

	bzp = bbl->bz_bad;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl < cyl; bzp++)
		continue;
	if (bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl)
		remark_spots(cyl, chp, bzp, CLONEFMT);

	/*
	 * Any revectoring here? If so, mark 'em.
	 */
	for (bzp = bbl->bz_bad; bzp < &bbl->bz_bad[bbl->bz_nelem] ; bzp++) {
		if (bzp->bz_rtype != BZ_SNF)
			continue;
		if (bzp->bz_rpladdr.da_cyl == cyl) {
			chp->c_free--;
			chp->c_trk[bzp->bz_rpladdr.da_head].t_free--;
			hp = chp->c_trk[bzp->bz_rpladdr.da_head].t_hdr;
			for (; hp < &chp->c_trk[bzp->bz_rpladdr.da_head].t_hdr[totspt]; hp++)
				if (hp->h_sect == bzp->bz_rpladdr.da_sect) {
					hp->h_type = ZD_GOODSECT;
					break;
				}
		}
	}
}

/*
 * setup_ddcyl
 *	Build in-core representation of cylinder's headers and
 *	mark defective sectors.
 */
setup_ddcyl(chp, bzp)
	struct cyl_hdr	*chp;
	struct	bz_bad	*bzp;		/* bad block list entries */
{
	init_cylmap(ZDD_DDCYL, chp);
	mark_spots(ZDD_DDCYL, chp, bzp);		/* All PHYS entries */
}

/*
 * map_ddcyl
 *	Build in-core representation of cylinder's headers.
 *	Called by format to set up cylinder headers taking into account
 *	defects only within the cylinder.
 */
map_ddcyl(chp)
	register struct cyl_hdr	*chp;
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	register struct hdr *badhp;
	int	ss_badsects;			/* no. of bad special sectors */

	/*
	 * Map bad sectors in ZDD_DDCYL
	 */
	if (chp->c_bad == 0)
		return;

	badhp = (struct hdr *)NULL;
	if (chp->c_trk[0].t_bad) {
		printf("Warning: %d error(s) on cyl %d, track 0.\n",
				chp->c_trk[0].t_bad, ZDD_DDCYL);
		/*
		 * Take care of special sectors separately since
		 * cannot slip the bad special sector.  Only 1 bad
		 * special sector and 1 other track 0 bad sector is allowed.
		 */
		ss_badsects = 0;
		tp = chp->c_trk;
		for (hp = tp->t_hdr; hp < &tp->t_hdr[ZDD_NDDSECTORS]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			badhp = hp;		/* save to patch after slip */
			tp->t_bad--;
			chp->c_bad--;
			ss_badsects++;
		}
		if (ss_badsects > 1 || chp->c_trk[0].t_bad > chancfg->zdd_spare) {
			printf("Ruptured disk - cyl 0, track 0 has too many bad sectors.\n");
			exit(9);
		}
	}

	if (chp->c_trk[1].t_bad)
		printf("Warning: %d error(s) on cyl %d, track 1.\n",
			chp->c_trk[1].t_bad, ZDD_DDCYL);

	/* Replace via slippage */
	do_slippage(ZDD_DDCYL, chp);

	if (badhp != (struct hdr *)NULL) {
		/*
		 * Now that do_slippage has been fooled. Go zap the
		 * header for the bad special sector.
		 */
		badhp->h_type = ZD_BADUNUSED;
		badhp->h_cyl = ZD_BUCYL;
		badhp->h_head = ZD_BUHEAD;
		badhp->h_sect = ZD_INVALSECT;
	}

	if (chp->c_bad == 0)
		return;			/* No more bad sectors */

	/*
	 * Mark bad sectors and make BZ_PHYS entries.
	 * These will NOT be revectored. However, the sectors will be
	 * unused in normal operation.
	 */
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad == 0)		/* no spots */
			continue;

		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (verbose)
				printf("Slipping past bad sector at physical (%d, %d, %d).\n",
					ZDD_DDCYL, (tp - chp->c_trk),
					(hp - tp->t_hdr));
		}
		slip_track(ZDD_DDCYL, (tp - chp->c_trk), chp);
		tp->t_bad = 0;
	}
}

/*
 * setup_dgncyl
 *	Build in-core representation of cylinder's headers and
 *	mark defective sectors.
 */
setup_dgncyl(cyl, chp, bzp)
	struct cyl_hdr	*chp;
	struct	bz_bad	*bzp;		/* bad block list entries */
{
	init_cylmap(cyl, chp);
	mark_spots(cyl, chp, bzp);		/* All PHYS entries */

}

/*
 * map_dgncyl
 *	Build in-core representation of cylinder's headers.
 *	Called by format to set up cylinder headers taking into account
 *	defects only within the cylinder.
 */
map_dgncyl(cyl, chp)
	int	cyl;
	register struct cyl_hdr	*chp;
{
	register struct hdr *hp;
	register struct track_hdr *tp;

	if (chp->c_bad == 0)
		return;

	/*
	 * Do slippage where necessary.
	 */
	do_slippage(cyl, chp);		/* Resolve via slippage? */

	if (chp->c_bad == 0)
		return;			/* Resolved via slippage. */

	/*
	 * Mark bad sectors and make BZ_PHYS entries.
	 * These will NOT be revectored. However, the sectors should not be
	 * used during normal operation.
	 */
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad == 0)		/* no spots */
			continue;
		if (tp->t_bad > (totspt - ZDD_NDGNSPT)) {
			printf("Ruptured disk - too many errors (%d) in DGN cyl %d, track %d.\n",
					tp->t_bad + chancfg->zdd_spare,
					cyl, (tp - chp->c_trk));
			exit(10);
		}

		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (verbose)
				printf("Slipping past bad sector at physical (%d, %d, %d).\n",
					cyl, tp - chp->c_trk, hp - tp->t_hdr);
		}
		slip_track(cyl, (tp - chp->c_trk), chp);
		tp->t_bad = 0;
	}
}

/*
 * format_cyl
 *	format cylinder.
 */
format_cyl(cyl, chp)
	int	cyl;		/* Cylinder number */
	struct cyl_hdr	*chp;
{
	register struct track_hdr *tp;
	register struct hdr *hp;
	register int size;
	struct cb lcb;			/* cb for ioctl */

	bzero((caddr_t)&lcb, sizeof(struct cb));
	size = totspt;
	if (chancfg->zdd_runt)
		size++;
	size = ROUNDUP(sizeof(struct hdr) * size, CNTMULT);

	/*
	 * Format cylinder via ioctl to stand-alone zdc driver.
	 */
	lcb.cb_cmd = ZDC_FMTTRK;
	lcb.cb_cyl = cyl;
	lcb.cb_iovec = 0;
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		/*
		 * Remark spares as ZD_GOODSECT so that they can be verified.
		 */
		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)
			if (hp->h_type == ZD_GOODSPARE)
				hp->h_type = ZD_GOODSECT;
		lcb.cb_head = tp - chp->c_trk;
		lcb.cb_count = size;
		lcb.cb_addr = (u_long)tp->t_hdr;
		if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) < 0) {
			printf("format_cyl ioctl failed!!!\n");
			exit(8);
		}
	}
}

/*
 * format_dd
 *	format disk description cylinder - cylinder 0.
 *	Format and write special sectors which contain the zdcdd structure.
 */
format_dd(chp)
	struct cyl_hdr *chp;
{
	register int i;	
	register u_char *cp, *sp;
	struct track_hdr *tp;
	u_char *wss;			/* Write special sector buffer */
	u_char *rss;			/* Read special sector buffer */
	struct hdr *sshdr;		/* special sector header buffer */
	int errors;
	u_char	chksum;
	struct cb lcb;			/* cb for ioctl */
	bool_t skip;

	/*
	 * Format the cylinder.
	 */
	format_cyl(ZDD_DDCYL, chp);
        if (errors = check_hdrs(ZDD_DDCYL, chp, TRUE)) {
                printf("Warning: %d sector header errors ", errors);
                printf("found on cylinder zero\n");
                printf("Warning: Drive may be unusable.\n");
        }

	/*
	 * Now set up Special Sectors.
	 */
	callocrnd(DEV_BSIZE);
	wss = (u_char *)calloc(DEV_BSIZE);
	rss = (u_char *)calloc(DEV_BSIZE);
	sshdr = (struct hdr *)calloc(ROUNDUP(sizeof(struct hdr), CNTMULT));
	/*
	 * initialize special sector with channel configuration.
	 */
	chancfg->zdd_format_desc.fd_rev = FMTVERSION;
	chancfg->zdd_format_desc.fd_passes = fullpasses + defectpasses;
	sp = (u_char *)wss;
	cp = (u_char *)chancfg;
	chksum = 0;
	while (cp < &chancfg->zdd_checksum) {
		chksum ^= *cp;
		*sp++ = *cp++;
	}
	*sp = chksum;				/* Append checksum */

	bzero((caddr_t)&lcb, sizeof(struct cb));
	lcb.cb_cyl = ZDD_DDCYL;
	lcb.cb_head = 0;
	lcb.cb_iovec = 0;
	tp = chp->c_trk;
	for (i = 0; i < ZDD_NDDSECTORS; i++) {
                /*
                 * An attempt must still be made to format a
                 * defective special sector, but not write it.
                 */
                if (tp->t_hdr[i].h_type == ZD_BADUNUSED) {
                        skip = 1;
                } else {
                        skip = 0;
                        tp->t_hdr[i].h_type = ZD_GOODSS;
                }

		lcb.cb_cmd = ZDC_FMT_SS;
		lcb.cb_sect = i;
		*sshdr = tp->t_hdr[i];
		lcb.cb_addr = (u_long)sshdr;
		lcb.cb_count = ROUNDUP(sizeof(struct hdr), CNTMULT);
                if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) < 0) {
                        printf("Warning: Special sector %d,%d,%d ",
                                lcb.cb_cyl, lcb.cb_head, i);
                        printf("fmt_ss ioctl failed!!!\n");
                }

		if (skip) continue;

		lcb.cb_addr = (u_long)wss;
		lcb.cb_cmd = ZDC_WRITE_SS;
		lcb.cb_count = ZDD_SS_SIZE;
                if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) < 0) {
                        printf("Warning: Special sector %d,%d,%d ",
                                lcb.cb_cyl, lcb.cb_head, i);
                        printf("write_ss ioctl failed!!!\n");
                }

		lcb.cb_addr = (u_long)rss;
		lcb.cb_cmd = ZDC_READ_SS;
		lcb.cb_count = ZDD_SS_SIZE;
                if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) < 0) {
                        printf("Warning: Special sector %d,%d,%d ",
                                lcb.cb_cyl, lcb.cb_head, i);
                        printf("read_ss ioctl failed!!!\n");
                }

		for (cp = wss, sp = rss; sp < &rss[ZDD_SS_SIZE]; cp++, sp++) {
			if (*cp != *sp) {
                                printf("WARNING: Special sector %d,%d,%d ",
                                        lcb.cb_cyl, lcb.cb_head, i);
                                printf("not written/read correctly.\n");
                                printf("First Mismatch in byte %d.\n", cp - wss);
				break;
			}
		}
	}
}

/*
 * check_hdrs
 *	Quickly check that the headers have been formatted 
 *	correctly.  If a defective one is found, add its 
 *	track to the header validation suspect list for 
 *	more thorough testing and correction in a later pass.
 *
 *	If the 'fix_bad_unused' flag is TRUE, attempt to 
 *	correct a defective header marked bad/unused.  This
 *	may be necessary because addbad will reformat the
 *	entire cylinder and write data back before the
 *	regular header validation is performed and we must
 *	attempt to ensure all bad/unused sectors needing 
 *	adjustments are redone.  Also, test the cylinder
 *	more heavily, since data will be restored onto it.
 *	Keep in mind that if the addbad is not the result
 *	of the header defect pass, we must adjust everything
 *	from scratch - no historical information available.
 */
int
check_hdrs(cyl, chp, fix_bad_unused)
	int	cyl;		/* Cylinder number */
	struct cyl_hdr	*chp;
{
	register struct track_hdr *tp;
	register struct hdr *hp;
	register struct hdr *rhp;
	register struct hdr thp;
	int track, errors, i, j, pass, sindex;

	errors = 0;
	if (fix_bad_unused) {
		readjust_headers(cyl);
		pass = ZDNBEATS;
	} else
		pass = 1;

	for ( ; pass > 0; pass--) {
		for (track = 0, tp = chp->c_trk; 
	     	     track < chancfg->zdd_tracks; track++, tp++) {

	    		if (read_hdrs(cyl, track)) {
				/*
				 * read_hdrs will verify data ECC which might 
				 * fail on sectors already marked bad so change
				 * the stratergy to first try and adjust the
				 * header to avoid the data sync and if that
				 * fail to pound on the track to
				 * verify that each sector is real and can be 
				 * read.
				 */
				if (errcode == EHER) {
					if (!fix_bad_unused) {
		    				sindex=add_suspect(cyl, track);
						continue;
					} else if (fix_data_sync(cyl, track, tp) != -1)
						continue;
				}
				if (poundtrack(cyl, track, chp) == FAIL) {
					printf("Bad headers on (%d,%d)\n", 
						cyl, track);
					fail_drive(cyl, track, errcode);
				} 
				/*
				 * Only pound once.
				 */
				pass=1;
				if (verbose) {
					printf("Pounded(%d,%d) - no problems found\n", 
						cyl, track);
				}
				continue;
			}
	    		for (i = 0, hp = tp->t_hdr; i < n_hdrs; hp++, i++) {
				rhp = &headers[i].hdr;
				if (!fix_bad_unused 
				||  hp->h_type != ZD_BADUNUSED 
				||  BUHDR_CMPR(rhp)) {
					if (!HDR_CMPR(hp, rhp)) { 
		    				sindex=add_suspect(cyl, track);
						if (hp->h_type != ZD_BADUNUSED 
						||  !BUHDR_CMPR(rhp)) 
		    					errors++;
		    				if (!fix_bad_unused)
		    					break;	
					}
		 			continue;
				}

				sindex = add_suspect(cyl, track);
				if (sindex < 0)
					break;  /* list full - give up */
				errors++;
				for ( ; ; ) {
					if (!BUHDR_CMPR(rhp)) {
			    			j = ZDNBEATS; 
			    			adjust_header(sindex, i);
					} else if (--j == 0) 
			    			break;	/* Fixed; next sector */
					if (read_hdrs(cyl, track)) {
						printf("cylinder %d track %d is can not be fixed\n", cyl, track);
						break;
					}
				} 
	    		}
		}
	}
	return (errors);
}

/*
 * fix a bad data sync problem by moving the header over as if it
 * were a sector zero problem.
 */
int
fix_data_sync(cyl, track, tp)
	int cyl;
	int track;
	struct track_hdr *tp;
{
	int sindex;
	struct hdr *hp;
	int	i;

	if ((sindex = add_suspect(cyl, track)) == -1)
		return(-1);
	for (i = 0, hp = tp->t_hdr; i < totspt; hp++, i++) {
		if (hp->h_type == ZD_BADUNUSED || hp->h_type == ZD_BADREVECT) {
			adjust_header(sindex, i);
			if (!read_hdrs(cyl, track)) {
				if (verbose) {
					printf("data sync fixed at phys(%d,%d,%d)\n", 
						cyl, track, i);
				}
				return(hp->h_sect);
			}
			/*
			 * did not work so undo adjust.
			 */
			suspect[sindex].adjustment[i]--;
		}
	}
	return (-1);
}

/*
 * get_chancfg
 *	Get the channel configuration for the channel
 *	with the drive in question.
 *
 * Results stored in "chancfg" global;
 */
bool_t
get_chancfg()
{
	struct cb lcb;			/* cb for ioctl */

	if (chancfg == NULL) {
		/*
		 * allocate memory for channel configuration.
		 */
		callocrnd(sizeof(struct zdcdd));
		chancfg = (struct zdcdd *)calloc(sizeof(struct zdcdd));
	} else
		bzero((caddr_t)&lcb, sizeof(struct cb));

	lcb.cb_cmd = ZDC_GET_CHANCFG;
	lcb.cb_addr = (u_long)chancfg;
	lcb.cb_count = sizeof(struct zdcdd);
	lcb.cb_iovec = 0;
	if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) < 0) {
		printf("get_chancfg ioctl failed!!!\n");
		exit(2);
	}

	if (lcb.cb_compcode == ZDC_NOCFG)
		return(FALSE);
	totspt = chancfg->zdd_sectors + chancfg->zdd_spare;
	return(TRUE);
}

/*
 * set_chancfg
 *	Get the channel configuration for the channel
 *	with the drive in question.
 *
 * Channel cfg stored in "chancfg" global;
 */
set_chancfg(cfg)
	struct zdcdd *cfg;
{
	struct cb lcb;			/* cb for ioctl */

	*chancfg = *cfg;
	totspt	= chancfg->zdd_sectors + chancfg->zdd_spare;

	bzero((caddr_t)&lcb, sizeof(struct cb));
	lcb.cb_cmd = ZDC_SET_CHANCFG;
	lcb.cb_addr = (u_long)chancfg;
	lcb.cb_count = sizeof(struct zdcdd);
	lcb.cb_iovec = 0;
	if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) < 0) {
		printf("set_chancfg ioctl failed!!!\n");
		exit(3);
	}
}

/*
 * print_chancfg
 *	print interesting items from disk description structure.
 */
print_chancfg(cfg)
	register struct zdcdd *cfg;
{
	printf("Channel configuration: ");
	printf("Drive type %d - %s. formated version %d\n",
		cfg->zdd_drive_type, zdinfo[cfg->zdd_drive_type].zi_name,
		cfg->zdd_format_desc.fd_rev);
	printf("Data: #cyls %d, #heads %d, #sectors %d, #spares %d.\n",
		cfg->zdd_cyls, cfg->zdd_tracks,
		cfg->zdd_sectors, cfg->zdd_spare);
}

/*
 * write_min_vtoc
 *	write a minimal VTOC on disk, following the partition information
 *	set in get_options().
 */
write_min_vtoc()
{
	struct vtoc *v;
	int where;

	callocrnd(ADDRALIGN);
	v = (struct vtoc *)calloc(V_SIZE);

	v->v_sanity = VTOC_SANE;
	v->v_version = V_VERSION_1;
	v->v_size = sizeof(struct vtoc);
	v->v_nparts = partno + 1;
	v->v_secsize = DEV_BSIZE;
	v->v_ntracks = chancfg->zdd_tracks;
	v->v_nsectors = chancfg->zdd_sectors;
	v->v_ncylinders = chancfg->zdd_cyls;
	v->v_rpm = DFLT_RPM;
	v->v_nseccyl = v->v_nsectors * v->v_ntracks;
	v->v_capacity = v->v_nseccyl * v->v_ncylinders;
	(void)strcpy(v->v_disktype, zdinfo[chancfg->zdd_drive_type].zi_name);
	v->v_part[partno] = part;
	v->v_cksum = 0;
	v->v_cksum = vtoc_get_cksum(v);

	printf("Writing VTOC to sector %d.\n",
		chancfg->zdd_sectors * chancfg->zdd_tracks + V_VTOCSEC);

	where = (chancfg->zdd_sectors * chancfg->zdd_tracks) << DEV_BSHIFT;
	where += V_VTOCSEC << DEV_BSHIFT;
	lseek(fd, where, L_SET);
	if (write(fd, v, V_SIZE) != V_SIZE) {
		printf("Ruptured disk - Cannot write VTOC\n");
		return;
	}
	printf("VTOC written.\n");
}
