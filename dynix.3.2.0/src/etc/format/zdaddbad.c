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
 * ident	"$Header: zdaddbad.c 1.7 1991/06/17 01:38:44 $"
 * addbad routines
 */

/* $Log: zdaddbad.c,v $
 *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
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
#include "format.h"
#include "zdformat.h"

extern caddr_t malloc();
extern long getsum();

struct addlist addlist[MAXADDBAD];	/* Addlist for addbad */
caddr_t	cylbuf;				/* buf to hold a cyl worth of data */
caddr_t	bootbuf;			/* buf to hold a track worth of data */

/*
 * addbad
 *	Add bad blocks to the bad block list and reformat as necessary.
 */
doaddbad(num, flag)
	int num;
	int flag;
{
	register int i;
	register struct	cyl_hdr *chp;
	register struct bz_bad	*bzp;
	register struct bz_bad	*fzp;
	register int j;
	int	nspots;			/* nspots in a given cylinder */
	int	cyl;
	int	nauto;			/* no. of autorevector sectors */
	struct zdbad *tbbl;

	/*
	 * Addbad will be a VERY rare event and normally 1 bad sector
	 * is added at any one time. But since disk addresses are
	 * logical addresses and since replacement is done via slippage,
	 * all sectors marked bad in the same cylinder must be processed
	 * at the same time.
	 */

	if (flag == Z_ADDBAD) {
		/*
		 * If ADDBAD, count number of entries needed.
		 * If VERIFY, let nature run its course.
		 */
		j = 0;
		for (i = 0; i < num; i++) {
			if (addlist[i].al_addr.da_cyl == 1 &&
			    addlist[i].al_addr.da_head == 0)
				++j;	/* need two slots for bootstrap track */
			++j;
		}

		/* Calculate max number of entries */
		i = ZDMAXBAD(chancfg);
		i -= (sizeof(struct zdbad) - sizeof(struct bz_bad)); 
		i /= sizeof(struct bz_bad);

		if (bbl->bz_nelem + j > i) {
			fflush(stdout);
			fprintf(stderr, "ERROR - Bad block list will overflow.\n");
			fprintf(stderr, "Only room for %d more entries.\n",
				i - bbl->bz_nelem + j);
			fprintf(stderr, "No entries added.\n");
			return;
		}
	}

	for (i = 0; i < num; i += nspots) {
		cyl = addlist[i].al_addr.da_cyl;
		if (debug)
			printf("addbad: cylinder %d\n", cyl);
		/*
		 * find no. of spots for same cylinder
		 */
		for (j = i+1; j < num && addlist[j].al_addr.da_cyl == cyl; j++)
			continue;
		nspots = j - i;
		chp = &cyls[cyl & 1];
		bzp = bbl->bz_bad;
		newbbl->bz_nelem = 0;
		newbbl->bz_nsnf = 0;
		tobzp = newbbl->bz_bad;
		if (debug)
			printf("addbad: %d bad spots \n", nspots);
		while (bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl < cyl) {
			if (bzp->bz_rtype == BZ_SNF)
				newbbl->bz_nsnf++;
			newbbl->bz_nelem++;
			*tobzp++ = *bzp++;
		}

		init_cylmap(cyl, chp);
		remark_spots(cyl, chp, bzp, Z_ADDBAD);

		/*
		 * If addbad into Disk Description cylinder or any diagnostic
		 * cylinder, treat specially.
		 */
		if (cyl == ZDD_DDCYL || cyl >= (chancfg->zdd_cyls-ZDD_NDGNCYL)) {
			if (debug)
				printf("cyl is special\n");
			mark_addspots(chp, &addlist[i], nspots);
			setskew(cyl, chp);
			if (cyl == ZDD_DDCYL) {
				map_ddcyl(chp);
				format_dd(chp);
			} else {
				/* DGN CYL */
				map_dgncyl(cyl, chp);
				format_cyl(cyl, chp);
			}
			while (bzp < &bbl->bz_bad[bbl->bz_nelem] &&
							bzp->bz_cyl == cyl)
				++bzp;	/* skip past this cylinder */
			while (bzp < &bbl->bz_bad[bbl->bz_nelem]) {
				if (bzp->bz_rtype == BZ_SNF)
					newbbl->bz_nsnf++;
				newbbl->bz_nelem++;
				*tobzp++ = *bzp++;
			}
			qsort(newbbl->bz_bad, newbbl->bz_nelem,
					sizeof(struct bz_bad), bblcomp);
			tbbl = bbl;
			bbl = newbbl;
			newbbl = tbbl;
			/*
			 * Calculate checksum.
			 */
			bbl->bz_csn = getsum((long *)bbl->bz_bad,
				(int)((bbl->bz_nelem * sizeof(struct bz_bad)) 
				/ sizeof(long)),
				(long)(bbl->bz_nelem ^ bbl->bz_nsnf));

			if (ioctl(fd, ZIOSETBBL, (char *)bbl) < 0) {
				perror("ioctl ZIOSETBBL error");
				return;
			}
			
			if (flag == Z_ADDBAD) {
				write_badlist();
				if (cyl == ZDD_DDCYL) 
					write_mfglist();
#ifdef notyet
				else
					write_dgndata();
#endif /* notyet */
			}
			continue;
		}

		/*
		 * Normal Filesystem Cylinders
		 *
		 * There are enough spares remaining in the cylinder to
		 * accommodate all in bad blocks in the list.
		 * Then reformat the cylinder.
		 * Special case where c_bad == c_free and bad block list
		 * entry is of type BZ_AUTOREVECT.
		 */
		nauto = 0;
		for (j = i; j < (i + nspots); j++) {
			fzp = find_bbl_entry(bbl, cyl,
						addlist[j].al_addr.da_head,
						addlist[j].al_addr.da_sect);
			if (fzp != (struct bz_bad *)NULL &&
			    fzp->bz_rtype == BZ_AUTOREVECT)
				++nauto;
		}
		if (chp->c_bad + nspots - nauto <= chp->c_free) {
			if (debug) 
				printf("attempting to reformat (%d,,)\n",cyl);
			/*
			 * First save all data in the cylinder.
			 */
			recover_cyl(cyl, chp, bbl);
			if (debug) 
				printf("saving data on (%d,,)\n",cyl);
			/*
			 * Add to snf_list any blocks revectored to this
			 * cylinder from the outside. Make sure to read
			 * each block so that data can be restored after
			 * new home is found. First check new list, then
			 * the rest of old "current" list.
			 */
			fzp = newbbl->bz_bad;
			for (; fzp < &newbbl->bz_bad[newbbl->bz_nelem]; ++fzp) {
				if (fzp->bz_rtype != BZ_SNF)
					continue;
				if (fzp->bz_rpladdr.da_cyl == cyl)
					append_snflist(fzp, chp);
			}
			fzp = bzp;
			for (; fzp < &bbl->bz_bad[bbl->bz_nelem]; ++fzp) {
				if (fzp->bz_rtype != BZ_SNF)
					continue;
				if (fzp->bz_cyl == cyl)
					continue;	/* outsiders only */
				if (fzp->bz_rpladdr.da_cyl == cyl)
					append_snflist(fzp, chp);
			}

			/*
			 * Mark new spot and relayout cylinder.
			 */
			mark_addspots(chp, &addlist[i], nspots);
			setskew(cyl, chp);
			map_cyl(cyl, chp);
			/*
			 * Search thru bad block list and remove all entries
			 * for this cylinder. New entries were added by
			 * map_cyl().
			 */
			while ((bzp < &bbl->bz_bad[bbl->bz_nelem]) &&
						bzp->bz_cyl == cyl)
				++bzp;	/* skip past this cylinder */
			while (bzp < &bbl->bz_bad[bbl->bz_nelem]) {
				if (bzp->bz_rtype == BZ_SNF) {
					if (debug) {
						printf("doaddbad: (%d,%d,%d) snf\n",
							
						 bzp->bz_cyl,
						 bzp->bz_head,
						 bzp->bz_sect);
					}
					newbbl->bz_nsnf++;
				}
				newbbl->bz_nelem++;
				*tobzp++ = *bzp++;
			}
			/*
			 * Tell driver to use new bad-list.
			 */
			qsort(newbbl->bz_bad, newbbl->bz_nelem,
					sizeof(struct bz_bad), bblcomp);
			tbbl = bbl;
			bbl = newbbl;
			newbbl = tbbl;
			/*
			 * Calculate checksum.
			 */
			bbl->bz_csn = getsum((long *)bbl->bz_bad,
				(int)((bbl->bz_nelem * sizeof(struct bz_bad)) 
				/ sizeof(long)),
				(long)(bbl->bz_nelem ^ bbl->bz_nsnf));

			if (debug) {
				printf("setting new bbl. chksum=0x%x size=%d\n",
						bbl->bz_csn,
					(bbl->bz_nelem * sizeof(struct bz_bad))
					 + sizeof(struct zdbad) - sizeof(struct bz_bad));
				printf("new bbl chksum=0x%x %d snf entries\n",
					getsum((long *)bbl->bz_bad,
				(int)((bbl->bz_nelem * sizeof(struct bz_bad)
				/ sizeof(long)),
				(long)(bbl->bz_nelem ^ bbl->bz_nsnf))),
					bbl->bz_nsnf);
			}

			/*
			 * Set new bbl in core for the following format.
			 */
			if (ioctl(fd, ZIOSETBBL, (char *)bbl) < 0) {
				perror("ioctl ZIOSETBBL error");
				return;
			}

			format_cyl(cyl, chp);
			check_hdrs(cyl, chp, TRUE);

			/*
			 * rewrite cylinder data
			 */
			restore_cyl(cyl);
			if (debug) 
				printf("data restored on (%d,,)\n",cyl);
			/*
			 * If snftogo, then do_snf_pass to resolve
			 * remaining blocks.
			 */
			if (snftogo > 0)
				do_snf_pass(0);
		} else {
			if (debug) {
				printf("need to SNF bad a bad spot on (%d,,)\n",
						cyl);
			}
			/*
			 * Not enough spares in current cylinder to
			 * accommodate all new bad sectors, need to find
			 * SNF type replacement elsewhere.
			 * Add bad block to snf_list, mark header(s),
			 * rewrite_headers and do_snf_pass.
			 */
			remap_cyl(cyl, chp);	/* clone current cyl format */

			/*
			 * Fast copy of bad block list to new bad block list.
			 */
			tbbl = newbbl;
			newbbl = bbl;
			bbl = tbbl;
			tobzp = &newbbl->bz_bad[newbbl->bz_nelem];
			j = i;
			while (j < (i + nspots)) {
				bzp = find_bbl_entry(newbbl, cyl,
					addlist[j].al_addr.da_head,
					addlist[j].al_addr.da_sect & ~ZD_AUTOBIT);
				if (bzp == (struct bz_bad *)NULL) {
					if (addlist[j].al_addr.da_sect >= chancfg->zdd_sectors) {
						addbad_snfrpl(chp, &addlist[j]);
						++j;
					} else
						j += addbad_sect(chp,
								&addlist[j],
								nspots);
				} else {
					/*
					 * Must be AUTOREVECT sector
					 * or AUTOREVECT replacement sector.
					 */
					addbad_auto(chp, &addlist[j], bzp);
					++j;
				}
			}
			/*
			 * Tell driver to use new bad-list.
			 */
			qsort(newbbl->bz_bad, newbbl->bz_nelem,
				sizeof(struct bz_bad), bblcomp);
			tbbl = bbl;
			bbl = newbbl;
			newbbl = tbbl;
			/*
			 * Calculate checksum.
			 */
			bbl->bz_csn = getsum((long *)bbl->bz_bad,
				(int)((bbl->bz_nelem * sizeof(struct bz_bad)) 
				/ sizeof(long)),
				(long)(bbl->bz_nelem ^ bbl->bz_nsnf));

			if (debug) 
				printf("setting new bbl\n");
			if (ioctl(fd, ZIOSETBBL, (char *)bbl) < 0) {
				perror("ioctl ZIOSETBBL error");
				return;
			}
			do_snf_pass(0);
		}
		if (flag == Z_ADDBAD)
			write_badlist();
	} /* for loop */
}

/*
 * addbad_snfrpl
 *	replacement sector for an BZ_SNF bad block entry has gone bad
 *
 * find snf entry, recover sector data, make snf_list entry, mark header
 * of bad replacement, write headers for the track and return. The upper-level
 * code will invoke do_snf_pass and update bad block list.
 *
 * Note: This routine is invoked only if the number of bad blocks in a
 * cylinder plus new bad sectors is greater than the number of spares (RARE).
 */
addbad_snfrpl(chp, alp)
	struct	cyl_hdr *chp;
	register struct addlist *alp;
{
	register struct bz_bad	*bzp;
	register struct hdr *hp;
	register struct track_hdr *tp;
	unchar	sect;

	bzp = newbbl->bz_bad;
	for (; bzp < &newbbl->bz_bad[newbbl->bz_nelem]; ++bzp) {
		if (bzp->bz_rtype != BZ_SNF)
			continue;
		if (bzp->bz_rpladdr.da_cyl != alp->al_addr.da_cyl)
			continue;
		if (bzp->bz_rpladdr.da_head != alp->al_addr.da_head)
			continue;
		if (bzp->bz_rpladdr.da_sect != alp->al_addr.da_sect)
			continue;
		/*
		 * Found it
		 */
		append_snflist(bzp, chp);
		break;
	}

	/*
	 * Bad block given is logical address. Find physical sector
	 * and mark as bad header.
	 */
	tp = &chp->c_trk[alp->al_addr.da_head];
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
		if (hp->h_sect == alp->al_addr.da_sect) {
			if (alp->al_type & BZ_BADPHYS) {
				if (debug) {
					printf("addbad_snfrpl: phys(%d,%d,%d)\n",
						hp->h_cyl,hp->h_head,hp->h_sect);
				}
			} else {
				hp->h_sect = hp - tp->t_hdr;
				if (debug) {
					printf("addbad_snfrpl: phys(%d,%d,%d) --> log(%d,%d,%d)\n",
						hp->h_cyl,hp->h_head,alp->al_addr.da_sect,
						hp->h_cyl,hp->h_head,hp->h_sect);
				}
			}
			hp->h_flag = alp->al_type;
			sect = hp->h_sect;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			rewrite_hdr(alp->al_addr.da_cyl, alp->al_addr.da_head,
					sect, hp);
			break;
		}
	}
}

/*
 * addbad_sect
 *	Add a previously good sector to the bad block list.
 *
 * recover sector data, newbad spot, make snf_list entry, mark header of
 * bad sector, rewrite headers and return. The upper-level code will
 * invoke do_snf_pass and update bad-block list.
 *
 * Note: This routine is invoked only if the number of bad blocks in a
 * cylinder plus new bad sectors is greater than the number of spares (RARE).
 */
int
addbad_sect(chp, alp, nspots)
	struct	cyl_hdr *chp;
	register struct addlist *alp;
	int 	nspots;
{
	register struct hdr *hp;
	register struct bz_bad	*bzp;
	register int	i;
	register struct track_hdr *tp;
	struct	addlist	*talp;
	unchar	sect;
	int	where;
	int	ncontig;		/* no of logically contig sectors */
	caddr_t	cbp;

	if (alp->al_addr.da_cyl == 1 && alp->al_addr.da_head == 0) {
		tp = chp->c_trk;
		if (tp->t_bad < chancfg->zdd_spare) {
			/*
			 * The replacements on this track are used
			 * as replacements for a block off-track.
			 * This track must get its own spares first.
			 * Therefore, find a new home for others.
			 */
			bzp = newbbl->bz_bad;
			for (; bzp < &newbbl->bz_bad[newbbl->bz_nelem]; ++bzp) {
				if (bzp->bz_rtype != BZ_SNF)
					continue;
				if (bzp->bz_rpladdr.da_cyl != 1)
					continue;
				if (bzp->bz_rpladdr.da_head != 0)
					continue;
				append_snflist(bzp, chp);
			}
		}
		/*
		 * determine the number of new spots on this track.
		 */
		for (i=0, talp=alp; i < nspots; i++, talp++)
			if (talp->al_addr.da_head != 0)
				break;
		nspots = i;
		tp->t_bad += nspots;
		ncontig = chancfg->zdd_sectors;
		if (tp->t_bad > chancfg->zdd_spare)
			ncontig -= (tp->t_bad - chancfg->zdd_spare);
		/*
		 * Check if enuff sectors are remaining in the track to
		 * handle bootstrap program.
		 */
		if (ncontig < (BBSIZE >> DEV_BSHIFT)) {
			fflush(stdout);
			fprintf(stderr, "Ruptured disk - too many errors (%d) in bootstrap track.\n",
					tp->t_bad);
			fprintf(stderr, "...exiting\n");
			exit(11);
		}
		/*
		 * recover cylinder 1, track 0.
		 */
		if (bootbuf == (caddr_t)NULL) {
			bootbuf = MALLOC_ALIGN((chancfg->zdd_sectors 
					       << DEV_BSHIFT),
					       DEV_BSIZE);
			if (bootbuf == NULL) {
				fflush(stdout);
				fprintf(stderr, "addbad_sect: malloc err\n");
				fprintf(stderr, "...exiting\n");
				exit(1);	
			}
			bzero(bootbuf, chancfg->zdd_sectors<<DEV_BSHIFT);
		} else
			bzero(bootbuf, (uint)(chancfg->zdd_sectors << DEV_BSHIFT));
		/*
		 * save bootstrap sectors.
		 */
		where = (chancfg->zdd_sectors * chancfg->zdd_tracks) << DEV_BSHIFT;
		lseek(fd, where, 0);
		if (read(fd, bootbuf, ncontig << DEV_BSHIFT)
					!= ncontig << DEV_BSHIFT) {
			/*
			 * If problem try a sector at a time.
			 */
			cbp = bootbuf;
			for (i = 0; i < ncontig; i++) {
				lseek(fd, where, 0);
				if (read(fd, cbp, DEV_BSIZE) != DEV_BSIZE) {
					/*
					 * Recover data
					 */
					recover_sect(chp, 1, 0, (unchar)i, cbp);
				}
				where += DEV_BSIZE;
				cbp += DEV_BSIZE;
			}
		}
		/*
		 * If need to find replacements elsewhere, create SNF
		 * entries.
		 */
		for(i=0; i < nspots && (ncontig+i) < chancfg->zdd_sectors; i++){
			for(hp=tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
				if (hp->h_sect == ncontig + i) {
					bzp = newbad(hp, BZ_SNF, hp);
					append_snflist(bzp, chp);
				}
			}
		}

		/*
		 * Mark the actual physical bad spots.
		 */
		for (i = 0; i < nspots; i++, alp++) {
			for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)  {
				if (hp->h_sect == alp->al_addr.da_sect) {
					if (alp->al_type & BZ_BADPHYS) {
						if (debug) {
							printf("addbad_sect: phys(%d,%d,%d)\n",
								hp->h_cyl,hp->h_head,hp->h_sect);
						}
					} else {
						hp->h_sect = hp - tp->t_hdr;
						if (debug) {
							printf("addbad_sect: phys(%d,%d,%d) --> log(%d,%d,%d)\n",
								hp->h_cyl,hp->h_head,alp->al_addr.da_sect,
								hp->h_cyl,hp->h_head,hp->h_sect);
						}
					}
					hp->h_flag = alp->al_type;
					(void)newbad(hp, BZ_PHYS,
							(struct hdr *)NULL);
					hp->h_type = ZD_BADUNUSED;
					hp->h_cyl = ZD_BUCYL;
					hp->h_head = ZD_BUHEAD;
					hp->h_sect = ZD_INVALSECT;
					break;
				}
			}
		}
		slip_track(1, 0, chp);
		reformat_trk(1, 0, tp);

		/*
		 * restore saved sectors.
		 */
		where = (chancfg->zdd_sectors * chancfg->zdd_tracks) << DEV_BSHIFT;
		lseek(fd, where, 0);
		if (write(fd, bootbuf, ncontig << DEV_BSHIFT)
					!= ncontig << DEV_BSHIFT) {
			/*
			 * If problem try a sector at a time.
			 */
			cbp = bootbuf;
			for (i = 0; i < ncontig; i++) {
				int icount;

				lseek(fd, where, 0);
				if ((icount = write(fd, cbp, DEV_BSIZE))
				     != DEV_BSIZE) {
					fflush(stdout);
					if (icount < 0)
						perror("write error");
					fprintf(stderr,
					"Warning: Could not restore data in cyl 1, bn %d).\n", i);
				}
				where += DEV_BSIZE;
				cbp += DEV_BSIZE;
			}
		}
		return (nspots);
	}

	/*
	 * Rest of file system cylinders
	 * Bad block given is logical address. Find physical sector
	 * and mark as bad header.
	 */
	tp = &chp->c_trk[alp->al_addr.da_head];
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)
		if (hp->h_sect == alp->al_addr.da_sect) {
			if (alp->al_type & BZ_BADPHYS) {
				if (debug) {
					printf("addbad_sect: phys(%d,%d,%d)\n",
						hp->h_cyl,hp->h_head,hp->h_sect);
				}
				sect = hp->h_sect;
			} else {
				sect = (unchar)(hp - tp->t_hdr);
			}
			hp->h_flag = alp->al_type;
			bzp = newbad(hp, BZ_SNF, hp);
			append_snflist(bzp, chp);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (alp->al_type & BZ_BADPHYS) {
				rewrite_hdr(alp->al_addr.da_cyl,
						alp->al_addr.da_head,
						alp->al_addr.da_sect, hp);
			} else {
				rewrite_hdr(alp->al_addr.da_cyl,
						alp->al_addr.da_head,
						sect, hp);
			}
			break;
		}
	return (1);
}

/*
 * addbad_auto
 *	Add a previously auto-revector sector or auto-revector replacement
 *	sector to the bad block list.
 *
 * recover sector data, newbad replacement sector (BZ_PHYS), change entry to
 * BZ_SNF, make snf_list entry, mark header of bad sector,
 * mark header of replacement sector, rewrite headers and return.
 * The upper-level code will invoke do_snf_pass and update bad-block list.
 *
 * Note: This routine is invoked only if the number of bad blocks in a
 * cylinder plus new bad sectors is greater than the number of spares (RARE).
 */
addbad_auto(chp, alp, bzp)
	struct	cyl_hdr *chp;
	struct addlist *alp;
	register struct bz_bad	*bzp;
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	ushort	rplcyl;
	unchar	rplhead;
	unchar	rplsect;

	/*
	 * recover data and store in snf_list
	 */
	rplcyl  = bzp->bz_rpladdr.da_cyl;
	rplhead = bzp->bz_rpladdr.da_head;
	rplsect = bzp->bz_rpladdr.da_sect;
	append_snflist(bzp, chp);

	/*
	 * mark header of replacement sector, add to bad block list
	 * as BZ_PHYS entry, and rewrite headers of track.
	 */
	tp = &chp->c_trk[rplhead];
	hp = &tp->t_hdr[rplsect];
	hp->h_flag = alp->al_type;
	hp->h_cyl  = rplcyl;
	hp->h_head = rplhead;
	hp->h_sect = rplsect;
	(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
	hp->h_type = ZD_BADUNUSED;
	hp->h_cyl = ZD_BUCYL;
	hp->h_head = ZD_BUHEAD;
	hp->h_sect = ZD_INVALSECT;
	rewrite_hdr(rplcyl, rplhead, rplsect, hp);

	/*
	 * Now change BZ_AUTOREVECT entry to BZ_SNF in bad block list,
	 * mark header and rewrite headers of the track.
	 */
	bzp->bz_rtype = BZ_SNF;
	bzp->bz_ftype = alp->al_type;
	tp = &chp->c_trk[bzp->bz_head];
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)
		if (hp->h_sect == bzp->bz_sect) {
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			if (alp->al_type & BZ_BADPHYS) {
				rewrite_hdr(bzp->bz_cyl, bzp->bz_head,
						bzp->bz_sect, hp);
			} else {
				rewrite_hdr(bzp->bz_cyl, bzp->bz_head,
						(unchar)(hp - tp->t_hdr), hp);
			}
			/* !!! */
			break;
		}
}

/*
 * Find_bbl_entry
 *	find bad block list entry for a given LOGICAL disk address.
 */
struct bz_bad *
find_bbl_entry(bad, cyl, trk, sect)
	struct	zdbad *bad;		/* bad list */
	register int	cyl;
	unchar	trk;
	unchar	sect;
{
	register struct bz_bad *bzp;

	for (bzp = bad->bz_bad; bzp < &bad->bz_bad[bad->bz_nelem]; bzp++) {
		if (bzp->bz_cyl < cyl)
			continue;
		if (bzp->bz_cyl > cyl)
			break;
		if (bzp->bz_head < trk)
			continue;
		if (bzp->bz_head > trk)
			break;
		if (bzp->bz_rtype == BZ_PHYS)	/* look at only logical addr */
			continue;
		if (bzp->bz_sect == sect)
			return (bzp);
	}
	return ((struct bz_bad *)NULL);
}

/*
 * mark_addspots
 * 	mark the sectors with the addbadlist defects.
 */
mark_addspots(chp, alp, nspots)
	register struct	cyl_hdr *chp;
	register struct addlist *alp;
	int	nspots;
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	int	psect;

	if (debug)
		printf("marking %d badspots\n", nspots);

	while (nspots) {
		if (alp->al_addr.da_sect & ZD_AUTOBIT)
			tp = &chp->c_trk[(alp->al_addr.da_head + 1) % chancfg->zdd_tracks];
		else
			tp = &chp->c_trk[alp->al_addr.da_head];

		/*
		 * If Bad block given is logical address. Find physical sector
		 * and mark as bad to-be-resolved.
		 */
		if (alp->al_type & BZ_BADPHYS) {
			psect = alp->al_addr.da_sect;
			hp = &tp->t_hdr[psect];
			hp->h_flag = alp->al_type | ZD_TORESOLVE;
		} else for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++) {
			if (hp->h_sect == alp->al_addr.da_sect)  {
				hp->h_flag = alp->al_type | ZD_TORESOLVE;
				psect = hp - tp->t_hdr;
				break;
			}
		}
		if (debug) {
			printf("mark_addspots:#%d log(%d,%d,%d)phys(,,%d)\n",
				chp->c_bad,
				hp->h_cyl, hp->h_head, hp->h_sect, psect);
		}
		chp->c_bad++;
		tp->t_bad++;
		--nspots;
		++alp;
	}
}

/*
 * append_snflist
 *	append bad block entry to the snf_list to get reassigned
 *
 * Called by addbad so that replacements from surrounding cylinders may be
 * remapped. "chp" points to the map of the cylinder where the replacements
 * currently reside.
 */
static
append_snflist(bzp, chp)
	register struct bz_bad *bzp;
	struct	cyl_hdr *chp;
{
	int	where;

	if (debug) {
		printf("appending (%d,%d,%d) to SNF bad block list\n",
					bzp->bz_cyl, bzp->bz_head, bzp->bz_sect);
	}

	snf_list[snftogo].snf_addr.da_cyl = bzp->bz_cyl;
	snf_list[snftogo].snf_addr.da_head = bzp->bz_head;
	snf_list[snftogo].snf_addr.da_sect = bzp->bz_sect;
	snf_list[snftogo].snf_data = MALLOC_ALIGN(DEV_BSIZE, DEV_BSIZE);
	if (snf_list[snftogo].snf_data == NULL) {
		fflush(stdout);
		fprintf(stderr, "append_snflist: malloc error\n");
		fprintf(stderr, "...exiting\n");
		exit(1);	
	}
	bzero((caddr_t)(snf_list[snftogo].snf_data), DEV_BSIZE);

	/*
	 * attempt to read sector.
	 */
	where = (bzp->bz_cyl * chancfg->zdd_sectors * chancfg->zdd_tracks)
		+ (bzp->bz_head * chancfg->zdd_sectors) + bzp->bz_sect;
	lseek(fd, where << DEV_BSHIFT, 0);
	if (read(fd, snf_list[snftogo].snf_data, DEV_BSIZE) != DEV_BSIZE) {
		/*
		 * cannot normally read - try ZDC_REC_DATA.
		 */
		if (debug)
			printf("could not read sector! trying REC_DATA\n");
		if (bzp->bz_rtype == BZ_AUTOREVECT)
			recover_sect(chp, bzp->bz_cyl, bzp->bz_head,
					(bzp->bz_sect | ZD_AUTOBIT),
					snf_list[snftogo].snf_data);
		else
			recover_sect(chp, bzp->bz_rpladdr.da_cyl,
					bzp->bz_rpladdr.da_head,
					bzp->bz_rpladdr.da_sect,
					snf_list[snftogo].snf_data);
	}
	snftogo++;
	bzp->bz_rpladdr.da_cyl  = 0;
	bzp->bz_rpladdr.da_head = 0;
	bzp->bz_rpladdr.da_sect = 0;
}

/*
 * recover_cyl
 *	recover (read) data from cylinder.
 *
 * ASSUMES that no off-cylinder revectoring will occur.
 */
recover_cyl(cyl, chp, blp)
	int	cyl;	/* cylinder number */
	struct	cyl_hdr *chp;
	struct 	zdbad	*blp;
{
	register int	track;
	register int	sect;
	register int	tracksize;
	register struct bz_bad *bzp;
	caddr_t cbp;
	long	where;

	if (debug)
		printf("recover_cyl(%d,,)\n",cyl);

	tracksize = chancfg->zdd_sectors << DEV_BSHIFT;
	/*
	 * allocate memory for cylinder
	 */
	if (cylbuf == (caddr_t)NULL) {
		cylbuf = MALLOC_ALIGN((tracksize*chancfg->zdd_tracks),
				      DEV_BSIZE);
		if (cylbuf == NULL) {
			fflush(stdout);
			fprintf(stderr, "recover_cyl: malloc failure\n");
			fprintf(stderr, "...exiting\n");
			exit(1);	
		}
		bzero((caddr_t)cylbuf, tracksize*chancfg->zdd_tracks);
	} else
		bzero(cylbuf, tracksize * chancfg->zdd_tracks);

	/*
	 * read the cylinder a track at at time.
	 */
	where = cyl * chancfg->zdd_tracks * tracksize;
	cbp = cylbuf;
	for (track = 0; track < chancfg->zdd_tracks; track++) {
	    lseek(fd, where, 0);
	    if (read(fd, cbp, tracksize) != tracksize) {
		/*
		 * If problem try a sector at a time.
		 */
		for (sect = 0; sect < chancfg->zdd_sectors; sect++) {
		    lseek(fd, where, 0);
		    if (read(fd, cbp, DEV_BSIZE) != DEV_BSIZE) {
			bzp = find_bbl_entry(blp, cyl, (unchar)track,
							(unchar)sect);
			if (bzp == NULL)
				recover_sect(chp, (ushort)cyl, (unchar)track,
							(unchar)sect, cbp);
			else	if (bzp->bz_rtype == BZ_AUTOREVECT)
					recover_sect(chp, (ushort)cyl,
						(unchar)track,
						(unchar)(sect | ZD_AUTOBIT), cbp);
			else	if (bzp->bz_rtype == BZ_SNF && bzp->bz_cyl == cyl)
					/*
					 * BZ_SNF revector on cylinder
					 */
					recover_sect(chp,
						bzp->bz_rpladdr.da_cyl,
						bzp->bz_rpladdr.da_head,
						bzp->bz_rpladdr.da_sect, cbp);
			else {
				fflush(stdout);
				fprintf(stderr,
				"ILLEGAL format! Unexpected revector off-cylinder\n");
				fprintf(stderr,
				"Revector (%d, %d, %d) to (%d, %d, %d).\n",
					bzp->bz_cyl, bzp->bz_head, bzp->bz_sect,
					bzp->bz_rpladdr.da_cyl,
					bzp->bz_rpladdr.da_head,
					bzp->bz_rpladdr.da_sect);
				fprintf(stderr, "Could not recover data from (%d, %d, %d).\n",
					bzp->bz_cyl, bzp->bz_head, bzp->bz_sect);
			}
		    }
		    where += DEV_BSIZE;
		    cbp += DEV_BSIZE;
		}
		continue;
	    }
	    where += tracksize;
	    cbp += tracksize;
	}
}

/*
 * recover_sect
 *	recover (read) data from sector. Use ZDC_REC_DATA command.
 *
 *	"cyl, head, sector" are what we match in the cylinder map. Then
 *	use index to determine actual physical offset from index pulse.
 */
recover_sect(chp, cyl, track, sector, bp)
	struct	cyl_hdr *chp;			/* setup of cylinder */
	ushort	cyl;
	unchar	track;
	unchar	sector;
	caddr_t	bp;				/* Where to put data */
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	int physect;
	unchar	trk;			/* adjusted in autorevect case */
	struct cb lcb;			/* cb for ioctl */

	trk = track;
	if (sector & ZD_AUTOBIT)
		trk = (track + 1) % chancfg->zdd_tracks;
	tp = &chp->c_trk[trk];

	/*
	 * Find physical sector number - offset from index mark.
	 */
	physect = -1;
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)  {
		if (hp->h_sect == sector && hp->h_head == track)  {
			physect = hp - tp->t_hdr;
			break;
		}
	}
	if (physect < 0) {
		fflush(stdout);
		fprintf(stderr, "Could not find address (%d, %d, %d) in cylinder map.\n",
			cyl, track, sector);
		fprintf(stderr, "Could not recover data from (%d, %d, %d).\n",
			cyl, track, sector);
		return;
	}
	/*
	 * Found desired sector offset. Now do ZDC_REC_DATA.
	 */
	if (verbose)
		printf("Using ZDC_REC_DATA from physical (%d, %d, %d).\n",
			cyl, trk, physect);
	bzero((caddr_t)&lcb, sizeof(struct cb));
	lcb.cb_cmd = ZDC_REC_DATA;
	lcb.cb_cyl = cyl;
	lcb.cb_head = trk;
	lcb.cb_sect = physect;
	lcb.cb_psect = physect;
	lcb.cb_addr = (ulong)bp;
	lcb.cb_count = DEV_BSIZE;
	if (ioctl(fd, ZIOCBCMD, (char *)&lcb) <  0) {
		perror("ZDC_REC_DATA ioctl error");
		return;
	}
	if (verbose)
		printf("Recovered data with ZDC_REC_DATA.\n");
}

/*
 * restore_cyl
 *	restore (write) data back onto cylinder.
 */
restore_cyl(cyl)
	int	cyl;	/* cylinder number */
{
	register int	track;
	register int	sect;
	register int	tracksize;
	register caddr_t cbp;
	register long	where;

	tracksize = chancfg->zdd_sectors << DEV_BSHIFT;
	/*
	 * write out cylinder a track at at time.
	 */
	where = cyl * chancfg->zdd_tracks * tracksize;
	cbp = cylbuf;
	for (track = 0; track < chancfg->zdd_tracks; track++) {
		lseek(fd, where, 0);
		if (write(fd, cbp, tracksize) != tracksize) {
			/*
			 * If problem try a sector at a time.
			 */
			for (sect = 0; sect < chancfg->zdd_sectors; sect++) {
				int icount;

				lseek(fd, where, 0);
				if ((icount = write(fd, cbp, DEV_BSIZE))
				     != DEV_BSIZE) {
					fflush(stdout);
					if (icount < 0)
						perror("write error");
					fprintf(stderr,
					"Warning: Could not restore data to (%d, %d, %d).\n",
						cyl, track, sect);
				}
				where += DEV_BSIZE;
				cbp += DEV_BSIZE;
			}
			continue;
		}
		where += tracksize;
		cbp += tracksize;
	}
}

/*
 * rewrite_hdr
 *	rewrite the header for a given sector (effectively reformats sector).
 */
rewrite_hdr(cyl, head, sect, hp)
	ushort	cyl;
	unchar	head;
	unchar	sect;
	struct	hdr	*hp;
{
	struct	cb	lcb;		/* cb for ZDC_WHDR_WDATA command */
	static	struct hdr *ahp;	/* aligned header for ZDC_WHDR_WDATA */

	if (ahp == (struct hdr *)NULL) {
		ahp = (struct hdr *)MALLOC_ALIGN(CNTMULT+DEV_BSIZE,
						 types[disk].align);
		if (ahp == NULL) {
			fflush(stdout);
			fprintf(stderr, "rewrite_hdr: malloc failure\n");
			fprintf(stderr, "...exiting\n");
			exit(1);	
		}
		bzero((caddr_t)ahp, CNTMULT+DEV_BSIZE);
	}
	*ahp = *hp;
	lcb.cb_cyl = cyl;
	lcb.cb_head = head;
	lcb.cb_sect = sect;
	lcb.cb_addr = (ulong)ahp;
	lcb.cb_count = CNTMULT + DEV_BSIZE;
	lcb.cb_iovec = 0;
	lcb.cb_cmd = ZDC_WHDR_WDATA;
	if (ioctl(fd, ZIOCBCMD, (char *)&lcb) < 0) {
		fflush(stdout);
		perror("ZDC_WHDR_WDATA ioctl error");
		fprintf(stderr, "...exiting\n");
		exit(8);
	}
}

/*
 * reformat_trk
 *	reformat a track.
 * called from addbad_sect when adding bad blocks into the "bootstrap"
 * track.
 */
reformat_trk(cyl, head, tp)
	ushort	cyl;
	unchar	head;
	struct track_hdr *tp;
{
	register struct	hdr *hp;
	register int size;
	struct cb lcb;

	size = totspt;
	if (chancfg->zdd_runt)
		size++;
	size = ROUNDUP(sizeof(struct hdr) * size, CNTMULT);

	lcb.cb_cyl = cyl;
	lcb.cb_head = head;
	lcb.cb_sect = 0;
	lcb.cb_addr = (ulong)tp->t_hdr;
	lcb.cb_count = size;
	lcb.cb_iovec = 0;
	lcb.cb_cmd = ZDC_FMTTRK;
	/*
	 * Remark spares as ZD_GOODSECT so that they can be verified.
	 */
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)
		if (hp->h_type == ZD_GOODSPARE)
			hp->h_type = ZD_GOODSECT;
	if (ioctl(fd, ZIOCBCMD, (char *)&lcb) < 0) {
		fflush(stdout);
		perror("ZDC_FMTTRK ioctl error");
		fprintf(stderr, "...exiting\n");
		exit(8);
	}
}

/*
 * addcomp
 *	Sort routine to sort defect list.
 *	Called via qsort.
 */
int
addcomp(l1, l2)
	register struct addlist *l1, *l2;
{
	if (l1->al_addr.da_cyl < l2->al_addr.da_cyl)
		return (-1);
	if (l1->al_addr.da_cyl > l2->al_addr.da_cyl)
		return (1);

	/* Same cylinder */
	if (l1->al_addr.da_head < l2->al_addr.da_head)
		return (-1);
	if (l1->al_addr.da_head > l2->al_addr.da_head)
		return (1);

	/* Same Head */
	if (l1->al_addr.da_sect < l2->al_addr.da_sect)
		return (-1);
	if (l1->al_addr.da_sect > l2->al_addr.da_sect)
		return (1);
	fflush(stdout);
	fprintf(stderr, "WARNING: More than one addbad at same sector.\n");
	return (0);		/* Same sector!?! */
}

/*
 * bad_addentry
 *	Validate addbad bad block entry
 * If error, return TRUE, else FALSE.
 */
bool_t
bad_addentry(cyl, head, sect, type)
	int cyl;
	int head;
	int sect;
	int type;
{
	register struct bz_bad *bzp;
	bool_t err;

	err = FALSE;
	if (cyl < 0 || cyl >= chancfg->zdd_cyls) {
		fflush(stdout);
		fprintf(stderr, "Bad cylinder number %d.\n", cyl);
		err = TRUE;
	}
	if (head < 0 || head >= chancfg->zdd_tracks) {
		fflush(stdout);
		fprintf(stderr, "Bad head number %d.\n", head);
		err = TRUE;
	}
	if (sect < 0 || sect > ZD_MAXSECT
	||  ((sect >= totspt) && ((sect & ZD_AUTOBIT) != ZD_AUTOBIT))) {
		fflush(stdout);
		fprintf(stderr, "Bad sector number %d.\n", sect);
		err = TRUE;
	}
	if (type != BZ_BADHEAD && type != BZ_BADDATA) {
		fflush(stdout);
		fprintf(stderr, 
		"Bad type %d. Enter Bad Header (%d) or Bad Data (%d).\n",
				type, BZ_BADHEAD, BZ_BADDATA);
		err = TRUE;
	}

	if (err == TRUE)
		return (TRUE);

	for(bzp = bbl->bz_bad; bzp < &bbl->bz_bad[bbl->bz_nelem]; bzp++) {
		if (bzp->bz_rtype == BZ_PHYS)
			continue;
		if (bzp->bz_rtype == BZ_AUTOREVECT) {
			if (bzp->bz_cyl != cyl || bzp->bz_head != head)
				continue;
			if (sect & ZD_AUTOBIT) {
				if (sect != (bzp->bz_sect | ZD_AUTOBIT))
					continue;
				/*
				 * Found entry. This means that replacement
				 * sector is now bad. Thus valid addbad
				 * request.
				 */
				return (FALSE);
			}
			if (sect != bzp->bz_sect)
				continue;
			/*
			 * Found entry in bad block list. If new error is
			 * Not a header error. Then entry is duplicate - error.
			 */
			if (type != BZ_BADHEAD) {
				fflush(stdout);
				fprintf(stderr,
				"(%d, %d, %d) already in bad block list.\n",
					cyl, head, sect);
				return (TRUE);
			}
			return (FALSE);		/* Now header is bad! */
		}

		/*
		 * BZ_SNF entry.
		 */
		if (sect >= chancfg->zdd_sectors && sect < totspt) {
			/*
			 * Must be BZ_SNF replacement.
			 */
			if (cyl != bzp->bz_rpladdr.da_cyl ||
			    head != bzp->bz_rpladdr.da_head ||
			    sect != bzp->bz_rpladdr.da_sect)
				continue;
			return (FALSE);		/* Replacement is bad */
		}

		/*
		 * Look for normal sector. Error - if already found in
		 * bad block list.
		 */
		if (cyl != bzp->bz_cyl || head != bzp->bz_head ||
		    sect != bzp->bz_sect)
			continue;
		fflush(stdout);
		fprintf(stderr, "(%d, %d, %d) already in bad block list.\n",
			cyl, head, sect);
		return (TRUE);
	} /* end of for */

	/*
	 * Not found in list. Error if autorevector replacement
	 * or if SNF replacement not found in bad block list.
	 */
	if (sect & ZD_AUTOBIT) {
		fflush(stdout);
		fprintf(stderr, "(%d, %d, %d) not auto-revector replacement.\n",
			cyl, head, sect);
		return (TRUE);
	}
	if (sect >= chancfg->zdd_sectors && sect < totspt) {
		fflush(stdout);
		fprintf(stderr, "(%d, %d, %d) not a SNF replacement.\n",
			cyl, head, sect);
		return (TRUE);
	}
	return (FALSE);
}

#define FIRST 0
#define VERB 1
#define NUMB 2

/*
 * get_addlist()
 *	Get list of sectors to add to bad block list.
 *
 * Operator can input directly or provide name of file with addbad list.
 * List contains sectors in diskaddr form (cyl, head, sector) - 1 per line.
 *
 * Return the number of valid addlist entries. If error, return 0.
 */
get_addlist()
{
	char *advance_str();
	int naddbad;		/* number of addbad entries */
	int cyl, head, sect, type;	/* Items to be read */
	char *ptr;
	int status;
	char str[80];
	char buff[80];
	char *ip;
	int ftype;

	naddbad = 0;

	ptr = a_arg;
	if (debug) 
		printf("a_arg = %s\n", a_arg);

	ftype = FIRST;

	for (;;) {
		if (a_file) {	/* read list from file */
			if (fgets (buff, 80, a_file) == NULL)
				break;
			if (ftype == FIRST) {

				if (strncmp(buff, "Channel", 7) == 0) {
					if (verbose)
						printf("using defect listing format\n");
					/*
					 * except output in the form of 
					 * format -l
					 */
					ftype = VERB;
					while ( fgets(buff, 80, a_file) != NULL) {
						if (strncmp(buff,"Bad block",9) == 0)
							break;
					}
					continue;
				}
				ftype = NUMB;
			} 

			if (ftype == NUMB) {
				status = sscanf(buff, "%d%d%d%d", &cyl,
					&head, &sect, &type);
			} else {
				ip = buff;
				if ((*ip == '+') || (*ip == '-'))
					ip++;
				if (strncmp(ip, "phys", 4) == 0 ) {
					type = ip[5] == 'd';
					status = sscanf(&ip[type?9:11], 
							"%d%d%d", 
							&cyl, &head, &sect) + 1;
					type |= BZ_BADPHYS;
				} else if ( (strncmp(ip, "auto", 4) == 0 ) ||
				            (strncmp(ip, "snf ", 4) == 0 ) ) {
					type = ip[5] == 'd';
					status = sscanf(&ip[type?9:11], 
							"%d%d%d", 
							&cyl, &head, &sect) + 1;
				} else
					continue;
			}
				
		} else {
			if ((status = sscanf(ptr, "%d%d%d%d", &cyl,
					&head, &sect, &type)) == EOF)
				break;
		}

		if (status != 4) {
			fflush(stdout);
			fprintf(stderr, "bad syntax: Addbad list format:");
			fprintf(stderr, " cyl head sector type\n");
			fprintf(stderr, "            valid types are: ");
			fprintf(stderr, "Bad Header (%d) or Bad Data (%d).\n",
				BZ_BADHEAD, BZ_BADDATA);
			if (a_file)
				fclose(a_file);
			return(0);
		}
		if (!a_file)	/* advance ptr */
			ptr = advance_str(ptr, 4);

		if (debug)
			printf("bad spot: %d %d %d %d\n", cyl, head,
				sect, type);
		/*
		 * Have spot description. Now verify.
		 */
		if (bad_addentry(cyl, head, sect, type&~BZ_BADPHYS) == FALSE) {
			if (add_addlist( cyl, head, sect, type, &naddbad))
				break;
		}
	} /* end of main loop */

	/*
	 * Sort list
	 */
	qsort(addlist, naddbad, sizeof(struct addlist), addcomp);

	if (a_file)
		fclose(a_file);
	if (debug)
		printf("%d Addbad address%s entered.\n", naddbad,
			(naddbad != 1) ? "es" : "");
	return (naddbad);
}

/*
 * add a bad block entry the the add bad list.
 */
add_addlist(cyl, head, sect, type, naddbad)
	int cyl;
	int head;
	int sect;
	int type;
	int *naddbad;
{
	if (debug)
		printf("put on addlist entry %d\n", *naddbad);
	addlist[*naddbad].al_addr.da_cyl = cyl;
	addlist[*naddbad].al_addr.da_head = head;
	addlist[*naddbad].al_addr.da_sect = sect;
	addlist[*naddbad].al_type = type;
	(*naddbad)++;
	if (*naddbad == MAXADDBAD) {
		fflush(stdout);
		fprintf(stderr,
		"Addbad list full. Only %d bad block entries will be used on the disk.\n",
			*naddbad);
		fprintf(stderr,
		"Last read entry: (cyl %d, head %d, sector %d, type %d)\n",
			cyl, head, sect, type);
		return(1);
	}
	return(0);
}
