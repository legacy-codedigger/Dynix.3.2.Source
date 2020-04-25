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


/*
 * ident	"$Header: zdlists.c 1.15 1991/06/17 01:39:20 $"
 * Miscellaneous routines to manipulate manufacturer's defect list
 * and the bad block list.
 */

/* $Log: zdlists.c,v $
 *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <sys/file.h>
#ifdef BSD
#include <sys/fs.h>
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#include <sys/ioctl.h>
#include <zdc/zdioctl.h>
#else
#include <sys/ufsfilsys.h>
#include <sys/zdc.h>
#include <sys/zdbad.h>
#include <sys/zdioctl.h>
#endif
#include <errno.h>
#include "format.h"
#include "zdformat.h"

#define	MIN(a,b) (((a)<(b))?(a):(b))

extern caddr_t malloc();
extern char openname[];

long getsum();
static bool_t	lcmp();

struct bad_mfg	*mfg = NULL;		/* mfg defect list */

static int	zd_errcode;

/*
 * get_mfglist
 *	get mfglist from specified file 
 *
 * There are four fields that describe a defect. These are the
 * cylinder, head, byte offset from index, and the length of the defect.
 * Each line contains 1 mfg defect list entry.
 *
 * Return:
 *	-1 - Error occurred reading list.
 *	 0 - No problem.
 */
get_mfglist()
{
	FILE *bfp;
	int cyl, head, byte, len;	/* Items to be read */
	register struct bm_mfgbad *bmp;	/* mfg bad list entry */
	struct bm_mfgbad *endmfg;	/* past end of list */
	int	ndefects;		/* number of defects */
	int	ts;			/* no. bytes in track */
	int	status;			/* status from fscanf */
	bool_t	err;			/* Entry bad */
	int	i;
	int	mfgcomp();		/* for qsort() */
	int	line;
	char	buf[80];

	/*
	 * Initialize fields
	 */
	ndefects = 0;
	ts = ZDBYTESPT(chancfg);
	i = ZDMAXBAD(chancfg);
	endmfg = (struct bm_mfgbad *)((uint)mfg + i);
	bmp = mfg->bm_mfgbad;

	if (b_arg == NULL) {
		fprintf(stderr, "No defect list file given ...exiting\n");
		exit(1);;
	}
	if ((bfp = fopen(b_arg, "r")) == NULL) {
		perror("unable to open bad block file");
		fprintf(stderr, "...exiting\n");
		exit(1);	
	}

	/*
	 * Read in mfg defect list.
	 */
	for (line=1;;line++) {
		if (fgets(buf, 80, bfp) == NULL)
			break;
		status = sscanf(buf, "%d%d%d%d", &cyl, &head, &byte, &len); 
		if (status != 4) {
			switch (*buf) {
			case '#':
				if (verbose)
					printf("%s", buf);
				continue; /* don't incr defects and bmp */
			case '\n':	
			case 'C':	/* see printf_mfg() */
			case 'N':
			case 'M':
			case 'R':
			case 'D':
				continue;
			case 'B':
				break;	/* Exit loop and stop reads */
			default:
				fprintf(stderr, 
					"warning: bad syntax in defect ");
				fprintf(stderr, 
					"file:%d - expect 4 numbers - skipping\n",line);
				continue;
			}
			break;
		} else {		/* verify input data */
			err = FALSE;
			if (cyl < 0 || cyl >= chancfg->zdd_cyls) {
				fprintf(stderr, "%s line %d: Bad cylinder number %d.\n", b_arg, line, cyl);
				err = TRUE;
			}
			if (head < 0 || head >= chancfg->zdd_tracks) {
				fprintf(stderr, "%s line %d: Bad head number %d.\n", b_arg, line, head);
				err = TRUE;
			}
			if (byte < 0 || byte >= ts) {
				fprintf(stderr, "%s line %d: Bad byte number %d.\n", b_arg, line, byte);
				err = TRUE;
			}
			if (len <= 0 || byte + HOWMANY(len, NBBY) - 1 >= ts) {
				fprintf(stderr, "%s line %d: Bad length %d.\n", b_arg, line, len);
				err = TRUE;
			}
			if (err == FALSE) {
				ndefects++;
				if (bmp == endmfg) {
					fprintf(stderr, "Ruptured disk! Too many defects - %d.\n",
							ndefects);
					fprintf(stderr, "...exiting\n");
					fclose(bfp);
					exit(1);
				}
				bmp->bm_cyl = cyl;
				bmp->bm_head = head;
				bmp->bm_pos = byte;
				bmp->bm_len = len;
				++bmp;
			}
		}
	} /* end of main loop */

	fclose(bfp);
			
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

	if (debug) printf("%d bad spots entered.\n", ndefects);
	if (verbose)
		print_mfglist(stdout);
	return;
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
	fprintf(stderr,
	"WARNING: Duplicate defect entry  (cyl %d, head %d, pos %d).\n",
		l1->bm_cyl, l1->bm_head, l1->bm_pos);
	return(0);		/* Same Position!?! */
}

/*
 * Read mfg defect list (post-wilson) from cylinder 0 on disk.
 */
read_mfglist()
{
	register int block;		/* Block number in list */
	register int track;		/* Track number */
	register int size;
	int	bbsize;			/* Bad block list size */
	char	*buf;			/* read buffer */
	bool_t	gotit;			/* flag */
	int 	sect;			/* sector on cylinder 0 */
	int	cnt;

	bbsize = (chancfg->zdd_sectors - ZDD_NDDSECTORS) / 2;
	printf("Reading mfg defect list from sector %d.\n",
			ZDD_NDDSECTORS + bbsize);
	buf = (char *)mfg;
	size = bbsize;
	for(block = 0; block < size; block++) {
		track = 0;
		gotit = FALSE;
		sect = (ZDD_NDDSECTORS + bbsize + block);
		lseek(fd, (ZDD_NDDSECTORS + bbsize + block) << DEV_BSHIFT, 0);
		while (track < MIN(chancfg->zdd_tracks, BZ_NBADCOPY)) {
			if ((cnt=read(fd, buf, DEV_BSIZE)) == DEV_BSIZE) {
				gotit = TRUE;
				break;
			}
			/*
			 * Could not read. Try next track.
			 */
			track++;
			lseek(fd, (track * chancfg->zdd_sectors + bbsize + block)
					<< DEV_BSHIFT, 0);
		}
		if (gotit == FALSE) {
			fprintf(stderr,
			"Could not read block %d of mfg defect list.\n",
					block);
			if (!(args & B_OVERWRITE)) {
				fprintf(stderr, "use -o to override\n");
				fprintf(stderr, "...exiting\n");
				exit(1);
			}
			return;
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
		fprintf(stderr, "Checksum for mfg. list bad!\n");
		if (!(args & B_OVERWRITE)) {
			fprintf(stderr, "use -o to override\n");
			fprintf(stderr, "...exiting\n");
			exit(1);
		}
	}
	return;
}


/*
 * Read mfg defect list (pre-wilson) from all tracks on disk.
 */
#define MAXCYL	16384	/* 64*256 */
#define MAXTRACK 255	/* 255 */
#define MAXSECT  255	/* 255 */

read_umfglist()
{
#ifdef READUMFG
	struct	bad_umfg *umfg;	
	FILE 	*bfp;
	char	*buf;		/* read buffer */
	int	cyl;
	int 	sect;		/* sector on cylinder 0 */
	int     track;		/* Track number */
	int	defect;		/* defect number */
	int	b_cyl;		/* bad cylinder */
	int	b_pos;		/* bad bit position */
	int	b_len;		/* bad bit lenght */
	int	def;
	int	byte;
	int	ret;
	int	track_ovflo = 0;
	int	cyl_ovflo = 0;
	int	disk_ovflo = 0;
	int proceed;
	register struct bm_mfgbad *bmp;	/* mfg bad list entry */
	struct bm_mfgbad *endmfg;	/* past end of list */
	int	i;

	printf("Reading mfg defect from unformatted disk.\n");

	/*
	 * Note this may not match relality when we switch to the
	 * real disk type.
	 */
	i = ZDMAXBAD(chancfg);
	endmfg = (struct bm_mfgbad *)((uint)mfg + i);
	bmp = mfg->bm_mfgbad;

	if ((usep->open_flags & O_EXCL) == 0 ) {
		close(fd);
		if ((fd = open(openname, usep->open_flags|O_EXCL)) == -1) {
			if (errno == EBUSY) 
				fprintf(stderr, "Device is busy \n");
			else
				perror("open exclusive failed: ");
		}
	}	

	if (b_arg == NULL) {
		bfp = stdout;
	} else if ((bfp = fopen(b_arg, "w")) == NULL) {
		perror("unable to open bad block file");
		fprintf(stderr, "...exiting\n");
		exit(1);	
	}

	if( (buf = MALLOC_ALIGN(sizeof(struct bad_umfg), DEV_BSIZE)) == NULL)
		fprintf(stderr,"read_umfglist: malloc problem\n");
	umfg = (struct bad_umfg *)buf;

	for(cyl=0; cyl<MAXCYL && !disk_ovflo; cyl++) {
		proceed = 1;
		for(track=0; track<MAXTRACK && (!cyl_ovflo||proceed) ; track++) {
			for (sect=0; sect<MAXSECT; sect++ ) {

				if ((ret = read_mfg(fd, buf, cyl, track, sect)) == 1){
				proceed = 0;
					if (cyl_ovflo ) 
						disk_ovflo = 1;
					else if ( track_ovflo)
						cyl_ovflo = 1;
					else  
				     		track_ovflo = 1;
					break;
				}
				track_ovflo = 0;
				cyl_ovflo = 0;
				disk_ovflo = 0;

				if (ret != 0)
					continue;
				b_cyl = umfg_cyl(umfg);
				for (def=0; def<4; def++) {
					b_pos = umfg_pos(umfg->def[def]);
					b_len = umfg_len(umfg->def[def]);
					/*
					 * terminate on zero defect
					 */
					if ((b_pos | b_len) == 0) 
						break;
					/*
					 * display using fujistu format.
					 */
					if (verbose) {
						fprintf(bfp, "# %s(%d,%d,%d)",
						    (b_cyl&0x8000)? "*" : " ",
						b_cyl&0x7fff, umfg->umfg_track, 
						umfg->umfg_sect);
						fprintf(bfp, "\t %d,%d\n", 
							 b_pos,
							 b_len);

						fflush(bfp);
					}

					if (bmp == endmfg) {
						fprintf(stderr, "Ruptured disk! Too many defects - %d.\n",
								mfg->bm_nelem);
						fprintf(stderr, "...exiting\n");
						printf("exiting\n");
						exit(1);
					}
					/*
					 * add to manufacturers list.
					 */
					bmp->bm_cyl = cyl;
					bmp->bm_head = track;
					bmp->bm_pos = b_pos;
					bmp->bm_len = b_len;
					++bmp;
					mfg->bm_nelem++;
				}
					break;
			}
		}
	}
	/*
	 * Sort list. (should already be)
	 */
	qsort(mfg->bm_mfgbad, mfg->bm_nelem, sizeof(struct bm_mfgbad), mfgcomp);
	/*
	 * calculate checksum
	 */
	i = (mfg->bm_nelem * sizeof(struct bm_mfgbad)) / sizeof(long);
	mfg->bm_csn = getsum((long *)mfg->bm_mfgbad, i, (long)mfg->bm_nelem);
	if (debug) printf("%d bad spots found.\n", mfg->bm_nelem);

	print_mfglist(bfp);
	if (bfp != stdout)
		fclose(bfp);
#else
	fprintf(stderr,"This version of format can not read an unformatted manufactures defect list\n");
#endif
	return;
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
	int count;
	error = 0;
	bbsize = (chancfg->zdd_sectors - ZDD_NDDSECTORS) / 2;
	printf("Writing mfg defect list to sector %d.\n", bbsize + ZDD_NDDSECTORS);
	/*
	 * Allocate read buffer
	 */
	if (ibuf == NULL) {
		if ((ibuf = MALLOC_ALIGN(DEV_BSIZE, DEV_BSIZE)) == NULL) {
			perror("");
			exit(1);
		}
		bzero(ibuf, DEV_BSIZE);
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
			lseek(fd, where, 0);
			if ((count = write(fd, buf, DEV_BSIZE))
			     != DEV_BSIZE) {
				if (count < 0)
					perror("write error");
				fprintf(stderr,
				"Cannot write block %d on track %d.\n",
						block, track);
				error++;
			} else {
				lseek(fd, where, 0);
				if ((count = read(fd, ibuf, DEV_BSIZE)) 
					!= DEV_BSIZE) {
					if (count < 0)
						perror("read error");
					fprintf(stderr,
					"Cannot read block %d on track %d.\n",
						block, track);
					error++;
				} else if (lcmp((long *)buf, (long *)ibuf,
					DEV_BSIZE/sizeof(long)) == FALSE) {
					fprintf(stderr,
					"Verify miscompare block %d on track %d\n",
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
		fprintf(stderr, "Warning: %d errors occurred.\n", error);
	printf("Mfg defect list written.\n");
}

/*
 * print_mfglist
 *	Print the mfg defect list in human readable form.
 */
print_mfglist(fp)
	FILE	*fp;
{
	register int i;
	register struct bm_mfgbad *bmp;

	fprintf(fp, "\nMfg defect list.\n\n");
	fprintf(fp, "Checksum: 0x%x\nNumber of entries: %d\n\n",
		mfg->bm_csn, mfg->bm_nelem);
	if (mfg->bm_nelem == 0)
		return;
	fprintf(fp, "Cylinder	Head	Position	Length\n");
	for (i = 0, bmp = mfg->bm_mfgbad; i < mfg->bm_nelem; i++, bmp++) {
		fprintf(fp, "%d\t\t%d\t%d\t\t%d\n",
			bmp->bm_cyl, bmp->bm_head, bmp->bm_pos, bmp->bm_len);
	}
	fprintf(fp, "\n");
}

/*
 * create_badlist()
 *	Create bad block list from mfg defect list.
 *
 * Note: all entries will have BZ_PHYS rtype.
 */
create_badlist(bbl_p)
	struct zdbad *bbl_p;
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
	bzp = bbl_p->bz_bad;

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
				printf("Ignored defect (%4.4d,%2.2d,) %d at end of track.\n",
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
		 * Note that the byte index is +-1.
		 * and that the len is +-1.
		 * so add 1 to len roundup (NBBY-1) and assume starts
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
				printf("Ignored defect (%4.4d,%2.2d,%2.2d) in Head Scatter.\n",
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
					printf("Ignored defect (%4.4d,%2.2d,%2.2d) in Gap3.\n",
						bmp->bm_cyl, bmp->bm_head,
						sect);
				continue;
			}

			if (debug)
				printf("overflows\n");
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
		if (bbl_p->bz_nelem != 0) {
			/*
			 * First check if already in defect list.
			 * If so, check if it goes into next sector.
			 */
			if (bmp->bm_cyl == bzp->bz_cyl &&
			    bmp->bm_head == bzp->bz_head &&
			    bzp->bz_sect == sect) {
				if (debug)
					printf("repeated defect(%4.4d,%2.2d,%2.2d)\n",
						bmp->bm_cyl,bmp->bm_head,sect);
				if (last >= ss) {
					sect++;
					start -= ss;	/* negative */
					last -= ss;
					goto overflow;  /* check next sector */
				} else 
					continue;  /* with next defect */
			}
			bzp++;
		}
		bbl_p->bz_nelem++;
		if (bbl_p->bz_nelem > maxentries) {
			fprintf(stderr,
			"Ruptured disk - too many bad sectors (%d).\n",
					maxentries);
			fprintf(stderr, "...exiting\n");
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
			printf("Bad sector at  (%4.4d,%2.2d,%2.2d) in %s. (%d-%d)\n",
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
	return;
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
		lseek(fd, (ZDD_NDDSECTORS + block) << DEV_BSHIFT, 0);
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
					<< DEV_BSHIFT, 0);
		}
		if (gotit == FALSE) {
			fprintf(stderr, 
			"Could not read block %d of bad block list.\n",
					block);
			if (!(args & B_OVERWRITE)) {
				fprintf(stderr, "use -o to override\n");
				fprintf(stderr, "...exiting\n");
				exit(1);	
			}
			return;
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
		fprintf(stderr, "Checksum for bad block list bad!\n");
		fprintf(stderr, "...exiting\n");
		exit(1);	
	}
	return;
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
	int count;

	error = 0;
	size = (bbl->bz_nelem * sizeof(struct bz_bad))
		+ sizeof(struct zdbad) - sizeof(struct bz_bad);
	size = HOWMANY(size, DEV_BSIZE);
	printf("Writing bad block list to sector %d.\n", ZDD_NDDSECTORS);
	fflush(stdout);
	/*
	 * Allocate read buffer
	 */
	if (ibuf == NULL) {
		ibuf = MALLOC_ALIGN(DEV_BSIZE, DEV_BSIZE);
		if (ibuf == NULL) {
			perror("");
			exit(1);
		}
		bzero(ibuf, DEV_BSIZE);
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
			lseek(fd, where, 0);
			if ((count = write(fd, buf, DEV_BSIZE))
				!= DEV_BSIZE) {
				if (count < 0)
					perror("Bad block write error");
				fprintf(stderr, 
				"Cannot write block %d on track %d.\n",
						block, track);
				error++;
			} else {
				lseek(fd, where, 0);
				if ((count = read(fd, ibuf, DEV_BSIZE))
					!= DEV_BSIZE) {
					if (count < 0)
						perror("bad block read error");
					fprintf(stderr,
					"Cannot read block %d on track %d.\n",
						block, track);
					error++;
				} else if (lcmp((long *)buf, (long *)ibuf,
					DEV_BSIZE/sizeof(long)) == FALSE) {
					fprintf(stderr,
					"Verify miscompare block %d on track %d\n",
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
		fprintf(stderr, "Warning: %d errors occurred.\n", error);
	printf("Bad block list written.\n");
}

/*
 * print_badlist
 *	Print the bad block list in human readable form.
 */
print_badlist(fp)
	FILE	*fp;
{
	register int i;
	register struct bz_bad *bzp;
	int autorevect;			/* count # BZ_AUTOREVECT entries */
	int phys;			/* count # BZ_PHYS entries */
	struct bz_bad *o_bzp;		/* original bad block list */

	autorevect = 0;
	phys = 0;

	fprintf(fp, "\nBad block list.\n\n");
	fprintf(fp, "Checksum: 0x%x\nNumber of entries: %d\n\n",
		bbl->bz_csn, bbl->bz_nelem);
	if (bbl->bz_nelem == 0)
		return;
	fprintf(fp, "			Bad Block			Replacement Block\n\n");
	fprintf(fp, "Type	Failure		Cyl	Head	Sector		Cyl	Head	Sector\n");

	o_bzp = newbbl->bz_bad;
	for (i = 0, bzp = bbl->bz_bad; i < bbl->bz_nelem; i++, bzp++) {
		/*
		 * If verbose see if its been added to bbl.
		 * newbbl has been set upto contain the bbl as generated from
		 * the mdl. (see zd_display).
		 */
		if (verbose) {
			switch (bblcomp(bzp, o_bzp)) {
			case 0:
				o_bzp++;
				break;
			case -1:
				fprintf(fp, "+");
				break;
			case 1:
				fprintf(fp, "-");
				o_bzp++;
				while(bblcomp(bzp, o_bzp) == 1)
					o_bzp++;
				break;
			}
		}
		switch(bzp->bz_rtype) {
		case BZ_PHYS:
			fprintf(fp, "phys");
			phys++;
			break;
		case BZ_AUTOREVECT:
			fprintf(fp, "auto");
			autorevect++;
			break;
		case BZ_SNF:
			fprintf(fp, "snf ");
			break;
		}
		fprintf(fp, "	%s", (bzp->bz_ftype == BZ_BADHEAD) ? "header" : "data");
		fprintf(fp, "		%d	%d	%d",
				bzp->bz_cyl, bzp->bz_head, bzp->bz_sect);
		if (bzp->bz_rtype != BZ_PHYS) {
			fprintf(fp, "		%d	%d	%d",
				bzp->bz_rpladdr.da_cyl,
				bzp->bz_rpladdr.da_head,
				bzp->bz_rpladdr.da_sect);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "\nSummary: %d Physical, %d Autorevector, %d SNF\n\n",
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
 * read tracks from the pre-wilson disk that is full of defect records.
 */

int
read_mfg(fd, buf, cyl, track, sect)
	int	fd;
	char	*buf;
	int	cyl;
	int	track;
	int	sect;
{
	struct cb	lcb;
	int		ret;

	ret = 0;
	zd_errcode = 0;
	bzero((caddr_t)&lcb, sizeof(struct cb));
	bzero((caddr_t)buf, DEV_BSIZE);
	lcb.cb_cmd = ZDC_REC_DATA;
	lcb.cb_cyl = cyl;
	lcb.cb_head = track;
	lcb.cb_sect = sect;
	lcb.cb_psect = sect;
	lcb.cb_addr = (ulong)buf;
	lcb.cb_count = DEV_BSIZE;
	if (ioctl(fd, ZIOCBCMD, (char *)&lcb) == -1 ) {
		/*
		 * end of track.
		 */
		if (errno == EINVAL) {
			return( 1 );
		}
		perror("ZDC_REC_DATA ioctl error");
		ret = -1;
	}
	if (ioctl(fd, ZIOGERR, &zd_errcode) == -1 ) {
		perror("ioctl ZIOGERR failed");
		ret = -1;
	}
	if (ret != 0)
		fprintf(stderr, "Could not read (%d,%d,%d). completion code 0x%x\n",
						cyl, track, sect, zd_errcode);
	return (ret);
}


/*
 * read and print sector headers.
 */
print_hdr(cyl, track, sectors)
	int	cyl;
	int	track;
	int	sectors;
{
	extern	union	headers *headers;
	int	sect;
	int	i;
	int	type;
	int	flag;
	struct	cb	cb;

	i = ROUNDUP(sectors*sizeof(union headers), CNTMULT);

	bzero((caddr_t)&cb, sizeof(struct cb));
	cb.cb_cyl = cyl;
	cb.cb_head = track;
	cb.cb_sect = 0;
	cb.cb_cmd = ZDC_READ_HDRS;
	cb.cb_addr = (ulong)headers;
	cb.cb_count = i;
	cb.cb_iovec = 0;
	printf ("count=%d addr = 0x%x\n",cb.cb_count,cb.cb_addr);
	if (ioctl(fd, ZIOCBCMD, (char *)&cb) == -1) {
		perror("ZIOCBCMD ZDC_READ_HDRS error");
		if (!(args & B_OVERWRITE)) {
			return(-1);
		}
	}
	if ( sectors ==0 ) {
		sectors = ((union headers *)cb.cb_addr) - headers;
		if ( sectors ==0 ) {
			sectors =  256 - cb.cb_count;
		}
	}
	printf("%d sectors errcode=0x%x\n",sectors,cb.cb_compcode);

	for (sect=0; sect<sectors; sect++) {
		if ((headers[sect].hdr.h_type == 0) &&
		    (headers[sect].hdr.h_flag == 0) &&
		    (headers[sect].hdr.h_cyl == 0) &&
		    (headers[sect].hdr.h_head == 0) &&
		    (headers[sect].hdr.h_sect == 0) )
		    continue;
		printf("%3.3d ",sect);

		switch (type=headers[sect].hdr.h_type ) {
		case ZD_GOODSECT:
			printf("ZD_GOODSECT  "); break;
		case ZD_GOODRPL:
			printf("ZD_GOODRPL   "); break;
		case ZD_GOODSS:
			printf("ZD_GOODSS    "); break;
		case ZD_BADREVECT:
			printf("ZD_BADREVECT "); break;
		case ZD_BADUNUSED:
			printf("ZD_BADUNSED  "); break;
		case ZD_GOODSPARE:
			printf("ZD_GOODSPARE "); break;
		default:
			printf("0x%2.2x         ", type);
			break;
		}
		printf("0x%2.2x (",
			headers[sect].hdr.h_flag);

		if ( headers[sect].hdr.h_cyl == ZD_BUCYL)
			printf("ZD_BUCYL,");
		else if ( headers[sect].hdr.h_cyl == cyl )
			printf("%d,", cyl);
		else {
			printf("%d/0x%x,", headers[sect].hdr.h_cyl & 0xffff,
					   headers[sect].hdr.h_cyl & 0xffff);
		}

		if ( headers[sect].hdr.h_head == ZD_BUHEAD )
			printf("ZD_BUHEAD,");
		else if ( headers[sect].hdr.h_head == track )
			printf("%d,", track);
		else {
			printf("%d/0x%x,", headers[sect].hdr.h_head & 0xff,
					   headers[sect].hdr.h_head & 0xff);
		}

		if ( headers[sect].hdr.h_sect == ZD_INVALSECT )
			printf("ZD_INVALSECT)");
		else if ( headers[sect].hdr.h_sect <= sectors )
			printf("%d)", headers[sect].hdr.h_sect);
		else  {
			printf("%d/0x%x)", headers[sect].hdr.h_sect,
					   headers[sect].hdr.h_sect);
		}
		printf("\n");
	}
}
