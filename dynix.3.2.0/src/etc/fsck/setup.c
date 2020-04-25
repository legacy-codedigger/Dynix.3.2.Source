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
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * #ifndef lint
 * static char sccsid[] = "@(#)setup.c	5.19 (Berkeley) 5/7/88";
 * #endif not lint
 */

#ident "$Header: setup.c 1.4 90/11/06 $"

/* $Log:	setup.c,v $
 */

#define DKTYPENAMES
#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/vtoc.h>
#include <sys/file.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/dir.h>
#include <strings.h>
#include <machine/endian.h>
#include "fsck.h"

BUFAREA asblk;
#define altsblock (*asblk.b_un.b_fs)
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)
#define DEVLEN (MAXPATHLEN)

char	*calloc();
#ifndef STANDALONE
char	*devnm();
extern char *rawname();
extern char *valloc();
#endif
extern int	errno;

setup(dev)
	char *dev;
{
	dev_t rootdev;
	int rawfd;
#ifdef notnow
	long cg, ncg;
#endif /* notnow */
	long size, asked, i, j;
	struct stat statb;

	havesb = 0;
	if (stat("/", &statb) < 0)
		errexit("Can't stat root\n");
	rootdev = statb.st_dev;
	if (stat(dev, &statb) < 0) {
#ifdef STANDALONE
		printf("%s errno=%d\n", dev, errno);
#else
		perror(dev);
#endif
		printf("Can't stat %s\n", dev);
		return (0);
	}
	rawflg = 0;
	if ((statb.st_mode & S_IFMT) == S_IFBLK)
		;
	else if ((statb.st_mode & S_IFMT) == S_IFCHR)
		rawflg++;
	else {
		if (reply("file is not a block or character device; OK") == 0)
			return (0);
	}
	if (rootdev == statb.st_rdev)
		hotroot++;
	if ((dfile.rfdes = open(dev, O_RDONLY)) < 0) {
#ifdef STANDALONE
		printf("%s errno=%d\n", dev, errno);
#else
		perror(dev);
#endif
		printf("Can't open %s\n", dev);
		return (0);
	}
#ifndef STANDALONE
	if (hotroot && !nflag) {
		if (strcmp(dev, "/dev/root") == 0)
			rawfd = open(devnm(rootdev), 0);
		else
			rawfd = open(rawname(dev), 0);
		if (rawfd < 0) {
			perror(rawname(dev));
			printf("Can't open raw device of root filesystem\n");
			return(0);
		}
#ifdef SYSV
		if (ioctl(rawfd, V_WRITEENABLE, 0) < 0) {
			perror(rawname(dev));
			printf("Can't make root writeable\n");
			(void)close(rawfd);
			return(0);
		}
#endif
	}
#endif
	if (preen == 0)
		printf("** %s", dev);
	if (nflag || (dfile.wfdes = open(dev, O_WRONLY | O_SYNC)) < 0) {
		dfile.wfdes = -1;
		if (preen)
			pfatal("NO WRITE ACCESS");
		printf(" (NO WRITE)");
	}
#ifdef SYSV
	/*
	 * we are able to reprotect the root after opening it, since
	 * write-protect simply prevents open-for-write.
	 */
	if (hotroot && dfile.wfdes != -1) {
		(void)ioctl(rawfd, V_WRITEPROTECT, 0);
		(void)close(rawfd);
	}
#endif
	if (preen == 0)
		printf("\n");
	if (!rawflg && !hotroot)
		pwarn("** Warning: %s\n",
	"checking a block device can result in \"CANNOT READ\" errors");
	dfile.mod = 0;
	rplyflag = 0;
	lfdir = 0;
	initbarea(&sblk);
	initbarea(&asblk);
	if (sblk.b_un.b_buf == NULL) {
#ifdef STANDALONE
		callocrnd(1024);
		sblk.b_un.b_buf = (char *)calloc(SBSIZE);
		asblk.b_un.b_buf = (char *)calloc(SBSIZE);
#else
		sblk.b_un.b_buf = (char *)valloc(SBSIZE);
		asblk.b_un.b_buf = (char *)valloc(SBSIZE);
#endif
	}
	bzero(sblk.b_un.b_buf, SBSIZE);
	bzero(asblk.b_un.b_buf, SBSIZE);
	if (sblk.b_un.b_buf == 0 || asblk.b_un.b_buf == 0)
		errexit("cannot allocate space for superblock\n");
#ifdef notnow
	if (lp = getdisklabel((char *)NULL, dfile.rfdes))
		dev_bsize = secsize = lp->d_secsize;
	else
#endif /*notnow*/
		dev_bsize = secsize = DEV_BSIZE;
	/*
	 * Read in the superblock, looking for alternates if necessary
	 */
	if (readsb(1) == 0) {
#ifdef nownow
		if (bflag || preen || calcsb(dev, dfile.rfdes, &proto) == 0) {
			(void) close(dfile.rfdes);
			(void) close(dfile.wfdes);
			return(0);
		}
		if (reply("LOOK FOR ALTERNATE SUPERBLOCKS") == 0) {
			(void) close(dfile.rfdes);
			(void) close(dfile.wfdes);
			return (0);
		}
		for (cg = 0; cg < proto.fs_ncg; cg++) {
			bflag = fsbtodb(&proto, cgsblock(&proto, cg));
			if (readsb(0) != 0)
				break;
		}
		if (cg >= proto.fs_ncg) {
			printf("%s %s\n%s %s\n%s %s\n",
				"SEARCH FOR ALTERNATE SUPER-BLOCK",
				"FAILED. YOU MUST USE THE",
				"-A OPTION TO FSCK TO SPECIFY THE",
				"LOCATION OF AN ALTERNATE",
				"SUPER-BLOCK TO SUPPLY NEEDED",
				"INFORMATION; SEE fsck(8).");
			(void) close(dfile.rfdes);
			(void) close(dfile.wfdes);
			return(0);
		}
		pwarn("USING ALTERNATE SUPERBLOCK AT %d\n", bflag);
#else /*notnow*/
		(void) close(dfile.rfdes);
		(void) close(dfile.wfdes);
		return(0);
#endif /*notnow*/
	}

	/*
	 * For PTXFS type file systems which are marked clean, return a
	 * no-action inidicator for preen operations
	 */
	if (preen && PTXFS(&sblock)) {
		if (hotroot && (sblock.fs_state == FS_ACTIVE)) {
			(void) close(dfile.rfdes);
			(void) close(dfile.wfdes);
			return(2);
		}
		if ((sblock.fs_state + (long)sblock.fs_time) == FS_OKAY) {
			(void) close(dfile.rfdes);
			(void) close(dfile.wfdes);
			return(2);
		}
	}

	fmax = sblock.fs_size;
	imax = sblock.fs_ncg * sblock.fs_ipg;
	/*
	 * Check and potentially fix certain fields in the super block.
	 */
	if ((sblock.fs_minfree < 0 || sblock.fs_minfree > 99)) {
		pfatal("IMPOSSIBLE MINFREE=%d IN SUPERBLOCK",
			sblock.fs_minfree);
		if (reply("SET TO DEFAULT") == 1) {
			sblock.fs_minfree = 10;
			sbdirty();
		}
	}
	if (sblock.fs_postblformat == FS_DYNAMICPOSTBLFMT) {
		if (sblock.fs_optim != FS_OPTTIME && sblock.fs_optim != FS_OPTSPACE) {
			pfatal("UNDEFINED OPTIMIZATION IN SUPERBLOCK");
			if (reply("SET TO DEFAULT") == 1) {
				sblock.fs_optim = FS_OPTTIME;
				sbdirty();
			}
		}
		if (sblock.fs_interleave < 1) {
			pwarn("IMPOSSIBLE INTERLEAVE=%d IN SUPERBLOCK",
				sblock.fs_interleave);
			sblock.fs_interleave = 1;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("SET TO DEFAULT") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		if (sblock.fs_npsect < sblock.fs_nsect) {
			pwarn("IMPOSSIBLE NPSECT=%d IN SUPERBLOCK",
				sblock.fs_npsect);
			sblock.fs_npsect = sblock.fs_nsect;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("SET TO DEFAULT") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
	}
	/*
	 * convert between file system formats.
	 */
	if (cvtflag) {
		if (sblock.fs_postblformat == FS_42POSTBLFMT) {
			/*
			 * Requested to convert from old format to new format
			 */
			if (preen)
				pwarn("CONVERTING TO NEW FILE SYSTEM FORMAT\n");
			else if (!reply("CONVERT TO NEW FILE SYSTEM FORMAT")) {
				(void) close(dfile.rfdes);
				(void) close(dfile.wfdes);
				return(0);
			}
			sblock.fs_postblformat = FS_DYNAMICPOSTBLFMT;
			sblock.fs_nrpos = 8;
			sblock.fs_postbloff =
			    (char *)(&sblock.fs_opostbl[0][0]) -
			    (char *)(&sblock.fs_link);
			sblock.fs_rotbloff = &sblock.fs_space[0] -
			    (unchar *)(&sblock.fs_link);
			sblock.fs_cgsize =
				fragroundup(&sblock, CGSIZE(&sblock));
			/*
			 * Planning now for future expansion.
			 */
#			if (BYTE_ORDER == BIG_ENDIAN)
				sblock.fs_qbmask.val[0] = 0;
				sblock.fs_qbmask.val[1] = ~sblock.fs_bmask;
				sblock.fs_qfmask.val[0] = 0;
				sblock.fs_qfmask.val[1] = ~sblock.fs_fmask;
#			endif /* BIG_ENDIAN */
#			if (BYTE_ORDER == LITTLE_ENDIAN)
				sblock.fs_qbmask.val[0] = ~sblock.fs_bmask;
				sblock.fs_qbmask.val[1] = 0;
				sblock.fs_qfmask.val[0] = ~sblock.fs_fmask;
				sblock.fs_qfmask.val[1] = 0;
#			endif /* LITTLE_ENDIAN */
			sblock.fs_id[0] = sblock.fs_un.fs_dsp.dfs_id[0];
			sblock.fs_id[1] = sblock.fs_un.fs_dsp.dfs_id[1];
			sblock.fs_state = FS_OKAY - (long)sblock.fs_time;
			sblock.fs_optim = FS_OPTSPACE;
			sblock.fs_npsect = sblock.fs_nsect;
			sblock.fs_interleave = 1;
			sblock.fs_trackskew = 0;
			sblock.fs_headswitch = 0;
			sblock.fs_trkseek = 0;
			sbdirty();
			dirty(&asblk);
		} else if (sblock.fs_postblformat == FS_DYNAMICPOSTBLFMT) {
			/*
			 * Requested to convert from new format to old format
			 */
			if (sblock.fs_nrpos != 8 || sblock.fs_ipg > 2048 ||
			    sblock.fs_cpg > 32 || sblock.fs_cpc > 16) {
				printf(
				"PARAMETERS OF CURRENT FILE SYSTEM DO NOT\n\t");
				errexit(
				"ALLOW CONVERSION TO OLD FILE SYSTEM FORMAT\n");
			}
			if (preen)
				pwarn("CONVERTING TO OLD FILE SYSTEM FORMAT\n");
			else if (!reply("CONVERT TO OLD FILE SYSTEM FORMAT")) {
				(void) close(dfile.rfdes);
				(void) close(dfile.wfdes);
				return(0);
			}
			sblock.fs_postblformat = FS_42POSTBLFMT;
			sblock.fs_un.fs_dsp.dfs_id[0] = sblock.fs_id[0];
			sblock.fs_un.fs_dsp.dfs_id[1] = sblock.fs_id[1];
			sblock.fs_un.fs_dsp.dfs_sparecon[0] = 0;
			sblock.fs_un.fs_dsp.dfs_sparecon[1] = 0;
			sblock.fs_un.fs_dsp.dfs_sparecon[2] = 0;
			sblock.fs_id[0] = -1;
			sblock.fs_id[1] = -1;
			sblock.fs_state = -1;
			sblock.fs_qbmask.val[0] = -1;
			sblock.fs_qbmask.val[1] = -1;
			sblock.fs_qfmask.val[0] = -1;
			sblock.fs_qfmask.val[1] = -1;
			sblock.fs_nrpos = -1;
			sblock.fs_postbloff = -1;
			sblock.fs_rotbloff = -1;
			sblock.fs_cgsize =
				fragroundup(&sblock, OCGSIZE(&sblock));
			sbdirty();
			dirty(&asblk);
		} else {
			errexit("UNKNOWN FILE SYSTEM FORMAT\n");
		}
	}
	if (asblk.b_dirty) {
		bcopy((char *)&sblock, (char *)&altsblock,
			(unsigned)sblock.fs_sbsize);
		flush(&dfile, &asblk);
	}
	/*
	 * read in the summary info.
	 */
	asked = 0;
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		size = sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize;
#ifdef STANDALONE
		callocrnd(1024);
		sblock.fs_csp[j] = (struct csum *)calloc((unsigned)size);
#else
		sblock.fs_csp[j] = (struct csum *)valloc((unsigned)size);
#endif
		if (bread(&dfile, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    size) != 0 && !asked) {
			pfatal("BAD SUMMARY INFORMATION");
			if (reply("CONTINUE") == 0)
				errexit("");
			asked++;
		}
	}
	/*
	 * allocate and initialize the necessary maps
	 *	This is to "fix" a bug in pass5(), where
	 *	it tried to clear bits beyond end of blockmap, and
	 *	ran into the statemap, clobbering it.  Must have worked
	 *	under tahoe due to rounding-up of sizes by tahoe malloc.
	 */
	bmapsz = roundup(howmany(fragroundup(&sblock, fmax), NBBY),
		sizeof(short));
#ifdef STANDALONE
	blockmap = calloc((unsigned)bmapsz*(sizeof (char)));
#else
	blockmap = calloc((unsigned)bmapsz, sizeof (char));
#endif
	if (blockmap == NULL) {
		printf("cannot alloc %d bytes for blockmap\n", bmapsz);
		goto badsb;
	}
#ifdef STANDALONE
	statemap = calloc((unsigned)(imax + 1)*(sizeof(char)));
#else
	statemap = calloc((unsigned)(imax + 1), sizeof(char));
#endif
	if (statemap == NULL) {
		printf("cannot alloc %d bytes for statemap\n", imax + 1);
		goto badsb;
	}
#ifdef STANDALONE
	lncntp = (short *)calloc((unsigned)(imax + 1)*(sizeof(short)));
#else
	lncntp = (short *)calloc((unsigned)(imax + 1), sizeof(short));
#endif
	if (lncntp == NULL) {
		printf("cannot alloc %d bytes for lncntp\n", 
		    (imax + 1) * sizeof(short));
		goto badsb;
	}

	bufinit();
	return (1);

badsb:
	ckfini();
	return (0);
}

/*
 * Read in the super block and its summary info.
 */
readsb(listerr)
	int listerr;
{
	daddr_t super = bflag ? bflag : SBOFF / dev_bsize;

	if (bread(&dfile, (char *)&sblock, super, (long)SBSIZE) != 0)
		return (0);
	sblk.b_bno = super;
	sblk.b_size = SBSIZE;
	/*
	 * run a few consistency checks of the super block
	 */
	if (sblock.fs_magic != FS_MAGIC)
		{ badsb(listerr, "MAGIC NUMBER WRONG"); return (0); }
	if (sblock.fs_ncg < 1)
		{ badsb(listerr, "NCG OUT OF RANGE"); return (0); }
	if (sblock.fs_cpg < 1)
		{ badsb(listerr, "CPG OUT OF RANGE"); return (0); }
	if (sblock.fs_ncg * sblock.fs_cpg < sblock.fs_ncyl ||
	    (sblock.fs_ncg - 1) * sblock.fs_cpg >= sblock.fs_ncyl)
		{ badsb(listerr, "NCYL LESS THAN NCG*CPG"); return (0); }
	if (sblock.fs_sbsize > SBSIZE)
		{ badsb(listerr, "SIZE PREPOSTEROUSLY LARGE"); return (0); }
	/*
	 * Compute block size that the filesystem is based on,
	 * according to fsbtodb, and adjust superblock block number
	 * so we can tell if this is an alternate later.
	 */
	super *= dev_bsize;
	dev_bsize = sblock.fs_fsize / fsbtodb(&sblock, 1);
	sblk.b_bno = super / dev_bsize;
	/*
	 * Set all possible fields that could differ, then do check
	 * of whole super block against an alternate super block.
	 * When an alternate super-block is specified this check is skipped.
	 */
	(void)getblk(&asblk, cgsblock(&sblock, sblock.fs_ncg - 1), sblock.fs_sbsize);
	if (asblk.b_errs != NULL)
		return (0);
/*
	if (bflag) {
		havesb = 1;
		return (1);
	}
*/
	altsblock.fs_link = sblock.fs_link;
	altsblock.fs_rlink = sblock.fs_rlink;
	altsblock.fs_time = sblock.fs_time;
	altsblock.fs_cstotal = sblock.fs_cstotal;
	altsblock.fs_cgrotor = sblock.fs_cgrotor;
	altsblock.fs_fmod = sblock.fs_fmod;
	altsblock.fs_clean = sblock.fs_clean;
	altsblock.fs_ronly = sblock.fs_ronly;
	altsblock.fs_flags = sblock.fs_flags;
	altsblock.fs_maxcontig = sblock.fs_maxcontig;
	altsblock.fs_minfree = sblock.fs_minfree;
	altsblock.fs_rotdelay = sblock.fs_rotdelay;
	altsblock.fs_maxbpg = sblock.fs_maxbpg;
	if (sblock.fs_postblformat != FS_42POSTBLFMT) {
		altsblock.fs_state = sblock.fs_state;
		/*
		 * The following should not have to be copied.
	 	*/
		altsblock.fs_optim = sblock.fs_optim;
		altsblock.fs_fsbtodb = sblock.fs_fsbtodb;
		altsblock.fs_interleave = sblock.fs_interleave;
		altsblock.fs_npsect = sblock.fs_npsect;
		altsblock.fs_nrpos = sblock.fs_nrpos;
	}
	bcopy((char *)sblock.fs_csp, (char *)altsblock.fs_csp,
		sizeof sblock.fs_csp);
	bcopy((char *)sblock.fs_fsmnt, (char *)altsblock.fs_fsmnt,
		sizeof sblock.fs_fsmnt);
	bcopy((char *)sblock.fs_sparecon, (char *)altsblock.fs_sparecon,
		sizeof sblock.fs_sparecon);
	if (bcmp((char *)&sblock, (char *)&altsblock,
			(unsigned)sblock.fs_sbsize)) {
		badsb(listerr,
		"VALUES IN SUPER BLOCK DISAGREE WITH THOSE IN LAST ALTERNATE");
		if (!bflag)
			return (0);
		/*
		 * Know we're not preen here, due to badsb() call reurning.
		 * So, ask if we should BELIEVE the alternate s.b.
		 * specified by the user.
		 */
		printf("SHOULD THE SPECIFIED ALTERNATE SUPERBLOCK\n");
		printf("BE UNCONDITIONALLY TRUSTED?");
		if (reply("ANSWERING YES WILL OVER-WRITE THE LAST ALTERNATE!")) {
			dirty(&asblk);
			return(1);
		}
		return (0);
	}
	havesb = 1;
	return (1);
}

badsb(listerr, s)
	int listerr;
	char *s;
{

	if (!listerr)
		return;
	if (preen)
		printf("%s: ", devname);
	pfatal("BAD SUPER BLOCK: %s\n", s);
}

#ifdef notnow
/*
 * Calculate a prototype superblock based on information in the disk label.
 * When done the cgsblock macro can be calculated and the fs_ncg field
 * can be used. Do NOT attempt to use other macros without verifying that
 * their needed information is available!
 */
calcsb(dev, devfd, fs)
	char *dev;
	int devfd;
	register struct fs *fs;
{
	register struct disklabel *lp;
	register struct partition *pp;
	register char *cp;
	int i;
	struct disklabel *getdisklabel();

	cp = index(dev, '\0') - 1;
	if (cp == (char *)-1 || (*cp < 'a' || *cp > 'h') && !isdigit(*cp)) {
		pfatal("%s: CANNOT FIGURE OUT FILE SYSTEM PARTITION\n", dev);
		return (0);
	}
	lp = getdisklabel(dev, devfd);
	if (isdigit(*cp))
		pp = &lp->d_partitions[0];
	else
		pp = &lp->d_partitions[*cp - 'a'];
	if (pp->p_fstype != FS_BSDFFS) {
		pfatal("%s: NOT LABELED AS A BSD FILE SYSTEM (%s)\n",
			dev, pp->p_fstype < FSMAXTYPES ?
			fstypenames[pp->p_fstype] : "unknown");
		return (0);
	}
	bzero(fs, sizeof(struct fs));
	fs->fs_fsize = pp->p_fsize;
	fs->fs_frag = pp->p_frag;
	fs->fs_cpg = pp->p_cpg;
	fs->fs_size = pp->p_size;
	fs->fs_ntrak = lp->d_ntracks;
	fs->fs_nsect = lp->d_nsectors;
	fs->fs_spc = lp->d_secpercyl;
	fs->fs_nspf = fs->fs_fsize / lp->d_secsize;
	fs->fs_sblkno = roundup(
		howmany(lp->d_bbsize + lp->d_sbsize, fs->fs_fsize),
		fs->fs_frag);
	fs->fs_cgmask = 0xffffffff;
	for (i = fs->fs_ntrak; i > 1; i >>= 1)
		fs->fs_cgmask <<= 1;
	if (!POWEROF2(fs->fs_ntrak))
		fs->fs_cgmask <<= 1;
	fs->fs_cgoffset = roundup(
		howmany(fs->fs_nsect, NSPF(fs)), fs->fs_frag);
	fs->fs_fpg = (fs->fs_cpg * fs->fs_spc) / NSPF(fs);
	fs->fs_ncg = howmany(fs->fs_size / fs->fs_spc, fs->fs_cpg);
	for (fs->fs_fsbtodb = 0, i = NSPF(fs); i > 1; i >>= 1)
		fs->fs_fsbtodb++;
	dev_bsize = lp->d_secsize;
	return (1);
}

struct disklabel *
getdisklabel(s, fd)
	char *s;
	int	fd;
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *)&lab) < 0) {
		if (s == NULL)
			return ((struct disklabel *)NULL);
		pwarn("");
#ifdef STANDALONE
		printf("ioctl (GDINFO) errno=%d\n", errno);
#else
		perror("ioctl (GDINFO)");
#endif
		errexit("%s: can't read disk label", s);
	}
	return (&lab);
}
#endif /*notnow*/

/*
 * devnm
 *	search /dev for a character-special device which matches fno.
 *
 * If "/dev/root" is being checked, we need to find the character special
 * device which matches /dev/root.  This needs to be done to allow it
 * to be write-protected.  Heavy borrowing from the devnm command to produce
 * these routines.
 *
 */

#ifndef STANDALONE
struct direct *dbuf;

char *
devnm(fno)
dev_t fno;
{
	int i;
	static struct devs {
		char *devdir;
		DIR *dfd;
	} devd[] = {		/* in order of desired search */
		"/dev/rdsk",0,
		"/dev",0,
		"/dev/dsk",0
	};
	static char devnam[DEVLEN+1];

	devnam[0] = '\0';
	if(!devd[1].dfd) {	/* if /dev isn't open, nothing happens */
		for(i = 0; i < 3; i++) {
			devd[i].dfd = opendir(devd[i].devdir);
		}
	}

	for(i = 0; i < 3; i++) {
		if((devd[i].dfd != 0) && (devd[i].dfd->dd_fd > 0)
		   && (chdir(devd[i].devdir) == 0)
		   && (dsearch(devd[i].dfd,fno))) {
			(void)strcpy(devnam, devd[i].devdir);
			(void)strcat(devnam,"/");
			(void)strncat(devnam,dbuf->d_name,DEVLEN-strlen(devnam));
			return(devnam);
		}
	}
	return(devnam);
}

dsearch(ddir,fno)
DIR *ddir;
dev_t fno;
{
	struct stat	S;

	rewinddir(ddir);
	while ((dbuf = readdir(ddir)) != NULL) {
		if(!dbuf->d_ino) continue;
		if(stat(dbuf->d_name, &S) == -1) {
			printf("fsck: cannot stat %s\n",dbuf->d_name);
			continue;
		}
		if((fno != S.st_rdev) 
		|| ((S.st_mode & S_IFMT) != S_IFCHR)
		|| (strcmp(dbuf->d_name,"swap") == 0)
		|| (strcmp(dbuf->d_name,"pipe") == 0)
		|| (strcmp(dbuf->d_name,"root") == 0)
			) continue;
		return(1);
	}
	return(0);
}
#endif
