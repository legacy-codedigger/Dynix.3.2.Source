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
static char rcsid[]= "@(#)$Header: zdaddbad.c 1.10 91/03/26 $";
#endif

/*
 * addbad routines
 */

/* $Log:	zdaddbad.c,v $
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#include "../saio.h"
#include "zdformat.h"

struct addlist addlist[MAXADDBAD];	/* Addlist for addbad */
caddr_t	cylbuf;				/* buf to hold a cyl worth of data */
caddr_t	bootbuf;			/* buf to hold a track worth of data */

/*
 * addbad
 *	Add bad blocks to the bad block list and reformat as necessary.
 */
addbad(num, flag)
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

	if (flag == ADDBAD) {
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
			printf("ERROR - Bad block list will overflow.\n");
			printf("Only room for %d more entries.\n",
				i - bbl->bz_nelem + j);
			printf("No entries added.\n");
			return;
		}
	}

	for (i = 0; i < num; i += nspots) {
		cyl = addlist[i].al_addr.da_cyl;
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
		while (bzp < &bbl->bz_bad[bbl->bz_nelem] && bzp->bz_cyl < cyl) {
			if (bzp->bz_rtype == BZ_SNF)
				newbbl->bz_nsnf++;
			newbbl->bz_nelem++;
			*tobzp++ = *bzp++;
		}

		init_cylmap(cyl, chp);
		remark_spots(cyl, chp, bzp, ADDBAD);

		/*
		 * If addbad into Disk Description cylinder or any diagnostic
		 * cylinder, treat specially.
		 */
		if (cyl == ZDD_DDCYL || cyl >= (chancfg->zdd_cyls-ZDD_NDGNCYL)) {
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

			ioctl(fd, SAIOSETBBL, (char *)bbl);
			if (flag == ADDBAD) {
				write_badlist();
				if (cyl == ZDD_DDCYL) 
					write_mfglist();
				else
					write_dgndata();
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
			/*
			 * First save all data in the cylinder.
			 */
			recover_cyl(cyl, chp, bbl);
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
			while (bzp < &bbl->bz_bad[bbl->bz_nelem] &&
						bzp->bz_cyl == cyl)
				++bzp;	/* skip past this cylinder */
			while (bzp < &bbl->bz_bad[bbl->bz_nelem]) {
				if (bzp->bz_rtype == BZ_SNF)
					newbbl->bz_nsnf++;
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

			ioctl(fd, SAIOSETBBL, (char *)bbl);

			format_cyl(cyl, chp);
			check_hdrs(cyl, chp, TRUE);

			/*
			 * rewrite cylinder data
			 */
			restore_cyl(cyl);
			/*
			 * If snftogo, then do_snf_pass to resolve
			 * remaining blocks.
			 */
			if (snftogo > 0)
				do_snf_pass(0);
		} else {
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

			ioctl(fd, SAIOSETBBL, (char *)bbl);
			do_snf_pass(0);
		}
		if (flag == ADDBAD)
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
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)
		if (hp->h_sect == alp->al_addr.da_sect) {
			hp->h_flag = alp->al_type;
			hp->h_sect = hp - tp->t_hdr;
			(void)newbad(hp, BZ_PHYS, (struct hdr *)NULL);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			rewrite_hdr(alp->al_addr.da_cyl, alp->al_addr.da_head,
					(u_char)(hp - tp->t_hdr), hp);
			break;
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
			printf("Ruptured disk - too many errors (%d) in bootstrap track.\n",
					tp->t_bad);
			exit(11);
		}
		/*
		 * recover cylinder 1, track 0.
		 */
		if (bootbuf == (caddr_t)NULL) {
			callocrnd(DEV_BSIZE);
			bootbuf = calloc(chancfg->zdd_sectors << DEV_BSHIFT);
		} else
			bzero(bootbuf, (u_int)(chancfg->zdd_sectors << DEV_BSHIFT));
		/*
		 * save bootstrap sectors.
		 */
		where = (chancfg->zdd_sectors * chancfg->zdd_tracks) << DEV_BSHIFT;
		lseek(fd, where, L_SET);
		if (read(fd, bootbuf, ncontig << DEV_BSHIFT)
					!= ncontig << DEV_BSHIFT) {
			/*
			 * If problem try a sector at a time.
			 */
			cbp = bootbuf;
			for (i = 0; i < ncontig; i++) {
				lseek(fd, where, L_SET);
				if (read(fd, cbp, DEV_BSIZE) != DEV_BSIZE) {
					/*
					 * Recover data
					 */
					recover_sect(chp, 1, 0, (u_char)i, cbp);
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
					hp->h_flag = alp->al_type;
					hp->h_sect = hp - tp->t_hdr;
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
		lseek(fd, where, L_SET);
		if (write(fd, bootbuf, ncontig << DEV_BSHIFT)
					!= ncontig << DEV_BSHIFT) {
			/*
			 * If problem try a sector at a time.
			 */
			cbp = bootbuf;
			for (i = 0; i < ncontig; i++) {
				lseek(fd, where, L_SET);
				if (write(fd, cbp, DEV_BSIZE) != DEV_BSIZE) {
					printf("Warning: Could not restore data in cyl 1, bn %d).\n", i);
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
			hp->h_flag = alp->al_type;
			bzp = newbad(hp, BZ_SNF, hp);
			append_snflist(bzp, chp);
			hp->h_type = ZD_BADUNUSED;
			hp->h_cyl = ZD_BUCYL;
			hp->h_head = ZD_BUHEAD;
			hp->h_sect = ZD_INVALSECT;
			rewrite_hdr(alp->al_addr.da_cyl, alp->al_addr.da_head,
					(u_char)(hp - tp->t_hdr), hp);
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
	u_short	rplcyl;
	u_char	rplhead;
	u_char	rplsect;

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
			rewrite_hdr(bzp->bz_cyl, bzp->bz_head,
					(u_char)(hp - tp->t_hdr), hp);
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
	u_char	trk;
	u_char	sect;
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

	while (nspots) {
		if (alp->al_addr.da_sect & ZD_AUTOBIT)
			tp = &chp->c_trk[(alp->al_addr.da_head + 1) % chancfg->zdd_tracks];
		else
			tp = &chp->c_trk[alp->al_addr.da_head];

		/*
		 * Bad block given is logical address. Find physical sector
		 * and mark as bad to-be-resolved.
		 */
		for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)
			if (hp->h_sect == alp->al_addr.da_sect)  {
				hp->h_flag = alp->al_type | ZD_TORESOLVE;
				break;
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

	snf_list[snftogo].snf_addr.da_cyl = bzp->bz_cyl;
	snf_list[snftogo].snf_addr.da_head = bzp->bz_head;
	snf_list[snftogo].snf_addr.da_sect = bzp->bz_sect;
	callocrnd(DEV_BSIZE);
	snf_list[snftogo].snf_data = calloc(DEV_BSIZE);

	/*
	 * attempt to read sector.
	 */
	where = (bzp->bz_cyl * chancfg->zdd_sectors * chancfg->zdd_tracks)
		+ (bzp->bz_head * chancfg->zdd_sectors) + bzp->bz_sect;
	lseek(fd, where << DEV_BSHIFT, L_SET);
	if (read(fd, snf_list[snftogo].snf_data, DEV_BSIZE) != DEV_BSIZE) {
		/*
		 * cannot normally read - try ZDC_REC_DATA.
		 */
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

	tracksize = chancfg->zdd_sectors << DEV_BSHIFT;
	/*
	 * allocate memory for cylinder
	 */
	if (cylbuf == (caddr_t)NULL) {
		callocrnd(DEV_BSIZE);
		cylbuf = calloc(tracksize * chancfg->zdd_tracks);
	} else
		bzero(cylbuf, tracksize * chancfg->zdd_tracks);

	/*
	 * read the cylinder a track at at time.
	 */
	where = cyl * chancfg->zdd_tracks * tracksize;
	cbp = cylbuf;
	for (track = 0; track < chancfg->zdd_tracks; track++) {
	    lseek(fd, where, L_SET);
	    if (read(fd, cbp, tracksize) != tracksize) {
		/*
		 * If problem try a sector at a time.
		 */
		for (sect = 0; sect < chancfg->zdd_sectors; sect++) {
		    lseek(fd, where, L_SET);
		    if (read(fd, cbp, DEV_BSIZE) != DEV_BSIZE) {
			bzp = find_bbl_entry(blp, cyl, (u_char)track,
							(u_char)sect);
			if (bzp == NULL)
				recover_sect(chp, (u_short)cyl, (u_char)track,
							(u_char)sect, cbp);
			else	if (bzp->bz_rtype == BZ_AUTOREVECT)
					recover_sect(chp, (u_short)cyl,
						(u_char)track,
						(u_char)(sect | ZD_AUTOBIT), cbp);
			else	if (bzp->bz_rtype == BZ_SNF && bzp->bz_cyl == cyl)
					/*
					 * BZ_SNF revector on cylinder
					 */
					recover_sect(chp,
						bzp->bz_rpladdr.da_cyl,
						bzp->bz_rpladdr.da_head,
						bzp->bz_rpladdr.da_sect, cbp);
			else {
				printf("ILLEGAL format! Unexpected revector off-cylinder\n");
				printf("Revector (%d, %d, %d) to (%d, %d, %d).\n",
					bzp->bz_cyl, bzp->bz_head, bzp->bz_sect,
					bzp->bz_rpladdr.da_cyl,
					bzp->bz_rpladdr.da_head,
					bzp->bz_rpladdr.da_sect);
				printf("Could not recover data from (%d, %d, %d).\n",
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
	u_short	cyl;
	u_char	track;
	u_char	sector;
	caddr_t	bp;				/* Where to put data */
{
	register struct hdr *hp;
	register struct track_hdr *tp;
	int physect;
	u_char	trk;			/* adjusted in autorevect case */
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
		printf("Could not find address (%d, %d, %d) in cylinder map.\n",
			cyl, track, sector);
		printf("Could not recover data from (%d, %d, %d).\n",
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
	lcb.cb_addr = (u_long)bp;
	lcb.cb_count = DEV_BSIZE;
	if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) <  0) {
		printf("Warning: ZDC_REC_DATA failed - data unrecoverable.\n");
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
		lseek(fd, where, L_SET);
		if (write(fd, cbp, tracksize) != tracksize) {
			/*
			 * If problem try a sector at a time.
			 */
			for (sect = 0; sect < chancfg->zdd_sectors; sect++) {
				lseek(fd, where, L_SET);
				if (write(fd, cbp, DEV_BSIZE) != DEV_BSIZE) {
					printf("Warning: Could not restore data to (%d, %d, %d).\n",
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
	u_short	cyl;
	u_char	head;
	u_char	sect;
	struct	hdr	*hp;
{
	struct	cb	lcb;		/* cb for ZDC_WHDR_WDATA command */
	static	struct hdr *ahp;	/* aligned header for ZDC_WHDR_WDATA */

	if (ahp == (struct hdr *)NULL) {
		callocrnd(ADDRALIGN);
		ahp = (struct hdr *)calloc(CNTMULT + DEV_BSIZE);
	}
	*ahp = *hp;
	lcb.cb_cyl = cyl;
	lcb.cb_head = head;
	lcb.cb_sect = sect;
	lcb.cb_addr = (u_long)ahp;
	lcb.cb_count = CNTMULT + DEV_BSIZE;
	lcb.cb_iovec = 0;
	lcb.cb_cmd = ZDC_WHDR_WDATA;
	if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) < 0) {
		printf("rewrite_hdr ioctl failed!!!\n");
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
	u_short	cyl;
	u_char	head;
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
	lcb.cb_addr = (u_long)tp->t_hdr;
	lcb.cb_count = size;
	lcb.cb_iovec = 0;
	lcb.cb_cmd = ZDC_FMTTRK;
	/*
	 * Remark spares as ZD_GOODSECT so that they can be verified.
	 */
	for (hp = tp->t_hdr; hp < &tp->t_hdr[totspt]; hp++)
		if (hp->h_type == ZD_GOODSPARE)
			hp->h_type = ZD_GOODSECT;
	if (ioctl(fd, SAIOZDCCMD, (char *)&lcb) < 0) {
		printf("reformat_trk ioctl failed!!!\n");
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
	printf("WARNING: More than one addbad at same sector.\n");
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
		printf("Bad cylinder number %d.\n", cyl);
		err = TRUE;
	}
	if (head < 0 || head >= chancfg->zdd_tracks) {
		printf("Bad head number %d.\n", head);
		err = TRUE;
	}
	if (sect < 0 || sect > ZD_MAXSECT
	||  ((sect >= totspt) && ((sect & ZD_AUTOBIT) != ZD_AUTOBIT))) {
		printf("Bad sector number %d.\n", sect);
		err = TRUE;
	}
	if (type != BZ_BADHEAD && type != BZ_BADDATA) {
		printf("Bad type %d. Enter Bad Header (%d) or Bad Data (%d).\n",
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
				printf("(%d, %d, %d) already in bad block list.\n",
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
		printf("(%d, %d, %d) already in bad block list.\n",
			cyl, head, sect);
		return (TRUE);
	} /* end of for */

	/*
	 * Not found in list. Error if autorevector replacement
	 * or if SNF replacement not found in bad block list.
	 */
	if (sect & ZD_AUTOBIT) {
		printf("(%d, %d, %d) not auto-revector replacement.\n",
			cyl, head, sect);
		return (TRUE);
	}
	if (sect >= chancfg->zdd_sectors && sect < totspt) {
		printf("(%d, %d, %d) not a SNF replacement.\n",
			cyl, head, sect);
		return (TRUE);
	}
	return (FALSE);
}

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
	register char *cp;
	int naddbad;			/* number of addbad entries */
	int cyl, head, sect, type;	/* Items to be read */
	int	dfd;
	int	nread;			/* no. of chars read */
	int	count;
	int	i;
	static char addbuf[1024];	/* Input buffer if from file */
	char	*lastline;		/* last line in addbuf */

	naddbad = 0;

again:
	/*
	 * Ask for add bad list. Enter manually or filename?
	 */
	do
		dfd = atoi(prompt("Enter addbad list manually (0) or from file (1)? "));
	while (dfd < 0 || dfd > 1);

	if (dfd == 0) {
		/* Manually */
		dfd = -1;
		printf("\nAddbad list format: cyl head sector type\n");
		printf("Valid types are: Bad Header (%d) or Bad Data (%d).\n\n",
			BZ_BADHEAD, BZ_BADDATA);
	} else {
		/*
		 * Get file and open.
		 */
		cp = prompt("File with addbad list information? ");
		if (!*cp || *cp == '\n')
			goto again;
		dfd = open(cp, 0);
		if (dfd < 0) {
			printf("Cannot open %s\n", cp);
			goto again;
		}
		count = 0;
		lastline = addbuf;
		cp = addbuf;
	}

	/*
	 * Read in addbad list.
	 */
	for(;;) {
		if (dfd < 0) {
			/*
			 * prompt user.
			 */
			printf("Enter addbad entry %d (cyl head sector type): ",
				naddbad + 1);
			cp = prompt("");
		} else {
			if (cp == lastline) {
				count = count - (lastline - addbuf);
				bcopy(lastline, addbuf, count);
				lastline = &addbuf[count];
				/* Note kludge for ts tape */
				nread = read(dfd, lastline,
					((sizeof addbuf) - count) & ~511);
				count += nread;
				/*
				 * Replace newlines with null.
				 */
				for (cp = addbuf; cp < addbuf + count; cp++)
					if (*cp == '\n') {
						*cp = 0;
						lastline = cp + 1;
					}
				cp = addbuf;
			}
			if (cp == lastline) {
				/*
				 * empty buffer.
				 */
				cp = addbuf;
				addbuf[0] = 0;		/* set to null */
			}
		}
		i = scani(cp, 4, &cyl, &head, &sect, &type);
		if (i < 0)
			break;		/* done */
		if (i == 4) {
			/*
			 * Have spot description. Now verify.
			 */
			if (bad_addentry(cyl, head, sect, type) == FALSE) {
				if (add_addlist( cyl, head, sect, type, &naddbad))
					break;
			}
		} else {
			printf("Bad syntax: %s. Expected four numbers, cyl  head  sector type, separated by tabs or spaces.\n", cp);
		}
		/*
		 * Skip to end of line
		 */
		for (; *cp++;)
			continue;
	} /* end of main loop */
	/*
	 * Sort list
	 */
	qsort(addlist, naddbad, sizeof(struct addlist), addcomp);

	if (dfd >= 0) {
		close(dfd);
		if (nread > 0) {
			printf("WARNING: Entire file not read!\n");
		}
	}
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
	addlist[*naddbad].al_addr.da_cyl = cyl;
	addlist[*naddbad].al_addr.da_head = head;
	addlist[*naddbad].al_addr.da_sect = sect;
	addlist[*naddbad].al_type = type;
	(*naddbad)++;
	if (*naddbad == MAXADDBAD) {
		printf("Addbad list full. %d elements will be processed.\n",
			*naddbad);
		printf(
		"Last read entry: (cyl %d, head %d, sector %d, type %d)\n",
			cyl, head, sect, type);
		return(1);
	}
	return(0);
}
