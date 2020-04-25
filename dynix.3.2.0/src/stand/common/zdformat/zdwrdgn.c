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

#ifdef RCS
static char rcsid[]= "@(#)$Header: zdwrdgn.c 1.2 86/05/06 $";
#endif

/*
 * Routines to initialize data in the diagnostic cylinders.
 *
 * The diagnostics assume that there will be at least 2 cylinders with
 * a minimum of 2 tracks per cylinder, for a minimum of 4 diagnostic tracks.
 * In addition, it will be assumed that all diagnostic tracks have a
 * minimum of 32 sectors. Only these 32 sectors will be initialized.
 * The first 4 diagnostic tracks have unique information in them. Any
 * remaining tracks will be identical to each other.
 *
 * Most sectors in the diagnostic tracks conform to a standard format,
 * described by the diagsec structure.  Some diagsec sectores supply
 * special unit test data in the "ds_pat" field. All other diagsec sectors
 * will have DSPAT in "ds_pat". The data in the "ds_rest" array defaults to
 * copies of RESTPAT.
 *
 * Track 0.
 *	All 32 sectors are of type diagsec. The "ds_pat field will is set to
 *	(0 | (1 << sector_number)), where 0 <= sector_number < 32.
 *
 * Track 1.
 *	All 32 sectors are of type diagsec. The "ds_pat field will is set to
 *	~(1 << sector_number), where 0 <= sector_number < 32.
 *
 * Track 2.
 *	The third track is a scratch track used for most write operations
 *	during the ZDC unit test. The 32 sectors are zeroed by default.
 *
 * Track 3.
 *	The fourth track contains more patterns for the ZDC unit test.
 *	The format for the 32 sectors are as follows:
 *	Sector 0:	128 4-byte words that supply an incrementing pattern.
 *			Each byte in a word contains the sequential number of
 *			that word. (For example, word 0 contains 0x00000000,
 *			word 32 contains 0x20202020, etc.).
 *	Sectors 1-15:	These sectors have a constant byte pattern per sector.
 *			All bytes in sector 1 are 0xA1, all bytes in sector
 *			2 are 0xA2, ..., all bytes in sector 15 are 0xAF.
 *	Sector 16:	This sector contains all 0xFF.
 *	Sectors 17-18:	These sectors have all zeroes.
 *	Sectors 19-31:	These sectors use the standard version of diagsec.
 */

/* $Log:	zdwrdgn.c,v $
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <zdc/zdc.h>
#include "../saio.h"
#include "zdformat.h"

#define	RESTPAT		0xdbb66d03	/* Pattern for ds_rest */
#define DSPAT		0x0f5a0f5a	/* Pattern for default ds_pat */
#define	INCRPAT		0xA0		/* increment pattern */
#define	ALLONPAT	0xFFFFFFFF
#define FIRSTINCRSECT	 1		/* track 3 increment sector start */
#define	LASTINCRSECT	15		/* track 3 increment sector end */
#define LASTTRK3	18

struct diagsec {
	u_int	ds_pat;
	u_int	ds_lba;
	u_char	ds_rest[DEV_BSIZE - (2 * sizeof(u_int))];
};

/*
 * write_dgndata
 *	Write dgn data in dgn cylinders.
 */
write_dgndata()
{
	register u_int i;
	register int j;
	register struct diagsec *dsec;
	register caddr_t cp;
	register int where;
	static caddr_t	dgntrk = NULL;		/* diagnostic track */

	printf("Writing diagnostic cylinders.\n");
	if (dgntrk == NULL) {
		callocrnd(DEV_BSIZE);
		dgntrk = calloc(DEV_BSIZE * ZDD_NDGNSPT);
	}

	/*
	 * Set up DGN track 0
	 */
	fill_dgn((u_int *)dgntrk, (u_int)RESTPAT,
				(DEV_BSIZE * ZDD_NDGNSPT) / sizeof(u_int));
	
	dsec = (struct diagsec *)dgntrk;
	for(i = 0; i < ZDD_NDGNSPT; i++) {
		dsec->ds_pat = 1 << i;
		dsec->ds_lba = i;
		++dsec;
	}
	where = (chancfg->zdd_cyls - ZDD_NDGNCYL) * 
		(chancfg->zdd_sectors * chancfg->zdd_tracks);
	where <<= DEV_BSHIFT;
	lseek(fd, where, L_SET);
	if (write(fd, dgntrk, ZDD_NDGNSPT * DEV_BSIZE) != ZDD_NDGNSPT * DEV_BSIZE) {
		printf("Ruptured disk - Cannot write DGN track 0\n");
		exit(12);
	}
	/*
	 * Set up DGN track 1
	 */
	where += chancfg->zdd_sectors << DEV_BSHIFT;
	dsec = (struct diagsec *)dgntrk;
	for (i = 0; i < ZDD_NDGNSPT; i++) {
		dsec->ds_pat = ~(1 << i);
		++dsec;
	}
	lseek(fd, where, L_SET);
	if (write(fd, dgntrk, ZDD_NDGNSPT * DEV_BSIZE) != ZDD_NDGNSPT * DEV_BSIZE) {
		printf("Ruptured disk - Cannot write DGN track 1\n");
		exit(12);
	}
	/*
	 * Set up DGN track 2
	 */
	where += chancfg->zdd_sectors << DEV_BSHIFT;
	bzero(dgntrk, ZDD_NDGNSPT * DEV_BSIZE);
	lseek(fd, where, L_SET);
	if (write(fd, dgntrk, ZDD_NDGNSPT * DEV_BSIZE) != ZDD_NDGNSPT * DEV_BSIZE) {
		printf("Ruptured disk - Cannot write DGN track 2\n");
		exit(12);
	}
	/*
	 * Set up DGN track 3
	 */
	where += chancfg->zdd_sectors << DEV_BSHIFT;
	cp = dgntrk;
	/*
	 * Sector 0
	 */
	for (i = 0; i < DEV_BSIZE / sizeof(int); i++) {
		for(j = 0; j < sizeof(int); j++)
			*cp++ = i;
	}
	/*
	 * Sectors 1 - 15
	 */
	for (i = FIRSTINCRSECT; i <= LASTINCRSECT; i++) {
		for (j = 0; j < DEV_BSIZE; j++)
			*cp++ = INCRPAT + i;
	}
	/*
	 * Sector 16
	 */
	fill_dgn((u_int *)cp, (u_int)ALLONPAT, DEV_BSIZE / sizeof(u_int));
	cp += DEV_BSIZE;
	/*
	 * Sectors 17-18
	 */
	bzero(cp, 2 * DEV_BSIZE);
	cp += 2 * DEV_BSIZE;
	/*
	 * Sectors 19-31
	 */
	fill_dgn((u_int *)cp, (u_int)RESTPAT,
			(ZDD_NDGNSPT-LASTTRK3-1)*DEV_BSIZE/sizeof(u_int));
	dsec = (struct diagsec *)cp;
	for (i = LASTTRK3+1; i < ZDD_NDGNSPT; i++) {
		dsec->ds_pat = DSPAT;
		dsec->ds_lba = i;
		++dsec;
	}
	lseek(fd, where, L_SET);
	if (write(fd, dgntrk, ZDD_NDGNSPT * DEV_BSIZE) != ZDD_NDGNSPT * DEV_BSIZE) {
		printf("Ruptured disk - Cannot write DGN track 3\n");
		exit(12);
	}

	/*
	 * Write any remaining tracks with default diagsec pattern.
	 */
	fill_dgn((u_int *)dgntrk, (u_int)RESTPAT,
				(DEV_BSIZE * ZDD_NDGNSPT) / sizeof(u_int));
	dsec = (struct diagsec *)dgntrk;
	for (i = 0; i < ZDD_NDGNSPT; i++) {
		dsec->ds_pat = DSPAT;
		dsec->ds_lba = i;
		++dsec;
	}
	for (i = ZDD_NDGNSPTRKS; i < (ZDD_NDGNCYL * chancfg->zdd_tracks); i++) {
		where += chancfg->zdd_sectors << DEV_BSHIFT;
		lseek(fd, where, L_SET);
		if (write(fd, dgntrk, ZDD_NDGNSPT * DEV_BSIZE) !=
		    ZDD_NDGNSPT * DEV_BSIZE) {
			printf("Ruptured disk - Cannot write DGN track %d\n", i);
			exit(12);
		}
	}
	printf("Diagnostic data written.\n");
}

/*
 * fill_dgn
 *	fill dgn track with pattern.
 */
static
fill_dgn(trk, pattern, count)
	register u_int *trk;		/* ZDD_NDGNSPT sector track */
	register u_int pattern;		/* pattern to write */
	int count;			/* int count */
{
	register u_int *endtrk;

	endtrk = trk + count;
	while (trk < endtrk)
		*trk++ = pattern;
}
