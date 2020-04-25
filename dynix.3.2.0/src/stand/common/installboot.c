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

#ident "$Header: installboot.c 2.7 90/12/18 $"

/*
 * installboot: install the appropriate bootstrap in the boot partition
 * of a disk.
 */
#define VTOC 1
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef BSD
#include <sys/ioctl.h>
#else
#include <sys/devsw.h>
#endif /* BSD */
#include <sys/vtoc.h>
#ifdef BSD
#include <sys/file.h>
#include <sys/fs.h>
#else
#include <sys/fcntl.h>
#include <sys/ufsfilsys.h>
#endif /* BSD */

#include <stdio.h>

#ifdef lint
struct streamtab {
	int dummy;
};
#endif	/* lint */

#define	BOOTDIR	"/stand"	/* directory for boot blocks */
#define SSMDIR "/usr/ssw/fw/ssm"/* directory for SSM level B firmware */
#define SSMLVLB "lvlB"		/* name of level B firmware */
#ifdef BSD
#define DIAGDIR "/dev" 		/* contains diagnostic interface for vtoc */
#define RAWDIR  "/dev"		/* directory for raw disk interfaces */
int	vtoc = 1;		/* vtoc device ? */
#else
#define DIAGDIR "/dev/diag/rdsk" /* contains diagnostic interface for vtoc */
#define RAWDIR  "/dev/rdsk"	/* directory for raw disk interfaces */
#endif
#define CHUNKSIZE  8 * 1024	/* size of IO buffer for SSM lvl2 fw */

#ifdef BSD
#define strrchr	rindex
#endif
extern	char	*strrchr();
extern	char	*valloc();
#ifndef BSD
extern	char	*memset();
#endif
extern	void	perror();

char	*get_part();

char diagdev[MAXPATHLEN];
char dev[MAXPATHLEN];
char type[5];

main(argc, argv)
	int argc;
	char *argv[];
{
	char *cp, *bp, *special;
	struct stat sb;

	argc--, argv++;
	if (argc != 1) {
		(void) fprintf(stderr, "usage: installboot special-device\n");
		(void) exit(1);
	}
	special = argv[0];

	cp = strrchr(special, '/');
	if (cp == NULL) {
#ifdef BSD
		if (*special != 'r') {
			dev[0] = 'r';
			strcpy(&dev[1], special);
			special = dev;
		}
#endif
		cp = special;
	} else 
		cp++;
	bp = cp;
	if (stat(special, &sb) >= 0 && (sb.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: Must specify a character-special device", special);
#ifdef BSD
	if (*cp == 'r')
		cp++;
#endif
	type[0] = *cp++;
	type[1] = *cp;
	type[2] = '\0';

#ifdef BSD
	/*
	 * derive the diagnostic device by nulling out the partition number
	 */
	for (cp = bp; *(cp+1) != '\0'; cp++)
		;
	if ( *cp >= 'A' )
		*cp = '\0';
#else
	/*
	 * derive the diagnostic device by nulling out the slice number
	 * (if present) and appending the diagnostic directory path
	 */
	cp = strrchr(bp, 's');
	if (cp != NULL && cp != bp)
		*cp = NULL;
#endif /* BSD */
	
	(void) sprintf(diagdev, "%s/%s", DIAGDIR, bp);

	switch (type[0]) {

	/*
	 * 'zd'
	 */
	case 'z':
		if (strcmp(type, "zd") == 0) {
			install_SCED_boot(get_part(diagdev, V_BOOT));
		} else {
			fatal("%s: Unknown controller type %s", special, type);
		}
		break;

	/*
	 * 'sd'
	 */
	case 's':
		if (strcmp(type, "sd") == 0) {
			install_SCED_boot(get_part(diagdev, V_BOOT));
		} else {
			fatal("%s: Unknown controller type %s", special, type);
		}
		break;

	/*
	 * 'xp'
	 */
	case 'x':
		if (strcmp(type, "xp") == 0) {
			install_SCED_boot(get_part(diagdev, V_BOOT));
		} else {
			fatal("%s: Unknown controller type %s", special, type);
		}
		break;

	/*
	 * 'wd'
	 */
	case 'w':
		if (strcmp(type, "wd") == 0) {
			install_SSM_boot(get_part(diagdev, V_FW));
		} else {
			fatal("%s: Unknown controller type %s", special, type);
		}
		break;

	default:
		fatal("%s: Unknown controller type %s", special, type);
	}

	(void) exit(0);
	/*NOTREACHED*/
}

char *bootimage;

/*
 * install_SCED_boot
 *
 *	Install the 8K bootstrap in the first 16 sectors of a disk.
 *
 * It is assumed that the device passed to this routine will be
 * mapped to the first 8K of usable disk space.  The 8K bootstrap
 * (which is needed on SCED-based systems) will be installed from
 * /stand/boot<type> into this partition.
 */
install_SCED_boot(dev)
	char *dev;
{
	int fd;
	char standalonecode[MAXPATHLEN];

	if (bootimage == (char *)NULL) {
		bootimage = valloc(BBSIZE);
		if (bootimage == (char *)NULL) {
			(void) fprintf(stderr,
				"installboot: Cannot valloc bootimage");
			(void) exit(1);
		}
	}
#ifdef BSD
	(void) bzero(bootimage, BBSIZE);
#else
	(void) memset(bootimage, '\0', BBSIZE);
#endif
	(void) sprintf(standalonecode, "%s/boot%s", BOOTDIR, type);
	(void) printf("installboot: installing %s into %s\n",
		standalonecode, dev);
	fd = open(standalonecode, 0);
	if (fd < 0) {
		(void) fprintf(stderr, "installboot: "); perror(standalonecode);
		(void) exit(1);
	}
	if (read(fd, bootimage, BBSIZE) < 0) {
		(void) fprintf(stderr, "installboot: "); perror(standalonecode);
		(void) exit(2);
	}
	(void) close(fd);
	fd = open(dev, 1);
	if (fd < 0) {
		(void) fprintf(stderr, "installboot: "); perror(dev);
		(void) exit(1);
	}
	if (write(fd, bootimage, BBSIZE) != BBSIZE) {
		(void) fprintf(stderr, "installboot: "); perror(dev);
		(void) exit(2);
	}
	(void) close(fd);
}

/*
 * install_SSM_boot
 */
/* ARGSUSED */
install_SSM_boot(dev)
	char *dev;
{
	int fd, fd1, nbytes;
	char firmware[MAXPATHLEN];

	if (bootimage == (char *)NULL) {
		bootimage = valloc(CHUNKSIZE);
		if (bootimage == (char *)NULL) {
			(void) fprintf(stderr,
				"installboot: Cannot valloc bootimage");
			(void) exit(1);
		}
	}
	(void) sprintf(firmware, "%s/%s", SSMDIR, SSMLVLB);
	(void) printf("installboot: installing %s into %s\n", firmware, dev);
	fd = open(firmware, 0);
	if (fd < 0) {
		(void) fprintf(stderr, "installboot: "); perror(firmware);
		(void) exit(1);
	}
	fd1 = open(dev, 1);
	if (fd1 < 0) {
		(void) fprintf(stderr, "installboot: "); perror(dev);
		(void) exit(1);
	}
	do {
#ifdef BSD
		(void) bzero(bootimage, CHUNKSIZE);
#else
		(void) memset(bootimage, '\0', CHUNKSIZE);
#endif /* BSD */
		if ((nbytes = read(fd, bootimage, CHUNKSIZE)) < 0) {
			(void) fprintf(stderr, "installboot: ");
			perror(firmware);
			(void) exit(2);
		}
		if (write(fd1, bootimage, CHUNKSIZE) != CHUNKSIZE) {
			(void) fprintf(stderr, "installboot: "); perror(dev);
			(void) exit(2);
		}
	} while (nbytes != 0);
	(void) close(fd);
	(void) close(fd1);
}

char partbuf[MAXPATHLEN];

/*
 * get_part
 *
 *	return the name of the boot partition
 *
 * The name passed into get_part() is a device which can be opened
 * and the VTOC read from.  The VTOC is searched for the first partition
 * of type "type", and the name which matches this partition is returned.
 */
char *
get_part(dev, v_type)
	char *dev;
	int	v_type;
{
	int fd, i;
	char *cp;
	struct vtoc *v;

	v = (struct vtoc *)valloc(V_SIZE);
	if (v == (struct vtoc *)NULL)
		fatal("Not enough memory");
	fd = open(dev, O_RDONLY);
	if (fd < 0)
		fatal("%s: unable to open device to read VTOC", dev);
	if (ioctl(fd, V_READ, v) < 0) {
#ifdef BSD
		(void) close(fd);
		(void) sprintf(partbuf, "%sc", dev);
		return(partbuf);
#else
		fatal("%s: unable to read VTOC from device", dev);
#endif
	}
	(void) close(fd);
	for (i = 0; i < v->v_nparts; i++) {
		if (v->v_part[i].p_type == v_type)
			break;
	}
	if (i == v->v_nparts) {
		fatal("%s: unable to find %s partition", dev,
		v_type == V_BOOT ? "bootstrap" : "firmware");
	}
	cp = strrchr(dev, '/');
	if (cp == NULL)
		cp = dev;
	else
		cp++;
#ifdef BSD
	(void) sprintf(partbuf, "%s/%s%c", RAWDIR, cp, i+'a');
	strcat(type, "_v" );
#else
	(void) sprintf(partbuf, "%s/%ss%d", RAWDIR, cp, i);
#endif /* BSD */
	return(partbuf);
}
	

/*VARARGS*/
fatal(fmt, arg1, arg2)
	char *fmt;
{

	(void) fprintf(stderr, "installboot: ");
	(void) fprintf(stderr, fmt, arg1, arg2);
	(void) putc('\n', stderr);
	(void) exit(10);
}
