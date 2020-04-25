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


/*
 * ident	"$Header: zdverify.c 1.9 91/03/26 $"
 * Disk verification routines
 */

/* $Log:	zdverify.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <errno.h>
#ifdef BSD
#include <sys/fs.h>
#include <sys/ioctl.h>
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#include <zdc/zdioctl.h>
#else
#include <sys/ufsfilsys.h>
#include <sys/zdc.h>
#include <sys/zdbad.h>
#include <sys/zdioctl.h>
#endif
#include "zdformat.h"
#include "format.h"

#define	MIN(a,b) (((a)<(b))?(a):(b))

extern caddr_t malloc();

static int hdrpassnum;          /* Number of current header pass over disk */
extern caddr_t	vwbp;		/* Verify write buffer */
extern caddr_t	vrbp;		/* Verify read buffer */
extern int	nspc;		/* number of sectors per cylinder */
extern int	errcode;	/* errcode for last I/O operation */

struct cnterr {
	int	ecc;		/* EECC */
	int	hdrecc;		/* EHDRECC */
	int	bse;		/* EBSE */
	int	so;		/* ESO  */
	int	her;		/* EHER */
	int	misc;		/* Miscellaneaous */
	int	all;		/* Sum of above errors */
} cnterr;

/*
 * Purdue/EE severe burn-in patterns.
 * 	Modified by Sequent, Inc. - Feb 86
 *	Swabbed for ZDC.
 */
unsigned ppat[] = {
	0x4CDBF6CC,	0xB66DDBB6,	0xAAAAAAAA,	0x55555555,
	0x33333333,	0x711CC771,	0xDBB66DDB,	0x8EE38EE3,
	0xC7711CC7,	0x8EE3388E,	0x1CC7711C,	0x388EE338,
	0x49922449,	0x24499224,	0x92244992,	0x6DDBB66D,
	0xB66DDBB6,	0x55555555,	0x00000000,	0xAAAAAAAA,
	0xFFFFFFFF,	0x4CDBF6CC,	0xAAAAAAAA,	0x4CDBF6CC,
	0x55555555,	0x4CDBF6CC,	0xAAAAAAAA,	0x4CDBF6CC,
	0x55555555,	0x4CDBF6CC,	0xAAAAAAAA,	0x4CDBF6CC,
	0x55555555,	0x599AA559,	0xAAAAAAAA,	0x599AA559,
	0x55555555,	0x599AA559,	0xAAAAAAAA,	0x599AA559,
	0x55555555,	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,
	0x55555555,	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,
};

int pps = (sizeof(ppat) / sizeof(ppat[0]));

/*
 * verify
 *	Verify the disk between startcyl and lastcyl. Typically, the
 *	whole disk is verified. 
 * WARNING: This will scribble on the disk.
 */
 
doverify()
{
	register int pass;
	int	npat;			/* Pattern number */
	int	tracksize;		/* bytes in a track */

	npat = 0;
	/*
	 * Set for severe burn-in (disable ECC correction).
	 */
	if (ioctl(fd, ZIOSEVERE, (char *)NULL) < 0) {
		fflush(stdout);
		perror("ioctl ZIOSEVERE error");
		fprintf(stderr, "Unable to set device for severe burn-in\n");
		fprintf(stderr, "...exiting\n");
		exit(1);
	}
	tracksize = chancfg->zdd_sectors << DEV_BSHIFT;
	sigints = 0;

	/*
	 * Do header defect passes. If error, add another pass.
	 * Since some tasks require at least one header pass,
	 * suppress the rest of verification if the noverification
	 * option was given.  A defect pass was manditory.
	 */
	for (hdrpassnum = 1; hdrpassnum <= hdrpasses; hdrpassnum++) {
		if (sigints) {
			printf("Interrupt - quit header verify passes.\n");
			break;
		}
		if (verbose)
			printf("Begin header verify pass %d.\n", hdrpassnum);
		if (dohdrpass() == FAIL) {
			fflush(stdout);
			fprintf(stderr,
			"Errors found - adding an additional header verify pass.\n");
			hdrpasses++;
		}
	}
	if (args & B_NOVERIFY) goto verify_done;

	/*
	 * Do full passes.
	 */
	sigints = 0;
	for (pass = 0; pass < fullpasses; pass++) {
		if (sigints) {
			printf("Interrupt - quit full verify passes.\n");
			break;
		}
		if (verbose)
			printf("Begin Full verify pass %d.\n", pass + 1);
		if (dofullpass(tracksize, npat) == FAIL) {
			fflush(stdout);
			fprintf(stderr,
			"Warning: Defect(s) found during full pass %d\n",
				pass + 1);
		}
		if (++npat > pps)
			npat = 0;
	}

	/*
	 * Do defect passes. If error, add another pass.
	 */
	for (pass = 0; pass < defectpasses; pass++) {
		if (sigints) {
			printf("Interrupt - quit defect passes.\n");
			break;
		}
		if (verbose)
			printf("Begin Defect verify pass %d.\n", pass + 1);
		if (dodefectpass(tracksize, npat) == FAIL) {
			defectpasses++;
			fflush(stdout);
			fprintf(stderr,
			"Error found - Adding an additional Defect verify pass.\n");
		}
		if (++npat > pps)
			npat = 0;
	}

verify_done:
	if ((args & B_NOVERIFY) == 0 || cnterr.all > 0) {

		/*
		 * Report Errors.
		 */
		printf("Errors found during verify:\n"); 
		printf("%d ECC errors.\n", cnterr.ecc);
		printf("%d HDR ECC errors.\n", cnterr.hdrecc);
		printf("%d BSE errors.\n", cnterr.bse);
		printf("%d SO errors.\n", cnterr.so);
		printf("%d HARD errors.\n", cnterr.her);
		printf("%d Misc. errors.\n", cnterr.misc);
		printf("%d Total errors.\n", cnterr.all);
	}

	/*
	 * Update various lists and data on disk since verify may have
	 * either found and added new bad blocks or clobbered the cylinder
	 * where the data resides.
	 */
	if (ioctl(fd, ZIONSEVERE, (char *)NULL) < 0)
		perror("ioctl ZIONSEVERE error");
	return;
}

/*
 * dofullpass
 *	Do full verify pass across entire disk.
 *
 * Return:	SUCCESS - No failures found.
 *		Fail	- At least 1 failure found during verify.
 */
static
dofullpass(tracksize, npat)
	int	tracksize;	/* tracksize in bytes */
	int	npat;		/* pattern (index into ppat array) */
{
	register int track;
	register int cyl;
	int	sector;
	int	trkadj;		/* delta for short tracks */
	int	ndefect;	/* number of defects - spare on short track */
	bool_t	badblk;
	int	errs;

	badblk = FALSE;

	/*
	 * Initialize buffer with pattern.
	 */
	bufinit(vwbp, npat, tracksize);

	/*
	 * Begin verify, for each track:
	 * 	- Write data in buffer
	 *	- Read data. Hardware checks header and data ECC.
	 *	- If error, add to bad block list and format appropriatly.
	 */
	sigints = 0;
	for (cyl = startcyl; cyl <= lastcyl; cyl++) {
		track = 0;
		/*
		 * quit only after processing a cylinder.
		 */
		if (sigints)
			break;
		errs = 0;
		while (track < chancfg->zdd_tracks) {
			if (cyl == ZDD_DDCYL && track == 0) {
				/*
				 * Special case: Do not try to write/read
				 * Special Sectors (zdcdd).
				 */
				testwrite(ZDD_NDDSECTORS, vwbp,
					tracksize - (ZDD_NDDSECTORS*DEV_BSIZE));
				if (testread(ZDD_NDDSECTORS, vrbp,
						tracksize - (ZDD_NDDSECTORS*DEV_BSIZE),
						vwbp) == FAIL) {
					badblk = TRUE;
					if (++errs > 3)
						continue;	/* redo track */
				}
			} else {
				trkadj = 0;
				if (cyl == ZDD_DDCYL
				|| cyl >= (chancfg->zdd_cyls - ZDD_NDGNCYL)) {
					/*
					 * Only write good part of short tracks
					 */
					ndefect = badsectontrk(cyl, track);
					ndefect -= chancfg->zdd_spare;
					if (ndefect > 0)
						trkadj = ndefect << DEV_BSHIFT;
				}
				sector = (cyl * nspc)
					 + (track * chancfg->zdd_sectors);
				testwrite(sector, vwbp, tracksize - trkadj);
				if (testread(sector, vrbp, tracksize - trkadj, vwbp) == FAIL) {
					badblk = TRUE;
					if (++errs > 3)
						continue;	/* redo track */
				}
			}
			++track;
			errs = 0;
		}
		/*
		 * test spares until no more problem
		 */
		while (testspares(cyl, vwbp, vrbp, npat) == FAIL)
			badblk = TRUE;
	}
	return((badblk == TRUE) ? FAIL : SUCCESS);
}

/*
 * dodefectpass
 *	Do verify pass on only tracks with defects.
 * This is done by simultaneously walking down both the bad block list
 * and the mfg defect list. The mfg defect list is needed so that tracks
 * with defects in the gap or head scatter are tested.
 *
 * Return:	SUCCESS - No failures found.
 *		Fail	- At least 1 failure found during verify.
 */
static
dodefectpass(tracksize, npat)
	int	tracksize;	/* tracksize in bytes */
	int	npat;		/* pattern (index into ppat array) */
{
	register struct bz_bad *bzp;
	register struct bm_mfgbad *bmp;
	register int	trk;		/* track to test */
	register bool_t	doanother;	/* telescope test pattern */
	register int	cyl;
	int	btrack;			/* base track */
	int	bcyl;			/* base cylinder */
	struct	cyl_hdr *chp;
	bool_t	foundbad;
	bool_t	badblk;
	struct	bz_bad	*find_trk();

	foundbad = FALSE;
	badblk = FALSE;
	bzp = bbl->bz_bad;
	bmp = mfg->bm_mfgbad;
	/*
	 * Initialize buffer with pattern.
	 */
	bufinit(vwbp, npat, tracksize);

	/*
	 * Skip to start cylinder.
	 */
	while (bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl < startcyl)
		++bzp;
	while (bmp < &mfg->bm_mfgbad[mfg->bm_nelem] && bmp->bm_cyl < startcyl)
		++bmp;
	/*
	 * Verify those tracks with defects
	 */
	for(;;) {
		if (sigints)
			break;
		/*
		 * Check if we are done.
		 * If not, find base (bcyl, btrack).
		 */
		if (bzp == &bbl->bz_bad[bbl->bz_nelem]) {
			if (bmp == &mfg->bm_mfgbad[mfg->bm_nelem])
				break;		/* DONE */
			if (bmp->bm_cyl > lastcyl)
				break;		/* DONE */
			bcyl = bmp->bm_cyl;
			btrack = bmp->bm_head;
		} else if (bmp == &mfg->bm_mfgbad[mfg->bm_nelem]) {
			if (bzp->bz_cyl > lastcyl)
				break;		/* DONE */
			bcyl = bzp->bz_cyl;
			btrack = bzp->bz_head;
		} else if (bzp->bz_cyl < bmp->bm_cyl) {
			bcyl = bzp->bz_cyl;
			btrack = bzp->bz_head;
		} else if (bmp->bm_cyl < bzp->bz_cyl) {
			bcyl = bmp->bm_cyl;
			btrack = bmp->bm_head;
		} else {
			bcyl = bzp->bz_cyl;
			btrack = MIN(bzp->bz_head, bmp->bm_head);
		}

		/*
		 * Test base (bcyl, btrack)
		 * remap_cyl so testtrack can find spares.
		 */
		chp = &cyls[bcyl & 1];
		remap_cyl(bcyl, chp);
		while (testtrack(bcyl, btrack, tracksize, npat, chp) == FAIL)
			foundbad = TRUE;

		/*
		 * Test tracks around (bcyl, btrack).
		 * First before then after.
		 */
		trk = btrack;
		do {
			trk = (trk-1+chancfg->zdd_tracks) % chancfg->zdd_tracks;
			doanother = FALSE;
			while (testtrack(bcyl,trk,tracksize,npat,chp) == FAIL) {
				doanother = TRUE;
				foundbad = TRUE;
			}
		} while (doanother == TRUE);

		trk = btrack;
		do {
			trk = (trk + 1) % chancfg->zdd_tracks;
			doanother = FALSE;
			while (testtrack(bcyl,trk,tracksize,npat,chp) == FAIL) {
				doanother = TRUE;
				foundbad = TRUE;
			}
		} while (doanother == TRUE);

		/*
		 * Test same track on adjacent cylinders.
		 * Do not go out of "startcyl, lastcyl" bounds.
		 */
		for (cyl = bcyl - 1; cyl >= startcyl; cyl--) {
			doanother = FALSE;
			chp = &cyls[cyl & 1];
			remap_cyl(cyl, chp);
			while (testtrack(cyl,btrack,tracksize,npat,chp) == FAIL)
				doanother = TRUE;
			if (doanother == FALSE)
				break;		/* done */
			foundbad = TRUE;
		}

		for (cyl = bcyl + 1; cyl <= lastcyl; cyl++) {
			doanother = FALSE;
			chp = &cyls[cyl & 1];
			remap_cyl(cyl, chp);
			while (testtrack(cyl,btrack,tracksize,npat,chp) == FAIL)
				doanother = TRUE;
			if (doanother == FALSE)
				break;		/* done */
			foundbad = TRUE;
		}

		/*
		 * Skip to next track or cylinder.
		 * If the bad block list has changed, then must find
		 * this track in the new bad block list.
		 */
		if (foundbad == TRUE) {
			badblk = TRUE;
			bzp = find_trk(bcyl, btrack);
			foundbad = FALSE;
		}
		for (; bzp < &bbl->bz_bad[bbl->bz_nelem]; bzp++) {
			if (bzp->bz_cyl == bcyl && bzp->bz_head == btrack)
				continue;
			break;
		}
		for (; bmp < &mfg->bm_mfgbad[mfg->bm_nelem]; bmp++) {
			if (bmp->bm_cyl == bcyl && bmp->bm_head == btrack)
				continue;
			break;
		}
	}
	return((badblk == TRUE) ? FAIL : SUCCESS);
}

/*
 * find_trk
 *	find entry in bad block list for given cyl and trk.
 * return pointer to first bad block entry on track. If there are no
 * entries for this track, return pointer to next highest entry or end of list.
 */
static struct bz_bad *
find_trk(cyl, trk)
	register int cyl;
	int trk;
{
	register struct bz_bad *bzp;

	for (bzp = bbl->bz_bad; bzp < &bbl->bz_bad[bbl->bz_nelem]; bzp++) {
		if (bzp->bz_cyl < cyl)
			continue;
		if (bzp->bz_cyl > cyl)
			break;
		if (bzp->bz_head < trk)
			continue;
		if (bzp->bz_head >= trk)
			break;
	}
	return(bzp);
}

/*
 * badsectontrk
 *	how many bad sectors on this track
 */
static int
badsectontrk(cyl, trk)
	register int cyl;
	register int trk;
{
	register struct bz_bad *bzp;
	int ndefect;

	ndefect = 0;
	for (bzp = bbl->bz_bad; bzp < &bbl->bz_bad[bbl->bz_nelem]; bzp++) {
		if (bzp->bz_cyl < cyl)
			continue;
		if (bzp->bz_cyl > cyl)
			break;
		if (bzp->bz_head < trk)
			continue;
		if (bzp->bz_head > trk)
			break;
		/* Found one */
		++ndefect;
	}
	return(ndefect);
}

/*
 * testtrack
 *	write/read track to verify disk integrity.
 *	Also test unused spare(s) on track.
 * Return: FAIL if error found; else SUCCESS.
 */
static
testtrack(cyl, track, tracksize, npat, chp)
	register int	cyl;
	int	track;
	int	tracksize;	/* tracksize in bytes */
	int	npat;		/* pattern (index into ppat array) */
	struct	cyl_hdr *chp;
{
	register struct hdr	*hp;
	register struct track_hdr *tp;
	register int	trkadj;	/* delta for short tracks */
	int	ndefect;	/* number of defects - spare on short track */
	int	sector;
	bool_t	retval;

	retval = SUCCESS;
	if (cyl == ZDD_DDCYL && track == 0) {
		/*
		 * Special case: Do not try to write/read
		 * Special Sectors (zdcdd).
		 */
		testwrite(ZDD_NDDSECTORS, vwbp,
				tracksize - (ZDD_NDDSECTORS*DEV_BSIZE));
		if (testread(ZDD_NDDSECTORS, vrbp,
				tracksize - (ZDD_NDDSECTORS*DEV_BSIZE), vwbp)
				== FAIL) {
			retval = FAIL;
			remap_cyl(cyl, chp);
		}
	} else {
		trkadj = 0;
		if (cyl == ZDD_DDCYL ||
		    cyl >= (chancfg->zdd_cyls - ZDD_NDGNCYL)) {
			/*
			 * Only write good part of short tracks
			 */
			ndefect = badsectontrk(cyl, track);
			ndefect -= chancfg->zdd_spare;
			if (ndefect > 0)
				trkadj = ndefect << DEV_BSHIFT;
		}
		sector = (cyl * nspc) + (track * chancfg->zdd_sectors);
		testwrite(sector, vwbp, tracksize - trkadj);
		if (testread(sector, vrbp, tracksize - trkadj, vwbp) == FAIL) {
			retval = FAIL;
			/*
			 * If there can be a spare, then we must remap
			 * the cylinder so that appropriate spare(s) can
			 * be tested.
			 */
			if (trkadj == 0)
				remap_cyl(cyl, chp);
		}
	}

	/*
	 * Test any spares on this track.
	 */
	tp = &chp->c_trk[track];
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
		if (hp->h_type == ZD_GOODSPARE) {
			if (rwspare(hp, vwbp, vrbp, npat) == FAIL) {
				retval = FAIL;
				/* short cut remap_cyl() */
				hp->h_type = ZD_BADUNUSED;
			}
		}
	}
	return(retval);
}

/*
 * poundtrack
 *	write/read track to verify disk integrity.
 *	Also test unused spare(s) on track.
 *	write uniq pattern to each sector and then read to detect sector 0
 *	like problems.
 *	This routine takes a long time.
 *	The basic stratergy is to write a sector then write a bunch more
 *       then re-read the first sector and check is the data is correct.
 *       this hopes to catch writing or reading from the wrong sector that
 *       would be caused by a "sector 0" problem.
 * Return: FAIL if error found; else SUCCESS.
 */
poundtrack(cyl, track, chp)
	register int	cyl;
	int	track;
	struct	cyl_hdr *chp;
{
	register struct hdr	*hp;
	register struct track_hdr *tp;
	int	ndefect;	/* number of defects - spare on short track */
	int	sectors;
	int	ss;
	bool_t	retval;
	int	i;
	int	old_checkdata;
	int	j,k,m;
	int	tracksize;
	int	i_off;

	if (verbose) {
		printf("%s: Pounding track at (%d,%d). This takes about %d minutes\n", 
			diskname, cyl, track, chancfg->zdd_sectors);
	}

	sigints = 0;
	/*
	 * turn on data checking.
	 */
	old_checkdata = checkdata;
	checkdata = 1;		

	retval = SUCCESS;
	i = 0;
	if (cyl == ZDD_DDCYL && track == 0) {
		retval = FAIL;
		remap_cyl(cyl, chp);
		sectors = chancfg->zdd_sectors;
	} else {
		ndefect = 0;
		if (cyl == ZDD_DDCYL ||
		    cyl >= (chancfg->zdd_cyls - ZDD_NDGNCYL)) {
			/*
			 * Only write good part of short tracks
			 */
			ndefect = badsectontrk(cyl, track);
			ndefect -= chancfg->zdd_spare;
		}
		ss = (cyl * nspc) + (track * chancfg->zdd_sectors);
		sectors = chancfg->zdd_sectors - ndefect;
		tracksize = sectors << DEV_BSHIFT;

		/*
		 * write a whole track 1 sector at a time.
		 */
		for (i = 0; i < sectors; i++) {
			bufset(vwbp+(i*DEV_BSIZE), i+ss, DEV_BSIZE);
			testwrite(ss+i, vwbp+(i*DEV_BSIZE), DEV_BSIZE);
		}

		/*
		 * read back the whole track.
		 */
		if (testread(ss, vrbp, tracksize, vwbp) == FAIL) {
			retval = FAIL;
		}

		/*
		 * now for each sector on the track write it
		 * then write a varying amount more and reverify the original
		 * sector.
		 */

		for(i = 0;(i < sectors) && (retval == SUCCESS); i++ ) {
			i_off = i << DEV_BSHIFT;
			if (sigints) 
				break;
			for(j = 1;(j < sectors-1) && (retval == SUCCESS); j++) {
				testwrite(ss + i, vwbp + i_off, DEV_BSIZE);
				/*
				 * now some blocks.
				 */
				for (m = 1;m < j; m++) {
					k = (i + m)%sectors;
					testwrite(ss + k, 
						vwbp + ((k * DEV_BSIZE)),
							DEV_BSIZE);
				}
				/*
				 * Read the first block we wrote to check
				 * if its the same. (bad sectors headers may
				 * cause dupilcate logical blocks.
				 */
				if (testread(ss + i, vrbp + i_off, DEV_BSIZE, 
						     vwbp + i_off) == FAIL) {
					retval = FAIL;
				}
				/*
				 * read back the whole track.
				 * This tends to read contigous blocks.
				 */
				if (testread(ss, vrbp, tracksize, vwbp) == FAIL) {
					retval = FAIL;
				}
			}
		}
		if (retval == FAIL) {
			/*
			 * If there can be a spare, then we must remap
			 * the cylinder so that appropriate spare(s) can
			 * be tested.
			 */
			if (ndefect > 0)
				remap_cyl(cyl, chp);
		}
	}

	/*
	 * Test any spares on this track.
	 */
	tp = &chp->c_trk[track];
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
		if (hp->h_type == ZD_GOODSPARE) {
			if (rwspare(hp, vwbp, vrbp, 2) == FAIL) {
				retval = FAIL;
				/* short cut remap_cyl() */
				hp->h_type = ZD_BADUNUSED;
			}
		}
	}
	checkdata = old_checkdata;
	if (retval == SUCCESS && verbose) {
		if (sigints) {
			printf("%s: Pounded(%d,%d) - no problems found\n", 
				diskname, cyl, track);
		} else {
			printf("%s: Pounded(%d,%d)interrupted, no problems foundafter %d%% testing\n", 
				diskname, cyl, track, (i*100)/sectors);
		}
	}
	return(retval);
}


/*
 * testwrite
 *	write test pattern to disk.
 */
static
testwrite(sector, cbp, length)
	register int sector;
	caddr_t	cbp;
	int length;
{
	register long sn;
	int	cc;

	lseek(fd, sector << DEV_BSHIFT, 0);
	if ((cc=write(fd, cbp, length)) == length)
		return;

	if (ioctl(fd, ZIOGERR, &errcode) < 0)
		perror("ioctl ZIOGERR failed");

	for (sn = sector; sn < sector + (length >> DEV_BSHIFT); 
	     sn++, cbp+=DEV_BSIZE) {
		lseek(fd, sn << DEV_BSHIFT, 0);
		cc = write(fd, cbp, DEV_BSIZE);
		if ((cc != DEV_BSIZE) && verbose) {
			printf("testwrite: error on write to (%d,%d,%d) %d bytes written, errorcode = %d\n", 

			 (sn / nspc),
			 (sn / chancfg->zdd_sectors) % chancfg->zdd_tracks,
			 sn % chancfg->zdd_sectors,
			 cc, errcode);
		}
	}
}


/*
 * testread
 *	read back previously written track. If checkdata, confirm data
 *	matches that written. Otherwise, rely on hardware to verify via
 *	ECC errors.
 *
 * Return:	SUCCESS - No failures found.
 *		Fail	- At least 1 failure found during verify.
 */
testread(sector, cbp, length, wbp)
	register int sector;
	caddr_t	cbp;		/* buffer to read to */
	int	length;
	caddr_t	wbp;		/* buffer that was written */
{
	int cc;
	int ln;
	register int i;
	register int sn;
	int cyl;
	int trk;
	int sect;
	int ok;
	int errs;
	int s;

	ok = SUCCESS;
	errs = 0;
again:
	lseek(fd, sector << DEV_BSHIFT, 0);
	if (!checkdata) {
		if ((ln = read(fd, cbp, length)) == length)
			return(SUCCESS);
	} else {
		bzero((caddr_t)cbp, (uint)length);

		if ((ln = read(fd, cbp, length)) == length) {
			/*
			 * Read all data. Check if valid.
			 */
			if (checkbuf(cbp, &ln, wbp)) {
				cyl = sector / nspc;
				trk = (sector % nspc) / chancfg->zdd_sectors;
				sect = (sector % nspc) % chancfg->zdd_sectors;

				if (ioctl(fd, ZIOGERR, &errcode) < 0)
					perror("ioctl ZIOGERR failed");
				fflush(stdout);
				fprintf(stderr, "%s: Data bad at (%d,%d,%d+%d)\n",
					diskname,
					cyl, trk, sect + (ln >> DEV_BSHIFT), 
					(ln % DEV_BSHIFT));
				recorderror(sector + (ln >> DEV_BSHIFT),
						errcode, (struct hdr *)NULL);
				return(FAIL);
			}
			return(SUCCESS);
		}
	}
	cyl = sector / nspc;
	trk = (sector % nspc) / chancfg->zdd_sectors;
	sect = (sector % nspc) % chancfg->zdd_sectors;

	if (ioctl(fd, ZIOGERR, &errcode) < 0)
		perror("ioctl ZIOGERR failed");
	if (debug)
		printf("testread: error at (%d,%d,) count=%d\n",cyl,trk,ln);

	/*
	 * Find the problem sector(s).
	 */
	s = 0;
	for (sn = sector; sn < sector + (length >> DEV_BSHIFT); sn++) {
		for (i = ZDNBEATS; i > 0; i--) {
			lseek(fd, sn << DEV_BSHIFT, 0);
			if (checkdata)
				bzero(cbp+s, DEV_BSIZE);
			cc = read(fd, cbp+s, DEV_BSIZE);
			if (cc == DEV_BSIZE) {
				if (checkdata && checkbuf(cbp+s, &cc, wbp+s)) {
					if (ioctl(fd, ZIOGERR, &errcode) < 0)
						perror("ioctl ZIOGERR failed");
					fflush(stdout);
					fprintf(stderr,
					"%s: Data bad at (%d,%d,%d+%d)\n",
						diskname,
						cyl, trk, sect+sn-sector, cc);
					recorderror(sn, errcode,
							(struct hdr *)NULL);
					ok = FAIL;
					break;
				}
			} else {
				if (ioctl(fd, ZIOGERR, &errcode) < 0)
					perror("ioctl ZIOGERR failed");
				fflush(stdout);
				fprintf(stderr, "%s: read error at (%d,%d,%d)\n",
					diskname, cyl, trk, sect+sn-sector);
				recorderror(sn, errcode, (struct hdr *)NULL);
				ok = FAIL;
				break;
			}
		}
		s += DEV_BSIZE;
	}
	if (ok) {
		/* data was ok second time ??? */
		fflush(stdout);
		if (ln != -1) {
			sn = sector + (ln >> DEV_BSHIFT );
			fprintf(stderr, "Sector %d, read error\n", sn);
			recorderror(sn, errcode, (struct hdr *)NULL);
			return( FAIL );
		} else if ( length == DEV_BSIZE ) {
			fprintf(stderr, "Sector %d, read error\n", sector);
			recorderror(sector, errcode, (struct hdr *)NULL);
			return( FAIL );
		}
		fprintf(stderr, "Sector %d-%d, read errors\n", sector,
			sector + (length >> DEV_BSHIFT));
		if (++errs < 5)
			goto again;
	}
	return(ok);
}

/*
 * rwspare
 *	If checkdata, confirm data matches that written.
 *	Otherwise, rely on hardware to verify via ECC errors.
 *
 * Return:	SUCCESS - No failures found.
 *		Fail	- At least 1 failure found during verify.
 */
static
rwspare(hp, wcbp, rcbp)
	register struct hdr	*hp;
	caddr_t	wcbp;			/* buffer with pattern to write */
	caddr_t	rcbp;			/* buffer with pattern to read  */
{
	int	cc;
	struct cb lcb;			/* cb for ioctl */

	bzero((caddr_t)&lcb, sizeof(struct cb));
	lcb.cb_cmd = ZDC_WRITE;
	lcb.cb_cyl = hp->h_cyl;
	lcb.cb_head = hp->h_head;
	lcb.cb_sect = hp->h_sect;
	lcb.cb_addr = (ulong)wcbp;
	lcb.cb_count = DEV_BSIZE;
	lcb.cb_mod = 0;
	lcb.cb_iovec = 0;
	/* ignore write errors */
	(void)ioctl(fd, ZIOCBCMD, (char *)&lcb);

	/*
	 * Now read back the written data.
	 */
	if (checkdata)
		bzero(rcbp, DEV_BSIZE);
	lcb.cb_cmd = ZDC_READ;
	lcb.cb_cyl = hp->h_cyl;
	lcb.cb_head = hp->h_head;
	lcb.cb_sect = hp->h_sect;
	lcb.cb_addr = (ulong)rcbp;
	lcb.cb_count = DEV_BSIZE;
	lcb.cb_mod = 0;
	lcb.cb_iovec = 0;
	errcode = 0;
	if (ioctl(fd, ZIOCBCMD, (char *)&lcb) < 0) {
		fflush(stdout);
		if (ioctl(fd, ZIOGERR, &errcode) < 0)
			perror("ioctl ZIOGERR failed");
		fprintf(stderr, "Read error at (%d, %d, %d)\n", hp->h_cyl,
			hp->h_head, hp->h_sect);
		recorderror((int)hp->h_sect, errcode, hp);
		return(FAIL);
	}

	cc = DEV_BSIZE;

	if (checkdata && checkbuf(rcbp, &cc, wcbp)) {
		fflush(stdout);
		fprintf(stderr,
		"Data Bad at (%d, %d, %d+%d).\n", hp->h_cyl, hp->h_head,
			hp->h_sect, cc);
		recorderror((int)hp->h_sect, 0, hp);
		return(FAIL);
	}
	return(SUCCESS);
}

/*
 * testspares
 *	Find and write/read the spare sectors in a cylinder.
 *	rwspare does actual dirty work.
 *
 * Return:	SUCCESS - No failures found.
 *		Fail	- At least 1 failure found during verify.
 */
static
testspares(cyl, wcbp, rcbp, npat)
	int	cyl;
	caddr_t	wcbp;		/* buffer with pattern to write */
	caddr_t	rcbp;		/* buffer with pattern to read  */
	int	npat;			/* Test pattern to check */
{
	register struct hdr	*hp;
	register struct track_hdr *tp;
	struct	cyl_hdr *chp;
	int	retval;

	chp = &cyls[cyl & 1];
	retval = SUCCESS;

	remap_cyl(cyl, chp);
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
			if (hp->h_type == ZD_GOODSPARE) {
				if (rwspare(hp, wcbp, rcbp, npat) == FAIL)
					retval = FAIL;
			}
		}
	}
	return(retval);
}

/*
 * recorderror
 *	record the error (from errno) and add sector to bad block list.
 *
 * if spare sector is in error, hp points to hdr structure where cyl, head
 * and sector are extracted. If USED sector hp == NULL.
 */
static
recorderror(sn, errcode, hp)
	register int	sn;		/* sector number */
	int	errcode;
	struct	hdr	*hp;
{
	register struct bz_bad *bzp;
	int cyl;
	int trk;

	(void)ioctl(fd, ZIONSEVERE, (char *)NULL);

	fflush(stdout);
	fprintf(stderr, "Zdc error %d on read.\n", errcode);
	cnterr.all++;

	switch (errcode) {

	case ZDC_ECC:
		cnterr.ecc++;
		break;

	case ZDC_HDR_ECC:
		cnterr.hdrecc++;
		break;

	case ZDC_SNF:
		cnterr.bse++;
		break;

	case ZDC_SO:
		cnterr.so++;
		break;

	case ZDC_ILLCHS:
	case ZDC_BADDRV:
	case ZDC_ACCERR:
		cnterr.misc++;
		break;

	default:			/* hard error */
		cnterr.her++;
		break;

	}

	/*
	 * Don't mess around - add bad spot to bad block list.
	 */
	if (errcode == ZDC_HDR_ECC || errcode == ZDC_SNF || errcode == ZDC_SO) 
		addlist[0].al_type = BZ_BADHEAD;
	else
		addlist[0].al_type = BZ_BADDATA;
	if (hp != (struct hdr *)NULL) {
		/*
		 * addbad the spare sector.
		 */
		addlist[0].al_addr.da_cyl  = hp->h_cyl;
		addlist[0].al_addr.da_head = hp->h_head;
		addlist[0].al_addr.da_sect = hp->h_sect;
		doaddbad(1, Z_VERIFY);
		(void)ioctl(fd, ZIOSEVERE, (char *)NULL);
		return;
	}
	cyl = sn / nspc;
	sn %= nspc;
	trk = sn / chancfg->zdd_sectors;
	sn %= chancfg->zdd_sectors;

	/*
	 * Search bad block list to determine if error is really in
	 * a replacement sector.
	 */
	bzp = find_bbl_entry(bbl, cyl, (unchar)trk, (unchar)sn);
	/*
	 * If not found in list enter disk address.
	 * If AUTOREVECT sector header now is bad, AUTOREVECT can
	 * no longer be done.
	 */
	if (bzp == (struct bz_bad *)NULL
	||  (bzp->bz_rtype == BZ_AUTOREVECT && errcode == ZDC_HDR_ECC)) {
		addlist[0].al_addr.da_cyl = cyl;
		addlist[0].al_addr.da_head = trk;
		addlist[0].al_addr.da_sect = sn;
	} else if (bzp->bz_rtype == BZ_AUTOREVECT) {
		/*
		 * Replacement for BZ_AUTOREVECT entry is bad.
		 */
		addlist[0].al_addr.da_cyl = cyl;
		addlist[0].al_addr.da_head = trk;
		addlist[0].al_addr.da_sect = sn | ZD_AUTOBIT;
	} else {
		/*
		 * Replacement of BZ_SNF type entry is bad.
		 */
		addlist[0].al_addr = bzp->bz_rpladdr;
	}
	doaddbad(1, Z_VERIFY);
	(void)ioctl(fd, ZIOSEVERE, (char *)NULL);
}

/*
 * bufinit
 *	Initialize the buffer with the requested pattern.
 */
static
bufinit(bp, npat, size)
	char *bp;
	int npat;
	int size;
{
	register long *lp;
	register long *end;
	register ptrn;

	size = (size + sizeof(long) -1) / sizeof(long);
	lp = (long *)bp;
	end = lp + size;
	ptrn = ppat[npat];

	if (verbose)
		printf("Write pattern 0x%x\n", ptrn);
	
	while (lp < end)
		*lp++ = ptrn;
}

/*
 * bufset
 *	Initialize the buffer with the requested pattern.
 */
static
bufset(bp, ptrn, size)
	char *bp;
	register ptrn;
	int size;
{
	register long *lp;
	register long *end;

	size = (size + sizeof(long) -1) / sizeof(long);
	lp = (long *)bp;
	end = lp + size;
	while (lp < end)
		*lp++ = ptrn;
}


/*
 * checkbuf
 *	compare buffer against expected pattern.
 *
 * Return: number of mismatches found. Size is modified to return character
 *	   position of first mismatch.
 */
static
checkbuf(rbp, size, wbp)
	char	*rbp;		/* read buffer */
	int	*size;
	char	*wbp;		/* write buffer */
{
	register long *rlp;
	register long *wlp;
	register nlongs;
	register nerrs = 0;

	nlongs = (*size + sizeof(long) -1) / sizeof(long);
	rlp = (long *)rbp;
	wlp = (long *)wbp;
	while (nlongs--) {
		if (*rlp != *wlp) {
			if (debug) {
				fflush(stdout);
				fprintf(stderr, "Data error 0x%x != 0x%x\n",
							*rlp, *wlp);
			}
			if (nerrs == 0)
				*size = (char *)rlp - rbp;
			nerrs++;
		}
		wlp++;
		rlp++;
	}
	return(nerrs);
}

/*
 * fail_drive
 * 	Warn of catastrophic failure to correct 
 *	a bad sector header.  Note in particular 
 *	if the sector was a runt sector which
 *	cannot be adjusted at all.
 */
void
fail_drive(cyl, head, sect)
	ushort cyl;
	u_char head;
	int sect;
{
	fflush(stdout);
	fprintf(stderr, "WARNING: Uncorrectable hdr-ecc ");
	if (sect < 0) 
		fprintf(stderr, "problems on track (%d, %d, ).\n", cyl, head);
	else
		fprintf(stderr, "problems with sector (%d, %d, %d).\n", 
			cyl, head, sect);

	if (sect == n_hdrs-1 && n_hdrs != totspt)
		fprintf(stderr, "WARNING: Defective runt sector.\n");

	fprintf(stderr, "WARNING: *** Defective drive or drive channel ***.\n");

	exit(1);	/* This is non-correctable */
}

/*
 * adjust_header
 *	Attempt to correct for a defect in the sector header
 *	by its track's suspect list id and the physical sector
 * 	address on that track.  Assume it is to be marked as 
 *	ZD_BADUNUSED.  The idea is to temporarily dummy
 *	up the channel configuration so that it original
 *	header area is skipped and then write what appears
 *	to be sync bytes, the header, etc, into what had
 *	been the data region of the original sector.

 *	The header suspect list will be used to track the
 *	'adjustment' last made to the header and the next 
 *	one will be used this time.  'position' indicates where 
 *	in the data area to dummy up the new header.  'pattern'
 *	indicates the replacement sync byte pattern to use during 
 *	this operation. The adjustment must be in the range 
 *	0..2*ZDNHDRSHFT-1 and will be divided by 2 and multiplied 
 *	by ZDHDRBSHFT to determine the actual start byte position.  
 *	I.e, there are ZDNHDRSHFT possible placements for the header 
 *	in the data region.  If adjustment is even, then the primary 
 *	sync value is used to reformat the sector.  Otherwise the 
 *	alternate value is used.  Assume that if position is invalid, 
 *	the caller has exhausted all other possibilities and fail the 
 *	drive.  Likewise, fail drives that require fixes to a runt 
 *	sector since they have truncated data areas, would require 
 *	special handling, and should occur very seldom.
 *	
 *	Assume you will never return from fail_drive().
 */
adjust_header(suspect_id, sect)
	int suspect_id;
	int sect;
{
	struct zdcdd save_cfg;
	struct cb cb;
	struct hdr_suspect *sp;
	ushort cyl;
	u_char head;
	int error, i, howfar, position;
        struct hdr *hp;                 /* Expected header of interest */
        register u_char *dp;              /* Data area of same sector */
	static	u_char *buf;		/* aligned data for ZDC_WHDR_WDATA */
	u_char pattern;

	if (suspect_id < 0 || suspect_id >= num_suspects) {
                fflush(stdout);
                fprintf(stderr, "adjust_header: internal error...exiting.\n");
		exit(1);
	}
	sp = &suspect[suspect_id];
	cyl = sp->addr.da_cyl;
	head = sp->addr.da_head;

	if (sect == n_hdrs-1 && n_hdrs != totspt) {
		/* 
		 * Cannot fix a defective runt sector.
		 * Not enough data area to do so.
		 */
		fail_drive(cyl, head, sect);
	} 

	position = sp->adjustment[sect] / 2;
	pattern = sp->adjustment[sect] ? ZDALTPAT : ZDSYNCPAT;
	if (verbose || debug) {
		printf("Adjust defective header of sector at physical ");
		printf("(%d, %d, %d) to position %d,\n",
			cyl, head, sect, position);
		if (sp->adjustment[sect] & 1)
			printf("    using alternate sync pattern.\n");
		else
			printf("    using primary sync pattern.\n");

	}

	if (position >= ZDNHDRSHFT) {
		/* 
		 * Cannot fix - this request exceeds 
		 * available adjustment locations.
		 */
		fail_drive(cyl, head, sect);
	}

	/*
	 * Reformat the sector so the real header
	 * area is ignored and a fake one is built
	 * in the data area.  It will be shifted
	 * over trying to skip the defect.
	 * 
	 * First allocate and clear an aligned buffer.
	 */
        if (!buf) {
                buf = (u_char *)
			MALLOC_ALIGN(CNTMULT+DEV_BSIZE, types[disk].align);
                if (!buf) {
                        fflush(stdout);
                        fprintf(stderr, "adjust_header: malloc failure\n");
                        exit(1);
                }
        }
	bzero(buf, CNTMULT + DEV_BSIZE);	/* Clear the data area */

	/*
	 * Locate where the fake header should 
	 * start and write the sync bytes and 
	 * bad/unused header values.  Leave ecc
	 * bytes zero, they are not relavent.
	 * Note that the real header is occupies
	 * the beginning of 'buf' and is to be
	 * left zeroed.
	 */
	dp = buf + sizeof(struct hdr);		/* skip past original header */
	howfar = position * ZDHDRBSHFT;
	for (i = 0; i < howfar; i++, dp++)	/* header preamble */
		*dp = chancfg->zdd_ddc_regs.dr_hpr_pat;

	for (i = 0; i < chancfg->zdd_ddc_regs.dr_hs1_bc; i++, dp++)
		*dp = chancfg->zdd_ddc_regs.dr_hs1_pat; /* header sync 1*/
	for (i = 0; i < chancfg->zdd_ddc_regs.dr_hs2_bc; i++, dp++)
		*dp = chancfg->zdd_ddc_regs.dr_hs2_pat; /* header sync 2*/

	hp = (struct hdr *)dp;			/* the dummy header */
	hp->h_type = ZD_BADUNUSED;
        hp->h_cyl = ZD_BUCYL;
        hp->h_head = ZD_BUHEAD;
        hp->h_sect = ZD_INVALSECT;
	dp += sizeof(struct hdr) + HDR_CRC_BC;

	/*
	 * Next dummy up the header postamble,
	 * data preamble, and data sync fields.
	 */
	for (i = 0; i < chancfg->zdd_hpo_fmt_bc; i++, dp++)
		*dp = chancfg->zdd_ddc_regs.dr_hpo_pat;
	for (i = 0; i < chancfg->zdd_ddc_regs.dr_dpr_wr_bc; i++, dp++)
		*dp = chancfg->zdd_ddc_regs.dr_dpr_pat;
	for (i = 0; i < chancfg->zdd_ddc_regs.dr_ds1_bc; i++, dp++)
		*dp = chancfg->zdd_ddc_regs.dr_ds1_pat;
	for (i = 0; i < chancfg->zdd_ddc_regs.dr_ds2_bc; i++, dp++)
		*dp = chancfg->zdd_ddc_regs.dr_ds2_pat;

	/*
 	 * Finally, make the rest of the data area 
	 * look like the data postamble.
	 */
	for (; dp < (buf + CNTMULT + DEV_BSIZE); dp++)
		*dp = chancfg->zdd_ddc_regs.dr_dpo_pat;

	/*
	 * Reserve the channel to temporarily change
 	 * its configuration, so the old header location
	 * is effectively skipped while reformatting
	 * the specified sector.
	 */
        if (ioctl(fd, ZIORESERVE, 0) < 0) {
        	fflush(stdout);
        	perror("ZIORESERVE failed while fixing header");
		fprintf(stderr, 
		  "WARNING: Must be the only open drive on this dcc channel.\n");
		fprintf(stderr, 
		  "WARNING: Format failed while fixing a defective header.\n");
		exit(1);
	}

	/*
	 * Alter the channel configuration so that the
	 * old configuration would not see the original 
	 * header area when a corrupt sector is dummied
	 * up in the data area.
	 */ 
	save_cfg = *chancfg;
	chancfg->zdd_ddc_regs.dr_hs2_pat = pattern;
	chancfg->zdd_ddc_regs.dr_ds2_pat = pattern;

	bzero((caddr_t)&cb, sizeof(struct cb)); 
	cb.cb_cmd = ZDC_SET_CHANCFG;
        cb.cb_addr = (ulong)chancfg;
        cb.cb_count = sizeof(struct zdcdd);
        cb.cb_iovec = 0;
        if (error = ioctl(fd, ZIOCBCMD, (char *)&cb) < 0) {
        	fflush(stdout);
        	perror("ZIOCBCMD ZDC_SET_CHANCFG failed while fixing header");
		goto release;
	}

	/* 
	 * Now reformat the sector using
	 * the special channel configuration.
	 */
	bzero((caddr_t)&cb, sizeof(struct cb)); 
  	cb.cb_cmd = ZDC_WHDR_WDATA;
  	cb.cb_cyl = cyl;
  	cb.cb_head = head;
  	cb.cb_sect = sect;
  	cb.cb_addr = (ulong)buf;
  	cb.cb_count = CNTMULT + DEV_BSIZE;
  	cb.cb_iovec = 0;
        if (error = ioctl(fd, ZIOCBCMD, (char *)&cb) < 0) {
        	fflush(stdout);
        	perror("ZIOCBCMD ZDC_WHDR_WDATA failed while fixing header");
	}
	sp->adjustment[sect]++;

	/*
	 * Now restore the original channel configuration.
	 * So its business as usual.
	 */
	*chancfg = save_cfg;
	bzero((caddr_t)&cb, sizeof(struct cb)); 
	cb.cb_cmd = ZDC_SET_CHANCFG;
        cb.cb_addr = (ulong)chancfg;
        cb.cb_count = sizeof(struct zdcdd);
        cb.cb_iovec = 0;
        if (ioctl(fd, ZIOCBCMD, (char *)&cb) < 0) {
        	fflush(stdout);
        	perror("ZIOCBCMD ZDC_SET_CHANCFG failed during restore");
		fprintf(stderr, 
		  "WARNING: This DCC channel's configuration is corrupt.\n");
		fprintf(stderr, 
		  "WARNING: Reboot required to correct this problem.\n");
		exit(1);
	}

release:
        if (ioctl(fd, ZIORELEASE, 0) < 0) {
        	fflush(stdout);
        	perror("ZIORELEASE failed while fixing header");
		fprintf(stderr, 
		  "WARNING: This could lock this DCC channel indefinitely.\n");
		fprintf(stderr, 
		  "WARNING: Reboot may be required to correct this problem.\n");
		exit(1);
	} 
	if (error)
		exit(1);
}

/*
 * readjust_headers
 *	Re-correct any sectors from the specified 
 *	cylinder that were previously corrected.
 *	This is necessary after a cylinder is
 *	reformated as a result of an addbad operation
 *	once any adjustments have been made, in an
 *	attempt to preserve the previous correction.
 *
 *	Scan the suspect list for any tracks that
 *	are within this cylinder and then look for
 *	those which were already adjusted - their
 *	adjustment values are greater than zero.  Since
 * 	the adjustment value indicates the placement
 *	for then next adjustment and we desire the
 *	previous adjustment, decrement it prior to
 *	calling adjust_header(), which will reformat
 *	the sector and increment the adjustment again.
 */
readjust_headers(cyl)
	ushort cyl;
{
	int suspect_id;
	int sect;
	struct hdr_suspect *sp;

	for (suspect_id = 0, sp = suspect; 
	     suspect_id < num_suspects; suspect_id++, sp++) {
		
		if (sp->addr.da_cyl == cyl) {
			for (sect = 0; sect < n_hdrs; sect++) {
				if (sp->adjustment[sect] > 0) {
					sp->adjustment[sect]--;
					adjust_header(suspect_id, sect);
				}
			}
		}
	}
}

/*
 * add_suspect
 *	Scan the defective header suspect list 
 *	linearly and append the track located at 
 *	cyl,head to list, if not in the list.
 *	The list will eventually be used by the
 *	header verification pass.
 *
 *	Return the index of the track in the list.
 *	If the list is full return -1;
 */
int
add_suspect(cyl, head)
	register ushort cyl;
	register u_char head;
{
	register int i;
	register struct hdr_suspect *sp;

	for (sp = suspect, i = 0; i < num_suspects; i++, sp++)
		if (sp->addr.da_cyl == cyl && sp->addr.da_head == head) 
			return(i);	/* already in the list */

	if (num_suspects == max_suspects) {
		fflush(stdout);
		fprintf(stderr, 
		"Cannot test sector headers on cyl %d, head %d - list full.\n",
			    cyl, head);
		return(-1);
	}

	if (debug || verbose) 
	    printf("Adding track (%d, %d) to header verify list (entry %d).\n",
			cyl, head, num_suspects);

	sp->addr.da_cyl = cyl;
	sp->addr.da_head = head;
	sp->adjustment = (short *) malloc(n_hdrs * sizeof(short));
	if (!sp->adjustment) {
		fflush(stdout);
		fprintf(stderr, "add_suspect: malloc failure - exiting.\n");
		exit(1);
	}
	bzero(sp->adjustment, n_hdrs * sizeof(short));
	return(num_suspects++);
}

/*
 * gen_bbl_suspects
 *	Scan the bad block list and add potential
 *	tracks that are candidates for having 
 *	HDR-ECC errors to a suspect list to be
 *	verified and corrected.  Assume the bad
 *	block list is sorted in ascending order
 *	by cylinder then head.  The add_suspect()
 *	function will ensure that tracks are only
 *	recorded once.
 *
 *	Tracks having known defects in the sector 
 *	header are suspects.  Also, tracks with
 *	UMFG_DPT defects of any type are suspected
 *	since manufactures don't report more than
 *	that per track, although they may exist.
 */
void
gen_bbl_suspects()
{
	register struct bz_bad *bzp = bbl->bz_bad;
	ushort cyl = bzp->bz_cyl;
	u_char head = bzp->bz_head;
	int def_on_track = 0;
	int skip = 0;

	if (debug > 1) {
		printf("Scan bbl for additions to header verify list.  ");
		printf("(#entries = %d).\n", num_suspects);
	}

	for (; bzp < &bbl->bz_bad[bbl->bz_nelem]; bzp++) {
		if (bzp->bz_cyl != cyl || bzp->bz_head != head) {
			cyl = bzp->bz_cyl; 
			head = bzp->bz_head; 
			def_on_track = skip = 0;
		}

		if (!skip && (bzp->bz_ftype == BZ_BADHEAD 
		||  	      ++def_on_track == UMFG_DPT)) {
			if (add_suspect(cyl, head) < 0)
				break;	/* The list is full - quit */
			skip = 1;		
		} 
	}

	if (debug > 1) {
		printf("Header verify list updated from bbl.  ");
		printf("(#entries = %d).\n", num_suspects);
	}
}

/*
 * read_hdrs
 *	Execute the READ_HDRS ioctl on the track
 *	specified by the cyl and head arguments.
 *	The headers are stored into the array of
 *	type 'union headers" addressed by the 
 *	global variable 'headers'.
 */
#define READ_HDR_TRIES  4

int
read_hdrs(cyl, head)
	ushort	cyl;
	u_char	head;
{
	int retry, status, size;
	struct cb lcb;

        for (retry = READ_HDR_TRIES, status = 1; status != 0 && retry; retry--) {
		bzero((caddr_t)&lcb, sizeof(struct cb));
		size = ROUNDUP(sizeof(union headers) * n_hdrs, CNTMULT);

		lcb.cb_cmd = ZDC_READ_HDRS;
		lcb.cb_cyl = cyl;
		lcb.cb_addr = (ulong)headers;
		lcb.cb_head = head;
		lcb.cb_count = size;
		lcb.cb_iovec = 0;
		if ((status = ioctl(fd, ZIOCBCMD, (char *)&lcb)) == -1){
			if (ioctl(fd, ZIOGERR, &errcode) < 0) {
				fprintf(stderr, "%s: ", diskname);
				perror("ioctl ZIOGERR failed");
			}
                }
        }

	if (status == -1) {
		if (verbose && errcode != ZDC_NDS) {
			fprintf(stderr, "%s: (%d,%d) ", diskname, cyl, head);
			perror("ZDC_READ_HDRS ioctl error");
			fprintf(stderr, "%s: errcode 0x%x\n", diskname, errcode);
		}
		return(1);
	}
	return(0);
}

/*
 * validate_hdrs
 *	Perform a validation pass of the track specified. Verify 
 *	the sector headers are correct and/or attempt to correct 
 *	invalid ones.  Each track must be successfully read 
 *	ZDNBEATS consecutive times without corrections to 
 *	be accepted.  Returns FAIL if corrective action is taken.
 *	Returns SUCCESS otherwise.
 *
 *	Invalid headers not marked as ZD_BADUNUSED will be
 *	added to the bad block list and correction attempted
 *	by the doaddbad() function.  
 *
 *	Corrective action for invalid headers marked as 
 *	ZD_BADUNUSED headers that miscompare such that they
 *	would result in bogus hdr-ecc errors will be shifted
 *	into the data area and retested.  Subsequent detected
 *	failures will be shifted ZDHDRSHFT bytes further, up
 *	to ZDNHDRSHFT times.  adjust_headers() will determine 
 *	the position to place the header next.
 */	
validate_hdrs(suspect_id)
	int suspect_id;
{
	ushort cyl = suspect[suspect_id].addr.da_cyl;
	u_char head = suspect[suspect_id].addr.da_head;
        register struct hdr *hp, *rhp;
	int limit = chancfg->zdd_sectors * ZDNBEATS * ZDNHDRSHFT;
        struct  cyl_hdr *chp = &cyls[cyl & 1];
        struct track_hdr *tp = &chp->c_trk[head];
	int retval = SUCCESS, ndefects = 0;
	int i, j, pass;
	bool_t changes_made = 0;
	int	suspect_id;
	int	tracksize;
	int	npat;

	/*
	 * Skip over DD cylinder, it is
	 * checked specially by format_dd()
	 */
	if (cyl == ZDD_DDCYL)
		return (retval);

	tracksize = chancfg->zdd_sectors << DEV_BSHIFT;

	/*
	 * We have to map out the entire
	 * cylinder to test this track.
	 */
        remap_cyl(cyl, chp);

	/*
	 * Verify that the track can be correctly read 
	 * ZDNBEATS times sequentially without error.
	 * Make adjustments as necessary, but there
	 * must be a limit to prevent an infinite loop.
	 */
	for (pass = 0, i = ZDNBEATS; i > 0; i--, pass++, changes_made = 0) {
		if (pass > limit) {
			fail_drive(cyl, head, -1);
			exit(1); 	/* fail_drive() should never return */
		}
		
		/*
		 * Re-read the headers for verification.
		 * Headers returned in global var 'headers'.
		 */
		if (read_hdrs(cyl, head)) {
			/*
			 * read_hdrs will verify data ECC which might fail
			 * on sectors already marked bad so change
			 * the stratagry to pound on the track to
			 * verify that each sector is real and can be read.
			 */

			if (errcode == ZDC_NDS) {
				if (verbose) {
					printf("%s: Possible problem at (%d,%d) - trying data sync fix\n", 
						diskname, cyl, head);

				}
				if ((i = fix_data_sync(cyl, head, tp)) != -1) {
					changes_made++;
					/*
					 * Change the type of the defect to
					 * bad header.
					 */
					if (add_addlist(cyl, head, i,
						      BZ_BADHEAD, &ndefects)) {
						break;
					}
					goto ok;
				}
			}

			if ((retval = poundtrack(cyl, head, chp)) == FAIL) {
				fprintf(stderr, "%s: Bad headers on (%d,%d)\n", 
					diskname, cyl, head);
			}

			if (debug) {
				printf("Validate_headers(%d,%d) done, %d passes.\n",
					cyl, head, pass);
			}
			return (retval);
		} 

		/*
		 * Defective sectors not in the bad 
		 * block list must be added to it and 
		 * the cylinder reformated.  Try to 
		 * do them all at once.  Since the
		 * track may now look different, get
		 * an updated copy of its layout.
		 */
		for (j = 0, hp = &tp->t_hdr[0]; j < totspt; j++, hp++) {
			rhp = &headers[j].hdr;
			if (hp->h_type != ZD_BADUNUSED && !HDR_CMPR(hp, rhp)) {
				if (verbose) {
					printf("Defective hdr at (%d,%d,%d) -",
					     hp->h_cyl, hp->h_head, hp->h_sect);
					printf("adding to bad block list.\n");
					if (debug) {
						dump_hdr(hp, rhp, j);
							dump_hdrs();
					}
				}
				changes_made++;
				if (add_addlist(hp->h_cyl, hp->h_head,
						hp->h_sect, BZ_BADHEAD, 
						&ndefects)) {
					break;
				}
			}
		}
		if (changes_made) {
ok:
			if (debug) {
				printf("(pass %d) ", pass);
				printf("Validate_headers(%d, %d): addbad %d.\n",
					cyl, head, ndefects);
			}

			/*
			 * Add badspots to bad block list,
			 * pick up new cylinder layout, note
			 * the failure, and start over looking
			 * for good passes.
			 */
                	(void)ioctl(fd, ZIONSEVERE, (char *)NULL);
			cnterr.all += ndefects;
			cnterr.hdrecc += ndefects;
			doaddbad(ndefects, Z_VERIFY);
                	(void)ioctl(fd, ZIOSEVERE, (char *)NULL);

			if (debug) { 
				printf("(pass %d) ", pass);
				printf("Validate_headers(%d, %d): remap_cyl.\n",
					cyl, head);
			}
        		remap_cyl(cyl, chp);
			ndefects = 0;
			retval = FAIL;	
		    	i = ZDNBEATS + 1;

			if (debug) {
				printf("(pass %d) ", pass);
				printf("Validate_headers(%d, %d): continue.\n",
					cyl, head);
			}
			continue;
		}
				
		/*
		 * Defective sectors marked ZD_BADUNUSED
		 * must be shifted over to correct them.
		 * Adjust as many as possible prior to
		 * rereading the track.  
		 *
		 * The adjustment may not be attempted
		 * if too little room is left or the
		 * sectors is a runt sector.  If they
		 * cannot be corrected give up on this
		 * track and abort the format.
		 */
		for (j = 0, hp = &tp->t_hdr[0]; j < n_hdrs; j++, hp++) {
			rhp = &headers[j].hdr;
			if (hp->h_type == ZD_BADUNUSED && !BUHDR_CMPR(rhp)) {
				cnterr.all++;
				cnterr.hdrecc++;
				
				/*
				 * Attempt to correct the defective header
				 * by moving it over into an otherwise 
				 * unused area.
				 */
				if (debug) {
					dump_hdr(hp, rhp, j);
					dump_hdrs();
				}
				if ((suspect_id = add_suspect(cyl, head)) == -1)
					break; /* the list is full */
				adjust_header(suspect_id, j);
				changes_made++;
			}
		}
		if (changes_made) {
			/*
			 * Note the failure and start over 
			 * looking for good passes.
			 */
			retval = FAIL;
		    	i = ZDNBEATS + 1;
		}
	}

	if (debug) {
		printf("Validate_headers(%d,%d) done, %d passes.\n",
			cyl, head, pass);
	}
	return (retval);	/* They either read back or we gave up */
}

/*
 * dohdrpass
 *	Perform a validation pass of tracks suspected of
 *	containing bad sector headers.  The target tracks
 *	are those that failed a header comparison during
 *	the format operation and tracks in the bad block
 *	list which contain header defects or have more
 *	than 3 defects listed (manufacturers limit).
 *
 *	validate_hdrs() is invoked to test and attempt to
 *	correct tracks individually.  It 'addbad's new 
 *	defects it locates and corrects any ZD_BADUNUSED 
 *	headers that might result in bogus hdr-ecc errors.  
 *	validate_hdrs() returns FAIL if corrective action
 *	had to be taken and aborts the formatter if it
 *	could not correct the track.
 */	
dohdrpass()
{
	register int s = 0;
	int  retval = SUCCESS;
	extern void gen_bbl_suspects();

	gen_bbl_suspects();		/* Add suspects using bad block list */

	for (; s < num_suspects; s++)
	    if (validate_hdrs(s) == FAIL) {
		if (verbose) {
		    fflush(stdout);
		    fprintf(stderr, 
		       "%s: header pass %d: corrections made to cyl %d, head %d.\n",
			diskname,
			hdrpassnum, suspect[s].addr.da_cyl, 
			suspect[s].addr.da_head);
		}
		retval = FAIL;
	    }
	return (retval);
}

/*
 * dump_hdrs
 *	Dump out the track worth of headers which are
 *	buffered up in the global array of 'headers'.
 */
dump_hdrs()
{
	struct hdr *rhp;
	int j;

	for (j = 0; j < n_hdrs; j++) {
		rhp = &headers[j].hdr;
		if ((j % 4) == 0) 
			printf("%02x:", j);
		printf("  %02x %02x %04x %02x %02x", rhp->h_type, rhp->h_flag, 
			rhp->h_cyl, rhp->h_head, rhp->h_sect);
		if ((j % 4) == 3) 
			printf("\n");
	}
	if ((j % 4) != 0) 
		printf("\n");
}

/*
 * dump_hdr
 *	Dump out the expected and read header
 */
dump_hdr(hp, rhp, sect)
        struct hdr *hp;
        struct hdr *rhp;
	int	sect;
{
	printf("  Expect %02x %02x %04x %02x %02x, ",
		hp->h_type, hp->h_flag, hp->h_cyl, hp->h_head, hp->h_sect);
	printf("read %02x %02x %04x %02x %02x @ physical 0x%x\n",
		rhp->h_type, rhp->h_flag, rhp->h_cyl, 
		rhp->h_head, rhp->h_sect, sect) ;
}
