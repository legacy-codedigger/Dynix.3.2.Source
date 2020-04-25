/* $Copyright:	$
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

#ifndef lint
static char rcsid[] = "$Header: mkswapconf.c 2.3 91/01/03 $";
#endif

/*
 * Build a swap configuration file and conf file.
 *
 * This module is BALANCE specific.
 */
#include "config.h"

#include <stdio.h>
#include <ctype.h>

#define	MAX(a,b) ((a)>(b)?(a):(b))
#define DO_EXTERNS	0
#define	DO_TABLE	1
char	*paren(), *sprintf();

static	int max_bdevsw = -1, max_cdevsw = -1;
static	int devtablenotread = 1;
static	struct devdescription {
	char	*dev_name;
	char	*dev_config;
	char	*dev_type;
	int	dev_major;
	char	*dev_funct;
	char	*dev_flags;
	int	dev_options;
	struct	devdescription *dev_next;
} *devtable;

/*
 * Possible dev_options.
 */

char	mono_option[] =	"MONO-PROCESSOR";
#define	DOPT_MONOP	1		/* mono-processor driver */

/*
 * For mono-processor drivers:
 *	bdevsw[] maps open, close, strat, minphys; not psize.
 *	cdevsw[] maps open, close, read, write, ioctl, select; not stop.
 *
 * A mono-processor driver is spepcified via a "mono-processor"
 * "option" in the devices file.
 */

int	bmp_cnt = 0;				/* # blk-dev mono-P's */
char	bmp_funct[] = "OCSMZZ";
char	bmp_name[] = "bmp";

int	cmp_cnt = 0;				/* # chr-dev mono-P's */
char	cmp_funct[] = "OCRWINLN";
char	cmp_name[] = "cmp";

swapconf()
{
	register struct file_list *fl;
	struct file_list *do_swap();

	fl = conf_list;
	while (fl) {
		if (fl->f_type != SYSTEMSPEC) {
			fl = fl->f_next;
			continue;
		}
		fl = do_swap(fl);
	}
}

struct file_list *
do_swap(fl)
	register struct file_list *fl;
{
	FILE *fp;
	char  swapname[80], *cp;
	register struct file_list *swap;
	dev_t dev;

	if (eq(fl->f_fn, "generic")) {
		fl = fl->f_next;
		return (fl->f_next);
	}
	(void) sprintf(swapname, "swap%s.c", fl->f_fn);
	fp = fopen(path(swapname), "w");
	if (fp == 0) {
		perror(path(swapname));
		exit(1);
	}
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/conf.h\"\n");
	fprintf(fp, "\n");
	/*
	 * If there aren't any swap devices
	 * specified, just return, the error
	 * has already been noted.
	 */
	swap = fl->f_next;
	if (swap == 0 || swap->f_type != SWAPSPEC) {
		(void) unlink(path(swapname));
		fclose(fp);
		return (swap);
	}
	fprintf(fp, "dev_t\trootdev = makedev(%d, %d);\t\t/* %s */\n",
		major(fl->f_rootdev), minor(fl->f_rootdev), 
		devtoname(fl->f_rootdev));
	fprintf(fp, "dev_t\targdev  = makedev(%d, %d);\t\t/* %s */\n",
		major(fl->f_argdev), minor(fl->f_argdev), 
		devtoname(fl->f_argdev));
	fprintf(fp, "\n");
	fprintf(fp, "struct\tswdevt swdevt[] = {\n");
	do {
		dev = swap->f_swapdev;
		fprintf(fp, "\t{ makedev(%d, %d),\t0,\t%d },\t/* %s */\n",
		    major(dev), minor(dev), swap->f_swapsize, swap->f_fn);
		swap = swap->f_next;
	} while (swap && swap->f_type == SWAPSPEC);
	fprintf(fp, "\t{ 0, 0, 0 }\n");
	fprintf(fp, "};\n\n");
	fprintf(fp, "/* stub */\n");
	fprintf(fp, "setconf(){}\n\n");
	fclose(fp);
	return (swap);
}

/*
 * Given a device name specification figure out:
 *	major device number
 *	partition
 *	device name
 *	unit number
 * This is a hack, but the system still thinks in
 * terms of major/minor instead of string names.
 */
dev_t
nametodev(name, defunit, defpartition)
	char *name;
	int defunit;
	char defpartition;
{
	char *cp, partition;
	int unit;
	register struct devdescription *dp;

	cp = name;
	if (cp == 0) {
		fprintf(stderr, "config: internal error, nametodev\n");
		exit(1);
	}
	while (*cp && !isdigit(*cp))
		cp++;
	unit = *cp ? atoi(cp) : defunit;
	/*
	 * Currently, disk minor numbers allow for upto 8*16 drives.
	 */
	if (unit < 0 || unit > 512) {
		fprintf(stderr,
"config: %s: invalid device specification, unit out of range\n", name);
		unit = defunit;			/* carry on more checking */
	}
	if (*cp) {
		*cp++ = '\0';
		while (*cp && isdigit(*cp))
			cp++;
	}
	partition = *cp ? *cp : defpartition;
	if (partition < 'a' || partition > 'h') {
		fprintf(stderr,
"config: %c: invalid device specification, bad partition\n", *cp);
		partition = defpartition;	/* carry on */
	}
	if (devtablenotread)
		initdevtable();
	for (dp = devtable; dp->dev_next; dp = dp->dev_next) {
		if (eq(name, dp->dev_name) && eq("b", dp->dev_type))
			break;
	}
	if (dp == 0) {
		fprintf(stderr, "config: %s: unknown device\n", name);
		return (NODEV);
	}
	return (makedev(dp->dev_major, (unit << 3) + (partition - 'a')));
}

char *
devtoname(dev)
	dev_t dev;
{
	char buf[80]; 
	register struct devdescription *dp;

	if (devtablenotread)
		initdevtable();
	for (dp = devtable; dp; dp = dp->dev_next) {
		if (major(dev) == dp->dev_major)
			break;
	}
	/* Not found in table, so pick first block device */
	if (dp == 0)
		dp = devtable;
	sprintf(buf, "%s%d%c", dp->dev_name,
		minor(dev) >> 3, (minor(dev) & 07) + 'a');
	return (ns(buf));
}

initdevtable()
{
	register i;
	char *p;
	char buf[BUFSIZ];
	char name[11], type[2], funct[17], flags[101], options[81];
	int maj;
	register struct devdescription **dp = &devtable;
	FILE *fp;
	register line;

	sprintf(buf, "../conf/devices.%s", machinename);
	fp = fopen(buf, "r");
	if (fp == NULL) {
		fprintf(stderr, "config: can't open %s\n", buf);
		exit(1);
	}
	line = 0;
	while (fgets(buf, sizeof (buf), fp) != NULL) {
		++line;
		options[0] = flags[0] = NULL;
		i = sscanf(buf, "%10s%1s%d%16s%100s%80s", name, type, &maj, funct, flags, options);
		if (i < 4 || name[0] == '#')
			continue;
		if (type[0] != 'b' && type[0] != 'c') {
			fprintf(stderr, 
				"config: bad device type, not 'b' or 'c', line %d\n", line);
			exit(1);
		}
		if (maj < 0 || maj > 100) {
			fprintf(stderr, 
				"config: %d is probably a bad major number, line %d\n", 
				maj, line);
		}
		*dp = (struct devdescription *)malloc(sizeof (**dp));
		p = index(name, '/');
		if (p == NULL)  {
			(*dp)->dev_config = ns(name);
		} else	{
			(*dp)->dev_config = ns(p+1);
			*p = NULL;
		}
		(*dp)->dev_name = ns(name);
		(*dp)->dev_type = ns(type);
		(*dp)->dev_major = maj;
		(*dp)->dev_funct = ns(funct);
		/*
		 * Collect flags and/or options.  Can only have flags if
		 * 'F' is in the functions list.  Can always have options.
		 * No checking on legality of flags or options yet.
		 */
		if (index(funct, 'F') != NULL) {
			(*dp)->dev_flags = ns(flags);
			(*dp)->dev_options = device_options(options, line);
		} else {
			(*dp)->dev_flags = "";
			(*dp)->dev_options = device_options(flags, line);
		}
		(*dp)->dev_next = (struct devdescription *)NULL;
		dp = &(*dp)->dev_next;
		if (eq(type, "b")) max_bdevsw = MAX(max_bdevsw, maj);
		if (eq(type, "c")) max_cdevsw = MAX(max_cdevsw, maj);
	}
	*dp = (struct devdescription *)NULL;
	fclose(fp);
	devtablenotread = 0;
}

conf()
{
	register FILE *fp;
	register struct devdescription *dp;
	register index, found;
	register char *f;
	int	map_idx;

	/*
	 * Generate bdevsw and cdevsw tables
	 */

	fp = fopen(path("conf.c"), "w");
	if (fp == 0) {
		perror(path("conf.c"));
		exit(1);
	}
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/systm.h\"\n");
	fprintf(fp, "#include \"../h/mutex.h\"\n");
	fprintf(fp, "#include \"../h/buf.h\"\n");
	fprintf(fp, "#include \"../h/ioctl.h\"\n");
	fprintf(fp, "#include \"../h/tty.h\"\n");
	fprintf(fp, "#include \"../h/conf.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "extern	int nulldev();\n");
	fprintf(fp, "extern	int nodev();\n");
	fprintf(fp, "extern	int seltrue();\n");
	fprintf(fp, "\n");

	if (devtablenotread)
		initdevtable();

	/*
	 * Do bdevsw table: externs and table.
	 * Note: the "index" loop around externs is not actually necessary;
	 * however it does preserve the order of the entries.
	 */

	fprintf(fp, "/*\n");
	fprintf(fp, " * Block Device Switch Table.\n");
	fprintf(fp, " */\n\n");

	for (index=0; index <= max_bdevsw; index++) {
		for (dp = devtable; dp; dp = dp->dev_next) {
			if (index == dp->dev_major
			&&  eq(dp->dev_type, "b")
			&&  configured(dp->dev_config)) {
				do_btable(dp->dev_funct, dp->dev_name,
						index, DO_EXTERNS, fp, "", 0);
				if ((dp->dev_options & DOPT_MONOP)
				&&  (++bmp_cnt == 1))
					do_btable(bmp_funct, bmp_name, -1, DO_EXTERNS, fp, "", 0);
				break;
			}
		}
	}
	fprintf(fp, "\n");
	fprintf(fp, "struct\tbdevsw\tbdevsw[] = {\n");
	for (index=0; index <= max_bdevsw; index++) {
		for (found = 0, dp = devtable; dp; dp = dp->dev_next) {
			if (index == dp->dev_major
			&&  eq(dp->dev_type, "b")
			&&  configured(dp->dev_config)) {
				do_btable(dp->dev_funct, dp->dev_name,
					index, DO_TABLE, fp,
					dp->dev_flags, dp->dev_options);
				found++;
				break;
			}
		}
		if (!found)
			do_btable("NNNUUZ", "xx", index, DO_TABLE, fp, "", 0);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "int\tnblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);\n");
	fprintf(fp, "\n");

	/*
	 * Do bmpmap[] and bmpdevsw[] if there are block mono-P drivers.
	 */

	if (bmp_cnt) {
		/*
		 * Generate bmpmap[] mapping table.
		 */
		fprintf(fp, "/*\n");
		fprintf(fp, " * Block Mono-Processor Driver Maps.\n");
		fprintf(fp, " */\n\n");
		map_idx = 0;
		fprintf(fp, "int\tbmpmap[] = {\n");
		for (index=0; index <= max_bdevsw; index++) {
			for (found = 0, dp = devtable; dp; dp = dp->dev_next) {
				if (index == dp->dev_major
				&&  eq(dp->dev_type, "b")
				&&  configured(dp->dev_config)) {
					++found;
					break;
				}
			}
			if (!found || (dp->dev_options & DOPT_MONOP) == 0)
				fprintf(fp, "\t\t-1,\t\t/* %2d */\n", index);
			else
				fprintf(fp, "\t\t%d,\t\t/* %2d */\n", map_idx++, index);
		}
		fprintf(fp, "};\n\n");
		/*
		 * Generate bmpdevsw[] procedure table.
		 */
		map_idx = 0;
		fprintf(fp, "struct\tbdevsw\tbmpdevsw[] = {\n");
		for (index=0; index <= max_bdevsw; index++) {
			for (found = 0, dp = devtable; dp; dp = dp->dev_next) {
				if (index == dp->dev_major
				&&  eq(dp->dev_type, "b")
				&&  configured(dp->dev_config)
				&&  (dp->dev_options & DOPT_MONOP)) {
					do_btable(dp->dev_funct, dp->dev_name,
						map_idx++, DO_TABLE, fp,
						dp->dev_flags,
						dp->dev_options & ~DOPT_MONOP);
					break;
				}
			}
		}
		fprintf(fp, "};\n\n");
	} 

	/*
	 * Do cdevsw table
	 */

	fprintf(fp, "/*\n");
	fprintf(fp, " * Character Device Switch Table.\n");
	fprintf(fp, " */\n\n");

	for (index=0; index <= max_cdevsw; index++) {
		for (dp = devtable; dp; dp = dp->dev_next) {
			if (index == dp->dev_major
			&&  eq(dp->dev_type, "c")
			&&  configured(dp->dev_config)) {
				do_ctable(dp->dev_funct, dp->dev_name,
						index, DO_EXTERNS, fp, "", 0);
				if ((dp->dev_options & DOPT_MONOP)
				&&  (++cmp_cnt == 1))
					do_ctable(cmp_funct, cmp_name, -1, DO_EXTERNS, fp, "", 0);
				break;
			}
		}
	}
	fprintf(fp, "\n");
	fprintf(fp, "struct\tcdevsw\tcdevsw[] = {\n");
	for (index=0; index <= max_cdevsw; index++) {
		for (found = 0, dp = devtable; dp; dp = dp->dev_next) {
			if (index == dp->dev_major
			&&  eq(dp->dev_type, "c")
			&&  configured(dp->dev_config)) {
				do_ctable(dp->dev_funct, dp->dev_name,
					index, DO_TABLE, fp,
					dp->dev_flags, dp->dev_options);
				found++;
				break;
			}
		}
		if (!found)
			do_ctable("NNNNNNNN", "xx", index, DO_TABLE, fp, "", 0);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "int\tnchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);\n");
	fprintf(fp, "\n");

	/*
	 * Do cmpmap[] and cmpdevsw[] if there are block mono-P drivers.
	 */

	if (cmp_cnt) {
		/*
		 * Generate cmpmap[] mapping table.
		 */
		map_idx = 0;
		fprintf(fp, "/*\n");
		fprintf(fp, " * Character Mono-Processor Driver Maps.\n");
		fprintf(fp, " */\n\n");
		fprintf(fp, "int\tcmpmap[] = {\n");
		for (index=0; index <= max_cdevsw; index++) {
			for (found = 0, dp = devtable; dp; dp = dp->dev_next) {
				if (index == dp->dev_major
				&&  eq(dp->dev_type, "c")
				&&  configured(dp->dev_config)) {
					++found;
					break;
				}
			}
			if (!found || (dp->dev_options & DOPT_MONOP) == 0)
				fprintf(fp, "\t\t-1,\t\t/* %2d */\n", index);
			else
				fprintf(fp, "\t\t%d,\t\t/* %2d */\n", map_idx++, index);
		}
		fprintf(fp, "};\n\n");
		/*
		 * Generate cmpdevsw[] procedure table.
		 */
		map_idx = 0;
		fprintf(fp, "struct\tcdevsw\tcmpdevsw[] = {\n");
		for (index=0; index <= max_cdevsw; index++) {
			for (found = 0, dp = devtable; dp; dp = dp->dev_next) {
				if (index == dp->dev_major
				&&  eq(dp->dev_type, "c")
				&&  configured(dp->dev_config)
				&&  (dp->dev_options & DOPT_MONOP)) {
					do_ctable(dp->dev_funct, dp->dev_name,
						map_idx++, DO_TABLE, fp,
						dp->dev_flags,
						dp->dev_options & ~DOPT_MONOP);
					break;
				}
			}
		}
		fprintf(fp, "};\n\n");
	} 

	/*
	 * Generate empty arrays to satisify the link (if needed)
	 * Note: use "[1]" to avoid ld'r bug (zero-size common
	 * can be interpreted wrong).
	 */
	if (bmp_cnt == 0 && (srcconfig || cmp_cnt)) {
		fprintf(fp, "int\tbmpmap[1];\n");
		fprintf(fp, "struct\tbdevsw\tbmpdevsw[1];\n\n");
	}
	if (cmp_cnt == 0 && (srcconfig || bmp_cnt)) {
		fprintf(fp, "int\tcmpmap[1];\n");
		fprintf(fp, "struct\tcdevsw\tcmpdevsw[1];\n\n");
	}

	/*
	 * Insist on /dev/*mem.
	 */

	for (found=0,index=0; !found && index <= max_cdevsw; index++) {
		for (dp = devtable; dp; dp = dp->dev_next) {
			if (eq(dp->dev_name, "mm") && eq(dp->dev_type, "c")) {
				fprintf(fp, "int\tmem_no = %d;\t", dp->dev_major);
				fprintf(fp, "/* Major device number of memory special file (\"mm\") */\n");
				++found;
			}
		}
	}
	if (!found) {
		fprintf(stderr, "config: No \"mm\" char device for memory driver\n");
		exit(1);
	}

	/*
	 * Insist on swap "device" and /dev/drum.
	 */

	for (found= -1,index=0; found == -1 && index <= max_bdevsw; index++) {
		for (dp = devtable; dp; dp = dp->dev_next) {
			if (eq(dp->dev_name, "sw") && eq(dp->dev_type, "b")) {
				found = dp->dev_major;
			}
		}
	}
	if (found == -1) {
		fprintf(stderr, "config: No \"sw\" block device for swapper\n");
		exit(1);
	}
	fprintf(fp, "\n");
	fprintf(fp, "/*\n");
 	fprintf(fp, " * Swapdev is a fake device implemented\n");
 	fprintf(fp, " * in sw.c used only internally to get to swstrategy.\n");
 	fprintf(fp, " * It cannot be provided to the users, because the\n");
 	fprintf(fp, " * swstrategy routine munches the b_dev and b_blkno entries\n");
 	fprintf(fp, " * before calling the appropriate driver.  This would horribly\n");
 	fprintf(fp, " * confuse, e.g. the hashing routines. Instead, /dev/drum is\n");
 	fprintf(fp, " * provided as a character (raw) device.\n");
 	fprintf(fp, " */\n");
	fprintf(fp, "dev_t\tswapdev = makedev(%d, %d);\n", found, 0);
	fclose(fp);
}


static	int firsttime;

do_btable(f, name, idx, type, fp, extra, opt)
	register FILE *fp;
	register char *f, *name, *extra;
	int	opt;
{
	char	buf[BUFSIZ];
	char 	*lf = f;
	int	fi;	/* function string index  */

#define	NAME(c)	(((opt & DOPT_MONOP) && (index(bmp_funct, 'c') != NULL)) ? "bmp" : name)

	if (strlen(f) != 6) {
		fprintf(stderr, 
			"config: Bad device entry functions for block device %s\n", name);
		exit(1);
	}
	if (type == DO_EXTERNS) {
		firsttime = 1;
		while (*f) {
			switch(*f++) {
			default:
				fprintf(stderr, 
				  "config: Unknown function %c for block device %s\n", 
				  *--f, name);
				exit(1);
				break;
			case 'O': i(fp); 
				fprintf(fp, "%s%sopen()", paren(), name); 
				break;
			case 'C': i(fp); 
				fprintf(fp, "%s%sclose()", paren(), name); 
				break;
			case 'S': i(fp); 
				fprintf(fp, "%s%sstrat()", paren(), name); 
				break;
			case 'M': i(fp); 
				fprintf(fp, "%s%sminphys()", paren(), name); 
				break;
			case 'P': i(fp); 
				fprintf(fp, "%s%ssize()", paren(), name); 
				break;
			case 'F': case 'Z': case 'N': case 'U':
				break;
			}
		}
		if (firsttime != 1)
			fprintf(fp, ";\n");
	} else if (type == DO_TABLE) {
		fprintf(fp, "{");
		firsttime = 0;
		fi = 0;
		while (*f) {
			fi++;
			switch(*f++) {
			default:
				fprintf(stderr, 
				  "config: Unknown function %c for block device %s\n", 
				  *--f, name);
				exit(1);
			funerr:
				fprintf(stderr, 
				  "config: Out of order function %c for block device %s\n", 
				  *--f, name);
				exit(1);
				break;
			case 'O':
				if( fi != 1 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sopen", NAME(O)));
				break;
			case 'C':
				if( fi != 2 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sclose", NAME(C)));
				break;
			case 'S':
				if( fi != 3 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sstrat", NAME(S)));
				break;
			case 'M':
				if( fi != 4 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sminphys", NAME(M)));
				break;
			case 'P':
				if( fi != 5 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%ssize", NAME(P)));
				break;
			case 'F': 
				if( fi != 6 ) goto funerr;
				fprintf(fp, "%s%11s", paren(),  extra);
				break;
			case 'Z':
				if( (fi != 5) && (fi != 6) ) goto funerr;
				fprintf(fp, "%s%11s", paren(), "0");
				break;
			case 'N': 
				fprintf(fp, "%s%11s", paren(), "nodev");
				break;
			case 'U': 
				fprintf(fp, "%s%11s", paren(), "nulldev");
				break;
			}
		}
		fprintf(fp, ", },%10s\n", sprintf(buf, "/* %2d */", idx));
	}
#undef	NAME
}

do_ctable(f, name, idx, type, fp, extra, opt)
	register FILE *fp;
	register char *f, *name, *extra;
	int	opt;
{
	char	buf[BUFSIZ];
	char 	*lf = f;
	int	fi;	/* function string index  */

#define	NAME(c)	(((opt & DOPT_MONOP) && (index(cmp_funct, 'c') != NULL)) ? "cmp" : name)

	if (strlen(f) != 8) {
		fprintf(stderr, 
			"config: Bad device entry functions for char device %s\n", name);
		exit(1);
	}
	if (type == DO_EXTERNS) {
		firsttime = 1;
		while (*f) {
			switch(*f++) {
			default:
				fprintf(stderr, 
				  "config: Unknown function %c for char device %s\n", 
				  *--f, name);
				exit(1);
				break;
			case 'O': i(fp); 
				fprintf(fp, "%s%sopen()", paren(), name); 
				break;
			case 'C': i(fp); 
				fprintf(fp, "%s%sclose()", paren(), name); 
				break;
			case 'R': i(fp); 
				fprintf(fp, "%s%sread()", paren(), name); 
				break;
			case 'W': i(fp); 
				fprintf(fp, "%s%swrite()", paren(), name); 
				break;
			case 'I': i(fp); 
				fprintf(fp, "%s%sioctl()", paren(), name); 
				break;
			case 'S': i(fp); 
				fprintf(fp, "%s%sstop()", paren(), name); 
				break;
			case 'L': i(fp); 
				fprintf(fp, "%s%sselect()", paren(), name); 
				break;
			case 'M': i(fp);
				fprintf(fp, "%s%smmap()", paren(), name);
				break;
			case 'Z': case 'N': case 'U': case 'T':
				break;
			}
		}
		if (firsttime != 1)
			fprintf(fp, ";\n");
	} else if (type == DO_TABLE) {
		fprintf(fp, "{");
		firsttime = 0;
		fi = 0;
		while (*f) {
			fi++;
			switch(*f++) {
			default:
				fprintf(stderr, 
				  "config: Unknown function %c for char device %s\n", 
				  *--f, name);
				exit(1);
			funerr:
				fprintf(stderr, 
				  "config: Out of order function %c for char device %s\n", 
				  *--f, name);
				exit(1);
				break;
			case 'O':
				if( fi != 1 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sopen", NAME(O)));
				break;
			case 'C':
				if( fi != 2 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sclose", NAME(C)));
				break;
			case 'R':
				if( fi != 3 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sread", NAME(R)));
				break;
			case 'W':
				if( fi != 4 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%swrite", NAME(W)));
				break;
			case 'I':
				if( fi != 5 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sioctl", NAME(I)));
				break;
			case 'S':
				if( fi != 6 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sstop", NAME(S)));
				break;
			case 'L':
				if( fi != 7 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%sselect", NAME(L)));
				break;
			case 'M':
				if( fi != 8 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), 
					sprintf(buf, "%smmap", NAME(L)));
				break;
			case 'N': 
				fprintf(fp, "%s%11s", paren(), "nodev");
				break;
			case 'U': 
				fprintf(fp, "%s%11s", paren(), "nulldev");
				break;
			case 'T': 
				if( fi != 7 ) goto funerr;
				fprintf(fp, "%s%11s", paren(), "seltrue");
				break;
			}
		}
		fprintf(fp, ", },%10s\n", sprintf(buf, "/* %2d */", idx));
	}
#undef	NAME
}

static
i(fp)
	FILE *fp;
{
	if (firsttime == 1) {
		fprintf(fp, "int\t");
		firsttime = 0;
	}
}

char *
paren()
{
	if (firsttime == 0) {
		firsttime = -1;
		return("");
	}
	return(",");
}

int
configured(s)
	register char *s;
{
	register struct device *dp;

	if (eq(s, "*"))
		return(1);

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (eq(dp->d_name, s))
			return(1);
	}
	return(0);
}

/*
 * device_options
 *	Determine options given ascii representation.
 *
 * Nor now, only "mono-processor" is avaialable.
 */

device_options(str, line)
	char	*str;
	int	line;
{
	if (str == NULL || str[0] == '\0')
		return(0);

	if (eq(raise(str), mono_option))
		return(DOPT_MONOP);

	fprintf(stderr, "config: unrecognized option `%s', line %d.\n",
			str, line);

	return(0);
}
