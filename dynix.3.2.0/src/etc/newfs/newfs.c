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

#ident "$Header: newfs.c 1.6 1991/05/16 20:53:10 $"

/* $Log: newfs.c,v $
 *
 */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /*not lint*/

/*
 * #ifndef lint
 * static char sccsid[] = "@(#)newfs.c	6.16 (Berkeley) 5/1/88";
 * #endif not lint
 */

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <ufs/fs.h>
#include <sys/ioctl.h>
#include <sys/vtoc.h>
#include <zdc/zdc.h>
#include <diskinfo.h>

#include <stdio.h>
#include <strings.h>

#include <mntent.h>
/*
 * The following two constants set the default block and fragment sizes.
 * Both constants must be a power of 2 and meet the following constraints:
 *	MINBSIZE <= DESBLKSIZE <= MAXBSIZE
 *	sectorsize <= DESFRAGSIZE <= DESBLKSIZE
 *	DESBLKSIZE / DESFRAGSIZE <= 8
 */
#define	DFL_FRAGSIZE	1024
#define	DFL_BLKSIZE	8192
#define DFL_SECSIZE	512
#define DFL_RPM		3600

/*
 * Cylinder groups may have up to many cylinders. The actual
 * number used depends upon how much information can be stored
 * on a single cylinder. The default is to use 16 cylinders
 * per group.
 */
#define	DESCPG		16	/* desired fs_cpg */

/*
 * MINFREE gives the minimum acceptable percentage of file system
 * blocks which may be free. If the freelist drops below this level
 * only the superuser may continue to allocate blocks. This may
 * be set to 0 if no reserve of free blocks is deemed necessary,
 * however throughput drops by fifty percent if the file system
 * is run at between 90% and 100% full; thus the default value of
 * fs_minfree is 10%. With 10% free space, fragmentation is not a
 * problem, so we choose to optimize for time.
 */
#define MINFREE		10
#define DEFAULTOPT	FS_OPTTIME

/*
 * ROTDELAY gives the minimum number of milliseconds to initiate
 * another disk transfer on the same cylinder. It is used in
 * determining the rotationally optimal layout for disk blocks
 * within a file; the default of fs_rotdelay is 4ms.
 */
#define ROTDELAY	4

/*
 * MAXCONTIG sets the default for the maximum number of blocks
 * that may be allocated sequentially. Since UNIX drivers are
 * not capable of scheduling multi-block transfers, this defaults
 * to 1 (ie no contiguous blocks are allocated).
 */
#define MAXCONTIG	1

/*
 * MAXBLKPG determines the maximum number of data blocks which are
 * placed in a single cylinder group. The default is one indirect
 * block worth of data blocks.
 */
#define MAXBLKPG(bsize)	((bsize) / sizeof(daddr_t))

/*
 * Each file system has a number of inodes statically allocated.
 * We allocate one inode slot per NBPI bytes, expecting this
 * to be far more than we will ever need.
 */
#define	NBPI		2048

/*
 * For each cylinder we keep track of the availability of blocks at different
 * rotational positions, so that we can lay out the data to be picked
 * up with minimum rotational latency.  NRPOS is the default number of
 * rotational positions that we distinguish.  With NRPOS of 8 the resolution
 * of our summary information is 2ms for a typical 3600 rpm drive.
 */
#define	NRPOS		8	/* number distinct rotational positions */

/*
 * data_part is true for partition types which can be used for user
 * data.
 */
#define data_part(T)            ((T) == V_RAW || (T) == V_DIAG)


int	vflag;			/* print out "mkfs" command line */
int	Nflag;			/* run without writing file system */
int	Cflag;			/* create inodes like old 4.2 fs */
daddr_t	fssize;			/* file system size */
int	ntracks;		/* # tracks/cylinder */
int	nsectors;		/* # sectors/track */
int	nphyssectors;		/* # sectors/track including spares */
int	secpercyl;		/* sectors per cylinder */
int	trackspares = -1;	/* spare sectors per track */
int	cylspares = -1;		/* spare sectors per cylinder */
int	sectorsize;		/* bytes/sector */
#ifdef tahoe
int	realsectorsize;		/* bytes/sector in hardware */
#endif
int	rpm;			/* revolutions/minute of drive */
int	interleave;		/* hardware sector interleave */
int	trackskew = -1;		/* sector 0 skew, per track */
int	headswitch;		/* head switch time, usec */
int	trackseek;		/* track-to-track seek, usec */
int	fsize = 0;		/* fragment size */
int	bsize = 0;		/* block size */
int	cpg = DESCPG;		/* cylinders/cylinder group */
int	cpgflg;			/* cylinders/cylinder group flag was given */
int	minfree = MINFREE;	/* free space threshold */
int	opt = DEFAULTOPT;	/* optimization preference (space or time) */
int	density = NBPI;		/* number of bytes per inode */
int	maxcontig = MAXCONTIG;	/* max contiguous blocks to allocate */
int	rotdelay = ROTDELAY;	/* rotational delay between blocks */
int	maxbpg;			/* maximum blocks per file in a cyl group */
int	nrpos = NRPOS;		/* # of distinguished rotational positions */
int	bbsize = BBSIZE;	/* boot block size */
int	sbsize = SBSIZE;	/* superblock size */

char	device[MAXPATHLEN];

extern	int errno;
char	*index();
char	*rindex();
char	*find_mount();
extern char *valloc();
static void get_from_disktab();

main(argc, argv)
	int argc;
	char *argv[];
{
	char *cmd;

	cmd = rindex(argv[0], '/');
	if (cmd == NULL)
		cmd = argv[0];
	else
		cmd++;
	if (strcmp(cmd, "mkfs") == 0)
		mkfs_main(argc, argv);
	else if (strcmp(cmd, "newfs") == 0)
		newfs_main(argc, argv);
	else {
		fprintf(stderr, "%s: unknown command", cmd);
		exit(1);
	}
}

#define DFLNSECT	32
#define DFLNTRAK	16

print_mkfsusg()
{
	printf(
"usage: mkfs special size [ nsect ntrak bsize fsize cpg minfree rps nbpi ]\n");
	exit(1);
}

mkfs_main(argc, argv)
	int argc;
	char *argv[];
{
	char *special;
	char cmd[BUFSIZ];
	int fsi, fso, i, status;
	struct stat st;

	if (argc < 2) 
		print_mkfsusg();

	for (i = 2; i < argc; i++) {
		switch (i) {
		case 2:
			fssize = atoi(argv[i]);
			if (fssize < 0)
				fatal("%s: bad file system size", argv[0]);
			break;
		case 3:
			nsectors = atoi(argv[i]);
			if (nsectors < 0)
				fatal("%s: bad #sectors/track", argv[0]);
			break;
		case 4:
			ntracks = atoi(argv[i]);
			if (ntracks < 0)
				fatal("%s: bad #tracks/cyl", argv[0]);
			break;
		case 5:
			bsize = atoi(argv[i]);
			if (bsize < 0)
				fatal("%s: bad block size", argv[0]);
			break;
		case 6:
			fsize = atoi(argv[i]);
			if (fsize < 0)
				fatal("%s: bad frag size", argv[0]);
			break;
		case 7:
			cpg = atoi(argv[i]);
			if (cpg < 0)
				fatal("%s: bad #cyls per group", argv[0]);
			break;
		case 8:
			minfree = atoi(argv[i]);
			if (minfree < 0)
				fatal("%s: bad minimum free", argv[0]);
			break;
		case 9:
			rpm = atoi(argv[i]);
			if (rpm < 0)
				fatal("%s: bad rpm", argv[0]);
			rpm *= 60;
			break;
		case 10:
			density = atoi(argv[i]);
			if (density < 0)
				fatal("%s: bad number bytes per inode",
					argv[0]);
			break;
		default:
			fprintf(stderr, "too many arguments\n");
			print_mkfsusg();
		}
	}
	special = argv[1];
	fso = creat(special, 0666);
	if (fso < 0) {
		perror(special);
		exit(1);
	}
	fsi = open(special, O_RDONLY);
	if (fsi < 0) {
		fprintf(stderr, "%s: unable to open device", special);
		perror(special);
		exit(1);
	}
	if (stat(special, &st) < 0) {
		fprintf(stderr, "newfs: "); perror(special);
		exit(2);
	}
	if ((st.st_mode & S_IFMT) != S_IFCHR)
		fprintf(stderr, "*** %s: not a character device\n", special);
	sectorsize = DFL_SECSIZE;
	if (nsectors == 0)
		nsectors = DFLNSECT;
	if (ntracks == 0)
		ntracks = DFLNTRAK;
	if (bsize == 0)
		bsize = DFL_BLKSIZE;
	if (fsize == 0)
		fsize = DFL_FRAGSIZE;
	if (rpm == 0) 
		rpm = DFL_RPM;
	if (trackskew == -1) 
		trackskew = 0;
	if (interleave == 0)
		interleave = 1;
	if (minfree < 10 && opt != FS_OPTSPACE) {
		fprintf(stderr, "Warning: changing optimization to space ");
		fprintf(stderr, "because minfree is less than 10%%\n");
		opt = FS_OPTSPACE;
	}
	if (trackspares == -1) 
		trackspares = 0;
	nphyssectors = nsectors + trackspares;
	if (cylspares == -1) 
		cylspares = 0;
	secpercyl = nsectors * ntracks - cylspares;
	if (maxbpg == 0)
		maxbpg = MAXBLKPG(bsize);
	headswitch = 0;
	trackseek = 0;
	mkfs(special, fsi, fso);
	(void) sprintf(cmd, "/etc/fsirand %s", special);
	if (status = system(cmd))
		printf("%s: failed, status = %d\n", cmd, status);
	exit(0);

}

newfs_main(argc, argv)
	int argc;
	char *argv[];
{
	char *cp, *special, *devname;
	register struct partition *pp;
	struct stat st;
	int fsi, fso;
	int status, partno;
	struct vtoc *vp;
	char cmd[BUFSIZ];
	int vtocfd;

	Cflag = 0;
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++)
			switch (*cp) {

			case 'N':
				Nflag++;
				break;
			
			case 'C':
				Cflag++;
				break;

			case 'S':
				if (argc < 1)
					fatal("-S: missing sector size");
				argc--, argv++;
				sectorsize = atoi(*argv);
				if (sectorsize <= 0)
					fatal("%s: bad sector size", *argv);
				goto next;

			case 'a':
				if (argc < 1)
					fatal("-a: missing max contiguous blocks\n");
				argc--, argv++;
				maxcontig = atoi(*argv);
				if (maxcontig <= 0)
					fatal("%s: bad max contiguous blocks\n",
						*argv);
				goto next;

			case 'b':
				if (argc < 1)
					fatal("-b: missing block size");
				argc--, argv++;
				bsize = atoi(*argv);
				if (bsize < MINBSIZE)
					fatal("%s: bad block size", *argv);
				goto next;

			case 'c':
				if (argc < 1)
					fatal("-c: missing cylinders/group");
				argc--, argv++;
				cpg = atoi(*argv);
				if (cpg <= 0)
					fatal("%s: bad cylinders/group", *argv);
				cpgflg++;
				goto next;

			case 'd':
				if (argc < 1)
					fatal("-d: missing rotational delay\n");
				argc--, argv++;
				rotdelay = atoi(*argv);
				if (rotdelay < 0)
					fatal("%s: bad rotational delay\n",
						*argv);
				goto next;

			case 'e':
				if (argc < 1)
					fatal("-e: missing blocks pre file in a cyl group\n");
				argc--, argv++;
				maxbpg = atoi(*argv);
				if (maxbpg <= 0)
					fatal("%s: bad blocks per file in a cyl group\n",
						*argv);
				goto next;

			case 'f':
				if (argc < 1)
					fatal("-f: missing frag size");
				argc--, argv++;
				fsize = atoi(*argv);
				if (fsize <= 0)
					fatal("%s: bad frag size", *argv);
				goto next;

			case 'i':
				if (argc < 1)
					fatal("-i: missing bytes per inode\n");
				argc--, argv++;
				density = atoi(*argv);
				if (density <= 0)
					fatal("%s: bad bytes per inode\n",
						*argv);
				goto next;

			case 'I':
				if (argc < 2)
					fatal("-I: missing bpi percentage\n");
				argc--, argv++;
				density = ((atoi(*argv)*density)+50)/100;
				if (density <= 0)
					fatal("%s: bad bytes per inode\n",
						*argv);
				goto next;

			case 'k':
				if (argc < 1)
					fatal("-k: track skew");
				argc--, argv++;
				trackskew = atoi(*argv);
				if (trackskew < 0)
					fatal("%s: bad track skew", *argv);
				goto next;

			case 'l':
				if (argc < 1)
					fatal("-l: interleave");
				argc--, argv++;
				interleave = atoi(*argv);
				if (interleave <= 0)
					fatal("%s: bad interleave", *argv);
				goto next;

			case 'm':
				if (argc < 1)
					fatal("-m: missing free space %%\n");
				argc--, argv++;
				minfree = atoi(*argv);
				if (minfree < 0 || minfree > 99)
					fatal("%s: bad free space %%\n",
						*argv);
				goto next;

			case 'n':
				if (argc < 1)
					fatal("-n: missing rotational layout count\n");
				argc--, argv++;
				nrpos = atoi(*argv);
				if (nrpos <= 0)
					fatal("%s: bad rotational layout count\n",
						*argv);
				goto next;

			case 'o':
				if (argc < 1)
					fatal("-o: missing optimization preference");
				argc--, argv++;
				if (strcmp(*argv, "space") == 0)
					opt = FS_OPTSPACE;
				else if (strcmp(*argv, "time") == 0)
					opt = FS_OPTTIME;
				else
					fatal("%s: bad optimization preference %s",
					    *argv,
					    "(options are `space' or `time')");
				goto next;

			case 'p':
				if (argc < 1)
					fatal("-p: spare sectors per track");
				argc--, argv++;
				trackspares = atoi(*argv);
				if (trackspares < 0)
					fatal("%s: bad spare sectors per track", *argv);
				goto next;

			case 'r':
				if (argc < 1)
					fatal("-r: missing revs/minute\n");
				argc--, argv++;
				rpm = atoi(*argv);
				if (rpm <= 0)
					fatal("%s: bad revs/minute\n", *argv);
				goto next;

			case 's':
				if (argc < 1)
					fatal("-s: missing file system size");
				argc--, argv++;
				fssize = atoi(*argv);
				if (fssize <= 0)
					fatal("%s: bad file system size",
						*argv);
				goto next;

			case 't':
				if (argc < 1)
					fatal("-t: missing track total");
				argc--, argv++;
				ntracks = atoi(*argv);
				if (ntracks <= 0)
					fatal("%s: bad total tracks", *argv);
				goto next;

			case 'u':
				if (argc < 1)
					fatal("-u: missing sectors/track");
				argc--, argv++;
				nsectors = atoi(*argv);
				if (nsectors <= 0)
					fatal("%s: bad sectors/track", *argv);
				goto next;

			case 'v':
				vflag++;
				break;

			case 'x':
				if (argc < 1)
					fatal("-x: spare sectors per cylinder");
				argc--, argv++;
				cylspares = atoi(*argv);
				if (cylspares < 0)
					fatal("%s: bad spare sectors per cylinder", *argv);
				goto next;

			default:
				fatal("-%c: unknown flag", *cp);
			}
next:
		argc--, argv++;
	}

	if (argc > 1) {
		devname = argv[1];
		argc -= 1;
	}

	if (argc != 1) {
		fprintf(stderr, "usage: newfs [ fsoptions ] special-device\n");
		fprintf(stderr, "where fsoptions are:\n");
		fprintf(stderr, "\t-N do not create file system, %s\n",
			"just print out parameters");
		fprintf(stderr, "\t-C use 4.2 default inode creation\n");
		fprintf(stderr, "\t-b block size\n");
		fprintf(stderr, "\t-f frag size\n");
		fprintf(stderr, "\t-m minimum free space %%\n");
		fprintf(stderr, "\t-o optimization preference %s\n",
			"(`space' or `time')");
		fprintf(stderr, "\t-a maximum contiguous blocks\n");
		fprintf(stderr, "\t-d rotational delay between %s\n",
			"contiguous blocks");
		fprintf(stderr, "\t-e maximum blocks per file in a %s\n",
			"cylinder group");
		fprintf(stderr, "\t-i number of bytes per inode\n");
		fprintf(stderr, "\t-I scale bytes/inode by this percentage\n");
		fprintf(stderr, "\t-c cylinders/group\n");
		fprintf(stderr, "\t-n number of distinguished %s\n",
			"rotational positions");
		fprintf(stderr, "\t-s file system size (sectors)\n");
		fprintf(stderr, "\t-r revolutions/minute\n");
		fprintf(stderr, "\t-S sector size\n");
		fprintf(stderr, "\t-u sectors/track\n");
		fprintf(stderr, "\t-t tracks/cylinder\n");
		fprintf(stderr, "\t-p spare sectors per track\n");
		fprintf(stderr, "\t-x spare sectors per cylinder\n");
		fprintf(stderr, "\t-l hardware sector interleave\n");
		fprintf(stderr, "\t-k sector 0 skew, per track\n");
		exit(1);
	}
	special = argv[0];

/*
 * The following DEBUG flag should be used for compiling in a way that
 * newfs will write a filesystem to a regular file, or more appropriately
 * /dev/null.  The usage is a bit weird as you must supply the size of
 * the partition (with -s) to be used....  This will then be used with
 * a hard coded partition to create the filesystem.  Note that the -N
 * flag should be used to suppress any attempted reading of the fs after
 * it has been written.
 * Example:
 *		newfs -N -s 100000 /dev/null wren3
 *
 * This would make a wren3 filesystem of 100000 sectors in /dev/null.
 * This is primarily used to check the behaviour of newfs/mkfs with many
 * different disks when those disks are not available.
 *
 * Note that compiling with DEBUG set will only affect the behaviour of
 * newfs, not mkfs.
 */
#ifndef DEBUG
	cp = rindex(special, '/');
	if (cp != 0)
		special = cp + 1;
	if (*special == 'r' && special[1] != 'a' && special[1] != 'b')
		special++;
	(void) sprintf(device, "/dev/r%s", special);
	special = device;
	if (stat(special, &st) < 0) {
		fprintf(stderr, "newfs: "); perror(special);
		exit(2);
	}
	if ((st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a character device", special);

	/* Generate VTOC interface name by leaving off partition field */
	(void)strcpy(cmd, device);
	cmd[strlen(cmd)-1] = '\0';
	if ((vtocfd = open(cmd, O_RDONLY)) < 0) {
		perror(cmd);
		fatal("Can't open VTOC special device.\n");
	}

#endif /* DEBUG */
	if (!Nflag) {
		/*
		 * Protect against hosing an active disk.
		 */
		if (cp = find_mount(special)) {
		  	fprintf(stderr, "%s busy: file system mounted on %s\n",
			  			special, cp);
		  	exit(1);
		}
		fso = open(special, O_WRONLY);
		if (fso < 0) {
			perror(special);
			exit(1);
		}
	} else
		fso = -1;
#ifndef DEBUG
	fsi = open(special, O_RDONLY);
	if (fsi < 0) {
		fprintf(stderr, "%s: unable to open device", special);
		perror(special);
		exit(1);
	}
	if ((vp = (struct vtoc *)valloc(V_SIZE)) == NULL)
		fatal("mkfs: out of memory");
	if (ioctl(vtocfd, V_READ, (char *)vp) < 0) {
		if (!devname)
			fatal("no VTOC on disk--must specify device type");
		get_from_disktab(devname, vp, vtocfd);
	}
#else /* DEBUG */
	if ((vp = (struct vtoc *)valloc(V_SIZE)) == NULL)
		fatal("mkfs: out of memory");
	get_from_disktab(devname, vp, vtocfd);
#endif /* !DEBUG */
	if (vp->v_sanity != VTOC_SANE)
		fatal("%s: No VTOC available for device", cmd);
	cp = argv[0] + strlen(argv[0]) - 1;
	if ((*cp < 'a') || (*cp > 'z'))
		fatal("%s: can't figure out file system partition", argv[0]);
#ifndef DEBUG
	partno = *cp - 'a';
#else /* DEBUG */
	partno=0; 	/* used the 0th partition ("a") for doing this */
#endif /* !DEBUG */
	if (partno < 0 || partno > vp->v_nparts)
		fatal("%s: illegal partition number", cp);
	pp = &vp->v_part[partno];

#ifndef DEBUG
	if (!data_part(pp->p_type))
		fatal("%s: must be V_RAW partition to contain a file system\n",
			argv[0]);
#endif /* !DEBUG */
	if (fssize == 0) {
		fssize = pp->p_size;
		if (fssize < 0)
			fatal("%s: no default size for `%s' partition",
				argv[0], cp);
	} else if (fssize != pp->p_size) {
		fprintf(stderr,
		   "Warning: partition size does not match VTOC\n");
	}
	if (nsectors == 0) {
		nsectors = vp->v_nsectors;
		if (nsectors < 0)
			fatal("%s: no default #sectors/track", argv[0]);
	} else if (nsectors != vp->v_nsectors) {
		fprintf(stderr,
		   "Warning: #sectors/track does not match VTOC\n");
	}
	if (ntracks == 0) {
		ntracks = vp->v_ntracks;
		if (ntracks < 0)
			fatal("%s: no default #tracks", argv[0]);
	} else if (ntracks != vp->v_ntracks) {
		fprintf(stderr, "Warning: #tracks does not match VTOC\n");
	}
	if (sectorsize == 0) {
		sectorsize = vp->v_secsize;
		if (sectorsize < 0)
			fatal("%s: no default sector size", argv[0]);
	} else if (sectorsize != vp->v_secsize) {
		fprintf(stderr,
		   "Warning: sector size does not match VTOC\n");
	}
	if (bsize == 0) {
		bsize = pp->p_bsize;
		if (bsize <= 0)
			fatal("%s: no default block size for `%s' partition",
				argv[0], cp);
	} else if (bsize != pp->p_bsize) {
		fprintf(stderr, "Warning: block size does not match VTOC\n");
	}
	if (fsize == 0) {
		fsize = pp->p_fsize;
		if (fsize <= 0)
			fatal("%s: no default frag size for `%s' partition",
				argv[0], cp);
	} else if (fsize != pp->p_fsize) {
		fprintf(stderr, "Warning: frag size does not match VTOC\n");
	}
	if (rpm == 0) {
		rpm = vp->v_rpm;
		if (rpm < 0)
			fatal("%s: no default revolutions/minute value",
				argv[0]);
	} else if (rpm != vp->v_rpm) {
		fprintf(stderr,
		   "Warning: revolutions/minute value does not match VTOC\n");
	}
	if (trackskew == -1) {
		trackskew = 0;
	}
	if (interleave == 0) {
		interleave = 1;
	}
	if (minfree < 10 && opt != FS_OPTSPACE) {
		fprintf(stderr, "Warning: changing optimization to space ");
		fprintf(stderr, "because minfree is less than 10%%\n");
		opt = FS_OPTSPACE;
	}
	if (trackspares == -1) {
		trackspares = 0;
	}
	nphyssectors = nsectors + trackspares;
	if (cylspares == -1) {
		cylspares = 0;
	}
	secpercyl = nsectors * ntracks - cylspares;
	if (maxbpg == 0)
		maxbpg = MAXBLKPG(bsize);
	headswitch = 0;
	trackseek = 0;
	if (vflag)
		printf("/etc/mkfs %s %d %d %d %d %d %d %d %d %d\n",
			special, fssize, nsectors, ntracks, bsize,
			fsize, cpg, minfree, rpm/60, density);
	mkfs(special, fsi, fso);
	(void)sprintf(cmd, "/etc/fsirand %s", special);
	if (status = system(cmd))
		printf("%s: failed, status = %d\n", cmd, status);
	exit(0);
}

/*VARARGS1*/
fatal(fmt, arg1, arg2)
	char *fmt;
{
	fprintf(stderr, "newfs: ");
	fprintf(stderr, fmt, arg1, arg2);
	(void)putc('\n', stderr);
	exit(10);
}

/*
 * A valid VTOC was not present on the disk; we must use disktab to figure
 * out the disk geometry.  Return 0 on success, 1 on failure.
 */
static void
get_from_disktab(devname, vt, vfd)
	char *devname;
	register struct vtoc *vt;
	int vfd;
{
	register struct geomtab *g;
	register int i;
	struct partition *pp;

#ifndef DEBUG
	/* Get the default partitioning information from the driver */
	if (ioctl(vfd, V_PART, (char *)vt) < 0)
		fatal("%s: can't read default partitioning", devname);
#endif /* !DEBUG */

	/* Initialize unchanging VTOC fields */
	vt->v_sanity = VTOC_SANE;
	vt->v_version = V_VERSION_1;
	vt->v_size = sizeof(struct vtoc);
	if (strlen(devname) > VTYPCHR) {
		(void)strncpy(vt->v_disktype, devname, VTYPCHR-1);
		vt->v_disktype[VTYPCHR-1] = '\0';
	} else
		(void)strcpy(vt->v_disktype, devname);

	/* Get disk geometry based on device type */
	if ((g = getgeombyname(devname)) == NULL)
		fatal("%s: unknown device type", devname);

	/* Fill in VTOC fields from this information */
	vt->v_secsize = g->g_secsize;
	vt->v_ntracks = g->g_ntracks;
	vt->v_nsectors = g->g_nsectors;
	vt->v_ncylinders = g->g_ncylinders;
	vt->v_rpm = g->g_rpm;
	vt->v_capacity = g->g_capacity;
	vt->v_nseccyl = g->g_nseccyl;
#ifdef DEBUG
	vt->v_nparts = 8;	/* DEBUG assumes 8 partitions on the disks */
#endif /* DEBUG */

	/* If we didn't get frag/block size from driver, use default */
	for (i = 0; i < vt->v_nparts; ++i) {
		pp = vt->v_part+i;
		if (pp->p_fsize == 0)
			pp->p_fsize = DFL_FRAGSIZE;
		if (pp->p_bsize == 0)
			pp->p_bsize = DFL_BLKSIZE;
#ifdef DEBUG
	pp->p_size = fssize;	/* DEBUG sets all partitions to fssize */
#endif /* DEBUG */
		if (pp->p_size)
			printf("Partition %d, offset %d, size %d\n", i,
				pp->p_start, pp->p_size);
	}
}

/*
 * find_mount()--query the mount table and see if this device has any
 * filesystem mounted
 */
static char *
find_mount(dev)
	char *dev;
{
	register char *p;
	char buf[128], buf2[128];
	int len;
	register struct mntent *mntp;
	FILE *fp;

	/*
	 * Turn raw device name into block one
	 */
	if ((p = rindex(dev, '/')) == NULL)
		return(NULL);
	++p;
	if (*p == 'r')
		++p;
	(void) sprintf(buf, "/dev/%s", p);
	len = strlen(buf);

	/*
	 * Walk through mount table, see if we match up with any entry
	 */
	if ((fp = setmntent(MOUNTED, "r")) == NULL)
		return(NULL);
	while (mntp = getmntent(fp)) {
		if (!strcmp(mntp->mnt_type, MNTTYPE_IGNORE))
			continue;
		/*
		 * If it matches, return the whole name
		 */
		if (!strcmp(mntp->mnt_fsname, buf)) {
			endmntent(fp);
			return (mntp->mnt_fsname);
		}
	}
	endmntent(fp);
	return(NULL);
}
