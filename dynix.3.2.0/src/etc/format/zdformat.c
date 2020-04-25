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
 * ident	"$Header: zdformat.c 1.14 91/03/26 $"
 * zdformat
 *
 * The basic algorithm is to format the disk a cylinder at a time while
 * marching through the disk with a 2 cylinder window. Off-cylinder
 * bad block replacements are satisfied via adjacent cylinders.
 * If Off-cylinder replacements cannot be satisfied via adjacent cylinders,
 * another "pass" finds replacements for the bad blocks in the closest
 * possible cylinder to the one with the bad block.
 */

/* $Log:	zdformat.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/file.h>
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
#include <diskinfo.h>
#include <errno.h>
#include "format.h"
#include "zdformat.h"

extern struct zdinfo *zdinfo;

extern caddr_t malloc();
extern long getsum();
extern struct zdinfo *getzdinfobydtype();
extern char openname[];

struct cyl_hdr 	cyls[2];
struct snf_list snf_list[SNF_LIST_SIZE];
int	snftogo;
struct	zdcdd	*chancfg;	/* Channel configuration - drive descriptor */
struct	zdbad	*bbl = NULL;	/* bad block list */
struct	zdbad	*newbbl = NULL;	/* new bad block list - built during format */
struct	bz_bad	*tobzp;		/* ptr to new list entry */
union	headers *headers = NULL;/* header list for READ_HDRS ioctl */
int 	n_hdrs;			/* Number of headers per track w/ runts */
int	nspc;			/* number of sectors per cylinder */
struct hdr_suspect *suspect;	/* Array of tracks containing potential
				 * header defects resulting in hdr-ecc
				 * errors.  Verified during hdr-pass */
int max_suspects;		/* Static size of header suspect list */
int num_suspects;		/* Number of elements in suspect list */
caddr_t vwbp;                   /* Verify write buffer */
caddr_t vrbp;                   /* Verify read buffer */
int	errcode;		/* error code for last I/O operation */


/*
 * alloc_structs
 *	allocate memory for data structures sized by channel configuration.
 */
alloc_structs()
{
	register int i;
	int	tracksize;

	alloc_cyls();
	/*
	 * Allocate memory for mfg defect list, bad block lists,
	 * and header suspect list.  The header list should never
	 * contain more elements than the bad block lists, since
	 * it basically corresponds to entries in the bad block 
	 * list or about to be added.
	 */
	i = ZDMAXBAD(chancfg);
	if (mfg)
		free(mfg);
	if (bbl)
		free(bbl);
	if (newbbl)
		free(newbbl);
	if (headers)
		free(headers);
	if (suspect)
		free(suspect);
	mfg = (struct bad_mfg *)MALLOC_ALIGN(i, DEV_BSIZE);
	bbl = (struct zdbad *)MALLOC_ALIGN(i, DEV_BSIZE);
	newbbl = (struct zdbad *)MALLOC_ALIGN(i, DEV_BSIZE);

	max_suspects = (i - (sizeof(struct zdbad) - sizeof(struct bz_bad))) /
		sizeof(struct bz_bad);
	suspect = (struct hdr_suspect *)
		malloc(max_suspects * sizeof(struct hdr_suspect));
	num_suspects = 0;

	if (!mfg || !bbl || !newbbl || !suspect) {
		fflush(stdout);
		fprintf(stderr, "alloc_structs: malloc error\n");
		fprintf(stderr, "...exiting\n");
		exit(1);	
	}
	bzero((caddr_t)mfg, i);
	bzero((caddr_t)bbl, i);
	bzero((caddr_t)newbbl, i);
	bzero((caddr_t)suspect, max_suspects * sizeof(struct hdr_suspect));

	n_hdrs = totspt + ((chancfg->zdd_runt) ? 1 : 0);
	i = ROUNDUP(n_hdrs * sizeof(union headers), CNTMULT);
	headers = (union headers *)MALLOC_ALIGN(i, DEV_BSIZE);
	if (!headers) {
		fflush(stdout);
		fprintf(stderr, "alloc_structs: malloc error\n");
		fprintf(stderr, "...exiting\n");
		exit(1);	
	}
	bzero((caddr_t)headers, i);

	if (vwbp)
		free(vwbp);
	if (vrbp)
		free(vrbp);

	tracksize = chancfg->zdd_sectors << DEV_BSHIFT;
	vwbp = MALLOC_ALIGN(tracksize, DEV_BSIZE);
	vrbp = MALLOC_ALIGN(tracksize, DEV_BSIZE);
	if (!vwbp || !vrbp) {
		fflush(stdout);
		fprintf(stderr, "alloc_structs: malloc error\n");
		fprintf(stderr, "...exiting\n");
		exit(1);
	}
	bzero((caddr_t)vwbp, tracksize);
	bzero((caddr_t)vrbp, tracksize);

	nspc = chancfg->zdd_tracks * chancfg->zdd_sectors;
}

/*
 * alloc_cyls
 *	Allocate memory for cylinder maps.
 */
alloc_cyls()
{
	register int	size;
	register int	i;

	/*
	 * allocate cylinder tables.
	 */
	cyls[0].c_trk = (struct track_hdr *)malloc(chancfg->zdd_tracks
			                    * sizeof(struct track_hdr));
	cyls[1].c_trk = (struct track_hdr *)malloc(chancfg->zdd_tracks
					    * sizeof(struct track_hdr));
	if (cyls[0].c_trk == NULL || cyls[1].c_trk == NULL) {
		fflush(stdout);
		fprintf(stderr, "alloc_cyls: malloc error");
		fprintf(stderr, "...exiting\n");
		exit(1);	
	}
	bzero((caddr_t)cyls[0].c_trk, 
	      (chancfg->zdd_tracks*sizeof(struct track_hdr)));
	bzero((caddr_t)cyls[1].c_trk, 
	      (chancfg->zdd_tracks*sizeof(struct track_hdr)));

	size = totspt;
	if (chancfg->zdd_runt)
		size++;
	size = ROUNDUP(sizeof(struct hdr) * size, CNTMULT);
	for (i = 0; i < chancfg->zdd_tracks; i++) {
		cyls[0].c_trk[i].t_hdr = 
		   (struct hdr *)MALLOC_ALIGN(size, types[disk].align);
		cyls[1].c_trk[i].t_hdr = 
		   (struct hdr *)MALLOC_ALIGN(size, types[disk].align);
		if (cyls[0].c_trk[i].t_hdr == NULL
		    || cyls[1].c_trk[i].t_hdr == NULL) {
			fflush(stdout);
			fprintf(stderr, "alloc_cyls: malloc error");
			fprintf(stderr, "...exiting\n");
			exit(1);	
		}
		bzero((caddr_t)(cyls[0].c_trk[i].t_hdr), size);
		bzero((caddr_t)(cyls[1].c_trk[i].t_hdr), size);
	}
}

/*
 * format
 *	format or reformat the disk
 */
int
doformat(flag)
	int flag;				/* Z_FORMAT or Z_REFORMAT */
{
	extern char *diskname;

	register int i;
	register struct bz_bad	*fbzp;	/* ptr to bad list entry */
	register int prev;			/* previous cylinder */
	register int last;			/* last cylinder */
	struct zdbad *tbbl;
	unchar state;

	if (debug) printf("doformat: begin\n");
	/*
	 * set pointers into bad block lists
	 */
	tobzp = newbbl->bz_bad;
	fbzp = bbl->bz_bad;

	/*
	 * Always start format with cylinder 0.
	 */
	if (debug) printf("call setup_ddcyl\n");
	setup_ddcyl(&cyls[ZDD_DDCYL & 1], fbzp);
	if (debug) printf("call map_ddcyl\n");
	map_ddcyl(&cyls[ZDD_DDCYL & 1]);
	if (debug) printf("call format_dd\n");
	format_dd(&cyls[ZDD_DDCYL & 1]);

	while (fbzp < &bbl->bz_bad[bbl->bz_nelem] && fbzp->bz_cyl == ZDD_DDCYL)
		++fbzp;		/* skip past this cylinder */

	last = lastcyl;
	if (lastcyl >= (chancfg->zdd_cyls - ZDD_NDGNCYL))
		last = chancfg->zdd_cyls - ZDD_NDGNCYL - 1;

	/*
	 * setup 1st cylinder.
	 */
	if (debug) printf("do 1st cyl\n");
	i = 1;
	if (i <= last) {
		setup_cyl(i, &cyls[i & 1], fbzp, flag);
		map_cyl(i, &cyls[i & 1]);
		while (fbzp < &bbl->bz_bad[bbl->bz_nelem] && fbzp->bz_cyl == i)
			++fbzp;		/* skip past this cylinder */
		prev = i;
		++i;
	}
	/*
	 * Now do rest.
	 */
	if (debug) printf("now doing rest\n");
	for (; i <= last; i++, prev++) {
		setup_cyl(i, &cyls[i & 1], fbzp, flag);
		map_cyl(i, &cyls[i & 1]);
		while (fbzp < &bbl->bz_bad[bbl->bz_nelem] && fbzp->bz_cyl == i)
			++fbzp;		/* skip past this cylinder */
		/*
		 * Check if this cylinder needs help from the previous
		 * cylinder.
		 */
		if (cyls[i&1].c_bad) {
			/* See if previous can help */
			if (cyls[prev&1].c_free) {
				do_snf(&cyls[prev&1], &cyls[i&1]);
			}
		}
		/*
		 * Check to see if this cylinder can help the
		 * previous cylinder (if it indeed needs help).
		 */
		if (cyls[prev&1].c_bad) {
			if (cyls[i & 1].c_free)
				do_snf(&cyls[i&1], &cyls[prev&1]);
			if (cyls[prev & 1].c_bad)
				save_snf_pass(&cyls[prev & 1]);
		}
		/*
		 * Format (on disk) the previous cylinder.
		 * Run a quick check of the hdrs and note
		 * any tracks needing corrections for the
		 * hdr validation/correction pass.
		 */
		format_cyl(prev, &cyls[prev & 1]);
		check_hdrs(prev, &cyls[prev & 1], FALSE);
	}

	/*
	 * Write the last cylinder out to disk
	 */
	if (last != ZDD_DDCYL) {
		if (debug) printf("writing last cyl out to disk\n");
		if (cyls[last&1].c_bad)
			save_snf_pass(&cyls[prev & 1]);
		format_cyl(last, &cyls[last & 1]);
		check_hdrs(last, &cyls[last & 1], FALSE);
	}
	/*
	 * Any dgn cylinders to format?
	 */
	if (last < lastcyl) {
		/*
		 * Yes.
		 */
		if (debug) printf("doing diag cyls\n");
		for (i = last + 1; i <= lastcyl; i++) {
			setup_dgncyl(i, &cyls[i & 1], fbzp);
			map_dgncyl(i, &cyls[i & 1]);
			while (fbzp < &bbl->bz_bad[bbl->bz_nelem] &&
							fbzp->bz_cyl == i)
				++fbzp;		/* skip past this cylinder */
			format_cyl(i, &cyls[i & 1]);
		}
	}

	/*
	 * Copy any remaining bad blocks after lastcyl.
	 */
	while (fbzp < &bbl->bz_bad[bbl->bz_nelem]) {
		if (fbzp->bz_rtype == BZ_SNF)
			newbbl->bz_nsnf++;
		newbbl->bz_nelem++;
		*tobzp++ = *fbzp++;
	}
	/*
	 * Sort list.
	 */
	qsort(newbbl->bz_bad, newbbl->bz_nelem, sizeof(struct bz_bad), bblcomp);
	tbbl = bbl;
	bbl = newbbl;
	newbbl = tbbl;
	/*
	 * Calculate checksum.
	 */
	bbl->bz_csn = getsum((long *)bbl->bz_bad,
		(int)((bbl->bz_nelem * sizeof(struct bz_bad)) / sizeof(long)),
		(long)(bbl->bz_nelem ^ bbl->bz_nsnf));

	if (debug) {
		printf("SETBBL to driver\n");
	}
	if (ioctl(fd, ZIOSETBBL, (char *)bbl) < 0)
		perror("ioctl ZIOSETBBL error");

	/*
	 * If snftogo > 0, then do_snf_pass() to find homes for
	 * bad blocks which could not be resolved by the first pass
	 * through the disk and update the drivers bbl.
	 */
	if (snftogo > 0) {
		if (debug) {
			printf("doing snf_pass\n");
		}
		do_snf_pass(2);
	}

	/*
	 * Now that disk is formatted, set device state to good. 
	 */
	state = ZU_GOOD;
	if (ioctl(fd, ZIOSETSTATE, &state) < 0) {
		perror("ZIOSETSTATE ioctl error");
		return;
	}
#ifdef	DEBUG
	(void)ioctl(fd, ZIODEBUG, (char *)debug);
#endif /* DEBUG */

	if (debug)printf("write badlist\n");
	write_badlist();
	if (debug) printf("write mfglist\n");
	write_mfglist();

	/*
	 * Close and reopen device so driver will see
	 * FORMAT'ted state
	 */
	close(fd);
	if ((fd = open(openname, usep->open_flags)) < 0) {
		fflush(stdout);
		fprintf(stderr, "unable to open %s after format: ", openname);
		perror("");
		exit(2);
	}
	return;
}

/*
 * do_snf_pass
 *	take a second pass across the disk to find replacements for the
 *	elements in the snf_list not resolved in adjacent cylinders.
 */
do_snf_pass(cyloff)
	int cyloff;
{
	register int i;
	register int offset;
	register struct bz_bad *bzp;
	int	where;
	int	cyl;
	struct	bz_bad *find_snf_rplcmnt();
	int count, limit;

	if (debug)
		printf("do_snf_pass(%d)\n",cyloff);

	for (i = 0; i < snftogo; i++) {
		offset = cyloff;
		cyl = snf_list[i].snf_addr.da_cyl;
		limit = cyl - 1;
		if (lastcyl - cyl > limit)
			limit = lastcyl - cyl;
		for (;;) {
			bzp = find_snf_rplcmnt(&snf_list[i], cyl + offset,
						&cyls[(cyl + offset) & 1]);
			if (bzp != (struct bz_bad *)NULL)
				break;
			bzp = find_snf_rplcmnt(&snf_list[i], cyl - offset,
						&cyls[(cyl - offset) & 1]);
			if (bzp != (struct bz_bad *)NULL)
				break;
			offset++;
			if (offset > limit) {
				/*
				 * Should never happen as typically there
				 * are many more spares than even the bad
				 * block list can hold.
				 */
				fflush(stdout);
				fprintf(stderr, "do_snf_pass: Cannot find ");
				fprintf(stderr, "replacement sector\n");
				fprintf(stderr, "...exiting\n");
				exit(14);
			}
		}
		/* 
		 * Calculate a new bad block list checksum,
		 * then update the drivers bad block list with
		 * the new SNF resolution before attempting 
		 * to write the recovered data.
		 */
		bbl->bz_csn = getsum((long *)bbl->bz_bad,
			(int)((bbl->bz_nelem * sizeof(struct bz_bad)) / sizeof(long)),
			(long)(bbl->bz_nelem ^ bbl->bz_nsnf));

		if (debug) {
			printf("SETBBL to driver\n");
		}
		if (ioctl(fd, ZIOSETBBL, (char *)bbl) < 0)
			perror("ioctl ZIOSETBBL error");

		/*
		 * write data in sector - if any saved.
		 */
		if (snf_list[i].snf_data != (caddr_t)NULL) {
			where = (cyl * chancfg->zdd_sectors * chancfg->zdd_tracks)
				+ (snf_list[i].snf_addr.da_head * chancfg->zdd_sectors)
				+ snf_list[i].snf_addr.da_sect;
			where <<= DEV_BSHIFT;
			lseek(fd, where, 0);
			if ((count = write(fd, snf_list[i].snf_data, DEV_BSIZE))
			    != DEV_BSIZE) {
				fflush(stdout);
				if (count < 0)
					perror("write error");
				fprintf(stderr, "Warning: cannot write ");
				fprintf(stderr, "replacement sector (%d, %d, %d).\n",
					bzp->bz_rpladdr.da_cyl,
					bzp->bz_rpladdr.da_head,
					bzp->bz_rpladdr.da_sect);
			}
		}
	}
	snftogo = 0;		/* No more */
}

/*
 * getskew
 *	Calculate skew for sector 0 on given cylinder/head.
 */
int
getskew(cyl, track)
	register int	cyl;
	int	track;
{

	/*
	 * Special case for cyl ZDD_DDCYL, and dgn cylinders.
	 */
	if (cyl == ZDD_DDCYL)
		return((track * ZDD_NDDSECTORS) % totspt);

	if (cyl >= (chancfg->zdd_cyls - ZDD_NDGNCYL))
		return(0);

	if (chancfg->zdd_tskew == 1)
		return((((chancfg->zdd_tracks - 1 + chancfg->zdd_cskew) * cyl)
				+ track) % totspt);
	/*
	 * track skew != 1
	 */
	return(( (( (chancfg->zdd_tskew * (chancfg->zdd_tracks - 1))
			+ chancfg->zdd_cskew) * cyl)
			+ (chancfg->zdd_tskew * track)) % totspt);
}

/*
 * setskew
 *	Calculate skew for this cylinder. And initialize headers.
 *	Assumes all sectors are good. Headers for spare sectors are
 *	also initialized.
 */
setskew(cyl, chp)
	int	cyl;		/* cylinder number */
	struct	cyl_hdr *chp;
{
	register struct hdr *hp;
	register int ls;		/* logical sector num */
	register int ps;		/* physical sector num */
	register int trk;

	for(trk = 0; trk < chancfg->zdd_tracks; trk++) {
		ps = getskew(cyl, trk);
		for (ls = 0; ls < chancfg->zdd_sectors; ls++) {
			hp = &chp->c_trk[trk].t_hdr[ps];
			hp->h_type = ZD_GOODSECT;
			hp->h_cyl = cyl;
			hp->h_head = trk;
			hp->h_sect = ls;
		 	ps = (ps + 1) % totspt;
		} 

		for (ls = chancfg->zdd_sectors; ls < totspt; ls++) {
			hp = &chp->c_trk[trk].t_hdr[ps];
			hp->h_type = ZD_GOODSPARE;
			hp->h_cyl = cyl;
			hp->h_head = trk;
			hp->h_sect = ls;
		 	ps = (ps + 1) % totspt;
		} 
	}
	if ((debug > 1)) {
		printf("setskew: all sectors skewed on (%d,[0-%d],[0-%d,%d])\n"
			,cyl,trk-1,chancfg->zdd_sectors-1,totspt-1);
	}
}

/*
 * mark_spots
 *	Mark sectors bad using bad block list data.
 *	Called only during FORMAT. Only BZ_PHYS entries will
 *	exist in bad block list.
 *
 *	REFORMAT, ADDBAD, and CLONEFORMAT typically use remark_spots.
 */
mark_spots(cyl, chp, bzp)
	int	cyl;			/* cylinder number */
	register struct cyl_hdr *chp;
	register struct bz_bad	*bzp;	/* bad block list entries */
{

	if (debug)
		printf("mark_spots (%d,,,)\n",cyl);
	while (bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl) {
		chp->c_trk[bzp->bz_head].t_hdr[bzp->bz_sect].h_flag =
						bzp->bz_ftype | ZD_TORESOLVE;
		chp->c_trk[bzp->bz_head].t_bad++;
		chp->c_bad++;
		bzp++;
	}
}

/*
 * remark_spots
 *	Mark sectors bad using bad block list data.
 *
 *	REFORMAT and ADDBAD use remark_spots.
 */
remark_spots(cyl, chp, abp, flag)
	int	cyl;		/* cylinder number */
	struct cyl_hdr *chp;
	struct	bz_bad *abp;	/* cylinder data in bad block list */
	int	flag;		/* Z_REFORMAT, Z_ADDBAD, or Z_CLONEFORMAT */
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	register struct bz_bad *bzp;
	struct hdr *rhp;

	if (abp->bz_cyl != cyl || abp == &bbl->bz_bad[bbl->bz_nelem])
		return;		/* no spots */

	if (debug)
		printf("remark_spots (%d,,,) BZ_PHYS\n",cyl);
	bzp = abp;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl; bzp++) {
		if (bzp->bz_rtype != BZ_PHYS)
			continue;
		/*
		 * Mark all BZ_PHYS entries. These will be slipped.
		 */
		hp = &chp->c_trk[bzp->bz_head].t_hdr[bzp->bz_sect];
		hp->h_type = ZD_BADUNUSED;
		if (flag != Z_CLONEFORMAT)
			hp->h_flag = ZD_TORESOLVE | bzp->bz_ftype;
		else
			hp->h_flag = 0;
		hp->h_cyl = ZD_BUCYL;
		hp->h_head = ZD_BUHEAD;
		hp->h_sect = ZD_INVALSECT;
		chp->c_trk[bzp->bz_head].t_bad++;
		chp->c_bad++;
		if (debug)
			printf("remark_spots: INVALSECT phys(%d,%d,%d)\n",cyl,bzp->bz_head,bzp->bz_sect);
	}

	/*
	 * Slip all BZ_PHYS entries.
	 */
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad)
			slip_track(cyl, (tp - chp->c_trk), chp);
	}

	if (debug)
		printf("remark_spots (%d,,,) BZ_AUTOREVECT\n",cyl);
	/*
	 * Now look through list for auto-revector sectors.
	 */
	bzp = abp;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl; bzp++) {
		if (bzp->bz_rtype != BZ_AUTOREVECT)
			continue;

		hp = chp->c_trk[bzp->bz_head].t_hdr;
		for (; hp < &chp->c_trk[bzp->bz_head].t_hdr[totspt]; hp++) {
			if (hp->h_sect == bzp->bz_sect)
				break;
		}
		hp->h_type = ZD_BADREVECT;
		if (flag != Z_CLONEFORMAT)
			hp->h_flag = ZD_TORESOLVE | bzp->bz_ftype;
		else
			hp->h_flag = 0;
		chp->c_trk[bzp->bz_head].t_bad++;
		chp->c_bad++;
		if (debug)
			printf("remark_spots: BADREVECT (%d,%d,%d)\n",cyl,bzp->bz_head,bzp->bz_sect);

		/*
		 * Now slip replacement track so that any SNF entries may
		 * be marked correctly.
		 */
		rhp = &chp->c_trk[bzp->bz_rpladdr.da_head].t_hdr[bzp->bz_rpladdr.da_sect];
		rhp->h_type = ZD_GOODRPL;
		rhp->h_cyl  = hp->h_cyl;
		rhp->h_head = hp->h_head;
		rhp->h_sect = hp->h_sect | ZD_AUTOBIT;
		slip_track(cyl, (int)bzp->bz_rpladdr.da_head, chp);
	}

	if (debug)
		printf("remark_spots (%d,,,) BZ_SNF\n",cyl);
	/*
	 * Now set SNF bad blocks
	 */
	bzp = abp;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl; bzp++) {
		if (bzp->bz_rtype != BZ_SNF)
			continue;

		/*
		 * Find physical sector corresponding to logical disk address.
		 */
		hp = chp->c_trk[bzp->bz_head].t_hdr;
		for (; hp < &chp->c_trk[bzp->bz_head].t_hdr[totspt]; hp++) {
			if (hp->h_sect == bzp->bz_sect)
				break;
		}
		if (hp->h_sect != bzp->bz_sect) {
			/*
			 * special case: some boot part blocks may have a
			 * BZ_SNF as well as a BZ_PHYS entry. Ignore BZ_SNF
			 * entry as already marked via BZ_PHYS.
			 */
			if (cyl == 1 && bzp->bz_head == 0)
				continue;
			fflush(stdout);
			fprintf(stderr, "remark_spots: Could not find SNF sector (%d, %d, %d).\n",
				cyl, bzp->bz_head, bzp->bz_sect);
			fprintf(stderr, "...exiting\n");
			exit(4);
		}
		hp->h_type = ZD_BADUNUSED;
		if (flag != Z_CLONEFORMAT)
			hp->h_flag = ZD_TORESOLVE | bzp->bz_ftype;
		else
			hp->h_flag = 0;
		hp->h_cyl = ZD_BUCYL;
		hp->h_head = ZD_BUHEAD;
		hp->h_sect = ZD_INVALSECT;
		if (debug)
			printf("remark_spots: BADUNUSED (%d,%d,%d)\n",cyl,bzp->bz_head,bzp->bz_sect);
		chp->c_trk[bzp->bz_head].t_bad++;
		chp->c_bad++;
	}
}

/*
 * newbad
 *	Add bad block to new bad block list being built.
 */
struct bz_bad *
newbad(hp, rtype, rhp)
	register struct hdr *hp;	/* Bad block */
	int	rtype;			/* replacement type */
	register struct hdr *rhp;	/* Replacement block - if germane */
{
	register int max;
	struct	bz_bad	*retval;

	max = ((chancfg->zdd_sectors - ZDD_NDDSECTORS) / 2) << DEV_BSHIFT;
	max -= (sizeof(struct zdbad) - sizeof(struct bz_bad)); 
	max /= sizeof(struct bz_bad);

	newbbl->bz_nelem++;
	if (newbbl->bz_nelem > max) {
		fflush(stdout);
		fprintf(stderr, "Cannot add (%d, %d, %d) to bad block list.\n",
			hp->h_cyl, hp->h_head, hp->h_sect);
		fprintf(stderr, "Ruptured disk - too many bad sectors (%d).\n",
			newbbl->bz_nelem);
		fprintf(stderr, "...exiting\n");
		exit(7);
	}

	/*
	 * Add bad block to new bad block list.
	 */
	tobzp->bz_sect = hp->h_sect;
	tobzp->bz_head = hp->h_head;
	tobzp->bz_cyl = hp->h_cyl;
	tobzp->bz_rtype = rtype;
	tobzp->bz_ftype = hp->h_flag & ZD_ERRTYPE;
	hp->h_flag = 0;
	if (rtype != BZ_PHYS) {
		tobzp->bz_rpladdr.da_sect = rhp->h_sect;
		tobzp->bz_rpladdr.da_head = rhp->h_head;
		tobzp->bz_rpladdr.da_cyl = rhp->h_cyl;
		if (rtype == BZ_SNF) {
			newbbl->bz_nsnf++;
			if (debug) {
				printf("newbad: %s(%d,%d,%d) --> log(%d,%d,%d) type SNF\n",
				hp->h_flag&BZ_BADPHYS? "phys" : "log",
				hp->h_cyl, hp->h_head, hp->h_sect,
				rhp->h_cyl, rhp->h_head, rhp->h_sect);
			}
		} else if (debug) {
			printf("newbad: %s(%d,%d,%d) --> phys(%d,%d,%d) type BZ_AUTORECT\n",
				hp->h_flag&BZ_BADPHYS? "phys" : "log",
				hp->h_cyl, hp->h_head, hp->h_sect,
				rhp->h_cyl, rhp->h_head, rhp->h_sect);
		}
	} else if (debug) {
		printf("newbad: phys(%d,%d,%d) type BZ_PHYS will be slipped\n",
				hp->h_cyl, hp->h_head, hp->h_sect);
	}
	retval = tobzp++;
	return(retval);
}

/*
 * slip_track
 *	slip the headers to skip past "Bad Unused" headers.
 *	Assumes NO other replacements besides slipped are set.
 */
slip_track(cyl, track, chp)
	int cyl;
	int track;
	struct	cyl_hdr *chp;
{
	register struct hdr *hp;
	register int	ps;		/* physical sector num */
	register int	ls;		/* logical sector num */
	int startps;

	ls = 0;
	ps = getskew(cyl, track);
	startps = ps;
	if (debug)
		printf("slip_track(%d,%d,) skew=%d\n",cyl,track,ps);
	do {
		hp = &chp->c_trk[track].t_hdr[ps];

		/*
		 * Look for ZD_GOODSECT or ZD_GOODSPARE.
		 * Skip past other header types.
		 */
		if (hp->h_type == ZD_GOODSECT) {
			if (debug > 1) {
				printf("slip_track phys(%d,%d,%d)/log(,,%d) --> log(%d,%d,%d)\n",
					hp->h_cyl, hp->h_head, ps, hp->h_sect,
					hp->h_cyl, hp->h_head, ls ); 
			}
			hp->h_sect = ls;
			ls++;
		} else if (hp->h_type == ZD_GOODSPARE) {
			/*
			 * Slipped into spares - format as ZD_GOODUSED.
			 */
			if (debug > 1) {
				printf("slip_track phys(%d,%d,%d)/log(,,%d) GOODSPARE --> log(%d,%d,%d) GOODSECT\n",
					hp->h_cyl, hp->h_head, ps, hp->h_sect,
					hp->h_cyl, hp->h_head, ls); 
			}
			hp->h_type = ZD_GOODSECT;
			hp->h_sect = ls;
			ls++;
		} else if (debug > 1) {
			switch (hp->h_type) {
			case ZD_GOODRPL:
				printf("slip_track phys(%d,%d,%d)/log(,,%d) type ZD_GOODRPL\n",
				
					hp->h_cyl, hp->h_head, ps, hp->h_sect); 
				break;
			case ZD_GOODSS:
				printf("slip_track phys(%d,%d,%d)/log(,,%d) type ZD_GOODSS\n",
					hp->h_cyl, hp->h_head, ps, hp->h_sect); 
				break;
			case ZD_BADUNUSED:
				if (hp->h_cyl == ZD_BUCYL) {
					printf("slip_track phys(%d,%d,%d)/log(BUCYL,BUHEAD,INVALIDSECT) type ZD_BADUNUSED\n",
						cyl, track, ps); 
				} else {
					printf("slip_track phys(%d,%d,%d)/log(%d,%d,%d) type ZD_BADUSED\n",
						cyl, track, ps,
						hp->h_cyl, hp->h_head, hp->h_sect); 
				}
				break;
			case ZD_BADREVECT:
				printf("slip_track phys(%d,%d,%d)log(%d,%d,%d) type ZD_BADREVECT\n",
					cyl, track, ps,
					hp->h_cyl, hp->h_head, hp->h_sect); 
				break;
			}
		}
		ps = (ps + 1) % totspt;
	} while (ls < chancfg->zdd_sectors && ps != startps);
}

/*
 * do_slippage
 *	Replace bad blocks via slippage on same track.
 *	The bad sectors will be marked with a "Bad Unused" header.
 */
do_slippage(cyl, chp)
	int	cyl;			/* cylinder number */
	struct	cyl_hdr *chp;
{
	register struct track_hdr *tp;
	register struct hdr *hp;
	register int trk;		/* Track number */
	unchar	sect;

	if (debug)
		printf("do_slippage(%d,,)\n",cyl);

	for (trk = 0; trk < chancfg->zdd_tracks; trk++) {
		tp = &chp->c_trk[trk];
		if (tp->t_bad == 0)		/* no spots */
			continue;

		/*
		 * If enuff free to cover bad spots, then replace via slippage.
		 */
		if (tp->t_free >= tp->t_bad) {
			for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
				if ((hp->h_flag & ZD_TORESOLVE) == 0)
					continue;
				hp->h_sect = hp - tp->t_hdr;
				sect = hp->h_sect;
				(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
				hp->h_type = ZD_BADUNUSED;
				hp->h_cyl = ZD_BUCYL;
				hp->h_head = ZD_BUHEAD;
				hp->h_sect = ZD_INVALSECT;
				if (debug)
					printf("bad spot\n");
				if (verbose)
					printf("Slipping past bad sector at physical (%d, %d, %d).\n",
						cyl, trk, sect);
			}
			slip_track(cyl, trk, chp);
			tp->t_free -= tp->t_bad;
			chp->c_free -= tp->t_bad;
			chp->c_bad -= tp->t_bad;
			tp->t_bad = 0;
			continue;
		}

		/*
		 * Place t_free on sectors with bad headers first. Do
		 * bad data sectors, if replacement sectors available.
		 */
		for (hp = tp->t_hdr; tp->t_free && hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			if ((hp->h_flag & ZD_ERRTYPE) == BZ_BADDATA)
				continue;
			/*
			 * Got header error.
			 */
			hp->h_sect = hp - tp->t_hdr;
			sect = hp->h_sect;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (debug)
				printf("bad header\n");
			if (verbose)
				printf("Slipping past bad sector at physical (%d, %d, %d).\n",
					cyl, trk, sect);
			tp->t_bad--;
			tp->t_free--;
			chp->c_free--;
			chp->c_bad--;
		}

		for (hp = tp->t_hdr; tp->t_free && hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			/*
			 * Got error in data.
			 */
			hp->h_sect = hp - tp->t_hdr;
			sect = hp->h_sect;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (debug)
				printf("bad data\n");
			if (verbose)
				printf("Slipping past bad sector at physical (%d, %d, %d).\n",
					cyl, trk, sect);
			tp->t_bad--;
			tp->t_free--;
			chp->c_free--;
			chp->c_bad--;
		}
		slip_track(cyl, trk, chp);
	}
}

/*
 * do_autorevect
 *	Replace bad blocks via revectoring to next track.
 *	To autorevector the bad block MUST have a good header.
 *	The bad block will have a "Bad Revector" header. The replacement
 *	sector will get a "Good Replacement" header.
 */
do_autorevect(cyl, chp)
	int	cyl;			/* cylinder number */
	struct	cyl_hdr *chp;
{
	register int trk;		/* Track number */
	register struct hdr *hp;	/* sector header */
	register struct track_hdr *tp;
	register int sect;
	struct	track_hdr *next;
	int	rsect;			/* Replacement sector */

	if (debug)
		printf("do_autorevect(%d)\n",cyl);

	for (trk = 0; trk < chancfg->zdd_tracks; trk++) {
		tp = &chp->c_trk[trk];
		if (tp->t_bad == 0)		/* no spots */
			continue;
		/*
		 * Have bad sector - check if next track can provide
		 * a replacement sector.
		 */
		next = &chp->c_trk[(trk + 1) % chancfg->zdd_tracks];
		for (sect = 0; next->t_free && sect < totspt; sect++) {
			/*
			 * Bad sector must have good header. If all
			 * bad sectors on track have bad headers, then
			 * cannot autorevector.
			 */
			hp = &tp->t_hdr[sect];
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			if ((hp->h_flag & ZD_ERRTYPE) == BZ_BADDATA) {
				hp->h_type = ZD_BADREVECT;
				tp->t_bad--;
				chp->c_bad--;

				/*
				 * set up replacement sector.
				 */
				if (chancfg->zdd_tskew > 1)
					rsect = sect + chancfg->zdd_tskew - 1;
				else
					rsect = sect + 1;
				rsect %= totspt;
				/*
				 * Find 1st good sector starting at "rsect".
				 */
				while ((next->t_hdr[rsect].h_flag & ZD_TORESOLVE)
				      || (next->t_hdr[rsect].h_type == ZD_BADUNUSED)
				      || (next->t_hdr[rsect].h_type == ZD_GOODRPL))
					rsect = (rsect + 1) % totspt;
				/*
				 * Init replacement sector
				 * Since newbad wants physical address
				 * for BZ_AUTOREVECT replacement, set sector
				 * in header to its physical address.
				 */
				next->t_hdr[rsect].h_sect = rsect;
				(void)newbad(hp, BZ_AUTOREVECT, &next->t_hdr[rsect]);
				next->t_hdr[rsect] = *hp;
				next->t_hdr[rsect].h_type = ZD_GOODRPL;
				next->t_hdr[rsect].h_sect |= ZD_AUTOBIT;
				next->t_free--;
				slip_track(cyl, next - chp->c_trk, chp);
				chp->c_free--;
				if (verbose)
					printf("Auto-Revector (%d, %d, %d) to physical (%d, %d, %d).\n",
						cyl, trk, hp->h_sect,
						cyl, next - chp->c_trk, rsect);
			}
		}
	}
	if (debug)
		printf("do_autorevect(%d) done\n",cyl);
}

/*
 * get_replace
 *	get replacement sector for SNF type replacements
 *
 * Must be called with pointer to cylinder with free spare sectors.
 * Return ptr to sector header.
 */
struct hdr *
get_replace(chp)
	register struct cyl_hdr *chp;
{
	register struct track_hdr *tp;
	register struct hdr *hp;

	for(tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_free == 0)
			continue;
		/*
		 * Find free replacement sector for SNF spare.
		 */
		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
			if (hp->h_type == ZD_GOODSPARE) {
				hp->h_type = ZD_GOODSECT;
				tp->t_free--;
				chp->c_free--;
				return(hp);
			}
		}
	}
	fflush(stdout);
	fprintf(stderr, "get_replace: no free replacements.\n");
	fprintf(stderr, "...exiting\n");
	exit(13);
	/* NOTREACHED */
}

/*
 * do_snf
 *	Mark bad blocks as "Bad Unused". Find replacement in "from"
 *	cylinder. Address of replacement entered in bad block list. And
 *	replacement cylinder will be marked as "Good Used" with its
 *	logical disk address instead of "Good Spare".
 */
do_snf(from, to)
	struct cyl_hdr *from;
	register struct cyl_hdr *to;
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	struct	hdr	*rpl;		/* pointer to replacement header */

	if (debug) {
		printf("do_snf(from(spares=%d,bad=%d)to(spares=%d,bad=%d))\n",
				from->c_free, from->c_bad,
				to->c_free, to->c_bad);
	}
	tp = to->c_trk;
	for (; from->c_free && tp < &to->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad == 0)		/* no spots */
			continue;
		hp = tp->t_hdr;
		for (; from->c_free && tp->t_bad && hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			/*
			 * find replacement in the cylinder. Mark sector
			 * as "Bad Unused".
			 */
			rpl = get_replace(from);
			(void)newbad(hp, BZ_SNF, rpl);
			if (verbose)
				printf("Revectoring (%d, %d, %d) to (%d, %d, %d).\n",
					hp->h_cyl, hp->h_head, hp->h_sect,
					rpl->h_cyl, rpl->h_head, rpl->h_sect);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			tp->t_bad--;
			to->c_bad--;
		}
	}
}

/*
 * save_snf_pass
 *	save to-be-resolved BZ_SNF bad blocks to be resolved by a second
 *	pass across the disk.
 */
save_snf_pass(chp)
	register struct cyl_hdr	*chp;
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	register struct bz_bad *bzp;

	tp = chp->c_trk;
	for (; chp->c_bad && tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad == 0)		/* no spots */
			continue;
		for (hp=tp->t_hdr; tp->t_bad && hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;

			if (snftogo == SNF_LIST_SIZE) {
				fflush(stdout);
				fprintf(stderr, "save_snf_pass: snf_list ");
				fprintf(stderr,
					"overflow. Increase SNF_LIST_SIZE.\n");
				fprintf(stderr, "...exiting\n");
				exit(5);
			}
			bzp = newbad(hp, BZ_SNF, hp);
			bzp->bz_rpladdr.da_cyl  = 0;	/* avoid confusion */
			bzp->bz_rpladdr.da_head = 0;
			bzp->bz_rpladdr.da_sect = 0;
			snf_list[snftogo].snf_addr.da_cyl = hp->h_cyl;
			snf_list[snftogo].snf_addr.da_head = hp->h_head;
			snf_list[snftogo].snf_addr.da_sect = hp->h_sect;
			snf_list[snftogo].snf_data = (caddr_t)NULL;
			snftogo++;
			if (verbose)
				printf("Save for pass2 (%d, %d, %d).\n",
					hp->h_cyl, hp->h_head, hp->h_sect);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			tp->t_bad--;
			chp->c_bad--;
		}
	}
}

/*
 * find_snf_rplcmnt
 *	find replacement cylinder for BZ_SNF bad block replacement.
 *	Used to resolve BZ_SNF bad blocks where 1st pass failed. That is,
 *	resolve outside 2 cylinder window used in initial pass.
 *
 * RETURN:
 *	Found: pointer to bad block list BZ_SNF entry.
 *	Not found: NULL pointer.
 */
struct bz_bad *
find_snf_rplcmnt(snf, cyl, chp)
	struct	snf_list *snf;
	int	cyl;
	register struct cyl_hdr	*chp;
{
	register struct	bz_bad	*bzp;
	register struct hdr *hp;

	/*
	 * Is requested cyl in range?
	 */
	if (cyl < 1 || cyl > lastcyl || cyl >= chancfg->zdd_cyls - ZDD_NDGNCYL)
		return((struct bz_bad *)NULL);		/* No spares */

	/*
	 * Got a real cylinder, check to see if spares exist.
	 */
	remap_cyl(cyl, chp);
	/*
	 * Compensate for BZ_SNF entries internal to the cylinder as
	 * this increments the c_bad count as well as decrements the
	 * c_free count in remap_cyl.
	 */
	bzp = bbl->bz_bad;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl < cyl; bzp++)
		continue;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl; bzp++)
		if (bzp->bz_rtype == BZ_SNF && bzp->bz_rpladdr.da_cyl == cyl)
			++chp->c_free;

	if (chp->c_bad >= chp->c_free)
		return((struct bz_bad *)NULL);		/* No spares */

	bzp = find_bbl_entry(bbl, (int)snf->snf_addr.da_cyl,
				snf->snf_addr.da_head, snf->snf_addr.da_sect);
	/*
	 * Spares exist. Now find one.
	 */
	hp = get_replace(chp);
	bzp->bz_rpladdr.da_sect = hp->h_sect;
	bzp->bz_rpladdr.da_head = hp->h_head;
	bzp->bz_rpladdr.da_cyl  = hp->h_cyl;
	if (verbose)
		printf("Revectoring (%d, %d, %d) to (%d, %d, %d).\n",
			bzp->bz_cyl, bzp->bz_head, bzp->bz_sect,
			hp->h_cyl, hp->h_head, hp->h_sect);
	return(bzp);
}

/*
 * init_cylmap
 *	init cyls structure for new cylinder.
 */
init_cylmap(cyl, chp)
	int cyl;
	register struct cyl_hdr	*chp;
{
	register struct hdr	*hp;
	register struct track_hdr *tp;

	chp->c_free = chancfg->zdd_spare * chancfg->zdd_tracks;
	chp->c_bad = 0;

	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		tp->t_free = chancfg->zdd_spare;
		tp->t_bad = 0;
	}
	/*
	 * Initialize runts - if any.
	 */
	if (chancfg->zdd_runt) {
		for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
			hp = &tp->t_hdr[totspt];
			hp->h_type = ZD_BADUNUSED;
			hp->h_flag = 0;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
		}
	}
	setskew(cyl, chp);
}

/*
 * setup_cyl
 *	Build in-core representation of cylinder's headers and
 *	mark defective sectors.
 */
setup_cyl(cyl, chp, bzp, flag)
	register int	cyl;		/* cylinder number */
	register struct cyl_hdr	*chp;
	struct	bz_bad	*bzp;		/* bad block list entries */
	int	flag;			/* Z_FORMAT or Z_REFORMAT */
{
	/*
	 * If format, set-up cylinder from simple (only BZ_PHYS) bad block list.
	 * If reformat, all 3 replacement entries possible.
	 */
	init_cylmap(cyl, chp);
	if (flag == Z_FORMAT)
		mark_spots(cyl, chp, bzp);		/* New format */
	else {
		remark_spots(cyl, chp, bzp, flag);
		setskew(cyl, chp);
	}
}

/*
 * map_cyl
 *	Called by format to set up cylinder headers taking into account
 *	defects only within the cylinder.
 */
map_cyl(cyl, chp)
	register int	cyl;		/* cylinder number */
	register struct cyl_hdr	*chp;
{
	register struct hdr *hp;
	int	bootbad;		/* # bad after slipping boot track */
	int	bootfree;		/* # free after slipping boot track */
	unchar	savflag;

	if (chp->c_bad == 0 || chp->c_free == 0)
		return;
	if (debug)
		printf("\nmap_cyl: (%d,,) do_slippage\n",cyl);

	do_slippage(cyl, chp);		/* Resolve via slippage */

	if (chp->c_bad == 0 || chp->c_free == 0)
		return;
	if (cyl == 1) {
		/*
		 * Don't allow autorevect into track 0.
		 */
		bootfree = chp->c_trk[0].t_free;
		chp->c_trk[0].t_free = 0;

		bootbad = 0;
		if (chp->c_trk[0].t_bad) {
			/*
			 * If additional errors are in the first track, then
			 * do NOT autorevector from this track.
			 * If no errors are in first track, then
			 * layout normally.
			 *
			 * Save count. SNF revectoring will be done from
			 * this track. Clear t_bad so that do_autorevect
			 * will ignore the track.
			 */
			bootbad = chp->c_trk[0].t_bad;
			if (bootbad > (chancfg->zdd_sectors - (BBSIZE >> DEV_BSHIFT))) {
				fflush(stdout);
				fprintf(stderr, "Ruptured disk - too many ");
				fprintf(stderr, "errors (%d) in bootstrap track.\n",
						bootbad + chancfg->zdd_spare);
				fprintf(stderr, "...exiting\n");
				exit(11);
			}
			chp->c_trk[0].t_bad = 0;
		}
	}

	if (debug)
		printf("\nmap_cyl: (%d,,) do_autorevect\n",cyl);

	do_autorevect(cyl, chp);	/* Resolve with auto-revectoring */
	if (chp->c_bad == 0 || chp->c_free == 0)
		return;

	if (cyl == 1) {
		if (debug)
			printf("map_cyl: cyl = 1\n");

		chp->c_trk[0].t_free = bootfree;

		if (bootbad) {
			chp->c_trk[0].t_bad = bootbad;
			bootbad = 0;
			/*
			 * Find bad blocks on track.
			 * slip the track to get BBSIZE/DEV_BSIZE logically
			 * contiguous GOODSECT sectors.
			 */
			hp = chp->c_trk[0].t_hdr;
			for (; hp < &chp->c_trk[0].t_hdr[totspt]; hp++) {
				if ((hp->h_flag & ZD_TORESOLVE) == 0)
					continue;
				hp->h_type = ZD_BADUNUSED;
				savflag = hp->h_flag;
				hp->h_sect = hp - chp->c_trk[0].t_hdr;
				(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
				hp->h_flag = savflag;
				if (verbose)
					printf("Boot track revector (1, 0, %d).\n",
						hp->h_sect);
				bootbad++;
			}
			slip_track(1, 0, chp);
			/*
			 * Now mark these ZD_BADUNUSED sectors to
			 * the logical disk addresses that were just slipped
			 * off the end of the track. Then do_snf.
			 */
			hp = chp->c_trk[0].t_hdr;
			for (; bootbad && hp < &chp->c_trk[0].t_hdr[totspt]; hp++) {
				if ((hp->h_flag & ZD_TORESOLVE) == 0)
					continue;
				hp->h_type = ZD_GOODSECT;
				hp->h_sect = chancfg->zdd_sectors - bootbad;
				--bootbad;
			}
		}
	}

	if (debug)
		printf("\nmap_cyl: (%d,,) do_sbf\n",cyl);

	do_snf(chp, chp);		/* Resolve with driver revectoring */
}

/*
 * remap_cyl
 *	Build in-core representation of cylinder's headers.
 *	Called by clonefmt to set up cylinder headers taking into account
 *	defects from within the cylinder and from adjacent cylinders.
 *
 *	Note: a new bad block list is not produced.
 */
remap_cyl(cyl, chp)
	register int	cyl;		/* cylinder number */
	register struct cyl_hdr	*chp;
{
	register struct hdr	*hp;
	register struct bz_bad	*bzp;

	/*
	 * First setup taking into account defects from within cylinder.
	 * surrounding cylinders which revector here.
	 * Then take into account bad blocks of SNF from current and
	 */
	init_cylmap(cyl, chp);

	bzp = bbl->bz_bad;
	for (; bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl < cyl; bzp++)
		continue;
	if (bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl == cyl)
		remark_spots(cyl, chp, bzp, Z_CLONEFORMAT);

	/*
	 * Any revectoring here? If so, mark 'em.
	 */
	for (bzp = bbl->bz_bad; bzp < &bbl->bz_bad[bbl->bz_nelem] ; bzp++) {
		if (bzp->bz_rtype != BZ_SNF)
			continue;
		if (bzp->bz_rpladdr.da_cyl == cyl) {
			chp->c_free--;
			chp->c_trk[bzp->bz_rpladdr.da_head].t_free--;
			hp = chp->c_trk[bzp->bz_rpladdr.da_head].t_hdr;
			for (; hp < &chp->c_trk[bzp->bz_rpladdr.da_head].t_hdr[totspt]; hp++)
				if (hp->h_sect == bzp->bz_rpladdr.da_sect) {
					hp->h_type = ZD_GOODSECT;
					break;
				}
		}
	}
}

/*
 * setup_ddcyl
 *	Build in-core representation of cylinder's headers and
 *	mark defective sectors.
 */
setup_ddcyl(chp, bzp)
	struct cyl_hdr	*chp;
	struct	bz_bad	*bzp;		/* bad block list entries */
{
	init_cylmap(ZDD_DDCYL, chp);
	mark_spots(ZDD_DDCYL, chp, bzp);		/* All PHYS entries */
}

/*
 * map_ddcyl
 *	Build in-core representation of cylinder's headers.
 *	Called by format to set up cylinder headers taking into account
 *	defects only within the cylinder.
 */
map_ddcyl(chp)
	register struct cyl_hdr	*chp;
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	register struct hdr *badhp;
	int	ss_badsects;			/* no. of bad special sectors */

	/*
	 * Map bad sectors in ZDD_DDCYL
	 */
	if (chp->c_bad == 0)
		return;

	badhp = (struct hdr *)NULL;
	if (chp->c_trk[0].t_bad) {
		fflush(stdout);
		fprintf(stderr, "Warning: %d error(s) on cyl %d, track 0.\n",
				chp->c_trk[0].t_bad, ZDD_DDCYL);
		/*
		 * Take care of special sectors separately since
		 * cannot slip the bad special sector.  Only 1 bad
		 * special sector and 1 other track 0 bad sector is allowed.
		 */
		ss_badsects = 0;
		tp = chp->c_trk;
		for (hp = tp->t_hdr; hp < &tp->t_hdr[ZDD_NDDSECTORS]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			badhp = hp;		/* save to patch after slip */
			tp->t_bad--;
			chp->c_bad--;
			ss_badsects++;
		}
		if (ss_badsects > 1 || chp->c_trk[0].t_bad > chancfg->zdd_spare) {
			fflush(stdout);
			fprintf(stderr, "Ruptured disk - cyl 0, track 0 has ");
			fprintf(stderr, "too many bad sectors.\n");
			fprintf(stderr, "...exiting\n");
			exit(9);
		}
	}

	if (chp->c_trk[1].t_bad) {
		fflush(stdout);
		fprintf(stderr, "Warning: %d error(s) on cyl %d, track 1.\n",
			chp->c_trk[1].t_bad, ZDD_DDCYL);
	}

	/* Replace via slippage */
	do_slippage(ZDD_DDCYL, chp);

	if (badhp != (struct hdr *)NULL) {
		/*
		 * Now that do_slippage has been fooled. Go zap the
		 * header for the bad special sector.
		 */
		badhp->h_type = ZD_BADUNUSED;
		badhp->h_cyl = ZD_BUCYL;
		badhp->h_head = ZD_BUHEAD;
		badhp->h_sect = ZD_INVALSECT;
	}

	if (chp->c_bad == 0)
		return;			/* No more bad sectors */

	/*
	 * Mark bad sectors and make BZ_PHYS entries.
	 * These will NOT be revectored. However, the sectors will be
	 * unused in normal operation.
	 */
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad == 0)		/* no spots */
			continue;

		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (verbose)
				printf("Slipping past bad sector at physical (%d, %d, %d).\n",
					ZDD_DDCYL, (tp - chp->c_trk),
					(hp - tp->t_hdr));
		}
		slip_track(ZDD_DDCYL, (tp - chp->c_trk), chp);
		tp->t_bad = 0;
	}
}

/*
 * setup_dgncyl
 *	Build in-core representation of cylinder's headers and
 *	mark defective sectors.
 */
setup_dgncyl(cyl, chp, bzp)
	struct cyl_hdr	*chp;
	struct	bz_bad	*bzp;		/* bad block list entries */
{
	init_cylmap(cyl, chp);
	mark_spots(cyl, chp, bzp);		/* All PHYS entries */

}

/*
 * map_dgncyl
 *	Build in-core representation of cylinder's headers.
 *	Called by format to set up cylinder headers taking into account
 *	defects only within the cylinder.
 */
map_dgncyl(cyl, chp)
	int	cyl;
	register struct cyl_hdr	*chp;
{
	register struct hdr *hp;
	register struct track_hdr *tp;

	if (chp->c_bad == 0)
		return;

	/*
	 * Do slippage where necessary.
	 */
	do_slippage(cyl, chp);		/* Resolve via slippage? */

	if (chp->c_bad == 0)
		return;			/* Resolved via slippage. */

	/*
	 * Mark bad sectors and make BZ_PHYS entries.
	 * These will NOT be revectored. However, the sectors should not be
	 * used during normal operation.
	 */
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		if (tp->t_bad == 0)		/* no spots */
			continue;
		if (tp->t_bad > (totspt - ZDD_NDGNSPT)) {
			fflush(stdout);
			fprintf(stderr, "Ruptured disk - too many errors ");
			fprintf(stderr, "(%d) in DGN cyl %d, track %d.\n",
					tp->t_bad + chancfg->zdd_spare,
					cyl, (tp - chp->c_trk));
			fprintf(stderr, "...exiting\n");
			exit(10);
		}

		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
			if ((hp->h_flag & ZD_TORESOLVE) == 0)
				continue;
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (verbose)
				printf("Slipping past bad sector at physical (%d, %d, %d).\n",
					cyl, tp - chp->c_trk, hp - tp->t_hdr);
		}
		slip_track(cyl, (tp - chp->c_trk), chp);
		tp->t_bad = 0;
	}
}

/*
 * format_cyl
 *	format cylinder.
 */
format_cyl(cyl, chp)
	int	cyl;		/* Cylinder number */
	struct cyl_hdr	*chp;
{
	register struct track_hdr *tp;
	register struct hdr *hp;
	register int size;
	struct cb lcb;			/* cb for ioctl */

	if (debug)
		printf("zd format_cyl(%d,,)\n",cyl);

	bzero((caddr_t)&lcb, sizeof(struct cb));
	size = totspt;
	if (chancfg->zdd_runt)
		size++;
	size = ROUNDUP(sizeof(struct hdr) * size, CNTMULT);

	/*
	 * Format cylinder via ioctl to zdc driver.
	 */
	lcb.cb_cmd = ZDC_FMTTRK;
	lcb.cb_cyl = cyl;
	lcb.cb_iovec = 0;
	for (tp = chp->c_trk; tp < &chp->c_trk[chancfg->zdd_tracks]; tp++) {
		/*
		 * Remark spares as ZD_GOODSECT so that they can be verified.
		 */
		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)
			if (hp->h_type == ZD_GOODSPARE)
				hp->h_type = ZD_GOODSECT;
		lcb.cb_head = tp - chp->c_trk;
		lcb.cb_count = size;
		lcb.cb_addr = (ulong)tp->t_hdr;
		lcb.cb_iovec = 0;
		if (ioctl(fd, ZIOCBCMD, (char *)&lcb) < 0) {
			perror("ZDC_FMTTRK ioctl error");
			exit(8);	
		}
	}
}

/*
 * format_dd
 *	format disk description cylinder - cylinder 0.
 *	Format and write special sectors which contain the zdcdd structure.
 */
format_dd(chp)
	struct cyl_hdr *chp;
{
	register int i;	
	register unchar *cp, *sp;
	struct track_hdr *tp;
	unchar *wss;			/* Write special sector buffer */
	unchar *rss;			/* Read special sector buffer */
	struct hdr *sshdr;		/* special sector header buffer */
	int size, errors;
	unchar	chksum;
	struct cb lcb;			/* cb for ioctl */
	bool_t skip;

	/*
	 * Format the cylinder. (normally);
	 */
	format_cyl(ZDD_DDCYL, chp);
	if (errors = check_hdrs(ZDD_DDCYL, chp, TRUE)) {
		fflush(stdout);
		fprintf(stderr, "Warning:  %d sector header errors ", errors);
		fprintf(stderr, " found on cylinder zero.\n");
		fprintf(stderr, "Warning:  Drive may be unusable.\n");
		if (!(args & B_OVERWRITE)) {
			fprintf(stderr,"Repeat using -o to overide and ");
			fprintf(stderr," continue format.\nExiting...\n");
			exit(1);	
		}
	}

	/*
	 * Now set up Special Sectors.
	 */
	wss = (unchar *)MALLOC_ALIGN(DEV_BSIZE, types[disk].align);
	rss = (unchar *)MALLOC_ALIGN(DEV_BSIZE, types[disk].align);
	size = ROUNDUP(sizeof(struct hdr), CNTMULT);
	sshdr = (struct hdr *)MALLOC_ALIGN(size, types[disk].align);
	if (!wss || !rss || !sshdr) {
		fflush(stdout);
		fprintf(stderr, "format_dd: malloc error\n");
		fprintf(stderr, "...exiting\n");
		exit(1);	
	}
	bzero((caddr_t)wss, DEV_BSIZE);
	bzero((caddr_t)rss, DEV_BSIZE);
	bzero((caddr_t)sshdr, size); 

	/*
	 * initialize special sector with channel configuration.
	 */
	chancfg->zdd_format_desc.fd_rev = FORMAT_VERSION;
	chancfg->zdd_format_desc.fd_passes = fullpasses + defectpasses;
	sp = (unchar *)wss;
	cp = (unchar *)chancfg;
	chksum = 0;
	while (cp < &chancfg->zdd_checksum) {
		chksum ^= *cp;
		*sp++ = *cp++;
	}
	*sp = chksum;				/* Append checksum */

	bzero((caddr_t)&lcb, sizeof(struct cb));
	lcb.cb_cyl = ZDD_DDCYL;
	lcb.cb_head = 0;
	tp = chp->c_trk;
	for (i = 0; i < ZDD_NDDSECTORS; i++) {
		/*
		 * An attempt must still be made to format a
		 * defective special sector, but not write it.
		 */
		if (tp->t_hdr[i].h_type == ZD_BADUNUSED) {
			skip = 1;
		} else {
			skip = 0;
			tp->t_hdr[i].h_type = ZD_GOODSS;
		}

		lcb.cb_iovec = 0;
		lcb.cb_cmd = ZDC_FMT_SS;
		lcb.cb_sect = i;
		*sshdr = tp->t_hdr[i];
		lcb.cb_addr = (ulong)sshdr;
		lcb.cb_count = ROUNDUP(sizeof(struct hdr), CNTMULT);
		if (ioctl(fd, ZIOCBCMD, (char *)&lcb) < 0) {
			fflush(stdout);
			fprintf(stderr, "Special sector %d,%d,%d ",
				lcb.cb_cyl, lcb.cb_head, i);
			perror("ZDC_FMT_SS ioctl error");
			exit(1);
		}
		if (skip) continue;

		lcb.cb_addr = (ulong)wss;
		lcb.cb_cmd = ZDC_WRITE_SS;
		lcb.cb_count = ZDD_SS_SIZE;
		if (ioctl(fd, ZIOCBCMD, (char *)&lcb) < 0) {
			fflush(stdout);
			fprintf(stderr, "Special sector %d,%d,%d ",
				lcb.cb_cyl, lcb.cb_head, i);
			perror("ZDC_WRITE_SS ioctl error");
			exit(1);	
		}
		lcb.cb_addr = (ulong)rss;
		lcb.cb_cmd = ZDC_READ_SS;
		lcb.cb_count = ZDD_SS_SIZE;
		if (ioctl(fd, ZIOCBCMD, (char *)&lcb) < 0) {
			fflush(stdout);
			fprintf(stderr, "Special sector %d,%d,%d ",
				lcb.cb_cyl, lcb.cb_head, i);
			perror("WARNING: ZDC_READ_SS ioctl error");
		}
		for (cp = wss, sp = rss; sp < &rss[ZDD_SS_SIZE]; cp++, sp++) {
			if (*cp != *sp) {
				fflush(stdout);
				fprintf(stderr, 
					"WARNING: Special sector %d,%d,%d ",
					lcb.cb_cyl, lcb.cb_head, i);
				fprintf(stderr,"not written/read correctly\n");
				fprintf(stderr, "First Mismatch in byte %d\n",
					cp - wss);
				break;
			}
		}
	}
}

/*
 * check_hdrs
 *	Quickly check that the headers have been formatted 
 *	correctly.  If a defective one is found, add its 
 *	track to the header validation suspect list for 
 *	more thorough testing and correction in a later pass.
 *
 *	If the 'fix_bad_unused' flag is TRUE, attempt to 
 *	correct a defective header marked bad/unused.  This
 *	may be necessary because addbad will reformat the
 *	entire cylinder and write data back before the
 *	regular header validation is performed and we must
 *	attempt to ensure all bad/unused sectors needing 
 *	adjustments are redone.  Also, test the cylinder
 *	more heavily, since data will be restored onto it.
 *	Keep in mind that if the addbad is not the result
 *	of the header defect pass, we must adjust everything
 *	from scratch - no historical information available.
 */
int
check_hdrs(cyl, chp, fix_bad_unused)
	int	cyl;		/* Cylinder number */
	struct cyl_hdr	*chp;
{
	register struct track_hdr *tp;
	register struct hdr *hp;
	register struct hdr *rhp;
	register struct hdr thp;
	int track, errors, i, j, pass, sindex;

	errors = 0;
	if (debug)
	    printf("zd check_hdrs(%d,,) tracks %d, count=%d\n",
	     	   cyl, chancfg->zdd_tracks, n_hdrs);

	if (fix_bad_unused) {
		readjust_headers(cyl);
		pass = ZDNBEATS;
	} else
		pass = 1;

	for ( ; pass > 0; pass--) {
		for (track = 0, tp = chp->c_trk; 
	     	     track < chancfg->zdd_tracks; track++, tp++) {

	    		if (read_hdrs(cyl, track)) {
				/*
				 * read_hdrs will verify data ECC which might 
				 * fail on sectors already marked bad so change
				 * the stratergy to first try and adjust the
				 * header to avoid the data sync and if that
				 * fail to pound on the track to
				 * verify that each sector is real and can be 
				 * read.
				 */
				if (errcode == ZDC_NDS) {
					if (!fix_bad_unused) {
		    				sindex=add_suspect(cyl, track);
						continue;
					} else if (fix_data_sync(cyl, track, tp) != -1)
						continue;
				}
				if (poundtrack(cyl, track, chp) == FAIL) {
					fprintf(stderr, "%s: Bad headers on (%d,%d)\n", 
						diskname, cyl, track);
					fail_drive(cyl, track, errcode);
				} 
				/*
				 * Only pound once.
				 */
				pass=1;
				if (verbose) {
					printf("%s: Pounded(%d,%d) - no problems found\n", 
						diskname, cyl, track);
				}
				continue;
			}
	    		for (i = 0, hp = tp->t_hdr; i < n_hdrs; hp++, i++) {
				rhp = &headers[i].hdr;
				if (!fix_bad_unused 
				||  hp->h_type != ZD_BADUNUSED 
				||  BUHDR_CMPR(rhp)) {
					if (!HDR_CMPR(hp, rhp)) { 
		    				sindex=add_suspect(cyl, track);
						if (hp->h_type != ZD_BADUNUSED 
						||  !BUHDR_CMPR(rhp)) 
		    					errors++;
		    				if (!fix_bad_unused)
		    					break;	
					}
		 			continue;
				}

				sindex = add_suspect(cyl, track);
				if (sindex < 0)
					break;  /* list full - give up */
				errors++;
				for ( ; ; ) {
					if (!BUHDR_CMPR(rhp)) {
			    			j = ZDNBEATS; 
						if (debug) {
							dump_hdr(hp, rhp, j);
							dump_hdrs();
						}
			    			adjust_header(sindex, i);
					} else if (--j == 0) 
			    			break;	/* Fixed; next sector */
					if (read_hdrs(cyl, track)) {
						fprintf(stderr,"cylinder %d track %d is can not be fixed\n", cyl, track);
						break;
					}
				} 
	    		}
		}
	}
	if (debug)
		printf("zd check_hdrs(%d,,) done (%d errors).\n",cyl, errors);

	return (errors);
}

/*
 * fix a bad data sync problem by moving the header over as if it
 * were a sector zero problem.
 */
int
fix_data_sync(cyl, track, tp)
	int cyl;
	int track;
	struct track_hdr *tp;
{
	int sindex;
	struct hdr *hp;
	int	i;

	if ((sindex = add_suspect(cyl, track)) == -1)
		return(-1);
	for (i = 0, hp = tp->t_hdr; i < totspt; hp++, i++) {
		if (hp->h_type == ZD_BADUNUSED || hp->h_type == ZD_BADREVECT) {
			if (debug)
				printf("fix_data_sync: trying sector %d %i\n");
			adjust_header(sindex, i);
			if (!read_hdrs(cyl, track)) {
				if (verbose) {
					printf("%s: data sync fixed at phys(%d,%d,%d)\n", 
						diskname, cyl, track, i);
				}
				return(hp->h_sect);
			}
			/*
			 * did not work so undo adjust.
			 */
			suspect[sindex].adjustment[i]--;
		}
	}
	return (-1);
}


/*
 * setup_chancfg
 *	Get the channel configuration for the channel with
 * 	the drive to be formatted.  If not set or set differently
 *	then specified by the -t drive type option, attempt
 *	to set it.  (When the chancfg is already set, the driver
 *	will only allow it to be reset if this is the only
 *	formatted drive on the channel)
 *
 * Results stored in "chancfg" global
 */
int
setup_chancfg()
{
	extern	int	trashes;
	struct cb cb;
	int notset = 0;

	/*
	 * Get channel configuration
	 */
	chancfg = (struct zdcdd *)MALLOC_ALIGN(sizeof(struct zdcdd), 
					       sizeof(struct zdcdd));
	if (chancfg == NULL) {
		perror("malloc error");
		return(-1);
	}
	bzero((caddr_t)&cb, sizeof(struct cb));
	cb.cb_cmd = ZDC_GET_CHANCFG;
	cb.cb_addr = (ulong)chancfg;
	cb.cb_count = sizeof(struct zdcdd);
	cb.cb_iovec = 0;
	if (ioctl(fd, ZIOCBCMD, (char *)&cb) < 0) {
		perror("ZIOCBCMD ZDC_GET_CHANCFG error");
		return(-1);
	}

	if (zdinfo == NULL) {
	 	if ((zdinfo = getzdinfobydtype(chancfg->zdd_drive_type)) == NULL) {
			fflush(stdout);
			fprintf(stderr, "unknown disk type type %d\n");
			if (!(args & B_OVERWRITE))
				return (-1);
		}
	}

	if (function == INFO)
		return (0);

	if (cb.cb_compcode == ZDC_NOCFG)
		notset = 1;	
	else
		notset = print_chancfg(chancfg);

	if (notset && !t_arg) {
		fflush(stdout);
		fprintf(stderr, "unable to set channel configuration - ");
		fprintf(stderr, "-t disktype option not specified\n");
		return(-1);
	}
	if (t_arg && (notset 
		      || (zdinfo->zi_zdcdd.zdd_drive_type 
		       != chancfg->zdd_drive_type))) {
		*chancfg = zdinfo->zi_zdcdd;
		bzero((caddr_t)&cb, sizeof(struct cb));
		cb.cb_cmd = ZDC_SET_CHANCFG;
		cb.cb_addr = (ulong)chancfg;
		cb.cb_count = sizeof(struct zdcdd);
		cb.cb_iovec = 0;
		trashes = 1;
#ifdef BSD
		if (args & B_OVERWRITE)
			trashes = 0;
		are_you_sure();
#endif
		/*
		 * we now need exclusive access to the disk.
		 * we use readonly mode for now.
		 */
		
		if ((usep->open_flags & O_EXCL) == 0 ) {
			close(fd);
			if ((fd = open(openname, usep->open_flags|O_EXCL)) == -1) {
				fflush(stdout);
				if (errno == EBUSY) 
					fprintf(stderr, "Device is busy \n");
				perror("open exclusive failed: ");
			}
		}
		if (ioctl(fd, ZIOCBCMD, (char *)&cb) < 0) {
			fflush(stdout);
			if (errno == EACCES)
				fprintf(stderr, "Unable to change channel configuration\nOther zdc discs on this controller may have different configurations\n");
			else
				perror("ZIOCBCMD ZDC_SET_CHANCFG failed");
			return(-1);
		}
		print_chancfg(chancfg);
	}
	return(0);
}

/*
 * print_chancfg
 *	print interesting items from disk description structure.
 */
int
print_chancfg(cfg)
	register struct zdcdd *cfg;
{
	int	ret;

	if (bcmp(&zdinfo->zi_zdcdd, cfg, 
		((char *)&cfg->zdd_format_desc - (char *)cfg))) {
		fflush(stdout);
		fprintf(stderr, "Channel config data in diskinfo file ");
		fprintf(stderr, "doesn't match actual channel config\n");
		if (verbose)
			print_zdcdd(&zdinfo->zi_zdcdd, cfg);
		if (!(args & B_OVERWRITE)) {
			fprintf(stderr, "...exiting use -o -t type to overide use -v for differences\n");
			exit(1);
		}
		ret = 1;
	} else
		ret = 0;
	printf("Channel configuration: ");
	printf("Drive type %d - %s. formated version %d\n",
		cfg->zdd_drive_type, zdinfo->zi_name,
		cfg->zdd_format_desc.fd_rev);
	printf("Data: #cyls %d, #heads %d, #sectors %d, #spares %d.\n",
		cfg->zdd_cyls, cfg->zdd_tracks,
		cfg->zdd_sectors, cfg->zdd_spare);
	return (ret);
}
