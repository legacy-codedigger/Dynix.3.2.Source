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
 * ident	"$Header: zd.c 1.13 1991/06/17 01:37:56 $
 * zd.c
 *	The top level of the zd subsystem of the online
 *	formatter.  This includes usage checking, setup,
 *	and the main zd routine which calls the appropriate
 *	zd function.
 */

/* $Log: zd.c,v $
 *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#ifdef BSD
#include <sys/ioctl.h>
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#include <zdc/zdioctl.h>
#else
#include <sys/zd.h>
#include <sys/zdc.h>
#include <sys/zdbad.h>
#include <sys/zdioctl.h>
#endif
#include <diskinfo.h>
#include <signal.h>
#include "format.h"
#include "zdformat.h"

extern caddr_t malloc();
extern struct zdinfo *getzdinfo(), *getzdinfobyname();

int zd_setup(), zd_main(), zd_usage(), zd_bootname();

struct zdinfo *zdinfo = 0;

/*
 * Variables used globally in zd subsystem
 */
int startcyl, lastcyl;			/* start and end cylinder */
int totspt;				/* total sectors/track */
int fullpasses, defectpasses;		/* # verify passes, defect passes */
int hdrpasses;				/* # sector hdr verification passes */
bool_t checkdata;			/* checkdata flag */
bool_t unformatted = 0;			/* disc formatted */
char	*tt_arg;
extern	sigint();			/* Signal handler during critical 
					   operations, like format */

/*
 * zd_usage
 *	Shows which functions and arguments are supported
 *	by zd subsystem
 */
zd_usage()
{
	fflush(stdout);
	fprintf(stderr, "%s is formatted by the zd subsystem\n",
		diskname);
	fprintf(stderr, "usage for zd subsystem:\n");
	fprintf(stderr, 
	"    format [-f|r|m|y|l|u|a xxx] [-wovnc] [-b file] [-s n] [-e n] \\\n");
	fprintf(stderr,
	"                   [-p n] [-d n] [-h n] [-t disktype] diskname\n");
	return;
}

/*
 * zd_bootnum
 *	Get the device number used in a bootstring for this
 *	device.
 */
int
zd_bootnum()
{
	struct zddev zddev;

	/*
	 * Get device-specific info. 
	 */
	if (ioctl(fd, ZIODEVDATA, (char *)&zddev) < 0)
		return(-1);
	return(ZD_BOOTNUM(zddev.zd_ctlr, zddev.zd_drive));
}

/*
 * zd_setup
 *	Check argument operands for valid values, and
 *	do setup which applies to any function for zd disks
 */
int
zd_setup()
{
	int i;
	struct zddev zddev;		/* data from device info ioctl */
	char *p, str[256];
	char	type[256];

	/*
	 * Look at type argument first and make sure it's valid
	 * then do setup for other argument values
	 */
	if (args & B_TYPE) {
		if (function == VFORMAT) {
			tt_arg = t_arg;
			strcpy(type, t_arg);
			strcat(type, "_ufmt");
			t_arg = type;
		}
		if ((zdinfo = getzdinfobyname(t_arg)) == NULL) {
			fflush(stdout);
			fprintf(stderr, "Invalid disk type: %s\n", t_arg);
			fprintf(stderr, "The following disk types are ");
			fprintf(stderr, "supported:\n");
			p = str;
			while ((zdinfo = getzdinfo()) != NULL) {
				sprintf(p, "%s ", zdinfo->zi_name);
				p += strlen(p);
			}
			fprintf(stderr, "%s\n", str);
			zd_usage();
			return(-1);
		}
	} 

	/*
	 * If the function is one which will wipe out the 
	 * disk and the disk has a valid VTOC on it, make sure
	 * the user specified the OVERWRITE option.
	 */
	if (function & ZD_OVERWRITES && !(args & B_OVERWRITE)) {
		if (validvtoc(fd)) {
			fflush(stdout);
			fprintf(stderr, "Disk has a valid VTOC on it -- ");
			fprintf(stderr, 
				"specify the -o option to overwrite it\n");
			return(-1);
		}
	}
	checkdata = (args & B_CHECKDATA) ? 1 : 0;
	fullpasses = (args & B_FULLPASS) ? p_arg : NFULLPASS;
	if (fullpasses < 0 || fullpasses > PASSMAX) {
		fflush(stdout);
		fprintf(stderr, 
			"invalid -p arg value (max passes = %d)\n",
			PASSMAX);
		zd_usage();
		return(-1);
	}
	defectpasses = (args & B_DEFECTPASS) ? d_arg : NDEFECTPASS;
	if (defectpasses < 0 || defectpasses > PASSMAX) {
		fflush(stdout);
		fprintf(stderr, "invalid -d arg value (max defect passes = %d)\n",
			PASSMAX);
		zd_usage();
		return(-1);
	}
	hdrpasses = (args & B_HDRPASS) ? h_arg : NHDRPASS;
	if (hdrpasses < 1 || hdrpasses > PASSMAX) {
		fflush(stdout);
		fprintf(stderr, 
		    "invalid -h arg value (header defect passes max = %d, min = 1)\n",
		    PASSMAX);
		zd_usage();
		return(-1);
	}
		

	/*
	 * Get device-specific info. 
	 */
	if (ioctl(fd, ZIODEVDATA, (char *)&zddev) < 0) {
		perror("ZIODEVDATA ioctl error");
		return(-1);
	}

	/*
	 * Is drive already formatted?
	 * If not and not formatting, then complain and exit.
	 */
	if ((function != FORMAT) && (function != VFORMAT)
		&& ((zddev.zd_cfg & (ZD_FORMATTED | ZD_MATCH))
			!= (ZD_FORMATTED | ZD_MATCH))) {
		fflush(stdout);
		fprintf(stderr, "Drive not formatted or channel ");
		fprintf(stderr, "mismatch.\n");
		fprintf(stderr, "Operation requires drive to be formatted\n");
		unformatted = 1;
		if (!(args & B_OVERWRITE)) {
			fprintf(stderr, "Or override with -o flag\n");
			return(-1);	
		}
	}
#ifdef BSD
	/*
	 * Set special Format mode
	 */
	if (function & ZD_OVERWRITES) {
		if (ioctl(fd, ZIOFORMATF, 0) < 0) {
			perror("ZIOFORMATF ioctl error");
			return(-1);
		}
	}
#endif

	/*
	 * Get and/or set channel configuration
	 */
	if (setup_chancfg() < 0)
		return(-1);

	/*
	 * The following calculation does not include
	 * the runt sector.  It should be factored in
	 * where needed.
	 */
	totspt = chancfg->zdd_sectors + chancfg->zdd_spare;

	/*
	 * Do some option checking which couldn't be done
	 * earlier.
	 */
	startcyl = (args & B_START) ? s_arg : 0;
	if (args & B_END)
		lastcyl = e_arg;
	if (!(args & B_END) || lastcyl >= chancfg->zdd_cyls)
		lastcyl = chancfg->zdd_cyls - 1;
	if (startcyl > lastcyl) {
		fflush(stdout);
		fprintf(stderr, "invalid start and/or end cylinder - ");
		fprintf(stderr, "last cyl on drive = %d\n", 
			chancfg->zdd_cyls -1);
		zd_usage();
		return(-1);
	}

	/*
	 * allocate structures used by all functions
	 */
	alloc_structs();
	return(0);
}

/*
 * zd_main
 *	This routine calls the various zd function routines
 *	according to the desired task.
 */
zd_main()
{
	switch (function) {

	case FORMAT:
	case VFORMAT:
		sigints = 0;
		signal(SIGINT, sigint);
		signal(SIGHUP, sigint);
		zd_format();
		break;

	case REFORMAT:
	case REFORMAT_MFG:
		sigints = 0;
		signal(SIGINT, sigint);
		signal(SIGHUP, sigint);
		zd_reformat(function);
		break;
		
	case ADDBAD:
		zd_addbad();
		break;

	case REPBAD:
		zd_repbad();
		break;

	case ADD_MFG:
		zd_addmfg();
		break;

	case VERIFY:
		zd_verify();
		break;

	case WRITEDIAG:
		zd_writediag();
		break;

	case DISPLAY:
		zd_display();
		break;

	case INFO:
		zd_info();
		return;

	case SHOW:
		zd_show();
		return;

	case USAGE:
		zd_usage();
		break;

	default:
		fflush(stdout);
		fprintf(stderr, "ERROR: zd_main: internal formatter error\n");
		break;
	}

#ifdef BSD
	/*
	 * Set special Format mode again
	 */
	if (function & ZD_OVERWRITES) {
		if (ioctl(fd, ZIOFORMATF, 0) < 0) {
			perror("ZIOFORMATF ioctl error");
			return(-1);
		}
	}
#endif
	if (sigints) {
		return;		/* Interrupted, skip the rest */
	}
	/*
	 * Now do verify and write diagnostics if appropriate. 
	 * Note that the header verification/correction 
	 * pass cannot be overridden - at least one pass
	 * must be performed.  doverify will skip other
	 * types of passes accordingly.
	 */
	if (usep->tasks & VERIFY) 
		zd_verify();

	if (sigints)
		return;		/* Interrupted, skip the rest */

	if (usep->tasks & WRITEDIAG && !(args & B_NOWRITEDIAG))
		zd_writediag();
}

/*
 * zd_format
 * 	set up and then call format to format a ZDC disk 
 */
zd_format()
{
	if (debug) printf("zd_format: begin\n"); 
	
	if (function == VFORMAT) {
		read_umfglist();
		/*
		 * now get correct type.
		 */
		function = FORMAT;
		t_arg = tt_arg;
		zd_setup();
	} else
		get_mfglist();
	create_badlist(bbl);

	printf("Beginning disk format ... \n");
	doformat(Z_FORMAT);
	printf("Format complete\n");
	return;
}

/*
 * zd_reformat
 * 	set up and then call format to reformat a ZDC disk 
 */
zd_reformat(func)
	int func;
{
	int flag;

	if (debug) printf("zd_reformat: begin\n"); 
	
	read_mfglist();
	if (func == REFORMAT) {
		flag = Z_REFORMAT;
		read_badlist();		/* reformat with existing bad list */
	} else {
		flag = Z_FORMAT;
		create_badlist(bbl);	/* reformat with mfg list only */
	}

	printf("Beginning disk reformat ... \n");
	doformat(flag);
	printf("Reformat complete\n");
	return;
}

/*
 * zd_addmfg
 *	do set up for adding manufactures defects list and then call
 *	the routine which actually does the work.
 */
zd_addmfg()
{
	int num;

	if (debug) printf("zd_addmfg: begin\n"); 
	
	if (b_arg == NULL)
		read_mfglist();
	else
		get_mfglist();
	create_badlist(bbl);
	write_badlist();
	write_mfglist();
	return;
}

/*
 * zd_addbad
 *	do set up for adding bad blocks and then call
 *	the routine which actually does the work.
 */
zd_addbad()
{
	int num;

	if (debug) printf("zd_addbad: begin\n"); 
	
	read_mfglist();
	read_badlist();
	if ((num = get_addlist()) == 0) {
		fflush(stdout);
		fprintf(stderr, "No valid entries found for addbad\n");
		exit(1);
	}
	printf("Beginning addbad ... \n");
	doaddbad(num, Z_ADDBAD);
	printf("Addbad complete\n");
	return;
}

/*
 * zd_repbad
 *	do set up for adding bad blocks and then call
 *	the routine which actually does the work.
 */
zd_repbad()
{
	int num;

	if (debug) printf("zd_repbad: begin\n"); 
	
	read_mfglist();
	if ((num = get_addlist()) == 0) {
		fprintf(stderr, "No valid entries found for repbad\n");
		exit(1);
	}
	printf("Beginning repbad ... \n");
	fflush(stdout);
	doaddbad(num, Z_ADDBAD);
	printf("Repbad complete\n");
	return;
}

/*
 * zd_verify
 *	Do setup and call the verify routine
 *	Suppress output if doing manditory 
 *	header pass.
 */
zd_verify()
{
	if (debug) printf("zd_verify: begin\n"); 

	read_mfglist();
	read_badlist();

	if (!(args & B_NOVERIFY)) printf("Beginning verify ... \n");
	signal(SIGINT, sigint);
	signal(SIGHUP, sigint);
	doverify();
	if (!(args & B_NOVERIFY)) printf("Verify complete\n");

	write_badlist();
	write_mfglist();

	if (debug) printf("zd_verify: done\n"); 

	return;
}

/*
 * zd_writediag
 *	Call the routine to write the diagnostics data
 */
zd_writediag()
{
	if (debug) printf("zd_writediag: begin\n"); 

	printf("Writing diagnostic data ...\n");
	write_dgndata();
	printf("Writing diagnostic data complete.\n");

	return;
}

/*
 * zd_display
 *	display zd defect lists
 */
zd_display()
{
	if (debug) printf("zd_display: begin\n"); 

	if (unformatted)
		read_umfglist();
	else {
		read_mfglist();
		print_mfglist(stdout);
		if (verbose) {
			create_badlist(newbbl);
			qsort(newbbl->bz_bad, newbbl->bz_nelem,
					sizeof(struct bz_bad), bblcomp);
			if (debug)
				print_badlist(stdout);
		}
		read_badlist();
		print_badlist(stdout);
	}
	return;
}


/*
 * Print the name of the disk.
 */
zd_info()
{
	printf("%s\n", zdinfo->zi_name);
}


/*
 * Display sector headers.
 */
zd_show()
{
	int	cyl;
	int	head;
	int	sect;

	sect = -1;
	(void) sscanf(show_arg,"%d %d %d", &cyl, &head, &sect);
	if (sect == -1)
		sect = totspt;
	printf("Read headers from (%d,%d,0) to (%d,%d,%d)\n",
		cyl, head, cyl, head, sect);
	print_hdr( cyl, head, sect);
}
