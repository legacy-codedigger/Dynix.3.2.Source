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

#ifndef lint
static char rcsid[] = "$Header: mkfs.c 1.3 90/10/09 $";
#endif not lint

#ifndef STANDALONE
#include <stdio.h>
#include <a.out.h>
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <ufs/fs.h>
#include <sys/dir.h>
#include <machine/endian.h>
#include <sys/timeb.h>

/*
 * make file system for cylinder-group style file systems
 */

/*
 * The size of a cylinder group is calculated by CGSIZE. The maximum size
 * is limited by the fact that cylinder groups are at most one block.
 * Its size is derived from the size of the maps maintained in the 
 * cylinder group and the (struct cg) size.
 */
#define CGSIZE(fs) \
    /* base cg */	(sizeof(struct cg) + \
    /* blktot size */	(fs)->fs_cpg * sizeof(long) + \
    /* blks size */	(fs)->fs_cpg * (fs)->fs_nrpos * sizeof(short) + \
    /* inode map */	howmany((fs)->fs_ipg, NBBY) + \
    /* block map */	howmany((fs)->fs_cpg * (fs)->fs_spc / NSPF(fs), NBBY))

/*
 * We limit the size of the inode map to be no more than a
 * third of the cylinder group space, since we must leave at
 * least an equal amount of space for the block map.
 *
 * N.B.: MAXIPG must be a multiple of INOPB(fs).
 */
#define MAXIPG(fs)	roundup((fs)->fs_bsize * NBBY / 3, INOPB(fs))
#define OLDMAXIPG	2048

#define UMASK		0755
#define MAXINOPB	(MAXBSIZE / sizeof(struct dinode))
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

/*
 * variables set up by front end.
 */
extern int	Nflag;		/* run mkfs without writing file system */
extern int	Cflag;		/* create inodes like old 4.2 fs */
extern daddr_t	fssize;		/* file system size */
extern int	ntracks;	/* # tracks/cylinder */
extern int	nsectors;	/* # sectors/track */
extern int	nphyssectors;	/* # sectors/track including spares */
extern int	secpercyl;	/* sectors per cylinder */
extern int	sectorsize;	/* bytes/sector */
extern int	rpm;		/* revolutions/minute of drive */
extern int	interleave;	/* hardware sector interleave */
extern int	trackskew;	/* sector 0 skew, per track */
extern int	headswitch;	/* head switch time, usec */
extern int	trackseek;	/* track-to-track seek, usec */
extern int	fsize;		/* fragment size */
extern int	bsize;		/* block size */
extern int	cpg;		/* cylinders/cylinder group */
extern int	cpgflg;		/* cylinders/cylinder group flag was given */
extern int	minfree;	/* free space threshold */
extern int	opt;		/* optimization preference (space or time) */
extern int	density;	/* number of bytes per inode */
extern int	maxcontig;	/* max contiguous blocks to allocate */
extern int	rotdelay;	/* rotational delay between blocks */
extern int	maxbpg;		/* maximum blocks per file in a cyl group */
extern int	nrpos;		/* # of distinguished rotational positions */
extern int	bbsize;		/* boot block size */
extern int	sbsize;		/* superblock size */

struct fs *sblock;
struct	csum *fscs;

struct cg *acg;

struct dinode *zino;
#define INOSZ ((MAXBSIZE/sizeof(struct dinode))*sizeof(struct dinode))
char *buf;

int	fsi, fso;
time_t	utime;
daddr_t	alloc();
extern char *valloc();
extern time_t time();

mkfs(fsys, fi, fo)
	char *fsys;
	int fi, fo;
{
	register long i, mincpc, mincpg, inospercg;
	long cylno, rpos, blk, j, warn = 0;
	long used, mincpgcnt, bpcg;
	long mapcramped, inodecramped;
	long postblsize, rotblsize, totalsbsize;

	/*
	 * First allocate data structures for superblock and friends.
	 */
	sblock = (struct fs *)valloc(SBSIZE);
	if (sblock == (struct fs *)NULL) {
		printf("Cannot valloc sblock.\n");
		exit(1);
	}
	acg = (struct cg *)valloc(SBSIZE);
	if (acg == (struct cg *)NULL) {
		printf("Cannot valloc acg.\n");
		exit(1);
	}
	zino = (struct dinode *)valloc(INOSZ);
	if (zino == (struct dinode *)NULL) {
		printf("Cannot valloc zino.\n");
		exit(1);
	}
	buf = valloc(MAXBSIZE);
	if (buf == (char *)NULL) {
		printf("Cannot valloc buf.\n");
		exit(1);
	}
	bzero((char *)sblock, SBSIZE);
	bzero((char *)acg, SBSIZE);
	bzero((char *)zino, INOSZ);
	bzero(buf, MAXBSIZE);
#ifndef STANDALONE
	(void)time(&utime);
#endif
	fsi = fi;
	fso = fo;
	/*
	 * Validate the given file system size.
	 * Verify that its last block can actually be accessed.
	 */
	if (fssize <= 0)
		printf("preposterous size %d\n", fssize), exit(1);
	wtfs(fssize - 1, sectorsize, (char *)sblock);
	/*
	 * collect and verify the sector and track info
	 */
	sblock->fs_nsect = nsectors;
	sblock->fs_ntrak = ntracks;
	if (sblock->fs_ntrak <= 0)
		printf("preposterous ntrak %d\n", sblock->fs_ntrak), exit(1);
	if (sblock->fs_nsect <= 0)
		printf("preposterous nsect %d\n", sblock->fs_nsect), exit(1);
	/*
	 * collect and verify the block and fragment sizes
	 */
	sblock->fs_bsize = bsize;
	sblock->fs_fsize = fsize;
	if (!POWEROF2(sblock->fs_bsize)) {
		printf("block size must be a power of 2, not %d\n",
		    sblock->fs_bsize);
		exit(1);
	}
	if (!POWEROF2(sblock->fs_fsize)) {
		printf("fragment size must be a power of 2, not %d\n",
		    sblock->fs_fsize);
		exit(1);
	}
	if (sblock->fs_fsize < sectorsize) {
		printf("fragment size %d is too small, minimum is %d\n",
		    sblock->fs_fsize, sectorsize);
		exit(1);
	}
	if (sblock->fs_bsize < MINBSIZE) {
		printf("block size %d is too small, minimum is %d\n",
		    sblock->fs_bsize, MINBSIZE);
		exit(1);
	}
	if (sblock->fs_bsize < sblock->fs_fsize) {
		printf("block size (%d) cannot be smaller than fragment size (%d)\n",
		    sblock->fs_bsize, sblock->fs_fsize);
		exit(1);
	}
	sblock->fs_bmask = ~(sblock->fs_bsize - 1);
	sblock->fs_fmask = ~(sblock->fs_fsize - 1);
	/*
	 * Planning now for future expansion.
	 */
#	if (BYTE_ORDER == BIG_ENDIAN)
		sblock->fs_qbmask.val[0] = 0;
		sblock->fs_qbmask.val[1] = ~sblock->fs_bmask;
		sblock->fs_qfmask.val[0] = 0;
		sblock->fs_qfmask.val[1] = ~sblock->fs_fmask;
#	endif /* BIG_ENDIAN */
#	if (BYTE_ORDER == LITTLE_ENDIAN)
		sblock->fs_qbmask.val[0] = ~sblock->fs_bmask;
		sblock->fs_qbmask.val[1] = 0;
		sblock->fs_qfmask.val[0] = ~sblock->fs_fmask;
		sblock->fs_qfmask.val[1] = 0;
#	endif /* LITTLE_ENDIAN */
	for (sblock->fs_bshift = 0, i = sblock->fs_bsize; i > 1; i >>= 1)
		sblock->fs_bshift++;
	for (sblock->fs_fshift = 0, i = sblock->fs_fsize; i > 1; i >>= 1)
		sblock->fs_fshift++;
	sblock->fs_frag = numfrags(sblock, sblock->fs_bsize);
	for (sblock->fs_fragshift = 0, i = sblock->fs_frag; i > 1; i >>= 1)
		sblock->fs_fragshift++;
	if (sblock->fs_frag > MAXFRAG) {
		printf("fragment size %d is too small, minimum with block size %d is %d\n",
		    sblock->fs_fsize, sblock->fs_bsize,
		    sblock->fs_bsize / MAXFRAG);
		exit(1);
	}
	sblock->fs_nrpos = nrpos;
	sblock->fs_nindir = sblock->fs_bsize / sizeof(daddr_t);
	sblock->fs_inopb = sblock->fs_bsize / sizeof(struct dinode);
	sblock->fs_nspf = sblock->fs_fsize / sectorsize;
	for (sblock->fs_fsbtodb = 0, i = NSPF(sblock); i > 1; i >>= 1)
		sblock->fs_fsbtodb++;
	sblock->fs_sblkno =
	    roundup(howmany(bbsize + sbsize, sblock->fs_fsize), sblock->fs_frag);
	sblock->fs_cblkno = (daddr_t)(sblock->fs_sblkno +
	    roundup(howmany(sbsize, sblock->fs_fsize), sblock->fs_frag));
	sblock->fs_iblkno = sblock->fs_cblkno + sblock->fs_frag;
	sblock->fs_cgoffset = roundup(
	    howmany(sblock->fs_nsect, NSPF(sblock)), sblock->fs_frag);
	for (sblock->fs_cgmask = 0xffffffff, i = sblock->fs_ntrak; i > 1; i >>= 1)
		sblock->fs_cgmask <<= 1;
	if (!POWEROF2(sblock->fs_ntrak))
		sblock->fs_cgmask <<= 1;
	/*
	 * Validate specified/determined secpercyl
	 * and calculate minimum cylinders per group.
	 */
	sblock->fs_spc = secpercyl;
	for (sblock->fs_cpc = NSPB(sblock), i = sblock->fs_spc;
	     sblock->fs_cpc > 1 && (i & 1) == 0;
	     sblock->fs_cpc >>= 1, i >>= 1)
		/* void */;
	mincpc = sblock->fs_cpc;
	bpcg = sblock->fs_spc * sectorsize;
	inospercg = roundup(bpcg / sizeof(struct dinode), INOPB(sblock));
	if (Cflag) {
		if (inospercg > OLDMAXIPG) {
			inospercg = OLDMAXIPG;
		}
	} else {
		if (inospercg > MAXIPG(sblock)) {
			inospercg = MAXIPG(sblock);
		}
	}
	used = (sblock->fs_iblkno + inospercg / INOPF(sblock)) * NSPF(sblock);
	mincpgcnt = howmany(sblock->fs_cgoffset * (~sblock->fs_cgmask) + used,
	    sblock->fs_spc);
	mincpg = roundup(mincpgcnt, mincpc);
	/*
	 * Insure that cylinder group with mincpg has enough space
	 * for block maps
	 */
	sblock->fs_cpg = mincpg;
	sblock->fs_ipg = inospercg;
	mapcramped = 0;
	while (CGSIZE(sblock) > sblock->fs_bsize) {
		mapcramped = 1;
		if (sblock->fs_bsize < MAXBSIZE) {
			sblock->fs_bsize <<= 1;
			if ((i & 1) == 0) {
				i >>= 1;
			} else {
				sblock->fs_cpc <<= 1;
				mincpc <<= 1;
				mincpg = roundup(mincpgcnt, mincpc);
				sblock->fs_cpg = mincpg;
			}
			sblock->fs_frag <<= 1;
			sblock->fs_fragshift += 1;
			if (sblock->fs_frag <= MAXFRAG)
				continue;
		}
		if (sblock->fs_fsize == sblock->fs_bsize) {
			printf("There is no block size that");
			printf(" can support this disk\n");
			exit(1);
		}
		sblock->fs_frag >>= 1;
		sblock->fs_fragshift -= 1;
		sblock->fs_fsize <<= 1;
		sblock->fs_nspf <<= 1;
	}
	/*
	 * Insure that cylinder group with mincpg has enough space for inodes
	 */
	inodecramped = 0;
	used *= sectorsize;
	inospercg = roundup((mincpg * bpcg - used) / density, INOPB(sblock));
	if (Cflag && inospercg > OLDMAXIPG) {
		inospercg = OLDMAXIPG;
	}
	sblock->fs_ipg = inospercg;
	while (inospercg > MAXIPG(sblock)) {
		inodecramped = 1;
		if (mincpc == 1 || sblock->fs_frag == 1 ||
		    sblock->fs_bsize == MINBSIZE)
			break;
		printf("With a block size of %d %s %d\n", sblock->fs_bsize,
		    "minimum bytes per inode is",
		    (mincpg * bpcg - used) / MAXIPG(sblock) + 1);
		sblock->fs_bsize >>= 1;
		sblock->fs_frag >>= 1;
		sblock->fs_fragshift -= 1;
		mincpc >>= 1;
		sblock->fs_cpg = roundup(mincpgcnt, mincpc);
		if (CGSIZE(sblock) > sblock->fs_bsize) {
			sblock->fs_bsize <<= 1;
			break;
		}
		mincpg = sblock->fs_cpg;
		inospercg =
		    roundup((mincpg * bpcg - used) / density, INOPB(sblock));
		if (Cflag && inospercg > OLDMAXIPG) {
			inospercg = OLDMAXIPG;
		}
		sblock->fs_ipg = inospercg;
	}
	if (inodecramped) {
		if (inospercg > MAXIPG(sblock)) {
			printf("Minimum bytes per inode is %d\n",
			    (mincpg * bpcg - used) / MAXIPG(sblock) + 1);
		} else if (!mapcramped) {
			printf("With %d bytes per inode, ", density);
			printf("minimum cylinders per group is %d\n", mincpg);
		}
	}
	if (mapcramped) {
		printf("With %d sectors per cylinder, ", sblock->fs_spc);
		printf("minimum cylinders per group is %d\n", mincpg);
	}
	if (inodecramped || mapcramped) {
		if (sblock->fs_bsize != bsize)
			printf("%s to be changed from %d to %d\n",
			    "This requires the block size",
			    bsize, sblock->fs_bsize);
		if (sblock->fs_fsize != fsize)
			printf("\t%s to be changed from %d to %d\n",
			    "and the fragment size",
			    fsize, sblock->fs_fsize);
		exit(1);
	}
	/* 
	 * Calculate the number of cylinders per group
	 */
	sblock->fs_cpg = cpg;
	if (sblock->fs_cpg % mincpc != 0) {
		printf("%s groups must have a multiple of %d cylinders\n",
			cpgflg ? "Cylinder" : "Warning: cylinder", mincpc);
		sblock->fs_cpg = roundup(sblock->fs_cpg, mincpc);
		if (!cpgflg)
			cpg = sblock->fs_cpg;
	}
	/*
	 * Must insure there is enough space for inodes
	 */
	sblock->fs_ipg = roundup((sblock->fs_cpg * bpcg - used) / density,
		INOPB(sblock));
	if (Cflag && sblock->fs_ipg > OLDMAXIPG) {
		sblock->fs_ipg = OLDMAXIPG;
	}
	while (sblock->fs_ipg > MAXIPG(sblock)) {
		inodecramped = 1;
		sblock->fs_cpg -= mincpc;
		sblock->fs_ipg = roundup((sblock->fs_cpg * bpcg - used) / density,
			INOPB(sblock));
		if (Cflag && sblock->fs_ipg > OLDMAXIPG) {
			sblock->fs_ipg = OLDMAXIPG;
		}
	}
	/*
	 * Must insure there is enough space to hold block map
	 */
	while (CGSIZE(sblock) > sblock->fs_bsize) {
		mapcramped = 1;
		sblock->fs_cpg -= mincpc;
		sblock->fs_ipg = roundup((sblock->fs_cpg * bpcg - used) / density,
			INOPB(sblock));
	}
	sblock->fs_fpg = (sblock->fs_cpg * sblock->fs_spc) / NSPF(sblock);
	if ((sblock->fs_cpg * sblock->fs_spc) % NSPB(sblock) != 0) {
		printf("newfs: panic (fs_cpg * fs_spc) % NSPF != 0");
		exit(2);
	}
	if (sblock->fs_cpg < mincpg) {
		printf("cylinder groups must have at least %d cylinders\n",
			mincpg);
		exit(1);
	} else if (sblock->fs_cpg != cpg) {
		if (!cpgflg)
			printf("Warning: ");
		else if (!mapcramped && !inodecramped)
			exit(1);
		if (mapcramped && inodecramped)
			printf("Block size and bytes per inode restrict");
		else if (mapcramped)
			printf("Block size restricts");
		else
			printf("Bytes per inode restrict");
		printf(" cylinders per group to %d.\n", sblock->fs_cpg);
		if (cpgflg)
			exit(1);
	}
	sblock->fs_cgsize = fragroundup(sblock, CGSIZE(sblock));
	/*
	 * Now have size for file system and nsect and ntrak.
	 * Determine number of cylinders and blocks in the file system.
	 */
	sblock->fs_size = fssize = dbtofsb(sblock, fssize);
	sblock->fs_ncyl = fssize * NSPF(sblock) / sblock->fs_spc;
	if (fssize * NSPF(sblock) > sblock->fs_ncyl * sblock->fs_spc) {
		sblock->fs_ncyl++;
		warn = 1;
	}
	if (sblock->fs_ncyl < 1) {
		printf("file systems must have at least one cylinder\n");
		exit(1);
	}
	/*
	 * Determine feasability/values of rotational layout tables.
	 *
	 * The size of the rotational layout tables is limited by the
	 * size of the superblock, SBSIZE. The amount of space available
	 * for tables is calculated as (SBSIZE - sizeof (struct fs)).
	 * The size of these tables is inversely proportional to the block
	 * size of the file system. The size increases if sectors per track
	 * are not powers of two, because more cylinders must be described
	 * by the tables before the rotational pattern repeats (fs_cpc).
	 */
	sblock->fs_interleave = interleave;
	sblock->fs_trackskew = trackskew;
	sblock->fs_npsect = nphyssectors;
	sblock->fs_postblformat = FS_DYNAMICPOSTBLFMT;
	sblock->fs_sbsize = fragroundup(sblock, sizeof(struct fs));
	if (sblock->fs_ntrak == 1) {
		sblock->fs_cpc = 0;
		goto next;
	}
	postblsize = sblock->fs_nrpos * sblock->fs_cpc * sizeof(short);
	rotblsize = sblock->fs_cpc * sblock->fs_spc / NSPB(sblock);
	totalsbsize = sizeof(struct fs) + rotblsize;
	if (sblock->fs_nrpos == 8 && sblock->fs_cpc <= 16) {
		/* use old static table space */
		sblock->fs_postbloff = (char *)(&sblock->fs_opostbl[0][0]) -
		    (char *)(&sblock->fs_link);
		sblock->fs_rotbloff = &sblock->fs_space[0] -
		    (u_char *)(&sblock->fs_link);
	} else {
		/* use dynamic table space */
		sblock->fs_postbloff = &sblock->fs_space[0] -
		    (u_char *)(&sblock->fs_link);
		sblock->fs_rotbloff = sblock->fs_postbloff + postblsize;
		totalsbsize += postblsize;
	}
	if (totalsbsize > SBSIZE ||
	    sblock->fs_nsect > (1 << NBBY) * NSPB(sblock)) {
		printf("%s %s %d %s %d.%s",
		    "Warning: insufficient space in super block for\n",
		    "rotational layout tables with nsect", sblock->fs_nsect,
		    "and ntrak", sblock->fs_ntrak,
		    "\nFile system performance may be impaired.\n");
		sblock->fs_cpc = 0;
		goto next;
	}
	sblock->fs_sbsize = fragroundup(sblock, totalsbsize);
	/*
	 * calculate the available blocks for each rotational position
	 */
	for (cylno = 0; cylno < sblock->fs_cpc; cylno++)
		for (rpos = 0; rpos < sblock->fs_nrpos; rpos++)
			fs_postbl(sblock, cylno)[rpos] = -1;
	for (i = (rotblsize - 1) * sblock->fs_frag;
	     i >= 0; i -= sblock->fs_frag) {
		cylno = cbtocylno(sblock, i);
		rpos = cbtorpos(sblock, i);
		blk = fragstoblks(sblock, i);
		if (fs_postbl(sblock, cylno)[rpos] == -1)
			fs_rotbl(sblock)[blk] = 0;
		else
			fs_rotbl(sblock)[blk] =
			    fs_postbl(sblock, cylno)[rpos] - blk;
		fs_postbl(sblock, cylno)[rpos] = blk;
	}
next:
	/*
	 * Compute/validate number of cylinder groups.
	 */
	sblock->fs_ncg = sblock->fs_ncyl / sblock->fs_cpg;
	if (sblock->fs_ncyl % sblock->fs_cpg)
		sblock->fs_ncg++;
	/*
	 * Cflag says we should do this the old fashioned way, like the 4.2
	 * based inode/cg calculation.  We couldn't do this farther up so
	 * we do it here once we know everything we need to.
	 */
	if (Cflag) {
		int i, inos, oldipg;

		oldipg = sblock->fs_ipg;
		i = sblock->fs_iblkno + OLDMAXIPG / INOPF(sblock);
		inos = (fssize - sblock->fs_ncg * i) * sblock->fs_fsize /
			density / INOPB(sblock);
		if (inos <= 0) {
			inos = 1;
		}
		sblock->fs_ipg = ((inos / sblock->fs_ncg) + 1) * INOPB(sblock);
		/*
		 * cap this at 2048 like in 4.2 fs
		 */
		if (sblock->fs_ipg > OLDMAXIPG) {
			sblock->fs_ipg = OLDMAXIPG;
		}
		/*
		 * make sure we don't give more than the algoritm would
		 */
		if (sblock->fs_ipg > oldipg) {
			sblock->fs_ipg = oldipg;
		}
	}
	sblock->fs_dblkno = sblock->fs_iblkno + sblock->fs_ipg / INOPF(sblock);
	i = MIN(~sblock->fs_cgmask, sblock->fs_ncg - 1);
	if (cgdmin(sblock, i) - cgbase(sblock, i) >= sblock->fs_fpg) {
		printf("inode blocks/cyl group (%d) >= data blocks (%d)\n",
		    cgdmin(sblock, i) - cgbase(sblock, i) / sblock->fs_frag,
		    sblock->fs_fpg / sblock->fs_frag);
		printf("number of cylinders per cylinder group (%d) %s.\n",
		    sblock->fs_cpg, "must be increased");
		exit(1);
	}
	j = sblock->fs_ncg - 1;
	if ((i = fssize - j * sblock->fs_fpg) < sblock->fs_fpg &&
	    cgdmin(sblock, j) - cgbase(sblock, j) > i) {
		printf("Warning: inode blocks/cyl group (%d) >= data blocks (%d) in last\n",
		    (cgdmin(sblock, j) - cgbase(sblock, j)) / sblock->fs_frag,
		    i / sblock->fs_frag);
		printf("    cylinder group. This implies %d sector(s) cannot be allocated.\n",
		    i * NSPF(sblock));
		sblock->fs_ncg--;
		sblock->fs_ncyl -= sblock->fs_ncyl % sblock->fs_cpg;
		sblock->fs_size = fssize = sblock->fs_ncyl * sblock->fs_spc /
		    NSPF(sblock);
		warn = 0;
	}
	if (warn) {
		printf("Warning: %d sector(s) in last cylinder unallocated\n",
		    sblock->fs_spc -
		    (fssize * NSPF(sblock) - (sblock->fs_ncyl - 1)
		    * sblock->fs_spc));
	}
	/*
	 * fill in remaining fields of the super block
	 */
	sblock->fs_csaddr = cgdmin(sblock, 0);
	sblock->fs_cssize =
	    fragroundup(sblock, sblock->fs_ncg * sizeof(struct csum));
	i = sblock->fs_bsize / sizeof(struct csum);
	sblock->fs_csmask = ~(i - 1);
	for (sblock->fs_csshift = 0; i > 1; i >>= 1)
		sblock->fs_csshift++;
	fscs = (struct csum *)valloc((unsigned)sblock->fs_cssize);
	if (fscs == (struct csum *)NULL) {
		printf("Cannot valloc fcs.\n");
		exit(1);
	}
	bzero((char *)fscs, (unsigned)sblock->fs_cssize);
	sblock->fs_magic = FS_MAGIC;
	sblock->fs_rotdelay = rotdelay;
	sblock->fs_minfree = minfree;
	sblock->fs_maxcontig = maxcontig;
	sblock->fs_headswitch = headswitch;
	sblock->fs_trkseek = trackseek;
	sblock->fs_maxbpg = maxbpg;
	sblock->fs_rps = rpm / 60;
	sblock->fs_optim = opt;
	sblock->fs_cgrotor = 0;
	sblock->fs_cstotal.cs_ndir = 0;
	sblock->fs_cstotal.cs_nbfree = 0;
	sblock->fs_cstotal.cs_nifree = 0;
	sblock->fs_cstotal.cs_nffree = 0;
	sblock->fs_fmod = 0;
	sblock->fs_ronly = 0;
	/*
	 * Dump out summary information about file system.
	 */
	printf("%s:\t%d sectors in %d cylinders of %d tracks, %d sectors\n",
	    fsys, sblock->fs_size * NSPF(sblock), sblock->fs_ncyl,
	    sblock->fs_ntrak, sblock->fs_nsect);
	printf("\t%.1fMB in %d cyl groups (%d c/g, %.2fMB/g, %d i/g)\n",
	    (float)sblock->fs_size * sblock->fs_fsize * 1e-6, sblock->fs_ncg,
	    sblock->fs_cpg, (float)sblock->fs_fpg * sblock->fs_fsize * 1e-6,
	    sblock->fs_ipg);
	/*
	 * Now build the cylinders group blocks and
	 * then print out indices of cylinder groups.
	 */
	printf("super-block backups (for fsck -b #) at:");
	for (cylno = 0; cylno < sblock->fs_ncg; cylno++) {
		initcg(cylno);
		if (cylno % 9 == 0)
			printf("\n");
		printf(" %d,", fsbtodb(sblock, cgsblock(sblock, cylno)));
	}
	printf("\n");
	if (Nflag)
		exit(0);
	/*
	 * Now construct the initial file system,
	 * then write out the super-block.
	 */
	fsinit();
	sblock->fs_time = utime;
	sblock->fs_state = FS_OKAY - (long)sblock->fs_time;
	wtfs((daddr_t)(SBOFF / sectorsize), sbsize, (char *)sblock);
	for (i = 0; i < sblock->fs_cssize; i += sblock->fs_bsize) {
		int size;

		size = (sblock->fs_cssize - i < sblock->fs_bsize) ?
		    (sblock->fs_cssize - i) : (sblock->fs_bsize);
		wtfs(fsbtodb(sblock, sblock->fs_csaddr + numfrags(sblock, i)),
			size, ((char *)fscs) + i);
	}

	/* 
	 * Write out the duplicate super blocks
	 */
	for (cylno = 0; cylno < sblock->fs_ncg; cylno++)
		wtfs(fsbtodb(sblock, cgsblock(sblock, cylno)),
		    sbsize, (char *)sblock);
}

/*
 * Initialize a cylinder group.
 */
initcg(cylno)
	long cylno;
{
	daddr_t cbase, d, dlower, dupper, dmax;
	long i;
	register struct csum *cs;

	/*
	 * Determine block bounds for cylinder group.
	 * Allow space for super block summary information in first
	 * cylinder group.
	 */
	cbase = cgbase(sblock, cylno);
	dmax = cbase + sblock->fs_fpg;
	if (dmax > sblock->fs_size)
		dmax = sblock->fs_size;
	dlower = cgsblock(sblock, cylno) - cbase;
	dupper = cgdmin(sblock, cylno) - cbase;
	if (cylno == 0)
		dupper += howmany(sblock->fs_cssize, sblock->fs_fsize);
	cs = fscs + cylno;
	acg->cg_time = utime;
	acg->cg_magic = CG_MAGIC;
	acg->cg_cgx = cylno;
	if (cylno == sblock->fs_ncg - 1)
		acg->cg_ncyl = sblock->fs_ncyl % sblock->fs_cpg;
	else
		acg->cg_ncyl = sblock->fs_cpg;
	acg->cg_niblk = sblock->fs_ipg;
	acg->cg_ndblk = dmax - cbase;
	acg->cg_cs.cs_ndir = 0;
	acg->cg_cs.cs_nffree = 0;
	acg->cg_cs.cs_nbfree = 0;
	acg->cg_cs.cs_nifree = 0;
	acg->cg_rotor = 0;
	acg->cg_frotor = 0;
	acg->cg_irotor = 0;
	acg->cg_btotoff = &acg->cg_space[0] - (u_char *)(&acg->cg_link);
	acg->cg_boff = acg->cg_btotoff + sblock->fs_cpg * sizeof(long);
	acg->cg_iusedoff = acg->cg_boff + 
		sblock->fs_cpg * sblock->fs_nrpos * sizeof(short);
	acg->cg_freeoff = acg->cg_iusedoff + howmany(sblock->fs_ipg, NBBY);
	acg->cg_nextfreeoff = acg->cg_freeoff +
		howmany(sblock->fs_cpg * sblock->fs_spc / NSPF(sblock), NBBY);
	for (i = 0; i < sblock->fs_frag; i++) {
		acg->cg_frsum[i] = 0;
	}
	bzero((caddr_t)cg_inosused(acg),
		(unsigned)(acg->cg_freeoff - acg->cg_iusedoff));
	acg->cg_cs.cs_nifree += sblock->fs_ipg;
	if (cylno == 0)
		for (i = 0; i < ROOTINO; i++) {
			setbit(cg_inosused(acg), i);
			acg->cg_cs.cs_nifree--;
		}
	for (i = 0; i < sblock->fs_ipg / INOPF(sblock); i += sblock->fs_frag)
		wtfs(fsbtodb(sblock, cgimin(sblock, cylno) + i),
		    (int)sblock->fs_bsize, (char *)zino);
	bzero((caddr_t)cg_blktot(acg),
		(unsigned)(acg->cg_boff - acg->cg_btotoff));
	bzero((caddr_t)cg_blks(sblock, acg, 0),
	    (unsigned)(acg->cg_iusedoff - acg->cg_boff));
	bzero((caddr_t)cg_blksfree(acg),
		(unsigned)(acg->cg_nextfreeoff - acg->cg_freeoff));
	if (cylno > 0) {
		/*
		 * In cylno 0, beginning space is reserved
		 * for boot and super blocks.
		 */
		for (d = 0; d < dlower; d += sblock->fs_frag) {
			setblock(sblock, cg_blksfree(acg),
				(int)(d/sblock->fs_frag));
			acg->cg_cs.cs_nbfree++;
			cg_blktot(acg)[cbtocylno(sblock, d)]++;
			cg_blks(sblock, acg, cbtocylno(sblock, d))
			    [cbtorpos(sblock, d)]++;
		}
		sblock->fs_dsize += dlower;
	}
	sblock->fs_dsize += acg->cg_ndblk - dupper;
	if (i = dupper % sblock->fs_frag) {
		acg->cg_frsum[sblock->fs_frag - i]++;
		for (d = dupper + sblock->fs_frag - i; dupper < d; dupper++) {
			setbit(cg_blksfree(acg), dupper);
			acg->cg_cs.cs_nffree++;
		}
	}
	for (d = dupper; d + sblock->fs_frag <= dmax - cbase; ) {
		setblock(sblock, cg_blksfree(acg),
			(int)(d / sblock->fs_frag));
		acg->cg_cs.cs_nbfree++;
		cg_blktot(acg)[cbtocylno(sblock, d)]++;
		cg_blks(sblock, acg, cbtocylno(sblock, d))
		    [cbtorpos(sblock, d)]++;
		d += sblock->fs_frag;
	}
	if (d < dmax - cbase) {
		acg->cg_frsum[dmax - cbase - d]++;
		for (; d < dmax - cbase; d++) {
			setbit(cg_blksfree(acg), d);
			acg->cg_cs.cs_nffree++;
		}
	}
	sblock->fs_cstotal.cs_ndir += acg->cg_cs.cs_ndir;
	sblock->fs_cstotal.cs_nffree += acg->cg_cs.cs_nffree;
	sblock->fs_cstotal.cs_nbfree += acg->cg_cs.cs_nbfree;
	sblock->fs_cstotal.cs_nifree += acg->cg_cs.cs_nifree;
	*cs = acg->cg_cs;
	wtfs(fsbtodb(sblock, cgtod(sblock, cylno)),
		(int)sblock->fs_bsize, (char *)acg);
}

/*
 * initialize the file system
 */
struct inode {
	ino_t		i_number;	/* i number, 1-to-1 with dev addr */
	struct	icommon	i_ic;		/* disk portion of inode */
} node;

#ifdef LOSTDIR
#define PREDEFDIR 3
#else
#define PREDEFDIR 2
#endif

struct direct root_dir[] = {
	{ ROOTINO, sizeof(struct direct), 1, "." },
	{ ROOTINO, sizeof(struct direct), 2, ".." },
#ifdef LOSTDIR
	{ LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" },
#endif
};
#ifdef LOSTDIR
struct direct lost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), 1, "." },
	{ ROOTINO, sizeof(struct direct), 2, ".." },
	{ 0, DIRBLKSIZ, 0, 0 },
};
#endif
char *buf;

fsinit()
{
	/*
	 * initialize the node
	 */
	node.i_atime = utime;
	node.i_mtime = utime;
	node.i_ctime = utime;
#ifdef LOSTDIR
	/*
	 * create the lost+found directory
	 */
	(void)makedir(lost_found_dir, 2);
	for (i = DIRBLKSIZ; i < sblock->fs_bsize; i += DIRBLKSIZ)
		bcopy(&lost_found_dir[2], &buf[i], DIRSIZ(&lost_found_dir[2]));
	node.i_number = LOSTFOUNDINO;
	node.i_mode = IFDIR | UMASK;
	node.i_nlink = 2;
	node.i_size = sblock->fs_bsize;
	node.i_db[0] = alloc(node.i_size, node.i_mode);
	node.i_blocks = btodb(fragroundup(sblock, node.i_size));
	wtfs(fsbtodb(sblock, node.i_db[0]), node.i_size, buf);
	iput(&node);
#endif
	/*
	 * create the root directory
	 */
	node.i_number = ROOTINO;
	node.i_mode = IFDIR | UMASK;
	node.i_nlink = PREDEFDIR;
	node.i_size = makedir(root_dir, PREDEFDIR);
	node.i_db[0] = alloc((int)sblock->fs_fsize,
		(int)node.i_mode);
	node.i_blocks = btodb(fragroundup(sblock, node.i_size));
	wtfs(fsbtodb(sblock, node.i_db[0]),
		(int)sblock->fs_fsize, buf);
	iput(&node);
}

/*
 * construct a set of directory entries in "buf".
 * return size of directory.
 */
makedir(protodir, entries)
	register struct direct *protodir;
	int entries;
{
	char *cp;
	int i, spcleft;

	spcleft = DIRBLKSIZ;
	for (cp = buf, i = 0; i < entries - 1; i++) {
		protodir[i].d_reclen = DIRSIZ(&protodir[i]);
		bcopy((char *)&protodir[i], cp,
			protodir[i].d_reclen);
		cp += protodir[i].d_reclen;
		spcleft -= protodir[i].d_reclen;
	}
	protodir[i].d_reclen = spcleft;
	bcopy((char *)&protodir[i], cp, DIRSIZ(&protodir[i]));
	return (DIRBLKSIZ);
}

/*
 * allocate a block or frag
 */
daddr_t
alloc(size, mode)
	int size;
	int mode;
{
	int i, frag;
	daddr_t d;

	rdfs(fsbtodb(sblock, cgtod(sblock, 0)),
		(int)sblock->fs_cgsize, (char *)acg);
	if (acg->cg_magic != CG_MAGIC) {
		printf("cg 0: bad magic number\n");
		return (0);
	}
	if (acg->cg_cs.cs_nbfree == 0) {
		printf("first cylinder group ran out of space\n");
		return (0);
	}
	for (d = 0; d < acg->cg_ndblk; d += sblock->fs_frag)
		if (isblock(sblock, cg_blksfree(acg),
				(int)(d / sblock->fs_frag)))
			goto goth;
	printf("internal error: can't find block in cyl 0\n");
	return (0);
goth:
	clrblock(sblock, cg_blksfree(acg),
		(int)(d / sblock->fs_frag));
	acg->cg_cs.cs_nbfree--;
	sblock->fs_cstotal.cs_nbfree--;
	fscs[0].cs_nbfree--;
	if (mode & IFDIR) {
		acg->cg_cs.cs_ndir++;
		sblock->fs_cstotal.cs_ndir++;
		fscs[0].cs_ndir++;
	}
	cg_blktot(acg)[cbtocylno(sblock, d)]--;
	cg_blks(sblock, acg, cbtocylno(sblock, d))[cbtorpos(sblock, d)]--;
	if (size != sblock->fs_bsize) {
		frag = howmany(size, sblock->fs_fsize);
		fscs[0].cs_nffree += sblock->fs_frag - frag;
		sblock->fs_cstotal.cs_nffree += sblock->fs_frag - frag;
		acg->cg_cs.cs_nffree += sblock->fs_frag - frag;
		acg->cg_frsum[sblock->fs_frag - frag]++;
		for (i = frag; i < sblock->fs_frag; i++)
			setbit(cg_blksfree(acg), d + i);
	}
	wtfs(fsbtodb(sblock, cgtod(sblock, 0)),
		(int)sblock->fs_cgsize, (char *)acg);
	return (d);
}

/*
 * Allocate an inode on the disk
 */
iput(ip)
	register struct inode *ip;
{
	static struct dinode *buf = (struct dinode *)NULL;
	daddr_t d;

	if (buf == (struct dinode *)NULL) {
		buf = (struct dinode *)valloc(MAXINOPB*sizeof(struct dinode));
		if (buf == (struct dinode *)NULL) {
			printf("Cannot valloc iput buf.\n");
			exit(1);
		}
	}
	ip->i_ic.ic_flags = 0;
	rdfs(fsbtodb(sblock, cgtod(sblock, 0)),
		(int)sblock->fs_cgsize, (char *)acg);
	if (acg->cg_magic != CG_MAGIC) {
		printf("cg 0: bad magic number\n");
		exit(1);
	}
	acg->cg_cs.cs_nifree--;
	setbit(cg_inosused(acg), ip->i_number);
	wtfs(fsbtodb(sblock, cgtod(sblock, 0)),
		(int)sblock->fs_cgsize, (char *)acg);
	sblock->fs_cstotal.cs_nifree--;
	fscs[0].cs_nifree--;
	if (ip->i_number >= sblock->fs_ipg * sblock->fs_ncg) {
		printf("fsinit: inode value out of range (%d).\n",
		    ip->i_number);
		exit(1);
	}
	d = fsbtodb(sblock, itod(sblock, ip->i_number));
	rdfs(d, (int)sblock->fs_bsize, (char *)buf);
	buf[itoo(sblock, ip->i_number)].di_ic = ip->i_ic;
	wtfs(d, (int)sblock->fs_bsize, (char *)buf);
}

/*
 * read a block from the file system
 */
rdfs(bno, size, bf)
	daddr_t bno;
	int size;
	char *bf;
{
	int n;

	if (lseek(fsi, (off_t)(bno * sectorsize), 0) < 0) {
		printf("seek error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
	n = read(fsi, bf, size);
	if(n != size) {
		printf("read error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
}

/*
 * write a block to the file system
 */
wtfs(bno, size, bf)
	daddr_t bno;
	int size;
	char *bf;
{
	int n;

	if (Nflag)
		return;
	if (lseek(fso, (off_t)(bno * sectorsize), 0) < 0) {
		printf("seek error: %ld\n", bno);
		perror("wtfs");
		exit(1);
	}
	n = write(fso, bf, size);
	if(n != size) {
		printf("write error: %ld\n", bno);
		perror("wtfs");
		exit(1);
	}
}

/*
 * check if a block is available
 */
isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
#ifdef STANDALONE
		printf("isblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "isblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return (0);
	}
}

/*
 * take a block out of the map
 */
clrblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("clrblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "clrblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}

/*
 * put a block into the map
 */
setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch (fs->fs_frag) {
	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("setblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "setblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}
