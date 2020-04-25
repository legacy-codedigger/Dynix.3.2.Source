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

#ident	"$Header: mkvtoc.c 1.10 1991/08/28 20:25:28 $
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/vtoc.h>
#include <sys/param.h>
#include <zdc/zdc.h>
#include <diskinfo.h>
#include <sys/ioctl.h>
#include <sys/file.h>

extern char *malloc(), *realloc();

#define	PROTODIR	"/etc/vtoc/"

#ifdef	lint
struct streamtab {
	int dummy
};
#endif	/* lint */

/*
 * ignore_part defines the class of partitions which do not need to be
 * sanity-checked.
 */
#define ignore_part(T)		((T) == V_NOPART || (T) == V_RESERVED)

#define isdigit(c) (((c) >= '0') && ((c) <= '9'))
#define ispart(c) (((c) >= 'a') && ((c) <= ('a'+NUMPARTS)))

/*
 * External functions.
 */
extern int errno;
void	exit();
char	*strcat();
char	*rindex();
char	*strcpy();
char	*strncpy();
int	getopt();
long	lseek();
void	perror();
struct	geomtab *getgeombyname();
char	*valloc();

/*
 * Internal functions.
 */
void	initial();
void	insert();
void	load();
void	pread();
void	pwrite();
void	usage();
void	validate();
void	checksum();

/*
 * Static variables.
 */
static char	*delta;			/* Incremental update */
static int	fflag;			/* force change of reserved partitions*/
static int	debug;			/* diagnostic flag */
static int	gflag;			/* Set disk geometry in VTOC */
static int	rawdisk;		/* No partitioning driver present? */
static int	no_old_vtoc;		/* No old VTOC on the disk? */
static int	cmpt;			/* disk is in compatability mode */
static int	offset;			/* VTOC is at offset + V_VTOCSEC */

static char	vproto[80];		/* path to VTOC prototype file */
static char	controller[10];		/* generic name for controller */
static char	*disk_name;		/* basename of disk path */

main(argc, argv)
int	argc;
char	**argv;
{
	int		fd;
	int		c;
	char		*dfile, *disktype;
	struct stat	statbuf;
	char		*disk;
	char		*oldvtoc;		/* buffer for old VTOC */
	char		*newvtoc;		/* buffer for new VTOC */
	struct vtoc	*ovtoc;
	struct vtoc	*nvtoc;
	extern char	*optarg;
	extern int	optind;

	dfile = NULL;
	disktype = NULL;
	while ((c = getopt(argc, argv, "Dd:fgis:")) != EOF)
		switch (c) {
		case 'd':
			delta = optarg;
			break;
		case 'f':
			++fflag;
			break;
		case 'g':
			++gflag;
			break;
		case 's':
			dfile = optarg;
			break;
		case 'D':
			debug++;
			break;
		default:
			usage();
		}

	if (argc - optind != 2)
		usage();
	disk = argv[optind];

	oldvtoc = valloc(V_SIZE);
	if (oldvtoc == (char *)0) {
		(void) fprintf(stderr, "mkvtoc: Not enough core\n");
		exit(1);
	}

	newvtoc = valloc(V_SIZE);
	if (newvtoc == (char *)0) {
		(void) fprintf(stderr, "mkvtoc: Not enough core\n");
		exit(1);
	}

	ovtoc = (struct vtoc *)oldvtoc;
	nvtoc = (struct vtoc *)newvtoc;


	if ((fd = tryopen(&disk)) < 0) {
		(void) fprintf(stderr, "mkvtoc:  Cannot open device %s\n", disk);
		exit(1);
	}
	if (stat(disk, (struct stat *) &statbuf) == -1) {
		(void) fprintf(stderr, "mkvtoc:  Cannot stat device %s\n", disk);
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		(void) fprintf(stderr, "mkvtoc:  Must specify a raw device.\n");
		exit(1);
	}	

	disktype = argv[optind + 1];

	/*
	 * Read the VTOC from disk, if it exists.
	 */

	pread(fd, V_VTOCSEC, V_SIZE, (char *) oldvtoc);

	if (ovtoc->v_sanity == 0xbadbad ) {
		cmpt = 1;
		no_old_vtoc++;
		if ( debug )
			printf(" This disk had a compatabilty mode\n");
	} else if (ovtoc->v_sanity != VTOC_SANE) {
		no_old_vtoc++;
	}

	if (!gflag && (no_old_vtoc || cmpt)) {
		(void) fprintf(stderr, "%s: Invalid VTOC\n", disk);
		(void) fprintf(stderr, "Must use -g flag to initialize VTOC\n");
		exit(1);
	}

	/*
	 * a one-line change in the VTOC requires an existing VTOC
	 * to be present and healthy.
	 */
	if (delta) {
		if (no_old_vtoc || cmpt) {
			(void) fprintf(stderr,
			     "%s: Invalid VTOC\n", disk);
			(void) fprintf(stderr,
			     "Must use -g flag to initialize VTOC\n");
			exit(1);
		}
		insert(delta, ovtoc);
		validate(ovtoc, (struct vtoc *)0);
		checksum(ovtoc);
		pwrite(fd, V_VTOCSEC, V_SIZE, (char *) oldvtoc);
		exit(0);
	}

	/*
	 * initialize new VTOC.
	 */
	initial(disktype, nvtoc, ovtoc);

	/*
	 * validate that the geometry specified really matches the
	 * disk.  Can only be done on an unpartitioned device.
	 */
#ifndef PTX
	if (rawdisk)
#endif
		size_check(fd, disk, disktype, nvtoc->v_capacity);

	/*
	 * If the user has specified a partition data file, take partition
	 * data from there.  If not, read the file in PROTODIR which
	 * matches the disk type name which has been specified.
	 */
	if (dfile) {
		load(dfile, nvtoc);
	} else {
		(void) sprintf(vproto, "%s/%s.%s", PROTODIR,
			disktype, disk_name);
		if (stat(vproto, &statbuf) < 0) {
			(void) sprintf(vproto, "%s/%s.%s", PROTODIR,
				disktype, controller);
			if (stat(vproto, &statbuf) < 0)
				(void) sprintf(vproto, "%s/%s",
					PROTODIR, disktype);
		}
		load(vproto, nvtoc);
	}

	validate(nvtoc, ovtoc);
	checksum(nvtoc);
	pwrite(fd, V_VTOCSEC, V_SIZE, (char *) newvtoc);
	(void) printf("mkvtoc:  New volume table of contents now in place.\n");
	exit(0);
	/*NOTREACHED*/
}

/*
 * tryopen()
 *
 * Open a disk device, even if only part of the name is given.  Convert
 * block device name into raw device.  Also, trim off generic device name
 * and copy into "controller[]" to support our VTOC description file
 * search algorithm.
 */
static int
tryopen(name)
char		**name;
{
	int fd, len;
	char *p, *buf;
	static char rawpath[] = "/dev/r";

	/* Point to basename */
	if ((p = rindex(*name, '/')) == NULL)
		p = *name;
	else
		++p;

	/* Trim off leading 'r' to get block device name */
	if (*p == 'r')
		++p;

	if (!isdigit(p[2]) ) {
		fprintf(stderr, "%s: incorrect device name\n", *name);
		return (-1);
	}

	/* Copy this as the disk name */
	disk_name = malloc(strlen(p)+1);
	strcpy(disk_name, p);

	/* Chop off trailing partition specification if any */
	len = strlen(p);
	while ( len > 2 && ispart(p[len-1])) {
		p[len-1] = '\0';
		len--;
	}

	/* Copy off generic controller name */
	strncpy(controller, p, 2);

	/*
	 * Now build the raw device path.  sizeof(rawpath) includes room
	 * for the terminating null.
	 */
	buf = malloc(strlen(p) + sizeof(rawpath));
	strcpy(buf, rawpath);
	strcat(buf, p);
	*name = buf;

	/* Try and open it */
	if ((fd = open(buf, O_RDWR)) < 0) {
		perror(buf);
		return(-1);
	} else
		return(fd);
}



/*
 * initial()
 *
 * Initialize a new VTOC.
 */
static void
initial(disktype, vtoc, old_vtoc)
char			*disktype;
register struct vtoc	*vtoc, *old_vtoc;
{
	register int	i;
	struct geomtab	*g;

	bzero(vtoc, sizeof(struct vtoc));
	vtoc->v_sanity = VTOC_SANE;
	vtoc->v_version = V_VERSION_1;
	vtoc->v_size = sizeof (struct vtoc);
	if (strlen(disktype) > VTYPCHR) {
		(void) strncpy(vtoc->v_disktype, disktype, VTYPCHR - 1);
		vtoc->v_disktype[VTYPCHR - 1] = '\0';
	} else {
		(void) strcpy(vtoc->v_disktype, disktype);
	}

	for(i = 0; i < V_NUMPAR; ++i) {
		vtoc->v_part[i].p_type = V_NOPART;
	}

	/*
	 * Initilize the disk geometry.  This gets done from
	 * the global geometry table if the -g flag has been
	 * specified.  Otherwise, initialize it from the old VTOC.
	 */
	if (gflag) {
		g = getgeombyname(disktype);
		if (g == (struct geomtab *)0) {
			(void) fprintf(stderr, "%s not in %s\n",
			   disktype, INFODIR);
			exit(1);
		}
		vtoc->v_secsize = g->g_secsize;
		vtoc->v_ntracks = g->g_ntracks;
		vtoc->v_nsectors = g->g_nsectors;
		vtoc->v_ncylinders = g->g_ncylinders;
		vtoc->v_rpm = g->g_rpm;
		vtoc->v_capacity = g->g_capacity;
		vtoc->v_nseccyl = g->g_nseccyl;
	} else {
		vtoc->v_secsize = old_vtoc->v_secsize;
		vtoc->v_ntracks = old_vtoc->v_ntracks;
		vtoc->v_nsectors = old_vtoc->v_nsectors;
		vtoc->v_ncylinders = old_vtoc->v_ncylinders;
		vtoc->v_rpm = old_vtoc->v_rpm;
		vtoc->v_capacity = old_vtoc->v_capacity;
		vtoc->v_nseccyl = old_vtoc->v_nseccyl;
	}
}

/*
 * size_check()
 *
 * Confirm that the device being referenced really has the number
 * of sectors advertized and no more.
 */
size_check(fd, disk, disktype, size)
int	fd;
char	*disk;
char	*disktype;
int	size;
{
	char	*buffer;
	int	retval;
	struct geomtab	*g;

	buffer = valloc(DEV_BSIZE);
	if (buffer == NULL) {
		fprintf(stderr, "mkvtoc: out of memory\n");
		exit(1);
	}

	if (lseek(fd, (size - 1) * DEV_BSIZE, 0) < 0) {
		(void) fprintf(stderr, "mkvtoc: %s: ", disk);
		perror("lseek");
		exit(1);
	}

	/*
	 * Try reading the last sector.  Should succeed with size == DEV_BSIZE.
	 * Note that non-embedded SCSI disks are special-cased.  If we
	 * think we are on a smaller disk than we should be, see if the
	 * "minimum capacity" geometry parameter is set.  If so, check
	 * that sector out.
	 */
	if ((retval = read(fd, buffer, DEV_BSIZE)) < 0) {
		if (errno != EINVAL) {
			(void) fprintf(stderr, "mkvtoc: %s: ", disk);
			perror("read");
			exit(1);
		}

		g = getgeombyname(disktype);
		if (g == (struct geomtab *)0) {
			(void) fprintf(stderr, "%s not in %s\n",
			   disktype, INFODIR);
			exit(1);
		}

		/*
		 * No "minimum capacity" parameter - disk must truly
		 * be too small.
		 */
		if ((size = g->g_mincap) <= 0) {
			(void) fprintf(stderr,
"mkvtoc: %s does not have enough sectors to be of type %s\n", disk, disktype);
			exit(1);
		}

		/*
		 * actually try to read that sector to confirm that
		 * it is accessable.
		 */
		if (lseek(fd, (size - 1) * DEV_BSIZE, 0) < 0) {
			(void) fprintf(stderr, "mkvtoc: %s: ", disk);
			perror("lseek");
			exit(1);
		}
		if (read(fd, buffer, DEV_BSIZE) < 0) {
			if (errno == EINVAL) {
				(void) fprintf(stderr,
"mkvtoc: %s does not have enough sectors to be of type %s\n", disk, disktype);
				exit(1);
			} else {
				(void) fprintf(stderr, "mkvtoc: %s: ", disk);
				perror("read");
				exit(1);
			}
		}

		/*
		 * success!
		 */
		return;
	}

	if (retval != DEV_BSIZE) {
			(void) fprintf(stderr,
"mkvtoc: %s does not have enough sectors to be of type %s\n", disk, disktype);
			exit(1);
	}

	/*
	 * Try reading the sector past the end.  Should return 0.
	 */
	retval = read(fd, buffer, DEV_BSIZE);
	if (retval <= 0)
		return;

	/*
	 * Success means that the disk is too big.
	 */
	(void) fprintf(stderr,
"mkvtoc: Warning: %s has sectors beyond %s's end of disk value.\n",
		disk, disktype);
	return;
/* NOTREACHED */
}

/*
 * insert()
 *
 * Insert a change into the VTOC.
 */
static void
insert(data, vtoc)
char		*data;
struct vtoc	*vtoc;
{
	auto int	part;
	auto int	type;
	auto int	start;
	auto int	size;
	auto int	bsize;
	auto int	fsize;

	if (sscanf(data, "%d:%d:%d:%d:%d:%d",
	    &part, &type, &start, &size, &bsize, &fsize) != 6) {
		(void) fprintf(stderr, "Delta syntax error on \"%s\"\n", data);
		exit(1);
	}
	if (part < 0) {
		fprintf(stderr, "mkvtoc: illegal partition number %d\n", part);
		exit(1);
	}
	if (part >= V_NUMPAR) {
		fprintf(stderr,
		    "mkvtoc: partition number %d is too high--max is %d\n",
		    part, V_NUMPAR-1);
		exit(1);
	}

	/*
	 * Check is redundant with a similar one in validate().  This is
	 * because validate() needs a copy of the old VTOC to check for
	 * a V_RESERVED violation, and we don't have a copy of the old VTOC
	 * when -d is used.
	 */
	if (vtoc->v_part[part].p_type == V_RESERVED && !fflag) {
		(void) fprintf(stderr, "\
mkvtoc: Partition %d is not allowed to change or remove a\n\
\tV_RESERVED partition, unless the -f flag is specified\n", part);
		exit(1);
	}
	vtoc->v_part[part].p_type = type;
	vtoc->v_part[part].p_start = start;
	vtoc->v_part[part].p_size = size;
	vtoc->v_part[part].p_bsize = bsize;
	vtoc->v_part[part].p_fsize = fsize;

	if (part + 1 > vtoc->v_nparts) {
		vtoc->v_nparts = part + 1;
	}
}

/*
 * load()
 *
 * Load VTOC information from a datafile.
 */
static void
load(dfile, vtoc)
char		*dfile;
struct vtoc	*vtoc;
{
	FILE		*dstream;
	auto int	part, npart = 0;
	auto int	type;
	auto int	start;
	auto int	size;
	auto int	bsize;
	auto int	fsize;
	char		line[256];

	if (strcmp(dfile, "-") == 0)
		dstream = stdin;
	else if ((dstream = fopen(dfile, "r")) == NULL) {
		(void) fprintf(stderr, "Cannot open file %s\n", dfile);
		exit(1);
	}
	while (fgets(line, sizeof(line) - 1, dstream)) {
		if (line[0] == '\0' || line[0] == '\n' || line[0] == '*')
			continue;
		line[strlen(line) - 1] = '\0';
		if (sscanf(line, "%d %d %d %d %d %d",
		    &part, &type, &start, &size, &bsize, &fsize) != 6) {
			(void) fprintf(stderr, "%s: Syntax error on \"%s\"\n",
			    dfile, line);
			exit(1);
		}
		if (part < 0) {
	fprintf(stderr, "mkvtoc: illegal partition number %d\n", part);
			exit(1);
		}
		if (part >= V_NUMPAR) {
			fprintf(stderr,
	    "mkvtoc: partition number %d is too high--max is %d\n",
			    part, V_NUMPAR-1);
			exit(1);
		}
		if ((type < V_MIN_PART_TYPE) || (type > V_MAX_PART_TYPE)) {
			fprintf(stderr,
				"mkvtoc: illegal type value %d\n", type);
			exit(1);
		}

		/*
		 * For active, usable partitions check the sanity of the
		 * fields
		 */
		if (type != V_NOPART) {
			if (start < 0) {
				fprintf(stderr,
					"mkvtoc: illegal start offset %d\n", start);
				exit(1);
			}
			if (size < 0) {
				fprintf(stderr,
					"mkvtoc: illegal size value %d\n", size);
				exit(1);
			}

			/*
			 * Sanity check the file system parameters for partitions
			 * which can contain file systems.
			 */
			if (type == V_RAW) {
				if (bsize <= 0) {
					fprintf(stderr,
						"mkvtoc: illegal block size %d\n", bsize);
					exit(1);
				}
				if (bsize % DEV_BSIZE) {
					fprintf(stderr,
						"mkvtoc: block size must be multiple of %d\n",
							DEV_BSIZE);
					exit(1);
				}
				if (fsize <= 0) {
					fprintf(stderr,
						"mkvtoc: illegal fragment size %d\n", fsize);
					exit(1);
				}
				if (fsize % DEV_BSIZE) {
					fprintf(stderr,
						"mkvtoc: fragment size must be multiple of %d\n", DEV_BSIZE);
					exit(1);
				}
			}
		}

		vtoc->v_part[part].p_type = type;
		vtoc->v_part[part].p_start = start;
		vtoc->v_part[part].p_size = size;
		vtoc->v_part[part].p_bsize = bsize;
		vtoc->v_part[part].p_fsize = fsize;
		npart++;

		if (part + 1 > vtoc->v_nparts) {
			vtoc->v_nparts = part + 1;
		}
	}
	if (dstream != stdin && fclose(dstream) == EOF) {
		(void) fprintf(stderr, "I/O error reading datafile %s\n", dfile);
		exit(1);
	}
	if (!npart) {
		(void) fprintf(stderr,
	"mkvtoc: Error: no VTOC partitions specified in file\n");
		exit(1);
	}
}


/*
 * pread()
 *	read the VTOC off the disk
 *
 * The disk drive which is mkvtoc's argument may be an unpartitioned
 * disk, or it may have a partitioning driver pushed on it.  Both
 * of these possibilities are supported.  We first try to do a V_READ
 * ioctl, which should return a sane VTOC from the disk.  If the
 * ioctl fails, then this is probably a raw disk, and we will try the
 * read straight off the raw pack.
 */
static void
pread(fd, block, len, buf)
int		fd;
unsigned long	block;
unsigned long	len;
char		*buf;
{
	int	err;
	struct stat	sb;

	err = ioctl(fd, RIOFIRSTSECT, &offset);
	if (err < 0) {
		offset = 0;
	}
	if (ioctl(fd, V_READ, buf) != -1) {
		if (debug)
			printf("VTOC read\n");
		return;
	}
	if (ioctl(fd, V_PART, buf) != -1) {
		if (debug)
			printf("VTOC read but from compatability disk\n");
		return;
	}
	rawdisk++;
	
	if (debug)
		printf("VTOC read but from raw disk\n");
	/*
	 * check to be sure the minor number looks correct for
	 * an unpartitioned device.  If not correct, may be trying to
	 * make a VTOC on a cmpt device.
	 */
	if (fstat(fd, &sb) < 0) {
		fprintf(stderr, "mkvtoc: cannot stat disk device\n");
		perror("");
		exit(1);
	}
#ifdef XXX
	This appears to be part of their minor numbering scheme.
	Do we have a comparable test?
					... Andy

	if ((sb.st_rdev & (V_RAW_MINOR | DEV_DIAG_MASK))
						!= V_RAW_MINOR) {
		fprintf(stderr,
"mkvtoc: improper minor device number for unpartitioned disk\n");
		fprintf(stderr,
"mkvtoc: make sure there is no non-vtoc partitioning driver in place on disk.\n");
		exit(1);
	}
#endif
	block += offset;
	if (lseek(fd, (long)(block * DEV_BSIZE), L_SET) < (long)0) {
		(void) fprintf(stderr,
		     "mkvtoc: Cannot seek disk to block %d\n", block);
		perror("");
		exit(1);
	}
	if (read(fd, buf, (unsigned)len) != len) {
		(void) fprintf(stderr,
		     "mkvtoc: Cannot read from disk block %d\n", block);
		perror("");
		exit(1);
	}
}

/*
 * pwrite()
 *	write the VTOC to disk
 *
 * The disk drive which is mkvtoc's argument may be an unpartitioned
 * disk, or it may have a partitioning driver pushed on it.  Both
 * of these possibilities are supported.  If a previous pread() has
 * detected that no partitioning driver is in place, then simply
 * write() to the raw disk.  Otherwise, have the partitioning driver
 * do the writing via V_WRITE.
 *
 * This routine assumes that pread() has been run.
 */
static void
pwrite(fd, block, len, buf)
int		fd;
unsigned long	block;
unsigned long	len;
char		*buf;
{
	int retval;

	if(!rawdisk) {
		retval = ioctl(fd, V_WRITE, buf);
	} else {
		block += offset;
		if (lseek(fd, (long)(block * DEV_BSIZE), L_SET) < (long)0) {
			(void) fprintf(stderr,
			     "mkvtoc: Cannot seek disk to block %d\n", block);
			perror("");
			exit(1);
		}
		retval = write(fd, buf, (unsigned)len);
	}
	if (retval < 0) {
		if (errno == EPERM)  {
			(void) fprintf(stderr,
			     "mkvtoc: Must have super-user privileges\n");
		} else if (errno == EBUSY) {
			(void) fprintf(stderr,
			     "mkvtoc: Disk has busy partitions.\n");
		} else  {
			(void) fprintf(stderr, "mkvtoc: Cannot write VTOC\n");
			perror("");
		}
		exit(1);
	}
}


static void
usage()
{
	(void) fprintf(stderr, "\
Usage:	mkvtoc [ -f ] [ -g ] [ -d deltaline ]\n\
	[ -s datafile ] device disktype\n");
	exit(2);
}

/*
 * validate()
 *
 * Validate the new VTOC.
 */
static void
validate(vtoc, ovtoc)
struct vtoc	*vtoc, *ovtoc;
{
	register int i;
	register struct	partition *p;
	register int j;
	int	fullsz, total;
	int	numpar;
	int	new_numpar;
	int	old_numpar;
	int	vtoc_seen;

	fullsz = vtoc->v_capacity;

	new_numpar = 0;
	old_numpar = 0;
	vtoc_seen = 0;
	for(i=V_NUMPAR-1; i>=0; --i) {
		if ((new_numpar == 0) && (vtoc->v_part[i].p_type != V_NOPART)) {
			new_numpar=i;
			if (old_numpar)
				break;
		}
		if (!no_old_vtoc && ovtoc && (old_numpar == 0) && (ovtoc->v_part[i].p_type != V_NOPART)) {
			old_numpar=i;
			if (new_numpar)
				break;
		}
	}
	if ( new_numpar > old_numpar)
		numpar = new_numpar;
	else
		numpar = old_numpar;
	if ( new_numpar > vtoc->v_nparts) {
		fprintf(stderr, 
			"Warning: new partition %d set but only %d defined\n",
			new_numpar, vtoc->v_nparts);
	}
	if ( !no_old_vtoc && !cmpt && ovtoc && (old_numpar > ovtoc->v_nparts)) {
		fprintf(stderr, 
			"Warning: old partition %d set but only %d defined\n",
			old_numpar, ovtoc->v_nparts);
	}
	if ( debug )
		printf("old npart=%d new nparts=%d\n", old_numpar, new_numpar);
	
	for(i=0, total=0; i<=numpar; i++) {
		p = &vtoc->v_part[i];

		/*
		 * "Reserved partitions" are those areas of the disk
		 * which are reserved for bad block lists, diagnostic
		 * utilities to exercise, whatever.  These are
		 * disk-specific, so they are protected by the V_RESERVED
		 * designation, which we do not allow someone to
		 * violate.  This check is a noop if there is no old VTOC
		 * so its not the safest of checks.
		 */

		if ( debug )
			printf("looking at partition %d\n",i);

		if (!no_old_vtoc && !fflag && ovtoc &&
		     ovtoc->v_part[i].p_type == V_RESERVED &&
		     bcmp(p, &ovtoc->v_part[i],
			    sizeof(struct partition)) != 0) {

				(void) fprintf(stderr, "\
mkvtoc: Partition %d is not allowed to change or remove a\n\
\tV_RESERVED partition, unless the -f flag is specified\n", i);
				exit(1);
		}

		/*
		 * Unused partition table slot.
		 */

		if (p->p_type == V_NOPART) {
			continue;
		}


		/*
		 * Make sure the partition fits within the
		 * stated disk geometry.
		 */

		if (p->p_start > fullsz
		    || p->p_start + p->p_size > fullsz) {
			(void) fprintf(stderr, "\
mkvtoc: Partition %d specified as %lu sectors starting at %lu\n\
\tdoes not fit. The full disk contains %lu sectors.\n",
			    i, p->p_size, p->p_start, fullsz);
			exit(1);
		}

		total += p->p_size;

		/*
		 * Make sure that the total number of sectors in
		 * partitions fits within the stated disk
		 * geometry.
		 */

		if (total > fullsz)	{
			(void) fprintf(stderr, "\
mkvtoc: Total of %lu sectors specified within partitions\n\
\t%d through %d exceeds the disk capacity of %lu sectors.\n",
				0, i, fullsz);
			exit(1);
		}

		/*
		 * Note if this is the VTOC partition itself.
		 */
		if ( (p->p_type == V_RESERVED) && 
				(p->p_start == (V_VTOCSEC + offset))) {
			vtoc_seen++;
		}

		/*
		 * Now check for overlap.
		 */

		for (j = i + 1; j <= numpar; j++)	{
			/*
			 * Ignore disabled partitions
			 */
			if (vtoc->v_part[j].p_type == V_NOPART) {
				continue;
			}
			if (V_OVLAP(p, &vtoc->v_part[j])) {
				(void) fprintf(stderr,"\
mkvtoc: Partition %d overlaps partition %d.\n\
\tOverlap is not allowed.\n", i, j);
				exit(1);
			}
		}

		/*
		 * Check for basic sanity
		 */

		if (!ignore_part(p->p_type)) {
			daddr_t start;
			long	size;
			short	bsize, fsize;

			size = p->p_size;
			start = p->p_start;
			bsize = p->p_bsize;
			fsize = p->p_fsize;

			if (start < 0) {
				(void) fprintf(stderr, "\
mkvtoc: Partition %d has a starting sector (%d) which is less than 0\n",
				i, start);
				exit(1);
			}
			if (size < 0) {
				(void) fprintf(stderr, "\
mkvtoc: Partition %d has a size (%d) which is less than 0\n", i, size);
				exit(1);
			}

#define boot(P)	   (((P)->p_type == V_BOOT) || ((P)->p_type == V_FW))

			if (bsize < DEV_BSIZE && !boot(p)) {
				(void) fprintf(stderr, "\
mkvtoc: Warning: Partition %d has a block size (%d)\n\
which is less than the minimum (%d).\n", i, bsize, DEV_BSIZE);
			}
			if (bsize > MAXBSIZE && !boot(p)) {
				(void) fprintf(stderr, "\
mkvtoc: Warning: Partition %d has a block size (%d)\n\
which is larger than the maximum (%d).\n", i, bsize, MAXBSIZE);
			}
			if (fsize > bsize && !boot(p)) {
				(void) fprintf(stderr, "\
mkvtoc: Warning: Partition %d has a fragment size (%d)\n\
which is larger than the block size (%d).\n", i, fsize, bsize);
			}
			if (fsize % DEV_BSIZE != 0 && !boot(p)) {
				(void) fprintf(stderr, "\
mkvtoc: Warning: Partition %d has a fragment size (%d)\n\
which is not an even multiple of the disk sector size (%d).\n",
				i, fsize, DEV_BSIZE);
			}

			/*
			 * Don't allow a partition to overlap
			 * the VTOC.  This can only be confirmed
			 * if the firstsect is known.
			 */

			if (p->p_type != V_RESERVED &&
			     ((start >= V_VTOCSEC + offset &&
			     start < V_VTOCSEC + btodb(V_SIZE) + offset)
			     || (start+size > V_VTOCSEC + offset &&
			     start+size <= V_VTOCSEC + btodb(V_SIZE) + offset)
			     || (start < V_VTOCSEC + offset &&
			     start+size > V_VTOCSEC + btodb(V_SIZE) + offset))){

				(void) fprintf(stderr, "\
mkvtoc: Partition %d overlaps the VTOC and is not type V_RESERVED (%d)\n",
					i, V_RESERVED);
				exit(1);
			}
		}

	}
	if (!vtoc_seen) {
		fprintf(stderr, "Warning: no VTOC partition in VTOC\n");
	}
}

/*
 * checksum
 *
 * Set the v_cksum field of the VTOC
 */

static void
checksum(vtoc)
struct	vtoc	*vtoc;
{
	vtoc->v_cksum = 0;
	vtoc->v_cksum = get_cksum(vtoc);
}

/*
 * get_cksum()
 * return a checksum for the VTOC
 */
get_cksum(v)
	struct	vtoc *v;
{
	register long sum;
	register int  nelem = sizeof(struct vtoc) / sizeof(long);
	register long *lptr = (long *)v;

	sum = 0;
	while (nelem-- > 0) {
		sum += *lptr;
		++lptr;
	}
	return (sum);
}

