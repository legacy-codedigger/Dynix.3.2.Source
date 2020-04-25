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

#ifdef RCS
static char rcsid[]= "@(#)$Header: zdlists.c 1.6 91/03/26 $";
#endif

/*
 * Miscellaneous routines to manipulate manufacturer's defect list
 * and the bad block list.
 */

/* $Log:	zdlists.c,v $
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#include "../saio.h"
#include "zdformat.h"
static bool_t	lcmp();

struct bad_mfg	*mfg;		/* mfg defect list */

/*
 * get_mfglist
 *	get mfglist from user.
 *
 * Operator can enter directly or enter name of file with defect list.
 * There are four fields that describe a defect. These are the
 * cylinder, head, byte offset from index, and the length of the defect.
 * Each line contains 1 mfg defect list entry.
 *
 * Return:
 *	FAIL - Error occurred reading list.
 *	SUCCESS - No problem.
 */
get_mfglist()
{
	register char *cp;
	register struct bm_mfgbad *bmp;	/* mfg bad list entry */
	int cyl, head, byte, len;	/* Items to be read */
	int	ndefects;		/* number of defects */
	int	dfd;
	int	nread;			/* no. of chars read */
	int	count;
	int	i;
	int	ts;			/* no. bytes in track */
	struct bm_mfgbad *endmfg;	/* past end of list */
	static char buffer[1024];	/* Input buffer if from file */
	char	*lastline;		/* last line in buffer */
	bool_t	err;			/* Entry bad */
	int	mfgcomp();		/* for qsort() */
	int	line;

	ndefects = 0;
	ts = (totspt * chancfg->zdd_sector_bc) + chancfg->zdd_runt;
	/*
	 * Allocate aligned memory for bad block list.
	 */
	i = ((chancfg->zdd_sectors - ZDD_NDDSECTORS) / 2) << DEV_BSHIFT;
	endmfg = (struct bm_mfgbad *)((u_int)mfg + i);
	bmp = mfg->bm_mfgbad;

nochmal:
	/*
	 * Ask for defect list. Enter manually or filename?
	 */
	do
		dfd = atoi(prompt("Enter defect list manually (0) or from file (1)? "));
	while (dfd < 0 || dfd > 1);

	if (dfd == 0)
		/* Manually */
		dfd = -1;
	else {
		/*
		 * Get file and open.
		 */
		cp = prompt("File with bad spot information? ");
		if (!*cp || *cp == '\n')
			goto nochmal;
		dfd = open(cp, 0);
		if (dfd < 0) {
			printf("Cannot open %s\n", cp);
			goto nochmal;
		}
		count = 0;
		lastline = buffer;
		cp = buffer;
	}

	/*
	 * Read in mfg defect list.
	 */
	for(line = 1;;line++) {
		if (dfd < 0) {
			/*
			 * prompt user.
			 */
			printf("Enter bad spot %d (cyl head byte len): ",
				ndefects + 1);
			cp = prompt("");
			if ( *cp == '\0')
				break;
		} else {
			if (cp == lastline) {
				count = count - (lastline - buffer);
				bcopy(lastline, buffer, count);
				lastline = &buffer[count];
				/* Note kludge for ts tape */
				nread = read(dfd, lastline,
					((sizeof buffer) - count) & ~511);
				if (nread <= 0 )
					break;
				count += nread;
				/*
				 * Replace newlines with null.
				 */
				for (cp = buffer; cp < buffer + count; cp++)
					if (*cp == '\n') {
						*cp = 0;
						lastline = cp + 1;
					}
				cp = buffer;
			}
			if (cp == lastline) {
				/*
				 * empty buffer.
				 */
				cp = buffer;
				buffer[0] = 0;		/* set to null */
			}
		}
		i = scani(cp, 4, &cyl, &head, &byte, &len);
		if (i != 4) {
			switch (*cp) {
			case '#':
				if (verbose)
					printf("%s\n", cp);
				/* fall through */
			case '\n':	
			case '\0':	
			case 'C':	/* see printf_mfg() */
			case 'N':
			case 'R':
			case 'M':
			case 'D':
				/*
				 * Skip to end of line
				 */
				for (; *cp++;);
				continue;
			case 'B':
				/*
				 * Done
				 */
				nread = 0;
				break;
			default:
				printf( "warning: line# %d bad syntax in defect ", line);
				printf( " - expecting 4 numbers - skipping\n");
				/*
				 * Skip to end of line
				 */
				for (; *cp++;);
				continue;
			}
			/*
			 * Done
			 */
			break;
		} else {
			/*
			 * Have spot description. Now verify.
			 */
			err = FALSE;
			if (cyl < 0 || cyl >= chancfg->zdd_cyls) {
				printf("Bad cylinder number %d.\n", cyl);
				err = TRUE;
			}
			if (head < 0 || head >= chancfg->zdd_tracks) {
				printf("Bad head number %d.\n", head);
				err = TRUE;
			}
			if (byte < 0 || byte >= ts) {
				printf("Bad byte number %d.\n", byte);
				err = TRUE;
			}
			if (len <= 0 || byte + HOWMANY(len, NBBY) - 1 >= ts) {
				printf("Bad length %d.\n", len);
				err = TRUE;
			}
			if (err == FALSE) {
				ndefects++;
				if (bmp == endmfg) {
					printf("Ruptured disk! Too many defects - %d.\n",
							ndefects);
					close(dfd);
					return(FAIL);
				}
				bmp->bm_cyl = cyl;
				bmp->bm_head = head;
				bmp->bm_pos = byte;
				bmp->bm_len = len;
				++bmp;
			}
			/*
			 * Skip to end of line
			 */
			for (; *cp++;)
				continue;
		} 
	} /* end of main loop */

	if (dfd >= 0) {
		close(dfd);
		if (nread > 0) {
			printf("Entire file not read!\n");
			return(FAIL);
		}
	}

	mfg->bm_nelem = ndefects;
	/*
	 * Sort list.
	 */
	qsort(mfg->bm_mfgbad, ndefects, sizeof(struct bm_mfgbad), mfgcomp);

	/*
	 * calculate checksum
	 */
	i = (mfg->bm_nelem * sizeof(struct bm_mfgbad)) / sizeof(long);
	mfg->bm_csn = getsum((long *)mfg->bm_mfgbad, i, (long)mfg->bm_nelem);

	printf("%d bad spots entered.\n", ndefects);
	if (verbose)
		print_mfglist();
	return(SUCCESS);
}

/*
 * mfgcomp
 *	Sort routine to sort defect list.
 *	Called via qsort.
 */
int
mfgcomp(l1, l2)
	register struct bm_mfgbad *l1, *l2;
{
	if (l1->bm_cyl < l2->bm_cyl)
		return(-1);
	if (l1->bm_cyl > l2->bm_cyl)
		return(1);

	/* Same cylinder */
	if (l1->bm_head < l2->bm_head)
		return(-1);
	if (l1->bm_head > l2->bm_head)
		return(1);

	/* Same Head */
	if (l1->bm_pos < l2->bm_pos)
		return(-1);
	if (l1->bm_pos > l2->bm_pos)
		return(1);
	printf("WARNING: Duplicate defect entry  (cyl %d, head %d, pos %d).\n",
		l1->bm_cyl, l1->bm_head, l1->bm_pos);
	return(0);		/* Same Position!?! */
}

/*
 * Read mfg defect list from cylinder 0 on disk.
 *
 * Return:
 *	FAIL - Could not read mfg defect list.
 *	SUCCESS - Success. (Global "mfg" points to defect list).
 */
read_mfglist()
{
	register int block;		/* Block number in list */
	register int track;		/* Track number */
	register int size;
	int	bbsize;			/* Bad block list size */
	char	*buf;			/* read buffer */
	bool_t	gotit;			/* flag */

	bbsize = (chancfg->zdd_sectors - ZDD_NDDSECTORS) / 2;
	printf("Reading mfg defect list from sector %d.\n",
			ZDD_NDDSECTORS + bbsize);
	buf = (char *)mfg;
	size = bbsize;
	for(block = 0; block < size; block++) {
		track = 0;
		gotit = FALSE;
		lseek(fd, (ZDD_NDDSECTORS + bbsize + block) << DEV_BSHIFT, L_SET);
		while (track < MIN(chancfg->zdd_tracks, BZ_NBADCOPY)) {
			if (read(fd, buf, DEV_BSIZE) == DEV_BSIZE) {
				gotit = TRUE;
				break;
			}
			/*
			 * Could not read. Try next track.
			 */
			track++;
			lseek(fd, (track * chancfg->zdd_sectors + bbsize + block)
					<< DEV_BSHIFT, L_SET);
		}
		if (gotit == FALSE) {
			printf("Could not read block %d of mfg defect list.\n",
					block);
			return(FAIL);
		}
		if (block == 0) {
			size = (mfg->bm_nelem * sizeof(struct bm_mfgbad))
				+ sizeof(struct bad_mfg) - sizeof(struct bm_mfgbad);
			size = HOWMANY(size, DEV_BSIZE);
		}
		buf += DEV_BSIZE;
	}
	/*
	 * Verify checksum
	 */
	size = (mfg->bm_nelem * sizeof(struct bm_mfgbad)) / sizeof(long);
	if (mfg->bm_csn != getsum((long *)mfg->bm_mfgbad, size,
				  (long)mfg->bm_nelem)) {
		printf("Checksum failed!\n");
		return(FAIL);
	}
	return(SUCCESS);
}

/*
 * write_mfglist
 *	write mfg defect list to cylinder 0.
 */
write_mfglist()
{
	register int block;		/* Block number in list */
	register int track;		/* Track number */
	register int size;
	register int where;		/* Where to seek */
	int	bbsize;			/* size of Bad block list area */
	int	error;			/* errors while writing bbl */
	caddr_t	buf;			/* write buffer */
	static caddr_t	ibuf = NULL;	/* read buffer */

	error = 0;
	bbsize = (chancfg->zdd_sectors - ZDD_NDDSECTORS) / 2;
	printf("Writing mfg defect list to sector %d.\n", bbsize + ZDD_NDDSECTORS);
	/*
	 * Allocate read buffer
	 */
	if (ibuf == NULL) {
		callocrnd(DEV_BSIZE);
		ibuf = calloc(DEV_BSIZE);
	}
	size = (mfg->bm_nelem * sizeof(struct bm_mfgbad))
		+ sizeof(struct bad_mfg) - sizeof(struct bm_mfgbad);
	size = HOWMANY(size, DEV_BSIZE);

	/*
	 * Write all the copies
	 */
	for(track = 0; track < MIN(chancfg->zdd_tracks, BZ_NBADCOPY); track++) {
		buf = (char *)mfg;

		/*
		 * Try writing 1 block at a time.
		 */
		for (block = 0; block < size; block++) {
			if (track == 0)
				where = ZDD_NDDSECTORS + bbsize + block;
			else
				where = track * chancfg->zdd_sectors + bbsize
						+ block;
			where <<= DEV_BSHIFT;
			lseek(fd, where, L_SET);
			if (write(fd, buf, DEV_BSIZE) != DEV_BSIZE) {
				printf("Cannot write block %d on track %d.\n",
						block, track);
				error++;
			} else {
				lseek(fd, where, L_SET);
				if (read(fd, ibuf, DEV_BSIZE) != DEV_BSIZE) {
					printf("Cannot read block %d on track %d.\n",
						block, track);
					error++;
				} else if (lcmp((long *)buf, (long *)ibuf,
					DEV_BSIZE/sizeof(long)) == FALSE) {
					printf("Verify miscompare block %d on track %d\n",
						block, track);
				}
			}
			buf += DEV_BSIZE;
		}
	}
	/*
	 * Report errors
	 */
	if (error)
		printf("Warning: %d errors occurred.\n", error);
	printf("Mfg defect list written.\n");
}

/*
 * print_mfglist
 *	Print the mfg defect list in human readable form.
 */
print_mfglist()
{
	register int i;
	register struct bm_mfgbad *bmp;

	printf("\nMfg defect list.\n\n");
	printf("Checksum: 0x%x\nNumber of entries: %d\n\n",
		mfg->bm_csn, mfg->bm_nelem);
	if (mfg->bm_nelem == 0)
		return;
	printf("Cylinder	Head	Position	Length\n");
	for (i = 0, bmp = mfg->bm_mfgbad; i < mfg->bm_nelem; i++, bmp++) {
		printf("%d		%d	%d		%d\n",
			bmp->bm_cyl, bmp->bm_head, bmp->bm_pos, bmp->bm_len);
	}
	printf("\n");
}

/*
 * create_badlist()
 *	Create bad block list from mfg defect list.
 *
 * Note: all entries will have BZ_PHYS rtype.
 */
create_badlist()
{
	register int i;
	register struct bm_mfgbad *bmp;	/* mfgbad list entry */
	register struct bz_bad *bzp;	/* bad block list entry */
	register int ss;		/* Sector size */
	int ts;				/* track size */
	int sect;			/* Sector # with possible error */
	int start;			/* start byte in sector of spot */
	int last;			/* last byte in sector of spot */
	int maxentries;			/* max bz_bad entries in bbl */
	int data_sync;			/* in sector position of data sync */
	int data_synce;			/* end of data sync */
	int header_sync;		/* in sector position of header sync */
	int header_synce;		/* end of header sync */
	int header_field;		/* in sector position of header info */
	int header_fielde;		/* end of header info */
	char *where;			/* where error is string */

	/*
	 * determine maxentries for bad block list.
	 */
	i = ZDMAXBAD(chancfg);
	i -= (sizeof(struct zdbad) - sizeof(struct bz_bad)); 
	maxentries = i / sizeof(struct bz_bad);
	bzp = bbl->bz_bad;

	/*
	 * Determine header size, data size, sector size, and track size.
	 */
	ts = ZDBYTESPT(chancfg);
	ss = chancfg->zdd_sector_bc;
	header_sync  = chancfg->zdd_ddc_regs.dr_hpr_bc;
	header_synce = header_sync + chancfg->zdd_ddc_regs.dr_hs1_bc +
		                     chancfg->zdd_ddc_regs.dr_hs2_bc;
	header_field = header_synce;
	header_fielde= header_field + chancfg->zdd_ddc_regs.dr_hbc;
	data_sync    = chancfg->zdd_hdr_bc + chancfg->zdd_ddc_regs.dr_dpr_wr_bc;
	data_synce   = data_sync +   chancfg->zdd_ddc_regs.dr_ds1_bc +
		     		     chancfg->zdd_ddc_regs.dr_ds2_bc;

	/*
	 * Find all bad blocks and add to bad block list.
	 * Ignore those errors which do not cause a bad block.
	 */
	if (verbose) {
		printf("Sector           size %d\n",ss);
		printf("header pre-amble size %d\n", chancfg->zdd_strt_ign_bc);
		printf("header sync is at     %d-%d\n", header_sync, 
								header_synce);
		printf("header address is at  %d-%d\n", header_field, 
								header_fielde);
		printf("header ends at        %d\n", chancfg->zdd_hdr_bc);
		printf("data sync is at       %d-%d\n", data_sync, data_synce);
		printf("data ends at          %d\n", ss - chancfg->zdd_end_ign_bc);
	}

	for (i = 0, bmp = mfg->bm_mfgbad; i < mfg->bm_nelem; i++, bmp++) {
		/*
		 * Don't care if spot occurs within runt or Gap3 of
		 * last sector. Add 1 as added margin for last sectors
		 * data part.  It will be tested during header validation.
		 */
		if (bmp->bm_pos >= (ts - chancfg->zdd_runt - chancfg->zdd_end_ign_bc)) {
			/*
			 * Don't care about errors in the runt sector.
			 */
			if (verbose)
				printf("Ignored defect (%d,%d,) %d at end of track.\n",
					bmp->bm_cyl, bmp->bm_head, bmp->bm_pos);
			continue;
		}

		sect = bmp->bm_pos / ss;
		start = bmp->bm_pos % ss;
		/*
		 * must assume worst case in that defect starts at
		 * end of byte boundary or start. Therefore a 1 bit error is
		 * in the byte at start but a 2 bit error must be
		 * assumed to span the proceding or the next byte.
		 * Note that the byte index is +-1
		 * and that the len is +-1.
		 * So add 1 to len roundup (NBBY-1) and assume starts
		 * at the 7th bit (-1).
	         */
		last = start + ((bmp->bm_len + 1 + (NBBY-1) - 1 )/NBBY) + 1;
		if (start > 2 )
			start -= 2;	/* assume error at start of -1 byte */
		else
			start = 0;

overflow:
		/*
		 * Check if spot within Head Scatter.
		 * If so, ignore spot.
		 */
		if (last < chancfg->zdd_strt_ign_bc) {
			if  (verbose)
				printf("Ignored defect (%d,%d,%d) in Head Scatter.\n",
					bmp->bm_cyl, bmp->bm_head, sect);
			continue;
		}
		/*
		 * Check if spot within Gap3.
		 * If so, ignore spot.
		 */
		if (start >= (ss - chancfg->zdd_end_ign_bc)) {
			if (last < ss) {
				if (verbose)
					printf("Ignored defect (%d,%d,%d) in Gap3.\n",
						bmp->bm_cyl, bmp->bm_head,
						sect);
				continue;
			}

			/*
			 * If last overflows this sector - skip to next sector.
			 */
			sect++;
			start -= ss; 	/* negative */
			last -= ss;
			goto overflow;
		}

		/*
		 * Sector is bad. Find if header or data section.
		 */
		if (bbl->bz_nelem != 0) {
			/*
			 * First check if already in defect list.
			 * If so, check if it goes into next sector.
			 */
			if (bmp->bm_cyl == bzp->bz_cyl &&
			    bmp->bm_head == bzp->bz_head &&
			    bzp->bz_sect == sect) {
                                if (last >= ss) {
                                        sect++;
                                        start = 0;
					start -= ss;	/* negative */
                                        last -= ss;
                                        goto overflow;  /* check next sector */                                } else
                                        continue;  /* with next defect */
			}
			bzp++;
		}
		bbl->bz_nelem++;
		if (bbl->bz_nelem > maxentries) {
			printf("Ruptured disk - too many bad sectors (%d).\n",
					maxentries);
			exit(6);
		}
		bzp->bz_cyl = bmp->bm_cyl;
		bzp->bz_head = bmp->bm_head;
		bzp->bz_sect = sect;
		bzp->bz_rtype = BZ_PHYS;

		if (start < chancfg->zdd_hdr_bc) {
			/*
			 * Header error.
			 */
			bzp->bz_ftype = BZ_BADHEAD;
			if ((start >= header_sync && start < header_synce) ||
			    (last > header_sync && last < header_synce))
				where = "Header (sync bytes)";
			else if ((start >= header_field && start < header_fielde) ||
			    (last > header_field && last < header_fielde))
				where = "Header (address bytes)";
			else
				where = "Header";
		} else {
			/*
			 * Data error.
			 */
			bzp->bz_ftype = BZ_BADDATA;
			if ((start >= data_sync && start < data_synce) ||
			    (last > data_sync && last < data_synce))
				where = "Data (sync bytes)";
			else
				where = "Data";
		}
		if (verbose)
			printf("Bad sector at  (%d,%d,%d) in %s. (%d-%d)\n",
				bzp->bz_cyl,
				bzp->bz_head, bzp->bz_sect,
				where,
				start,last);

		/*
		 * Does spot overflow into next sector?
		 */
		if (last >= ss) {
			sect++;
			start -= ss;	/* negative */
			last -= ss;
			goto overflow;
		}
	} /* end of for */
}

/*
 * bblcomp
 *	Sort routine to sort bad block list.
 *	Called via qsort.
 */
int
bblcomp(l1, l2)
	register struct bz_bad *l1, *l2;
{
	if (l1->bz_cyl < l2->bz_cyl)
		return(-1);
	if (l1->bz_cyl > l2->bz_cyl)
		return(1);

	/* Same cylinder */
	if (l1->bz_head < l2->bz_head)
		return(-1);
	if (l1->bz_head > l2->bz_head)
		return(1);

	/* Same Head */
	if (l1->bz_sect < l2->bz_sect)
		return(-1);
	if (l1->bz_sect > l2->bz_sect)
		return(1);

        /* Same sector */
        if (l1->bz_rtype < l2->bz_rtype)
                return(-1);
        if (l1->bz_rtype > l2->bz_rtype)
                return(1);

        /* same replacement type (phys auto snf ) */
        if (l1->bz_ftype < l2->bz_ftype)
                return(-1);
        if (l1->bz_ftype > l2->bz_ftype)
                return(1);
        /* same failure type (header data) */

	return(0);
}

/*
 * Read bad block list from cylinder 0 on disk.
 *
 * Return:
 *	FAIL - Could not read bad block list.
 *	SUCCESS - Success. (Global "bbl" points to defect list).
 */
read_badlist()
{
	register int block;		/* Block number in list */
	register int track;		/* Track number */
	register int size;
	char *buf;			/* read buffer */
	bool_t	gotit;			/* flag */

	size = (chancfg->zdd_sectors - ZDD_NDDSECTORS) / 2;
	printf("Reading bad block list from sector %d.\n", ZDD_NDDSECTORS);

	buf = (char *)bbl;
	for (block = 0; block < size; block++) {
		track = 0;
		gotit = FALSE;
		lseek(fd, (ZDD_NDDSECTORS + block) << DEV_BSHIFT, L_SET);
		while (track < MIN(chancfg->zdd_tracks, BZ_NBADCOPY)) {
			if (read(fd, buf, DEV_BSIZE) == DEV_BSIZE) {
				gotit = TRUE;
				break;
			}
			/*
			 * Could not read. Try next track.
			 */
			track++;
			lseek(fd, (track * chancfg->zdd_sectors + block)
					<< DEV_BSHIFT, L_SET);
		}
		if (gotit == FALSE) {
			printf("Could not read block %d of bad block list.\n",
					block);
			return(FAIL);
		}
		if (block == 0) {
			size = (bbl->bz_nelem * sizeof(struct bz_bad))
				+ sizeof(struct zdbad) - sizeof(struct bz_bad);
			size = HOWMANY(size, DEV_BSIZE);
		}
		buf += DEV_BSIZE;
	}
	size = (bbl->bz_nelem * sizeof(struct bz_bad)) / sizeof(long);
	if (bbl->bz_csn != getsum((long *)bbl->bz_bad, size,
				  (long)(bbl->bz_nelem ^ bbl->bz_nsnf))) {
		printf("Checksum failed!\n");
		return(FAIL);
	}
	return(SUCCESS);
}

/*
 * write_badlist
 *	write bad block list to cylinder 0.
 */
write_badlist()
{
	register int block;		/* Block number in list */
	register int track;		/* Track number */
	register int size;
	register int where;		/* Where to lseek */
	int error;			/* errors while writing bbl */
	caddr_t buf;			/* write buffer */
	static caddr_t ibuf = NULL;	/* read buffer */

	error = 0;
	size = (bbl->bz_nelem * sizeof(struct bz_bad))
		+ sizeof(struct zdbad) - sizeof(struct bz_bad);
	size = HOWMANY(size, DEV_BSIZE);
	printf("Writing bad block list to sector %d.\n", ZDD_NDDSECTORS);
	/*
	 * Allocate read buffer
	 */
	if (ibuf == NULL) {
		callocrnd(DEV_BSIZE);
		ibuf = calloc(DEV_BSIZE);
	}

	/*
	 * Calculate checksum.
	 */
	bbl->bz_csn = getsum((long *)bbl->bz_bad,
		(int)((bbl->bz_nelem * sizeof(struct bz_bad)) / sizeof(long)),
		(long)(bbl->bz_nelem ^ bbl->bz_nsnf));

	/*
	 * Write all the copies
	 */
	for(track = 0; track < MIN(chancfg->zdd_tracks, BZ_NBADCOPY); track++) {
		buf = (char *)bbl;

		/*
		 * try writing 1 block at a time.
		 */
		for (block = 0; block < size; block++) {
			if (track == 0)
				where = block + ZDD_NDDSECTORS;
			else
				where = (track * chancfg->zdd_sectors + block);
			where <<= DEV_BSHIFT;
			lseek(fd, where, L_SET);
			if (write(fd, buf, DEV_BSIZE) != DEV_BSIZE) {
				printf("Cannot write block %d on track %d.\n",
						block, track);
				error++;
			} else {
				lseek(fd, where, L_SET);
				if (read(fd, ibuf, DEV_BSIZE) != DEV_BSIZE) {
					printf("Cannot read block %d on track %d.\n",
						block, track);
					error++;
				} else if (lcmp((long *)buf, (long *)ibuf,
					DEV_BSIZE/sizeof(long)) == FALSE) {
					printf("Verify miscompare block %d on track %d\n",
						block, track);
				}
			}
			buf += DEV_BSIZE;
		}
	}
	/*
	 * Report errors
	 */
	if (error)
		printf("Warning: %d errors occurred.\n", error);
	printf("Bad block list written.\n");
}

/*
 * print_badlist
 *	Print the bad block list in human readable form.
 */
print_badlist()
{
	register int i;
	register struct bz_bad *bzp;
	int autorevect;			/* count # BZ_AUTOREVECT entries */
	int phys;			/* count # BZ_PHYS entries */

	autorevect = 0;
	phys = 0;

	printf("\nBad block list.\n\n");
	printf("Checksum: 0x%x\nNumber of entries: %d\n\n",
		bbl->bz_csn, bbl->bz_nelem);
	if (bbl->bz_nelem == 0)
		return;
	printf("			Bad Block			Replacement Block\n\n");
	printf("Type	Failure		Cyl	Head	Sector		Cyl	Head	Sector\n");

	for (i = 0, bzp = bbl->bz_bad; i < bbl->bz_nelem; i++, bzp++) {
		switch(bzp->bz_rtype) {
		case BZ_PHYS:
			printf("phys");
			phys++;
			break;
		case BZ_AUTOREVECT:
			printf("auto");
			autorevect++;
			break;
		case BZ_SNF:
			printf("snf");
			break;
		}
		printf("	%s", (bzp->bz_ftype == BZ_BADHEAD) ? "header" : "data");
		printf("		%d	%d	%d",
				bzp->bz_cyl, bzp->bz_head, bzp->bz_sect);
		if (bzp->bz_rtype != BZ_PHYS) {
			printf("		%d	%d	%d",
				bzp->bz_rpladdr.da_cyl,
				bzp->bz_rpladdr.da_head,
				bzp->bz_rpladdr.da_sect);
		}
		printf("\n");
	}
	printf("\nSummary: %d Physical, %d Autorevector, %d SNF\n\n",
			phys, autorevect, bbl->bz_nsnf);
}

/*
 * Miscellaneous functions
 */

/*
 * lcmp
 *	Compare array of longs.
 * Return:
 *	TRUE if arrays are equal
 *	FALSE if arrays differ.
 */
static bool_t
lcmp(s1, s2, len)
	register long *s1, *s2;
	register int len;
{

	while (len--)
		if (*s1++ != *s2++)
			return (FALSE);
	return (TRUE);
}

/*
 * Calculate checksum (xor)
 */
long
getsum(lptr, nelem, seed)
	register long *lptr;
	register int nelem;
	long seed;
{
	register long sum;

	sum = seed;
	while (nelem-- > 0) {
		sum ^= *lptr;
		++lptr; 
	}
	return(sum);
}

/*
 * scani - scanf like function
 * returns -1 for null pointer or string
 * else MIN(nargs, WS/DIGITS)
 */
/* VARARGS2 */
scani(s, nargs, argbase)
	register char *s;
	int nargs;
	int argbase;
{
	int *ip = &argbase;
	int count, n, anydigits;

	if (s == NULL || *s == '\0')
		return (-1);
	count = anydigits = n = 0;
	while (count != nargs) {
		switch(*s) {
		case ' ': 	/* white space */
		case '\t': 
		case '\n':
			if (anydigits) {
				anydigits = 0;
				++count;
				*(int *)(*ip++) = n;
				n = 0;
			}
			++s;
			continue;

		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			anydigits = 1;
			n = (n * 10) + *s++ - '0';
			continue;

		case '\0':	/* end of string */
		default:
			if (anydigits) {
				*(int *)(*ip++) = n;
				++count;
			}
			return (count);
		}
	}
	return (count);
}
