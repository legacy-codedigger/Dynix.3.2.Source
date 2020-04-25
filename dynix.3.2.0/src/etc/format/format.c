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
 *ident	"$Header: format.c 1.14 1991/12/17 22:39:35 $
 * format
 *	top level (main) of the combined scsi/zd online
 *	formatter.  This does some parsing of command
 *	line arguments and some checking based on tables,
 *	then branches to the appropriate subsytem.
 */

/* $Log: format.c,v $
 *
 *
 *
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef  BSD
#include <sys/sysmacros.h>
#include <sys/devsw.h>
#include <sys/vtoc.h>
#endif /* BSD */
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include "format.h"

extern caddr_t malloc();
int	trashes = 0;
#define LINELENGTH	80
char openname[LINELENGTH];

main(argc, argv)
	int argc;
	char **argv;
{
	extern char	*optarg;
	extern int	optind;
	int c;
	struct stat statbuf;
	struct usage_check *use;
	char str[LINELENGTH+1], *p;
	int i, len;

	/*
	 * No bufferring since output is rare but important.
	 */
	setbuf(stderr, NULL);
	if (argc == 1)
		goto usage;
	while ((c = getopt(argc, argv, "FifrumMla:A:qyonwcvYDt:b:s:e:p:d:h:S:")) != EOF) {
		switch (c) {

		case 'f':	/* format using defect file */
			if (function)
				goto usage;
			function = FORMAT;
			break;
		case 'F':	/* Virgin disk Format */
			if (function)
				goto usage;
			function = VFORMAT;
			break;
		case 'r':	/* reformat using current disk bbl */
			if (function)
				goto usage;
			function = REFORMAT;
			break;
		case 'm':	/* reformat using current mdl */
			if (function)
				goto usage;
			function = REFORMAT_MFG;
			break;
		case 'M':	/* replace mdl from defect file - test only */
			if (function)
				goto usage;
			function = ADD_MFG;
			break;
		case 'l':	/* list disks bbl and mdl */
			if (function)
				goto usage;
			function = DISPLAY;
			break;
		case 'i':	/* print info on disk */
			if (function)
				goto usage;
			function = INFO;
			break;
		case 'u':	/* print usage */
			if (function)
				goto usage;
			function = USAGE;
			break;
		case 'S':	/* display the headers on a cylinder */
			if (function)
				goto usage;
			function = SHOW;
			show_arg = optarg;
			break;
		case 'a':	/* add a bad block */
			if (function)
				goto usage;
			function = ADDBAD;
			if ((a_file = fopen(optarg, "r")) != NULL) 
				break;

			a_arg = malloc(ADDLENGTH);
			if (strlen(optarg) < ADDLENGTH)
				strcpy(a_arg, optarg);
			else {
				printf("args too long -- use file!\n");
				exit(1);
			}
			break;
		case 'A':	/* replace the bbl - test only */
			if (function)
				goto usage;
			function = REPBAD;
			if ((a_file = fopen(optarg, "r")) != NULL) 
				break;

			a_arg = malloc(ADDLENGTH);
			if (strlen(optarg) < ADDLENGTH)
				strcpy(a_arg, optarg);
			else {
				printf("args too long -- use file!\n");
				exit(1);
			}
			break;
		case 'q':	/* write diagnostic cylinders */
			if (function)
				goto usage;
			function = WRITEDIAG;
			break;
		case 'y':	/* verify the disk */
			if (function)
				goto usage;
			function = VERIFY;
			break;
		case 'o':	/* override potential dangerous error case */
			args |= B_OVERWRITE;
			break;
		case 'c':	/* check the date (rather than ECC) */
			args |= B_CHECKDATA;
			break;
		case 'v':	/* verbose output */
			verbose++;
			break;
		case 'n':	/* do not run verify pass */
			args |= B_NOVERIFY;
			break;
		case 'w':	/* do not write diagnostics cylinders */
			args |= B_NOWRITEDIAG;
			break;
		case 'D':	/* Debug flag */
			debug++; 
			setbuf(stdout, NULL);
			setbuf(stderr, NULL);
			break;
		case 't':	/* provide disk type */
			args |= B_TYPE;
			t_arg = optarg;
			break;
		case 'b':	/* provide bbl */
			args |= B_BADFILE;
			b_arg = optarg;
			break;
		case 's':	/* give cylinder start */
			args |= B_START;
			s_arg = atoi(optarg);
			break;
		case 'e':	/* guve cylinder end */
			args |= B_END;
			e_arg = atoi(optarg);
			break;
		case 'p':	/* give number of full passes */
			args |= B_FULLPASS;	
			p_arg = atoi(optarg);
			break;
		case 'd':	/* give nuber of defect passes */
			args |= B_DEFECTPASS;
			d_arg = atoi(optarg);
			break;
		case 'h':	/* give nuber of header passes */
			args |= B_HDRPASS;
			h_arg = atoi(optarg);
			break;
		case 'Y':	/* MFG only */
			force++;
			break;
		case '?':
			goto usage;
			
		default:
			break;
		}
	}
	diskname = argv[optind];
#ifdef BSD
	if (*diskname == '/')
		diskname += 5;
	if (*diskname == 'r')
		diskname++;
#endif
	p = diskname;
	if (debug) printf("diskname = %s\n", diskname);
	disk = -1;
	for (i = 0, len = 0; i < nformat_types; i++) {
		if (!strncmp(p, types[i].disk_name, 
			     strlen(types[i].disk_name)))  {
			if (strlen(types[i].disk_name) > len) {
				len = strlen(types[i].disk_name);
				disk = i;
			}
		}
	}
	if (disk < 0) {
		fprintf(stderr, "Error: %s disk type not supported\n", 
			diskname);
		fprintf(stderr, "The following disk types are supported\n");
		for (i = 0, len = 0, p = str; i < nformat_types; i++) {
			if ((len+strlen(types[i].disk_name)+1) > LINELENGTH) {
				fprintf(stderr, "%s\n", str);
				p = str;
			}
			sprintf(p, "%s ", types[i].disk_name);
			len = strlen(str);
			p += strlen(p);
		}
		if (p != str)
			fprintf(stderr, "%s\n", str);
		exit(1);
	}

	/*
	 * Do usage checking in relation to specific disk
	 * subsystem.
	 */
	for (i = 0, use = types[disk].use; i < *types[disk].numfuncs; 
	     i++, use++) {
		if (use->function == function) {
			usep = use;
			if (args & ~(use->opts_supported)
			    || (args & use->opts_required) 
				!= use->opts_required) {
				fprintf(stderr, "usage: %s\n",
					use->usage_string);
				exit(1);
			}
			break;
		}
	}
	if (i == *types[disk].numfuncs) {
		fprintf(stderr, "function not supported for specified ");
		fprintf(stderr, "subsystem\n");
		(*types[disk].usage)();
		general_usage();
		exit(1);
	}

	openname[0] = '\0';
	strcat(openname, DEVPATH);
	strcat(openname, diskname);
	if (stat(openname, &statbuf) < 0) {
		fprintf(stderr, "unable to stat %s: ", openname);
		perror("");
		exit(1);
	}
#ifndef BSD
	if (VPART(statbuf.st_rdev) != V_RAW_MINOR) {
		fprintf(stderr, "must open raw disk\n");
		exit(1);
	}
#endif
	/*
	 * If we are going to write to the disk then
	 * Open device for EXCLUSIVE access and make sure
	 * it's actually a device driver rather than a
	 * pseudo driver.
	 */
	if ((fd = open(openname, usep->open_flags)) < 0
	    || ioctl(fd, RIODRIVER, (char *)NULL) < 0) {
		if (fd < 0)
			fprintf(stderr, 
				"unable to open %s: ", openname);
		else
			fprintf(stderr, "RIODRIVER ioctl failed ");
		perror("");
		exit(2);
	}

	/*
	 * Do setup for specific device, and then device_main
	 * to perform the specified function.
	 */
	if ((*types[disk].setup)())
		exit(1);
#ifdef BSD
	if (!force)
		will_trash();
	are_you_sure();
#endif
	(*types[disk].dev_main)();
	exit(0);
usage:
	general_usage();
	exit(1);
}

/*
 * general_usage
 *	Give generic usage information
 */
general_usage()
{
	fprintf(stderr, "General usage of format command:\n");
	fprintf(stderr, 
	 "format [-f|r|m|y|l|u|a badlist] [-wovnc] [-b file] [-s n] [-e n] \\\n");
	fprintf(stderr,
	 "                 [-p n] [-d n] [-h n] [-t type] diskname\n");
	fprintf(stderr, 
	 "\n(Note that all options do not apply to all disk types.\n");
	fprintf(stderr,
	 "Do 'format -u diskname' to get more specific information\n");
}

will_trash()
{
	switch (function) {
	case VFORMAT:
	case FORMAT:
	case REFORMAT:
	case REFORMAT_MFG:
	case ADDBAD:
	case REPBAD:
	case VERIFY:
	case WRITEDIAG:
	case ADD_MFG:
		trashes = 1;
	case DISPLAY:
	case INFO:
	case SHOW:
	case USAGE:
		break;
	}
}

are_you_sure()
{
	char buf[80];
	FILE *ttyo;
	FILE *ttyi;

	fflush(stdout);
	fflush(stderr);
	if ((ttyo = fopen("/dev/tty", "w")) == NULL) {
		fprintf(stderr, "format must be run interactively\n");
		exit(1);
	}
	if ((ttyi = fopen("/dev/tty", "r")) == NULL) {
		fprintf(stderr, "format must be run interactively\n");
		exit(1);
	}
	while (trashes) {
		fprintf(ttyo, "Last chance, about to change disk. Are you sure? ");
		fflush(ttyo);
		fgets(buf, 80, ttyi);
		if (*buf == 'y')
			trashes = 0;
		if (*buf == 'n')
			exit(0);
		sleep(4);
	}
	fclose(ttyi);
	fclose(ttyo);
}
