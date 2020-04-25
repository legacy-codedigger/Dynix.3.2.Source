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

#ifdef RCS
static char rcsid[]= "@(#)$Header: xpformat.c 2.8 1991/07/02 01:06:32 $";
#endif

/*
 * Standalone program to do media checking
 * and record bad block information on any
 * disk supported by the Xylogics 450 controller.
 */

#include <sys/param.h>
#include <sys/fs.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/vtoc.h>
#include <mbad/dkbad.h>
#include <sys/vmmac.h>

#include "saio.h"
#include "xp.h"

#define	DEBUG			/* this code really always wants this on */

#define NBEATS		15	/* nbr times to beat up a track after error */
#define NCLEAN		3	/* nbr of clean passes needed to end severe */

#define MAXBADDESC	DK_MAXBAD	/* size of bad block table */

#define SECTSIZ		512		/* standard sector size */
#define TRKSIZE		28160		/* total bytes/track */
#define SECTBYTES	600		/* bytes/sector including header */
#define RUNT		560		/* nbr bytes in runt sector */
#define BYTESIZE	8		/* nbr bits/byte */
#define MAXBYTELEN	64		/* max nbr bytes in defective spot */
#define MAXBITLEN	MAXBYTELEN * BYTESIZE

#define SSERR		0
#define BSERR		1

#define DFLT_RPM        3600

struct	dkbad		dkbad[DK_NBADMAX];		/* bad sector table */
union 	bt_bad		*bt = dkbad[0].bt_bad;
struct	dkbad_mfg	dkbad_mfg[DK_NBADMAX];		/* mfg defect list */
union 	bt_mfgbad	*bt_mfg = dkbad_mfg[0].bt_mfgbad;

struct	x450ioctl	x450;		/* ioctl command for controller */
int	partno = 1;			/* no. of partition to make */
int	writevtoc = 0;
extern struct partition xp_proto[];

#define	NERRORS		5
static char *
errornames[NERRORS] = {
#define	FE_BSE		0
	"Bad sector",
#define	FE_WCE		1
	"Write check",
#define	FE_ECC		2
	"ECC",
#define	FE_HARD		3
	"Other hard",
#define	FE_TOTAL	4
	"Total",
};

int	errors[NERRORS];	/* histogram of errors */
int	pattern;

/*
 * Purdue/EE severe burnin patterns.
 * 	Modified by Sequent, Inc. - Jan 85
 */
unsigned ppat[] = {
	0xB6DB6DB6,	0xAAAAAAAA,	0x55555555,
#define NFDP	3	/* Nbr of full-disk pass patterns */
							0x33333333,
	0x71C71C71,	0xDB6DB6DB,	0xE38EE38E,	0xC71C71C7,
	0x8E38E38E,	0x1C71C71C,	0x38E38E38,	0x49249249,
	0x24924924,	0x92492492,	0x6DB6DB6D,	0xB6DB6DB6,
	0x55555555,	0x00000000,	0xAAAAAAAA,	0xFFFFFFFF,
	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,	0x55555555,
	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,	0x55555555,
	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,	0x55555555,
	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,	0x55555555,
	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,	0x55555555,
	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,	0x55555555,
	0xAAAAAAAA,	0x55555555,	0xAAAAAAAA,	0x55555555
};

/*
 * sector array
 */

int sector[20][46] = {
{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
 42, 43, 44, 45},

{45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 
 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
 41, 42, 43, 44},

{44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43},

{43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
 39, 40, 41, 42},

{42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
 38, 39, 40, 41},

{41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
 37, 38, 39, 40},

{40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
 36, 37, 38, 39},

{39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
 35, 36, 37, 38},

{38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
 34, 35, 36, 37},

{37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
 33, 34, 35, 36},

{36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
 32, 33, 34, 35},

{35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
 31, 32, 33, 34},

{34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33},

{33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7, 8,
 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
 29, 30, 31, 32},

{32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5, 6, 7,
 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
 28, 29, 30, 31},

{31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4, 5,
 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
 27, 28, 29, 30},

{30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3, 4,
 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
 26, 27, 28, 29},

{29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1, 2, 3,
 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
 25, 26, 27, 28},

{28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0, 1,
 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
 24, 25, 26, 27},

{27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 0,
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
 23, 24, 25, 26}
 };

#define	NPT	(sizeof (ppat) / sizeof (ppat[0]))

#define NEAR 1
#define TESTSPOT(s) (spots[(s / st.nsect) / 8] & (1 << ((s / st.nsect) % 8)))

int	npat;		/* subscript to ppat[] */
int	severe;		/* nz if running "severe" burnin */
int	testall;	/* nz if testing entire disk (no bad spot list) */
int	nbads;		/* subscript for bads */
long	bads[MAXBADDESC]; /* Bad blocks accumulated */

caddr_t	calloc();
int	qcompar();
char	*prompt();

struct st st;
char *wbp;
char *rbp;
int tracksize;
int maxcyl;
int pass;
int checkdata;
int fd;
int nextbad;
int lastbads;
char	devstr[132];		/* Save disk device name for reopen */
#ifdef	DEBUG
int debug;
#endif
int badfound;	/* true if bad found in a severe test pass */

char spots[5000];

main()
{
	char *cp;
	int i;
	int j;
	int maxpass;
	int ncyls;
	int flags;
	int dfault;

	printf("\nDisk format/check utility for Xylogics 450 SMD controller\n");

again:
	printf("\n");
#ifdef DEBUG
	debug = atoi(prompt("Enable debugging [0=none, 1=bse, 2=ecc, 3=bse+ecc]? "));
	if (debug < 0 || debug > 3)
		debug = 0;
#endif DEBUG
	nbads = 0;
	for (i = 0; i < NERRORS; i++)
		errors[i] = 0;
	maxpass = 0;
	fd = getdevice();
	dfault = *prompt("Use default options [y/n]? ") == 'y';
	if (dfault) {
		printf("Format & verify with severe burnin (%d disk passes) and no data check.\n", NPT);
		flags = 3;
		maxcyl = 0;
		checkdata = 0;
		severe = 1;
	} else {
		do flags = atoi(prompt("Format (1), Verify (2), Both (3), Reformat (4) write VTOC (5)? "));
		while (flags <= 0 || flags > 5);
		maxcyl = atoi(prompt("Number of cylinders (0 for all)? "));
		if (maxcyl < 0)
			maxcyl = 0;
		checkdata = 0;
		if (flags & 0x2)
			checkdata = *prompt("Check data [y/n]? ") == 'y';
		if (flags == 5)
			writevtoc = 1;
	}
	if (flags != 2 && (flags != 5))
		writevtoc = *prompt("Write minamal VTOC [y/n]? ") == 'y';

	ioctl(fd, SAIODEVDATA, &st);
	printf("Device data: #cylinders=%d, #tracks=%d, #sectors=%d\n",
						st.ncyl, st.ntrak, st.nsect);
	if (!dfault) {
		severe = 0;
		if ((flags & 0x2) && getpattern()) {
			close(fd);
			goto again;
		}
	}
	if (severe) {
		getspots(dfault);
		npat = 0;
		if (dfault || testall)
			maxpass = NPT;
		else {
			printf("Severe burnin requested. Number of disk passes available: %d - %d.\n", NFDP, NPT);
			do maxpass = atoi(prompt("Number of disk passes? "));
			while (maxpass < NFDP || maxpass > NPT);
		}
	}
	else
		maxpass = 1;

	do {
		cp = prompt("Proceed [y/n]? ");
		if (*cp == 'n')
			goto fini;
		} while (*cp != 'y');
	if (severe)
		ioctl(fd, SAIOSEVRE, (char *) 0);
	ioctl(fd, SAIONOBAD, (char *)0);
	ioctl(fd, SAIOECCLIM, (char *)0);
#ifdef DEBUG
	ioctl(fd, SAIODEBUG, (char *)debug);
#endif DEBUG
	if (flags == 2)
		goto noformat;
	if (flags == 5)
		goto writev;
	if (flags == 4) {
		int maxentries, j;

		if(readbad(fd, &st) == 0) {
			printf("Cannot read bad block list\n");
			goto done;
		}
		maxentries = DK_NBAD_N * dkbad->bt_lastb + DK_NBAD_0;
		for (i = 0; i < maxentries; i++)
			if (bt[i].bt_cyl == (unsigned) -1)
				break;
		errors[FE_TOTAL] = i;
		j = SECTSIZ * DK_MDBSF((st.ncyl * st.nspc - st.nsect));
		lseek(fd, j, 0);
		read(fd, (char *)dkbad_mfg, DK_NBADMAX * 
						sizeof(struct dkbad_mfg));
	}
	/*
	 * first format the drive using the hardware format command
	 */
#define	NCPF	71	/* cylinders per format */
	printf("Formatting");
	ncyls = maxcyl ? maxcyl : st.ncyl;
	for (i = 0; ncyls > 0; i++) {
		printf(".");
		x450.cmd = XP_FORMAT;
		x450.cyl  = i * NCPF;
		x450.head = 0;
		x450.sect = 0;
		x450.scnt = st.nspc * (ncyls < NCPF ? ncyls : NCPF);
		ioctl(fd, SAIOX450CMD, &x450);
		ncyls -= NCPF;
	}
	printf("done\n");
noformat:
	/*
	 * now write and read data, looking for bad spots
	 */
	tracksize = st.nsect * SECTSIZ;
	callocrnd(SECTSIZ);
	wbp = calloc(tracksize);
	if (flags == 1)
		goto final;
	if (flags == 4)
		goto out;

	rbp = calloc(tracksize);
	for (pass = 0; pass < maxpass; pass++) {
		if (severe) {
			printf("Begin pass %d\n", pass);
			if (nbads)
				qsort(bads, nbads, sizeof (long), qcompar);
			nextbad = 0;
			lastbads = nbads;
		}
		badfound = 0;
		if (dopass())
			goto out;
#if NCLEAN > 0
		/* force NCLEAN clean passes at end of severe test */
		if (badfound && pass + NCLEAN >= maxpass)
			maxpass = pass + NCLEAN + 1;
#endif
	}
	/*
	 * Checking finished.
	 */
out:
	if (severe && nbads) {
		/*
		 * Sort bads and insert in bad block table.
		 */
		qsort(bads, nbads, sizeof (long), qcompar);
		severe = 0;
		for (i = 0; i < nbads; i++) {
			errno = EECC;	/* for now */
			(void) recorderror(bads[i], &st, debug & 0x1);
		}
		severe++;
	}
	if (errors[FE_TOTAL]) {
		if ((flags & 0x2) != 0) {
			printf("Total of %d hard errors found\n",
				errors[FE_TOTAL]);
		}
		/* change the headers of all the bad sectors */
		rewrite(fd, errors[FE_TOTAL], &st, debug & 0x1);
	}
final:
	i = errors[FE_TOTAL];
	if(i <= DK_NBAD_0)
		dkbad[0].bt_lastb = 0;
	else
		dkbad[0].bt_lastb = ((i - DK_NBAD_0)/DK_NBAD_N) + 1;

	while (i < MAXBADDESC) {
		bt[i].bt_cyl = -1;
		bt[i++].bt_trksec = -1;
	}

	printf("\nWriting bad sector table at sector #%d\n",
		st.ncyl * st.nspc - st.nsect);
	/* place on disk */
	writebad(fd, &st);

	if((severe) || (flags == 4)) {
		printf("\nWriting manufactures defect list at sector #%d\n",
			DK_MDBSF(st.ncyl * st.nspc - st.nsect));
		j = SECTSIZ * DK_MDBSF((st.ncyl * st.nspc - st.nsect));
		lseek(fd, j, 0);
		write(fd, (char *)dkbad_mfg, DK_NBADMAX * 
						sizeof(struct dkbad_mfg));
	}
	if (severe)
		ioctl(fd, SAIONSEVRE, (char *) 0);
	ioctl(fd, SAIOECCUNL, (char *)0);
	printf("\nWriting diagnostic cylinders\n");
	for (i = (st.ncyl * st.nspc) - 3 * st.nspc;
	     i < (st.ncyl * st.nspc) - 2 * st.nspc + st.nsect * 13;
	     i += st.nsect) {
		for (j = 0; j < st.nsect; j++)
			mk_diag(wbp + j * SECTSIZ, i + j);
		lseek(fd, SECTSIZ * i, 0);
		j = SECTSIZ * st.nsect;
		if (write(fd, wbp, j) == j)
			continue;
		lseek(fd, SECTSIZ * i, 0);
		for (j = 0; j < st.nsect; j++)
			(void) write(fd, wbp + j * SECTSIZ, SECTSIZ);
	}
	for (j = 0; j < st.nsect - DK_NBADMAX; j++)
		mk_diag(wbp + j * SECTSIZ, i + j);
	lseek(fd, SECTSIZ * i, 0);
	for (j = 0; j < st.nsect - DK_NBADMAX; j++)
			(void) write(fd, wbp + j * SECTSIZ, SECTSIZ);

writev:
	if (writevtoc)
		write_min_vtoc();
done:
	printf("Done\n");
fini:
	close(fd);
}

/*
 * Read the bad block list
 */

readbad(fd, st)
int fd;
struct st *st;
{
	int block, copy;
	int i;

	block = 0;
	do {
		for(copy=0; copy<DK_NBADCOPY; copy++) {
			if(lseek(fd, SECTSIZ * (st->ncyl * st->nspc - st->nsect
					 + DK_LOC(block, copy)), L_SET) == -1)
				continue;
			if(read(fd, (char *)&dkbad[block], sizeof(struct dkbad))
					== sizeof(struct dkbad))
				break;
		}
		if(copy == DK_NBADCOPY)
			return(0);
	
	} while(++block < DK_NBADMAX);
	return(1);
}

/*
 * write the bad block list
 */

writebad(fd, st)
int fd;
struct st *st;
{
	int block, copy;

	block = 0;
	do {
		for(copy=0; copy<DK_NBADCOPY; copy++) {
			if(lseek(fd, SECTSIZ * (st->ncyl *st->nspc - st->nsect
					+ DK_LOC(block, copy)), L_SET) == -1) {
				printf("warning: can't write bad block %d copy\
							%d\n", block, copy);
				continue;
			}
			if(write(fd, (char *)&dkbad[block], sizeof(struct dkbad))
					!= sizeof(struct dkbad))
				printf("warning: can't write bad block %d copy\
							%d\n", block, copy);
		}
	} while(++block < DK_NBADMAX);
}

dopass()
{
	int cyl;
	register long sn;
	register long sector;
	long lastsector;
	int resid;
	int nsec;

	bufinit(wbp, tracksize);
	/*
	 * Begin check, for each track,
	 *
	 * 1) Write data
	 * 2) Read data.  Hardware checks header and data ECC.
	 *    Read data (esp on Eagles) is much faster when write check.
	 */
	lastsector = st.nspc * (maxcyl ? maxcyl : st.ncyl);
	for (sector = 0; sector < lastsector; sector += st.nsect) {
		if (pass >= NFDP && !(testall || TESTSPOT(sector)))
			continue;

		cyl = sector / st.nspc;
		if ( (pass < NFDP || testall) && (sector % (st.nspc * 50)) == 0)
			printf("Starting cylinder %d\n", cyl);

		/*
		 * Read/Write test patterns.
		 * Skip sectors already found to be bad.
		 */
		resid = st.nsect;
		sn = sector;
		for (; resid;) {
			nsec = resid;
			if (nextbad < lastbads && sn + nsec > bads[nextbad]) {
				resid = sn + nsec - bads[nextbad] - 1;
				nsec = bads[nextbad] - sn;
				nextbad++;
			} else
				resid = 0;
			if (nsec) {
				testwrite(sn, wbp, SECTSIZ * nsec);
				if (testread(sn, rbp, SECTSIZ * nsec))
					return(1);
			}
			sn += nsec + 1;
		}
	}
	if (++npat >= NPT)
		npat = 0;
	return(0);
}

testwrite(sector, cbp, length)
register long sector;
char *cbp;
{
	register long sn;

	lseek(fd, sector * SECTSIZ, 0);
	if (write(fd, cbp, length) == length)
		return(0);

	for (sn = sector; sn < sector + length/SECTSIZ; sn++) {
		lseek(fd, sn * SECTSIZ, 0);
		(void) write(fd, cbp, SECTSIZ);
	}
	return(0);
}

testread(sector, cbp, length)
register long sector;
char *cbp;
{
	int cc;
	register long sn;
	int cyl;
	int trk;
	int nbeats;

	cyl = sector / st.nspc;
	trk = (sector % st.nspc) / st.nsect;
	lseek(fd, sector * SECTSIZ, 0);
	if (checkdata)
		bzero(cbp, length);
	cc = read(fd, cbp, length);
	if (cc == length) {
		if (checkdata && checkbuf(cbp, &cc)) {
			printf("Data bad at (%d, %d, %d+%d)\n",
				cyl, trk, sector, cc);
			if (recorderror(sector + cc/SECTSIZ, &st, 0) < 0)
				return(1);
		}
		return(0);
	}
	for (nbeats = severe ? NBEATS : 1; nbeats; --nbeats) {
		for (sn = sector; sn < sector + length/SECTSIZ; sn++) {
			if (severe) {
				/* check if sector already known as bad */
				for (cc = nbads; cc; --cc)
					if (bads[cc - 1] == sn)
						break;
				if (cc)
					continue;
			}
			lseek(fd, sn * SECTSIZ, 0);
			if (checkdata)
				bzero(cbp, SECTSIZ);
			cc = read(fd, cbp, SECTSIZ);
			if (cc == SECTSIZ) {
				if (checkdata && checkbuf(cbp, &cc)) {
					printf("Data bad at (%d, %d, %d+%d)\n",
						cyl, trk, sn, cc);
					if (recorderror(sn, &st, 0) < 0)
						return(1);
				}
			} else {
				printf("Sector %d, read error\n", sn);
				if (recorderror(sn, &st, 0) < 0)
					return(1);
			}
		}
	}
	return(0);
}

qcompar(l1, l2)
register long *l1, *l2;
{
	if (*l1 < *l2)
		return(-1);
	if (*l1 == *l2)
		return(0);
	return(1);
}

struct	eccsect	{
	long	header;		/* header */
	char	data[512];	/* data */
	char	ecc[4];		/* data ecc */
} eccsect;

/*
 * Rewrite the headers on the bad blocks
 * so they will produce errors when accessed.
 */

rewrite(fd, nbadsect, st, debug)
register int fd, nbadsect, debug; 
register struct st *st;
{
	register i;

	for (i = 0; i < nbadsect; i++) {
		x450.cmd  = XP_XREAD;
		x450.cyl  = bt[i].bt_cyl;
		x450.head = bt[i].bt_trksec >> 8;
		x450.sect = bt[i].bt_trksec & 0xFF;
		x450.sect = (x450.sect + x450.head) % st->nsect;
		x450.buf  = (caddr_t) &eccsect;
		x450.scnt = 1;
		ioctl(fd, SAIOX450CMD, &x450);
#ifdef DEBUG
		if (debug)
			printf("header (%d, %d, %d) from 0x%x to 0x%x\n",
				x450.cyl, x450.head, x450.sect, eccsect.header,
				0xEEEEEEEE);
#endif DEBUG
		eccsect.header = 0xEEEEEEEE;
		x450.cmd  = XP_XWRITE;
		ioctl(fd, SAIOX450CMD, &x450);
	}
}

/*
 * Record an error, and if there's room, put
 * it in the appropriate bad sector table.
 *
 * If severe burnin store block in a list after making sure
 * we have not already found it on a prev pass.
 */
recorderror(bn, st, debug)
	long bn;
	register struct st *st;
{
	int cn, tn, sn;
	register i;


	if (severe) {
		for (i = 0; i < nbads; i++)
			if (bads[i] == bn)
				return(0);	/* bn already flagged */
		if (nbads >= MAXBADDESC) {
			printf("Bad sector table full, burnin terminating\n");
			return(-1);
		}
		bads[nbads++] = bn;
		setspot(bn, NEAR);
		badfound = 1;
		return(0);
	}
	if (errors[FE_TOTAL] == MAXBADDESC) {
		printf("Too many bad sectors\n");
		return(-1);
	}
	if (errno >= EBSE && errno <= EHER) {
		errno -= EBSE;
		errors[errno]++;
	}
	cn = bn / st->nspc;
	sn = bn % st->nspc;
	tn = sn / st->nsect;
	sn %= st->nsect;
	/* record the bad sector address and continue */
#ifdef DEBUG
	if (debug)
		printf("marking (%d, %d, %d) bad\n", cn, tn, sn);
#endif DEBUG
	bt[errors[FE_TOTAL]].bt_cyl = cn;
	bt[errors[FE_TOTAL]++].bt_trksec = (tn << 8) + sn;
	return(0);
}

/*
 * Prompt and verify a device name from the user.
 */
getdevice()
{
	extern char *index();
	register lfd;
	register char *pp;
	char partbuf[20];
	char *dp, *cp, *fn;
	int n = V_NUMPAR;

	/*
	 * Load up partbuf with the character translation of V_NUMPAR.
	 */
	bzero(partbuf, 20);
	pp = partbuf + sizeof(partbuf) - 1;
	*pp-- = '\0';
	do {
		*pp-- = "0123456789"[n%10];
		n /= 10;
	} while (n);
	pp++;

	for (;;) {
		cp = prompt("Device to format? (or q to quit) ");
		if (cp[0] == 'q' && cp[1] == '\0') {
			exit(0);
		} else if (cp[0] != 'x' || cp[1] != 'p') {
			printf("Example for XP 1, drive 0: xp(1,0)\n");
			continue;
		}
		/*
		 * Save for reopen.
		 */
		dp = devstr;
		while (*dp++ = *cp++)
			continue;
		/*
		 * Replace the partition number with V_NUMPAR
		 */
		dp = index(devstr, ',');
		dp++;
		strcpy(dp, pp);
		dp = devstr + strlen(devstr);
		strcpy(dp, ")");
		/*
		 * Now try the open
		 */
		if ((lfd = open(devstr, 2)) < 0) {
			printf("Open failed. Check input, cables and drive switch settings.\n");
			printf("Example for ZDC 1, drive 0: zd(16,0)\n");
			continue;
		}

		printf("Formatting drive xp%d on controller %d: ", 
			iob[lfd - 3].i_unit % 8, iob[lfd - 3].i_unit / 8);
		do {
			cp = prompt("Verify [y/n]? ");
			if (*cp == 'y')
				return (lfd);
		} while (*cp != 'n');
		close(lfd);
	}
}

getspots(dflt)
{
	register i, j, k;
	register char *cp;
	int fd;
	static char buffer[1000];
	int count;
	char *lastline;
	int cyl, head, byte, len, resid;
	int nspots;
	int nread;
	long sum;
	int bn, sec;
	char err;

	bzero(spots, sizeof spots);
	nspots = 0;
again:
	if (dflt) {
		dflt = 0;
		fd = 1;
	} else {
		printf("Test options are:\n\
 1 - Test bad spots, spots entered from file\n\
 2 - Test bad spots, spots entered manually\n\
 3 - Test entire disk (takes %d hours)\n", checkdata ? 5 * NPT / 4 : NPT / 2); 
		do fd = atoi(prompt("Enter test option [1-3]? "));
		while (fd <= 0 || fd > 3);
	}
	if (fd == 3) {
		testall = 1;
		printf("Checking entire disk\n");
		return;
	}
	testall = 0;
	if (fd == 2)
		fd = -1;
	else {
		cp = prompt("File with bad spot information? ");
		if (!*cp || *cp == '\n')
			goto again;
		fd = open(cp, 0);
		if (fd < 0) {
			printf("Cannot open %s\n", cp);
			goto again;
		}
		count = 0;
		lastline = buffer;
		cp = buffer;
	}
	j = 0;
	sum = 0;
	dkbad_mfg[0].bt_flag = DK_FLAG_MDBSF;
	for (;;) {
		if (fd < 0) {
			printf("Enter bad spot %d (cyl head byte len): ",
								nspots + 1);
			cp = prompt("");
		} else {
			if (cp == lastline) {
				count = count - (lastline - buffer);
				bcopy(lastline, buffer, count);
				lastline = &buffer[count];
				/* Note kludge for ts tape */
				nread = read(fd, lastline,
					((sizeof buffer) - count) & ~511);
				count += nread;
				for (cp = buffer; cp < buffer + count; cp++)
					if (*cp == '\n') {
						*cp = 0;
						lastline = cp + 1;
					}
				cp = buffer;
			}
			if (cp == lastline) {
				cp = buffer;
				buffer[0] = 0;
			}
		}
		i = scani(cp, 4, &cyl, &head, &byte, &len);
		if (i < 0)
			break;
		if (i == 4) {
			err = 0;
			bt_mfg[j].bt_mfgcyl = cyl;
			if (cyl < 0 || cyl >= st.ncyl) {
				printf("Bad cylinder number %d\n", cyl);
				cyl = 0;
				err = 1;
			}
			bt_mfg[j].bt_mfghead = head;
			if (head < 0 || head >= st.nspc/st.nsect) {
				printf("Bad head number %d\n", head);
				head = 0;
				err = 1;
			}
			bt_mfg[j].bt_mfgbyte = byte;
			sum += bt_mfg[j++].bt_sect;
			if (byte < 0 || byte >= TRKSIZE) {
				printf("Bad byte number %d\n", byte);
				err = 1;
			}
			setspot((long)cyl*st.nspc + head * st.nsect, 0);
			nspots++;
			if( err == 0) {
				if ( byte >=  TRKSIZE - RUNT)
					goto nextentry;
				bn = byte/SECTBYTES;
				sec = cyl * st.nspc + head * st.nsect
					+ sector[head][bn];
				for(k=0; k<nbads; k++)
					if(bads[k] == sec)
						break;
				if(k == nbads)
					bads[nbads++] = sec;
				if(len > MAXBITLEN) {
					printf("Bad length number %d\n",
							len);
					goto nextentry;
				}
				resid = len/BYTESIZE +
					(len % BYTESIZE ? 1 : 0);
				if( resid < 2)
					goto nextentry;
				byte += resid - 1;
				if ( byte >=  TRKSIZE - RUNT)
					goto nextentry;
				if( byte/SECTBYTES == bn + 1) {
					bn++;
					sec = cyl * st.nspc + head * st.nsect
						+ sector[head][bn];
					for(k=0; k<nbads; k++)
						if(bads[k] == sec)
							break;
					if(k == nbads)
						bads[nbads++] = sec;
				}
			}
		} else {
			printf("Bad syntax: %s. Expected four numbers, cyl  head  byte  length, seperated by tabs or spaces.\n", cp);
		}
nextentry:
		for (; *cp++;);
	}
	dkbad_mfg[0].bt_csn = sum;
	if(j <= DK_NBAD_0)
		dkbad_mfg[0].bt_lastb = 0;
	else
		dkbad_mfg[0].bt_lastb = ((j - DK_NBAD_0)/DK_NBAD_N) + 1;
	while (j < MAXBADDESC)
		bt_mfg[j++].bt_sect = DK_END;
	if (fd >= 0) {
		close(fd);
		if (nread > 0)
			printf("Entire file not read\n");
	}
	printf("%d bad spots entered\n", nspots);
}

setspot(s, far)
register long s;
register int far;
{
	register i;

	for (i = (s - far*st.nspc) / st.nsect;
	     i <= (s + far*st.nspc) / st.nsect;
	     i += st.nspc/st.nsect)
		if (i >= 0)
			spots[i / 8] |= 1 << (i % 8);
}

static struct pattern {
	unsigned	pa_value;
	char		*pa_name;
} pat[] = {
	{ 0xf00ff00f, 	"Controller worst case (1 pass)" },
	{ 0xec6dec6d,	"Media worst case (1 pass)" },
	{ 0xa5a5a5a5,	"Alternate 1's and 0's (1 pass)" },
	{ -1,		"Severe burnin (3 - 48 passes, needs bad spots)" },
	{ 0, 0 },
};

getpattern()
{
	register struct pattern *p;
	int npatterns;
	char *cp;

	printf("Available test patterns are:\n");
	for (p = pat; p->pa_value; p++)
		printf("        %d - (%x) %s\n", (p - pat) + 1,
			p->pa_value, p->pa_name);
	npatterns = p - pat;
	pattern = atoi(prompt("Pattern (one of the above, other to restart)? "))
			- 1;
	severe = pat[pattern].pa_value == (unsigned)-1;
	return (pattern < 0 || pattern >= npatterns);
}

/*
 * Initialize the buffer with the requested pattern.
 */
bufinit(bp, size)
	char *bp;
{
	register long *lp;
	register long *end;
	register ptrn;

	size = (size + sizeof(long) -1) / sizeof(long);
	lp = (long *) bp;
	end = lp + size;
	if (severe) {
		ptrn = ppat[npat];
		printf("Write pattern 0x%x\n", ptrn);
	} else
		ptrn = pat[pattern].pa_value;
	while (lp < end)
		*lp++ = ptrn;
}

checkbuf(bp, size)
	char *bp;
	int *size;
{
	register nlongs;
	register long *lp;
	register long p;
	register nerrs = 0;

	nlongs = *size / sizeof (long);
	lp = (long *)bp;
	if (severe)
		p = ppat[npat];
	else
		p = pat[pattern].pa_value;
	while (nlongs--) {
		if (*lp != p) {
			printf("Data error 0x%x != 0x%x\n", *lp, p);
			if (nerrs == 0)
				*size = (char *)lp - bp;
			nerrs++;
		}
		lp++;
	}
	return(nerrs);
}

/*
 * Code to write data patterns onto the diagnostic cylinders
 */

/*
 * Diagnostic block
 */
struct db {
	char	db_magic;
	int	db_blkno;
	char	db_chksum;
	char	db_rest[512 - 6];
};

#define	CHECKSUM	0
#define	MAGIC_DIAG	0x2B


/*
 * Data Patterns
 */
#define PAT_DEF_1	3	/* 6db */

/*
 * chksum - compute the checksum for a diagnostic block
 */
short
chksum(buf, nbytes)
char *buf;
int nbytes;
{
	register short sum = 0;
	register short *p = (short *) buf;
	register n = (int) nbytes;
	short old_cs = ((struct db *)p)->db_chksum;

	((struct db *)p)->db_chksum = CHECKSUM;
	for (; n > 0; n -= sizeof(*p))
		sum += *p++;
	((struct db *)buf)->db_chksum = old_cs;
	return sum;
}


/*
 * mk_diag - make a diag block in the buffer db
 */
mk_diag(db, lba)
register struct db *db;
int lba;
{
	mk_pat(PAT_DEF_1, (char*)db, SECTSIZ);
	db->db_magic = MAGIC_DIAG;
	db->db_blkno = lba;
	db->db_chksum = chksum((char*)db, SECTSIZ);
}


char pat_zero[]	= {1, 0x00};
char pat_ones[]	= {1, 0xFF};
char pat_random[]	= {0};	/* zero length ==> random */
char pat_default1[]	= {3, 0x6D, 0xB6, 0xDB};
char pat_default2[]	= {3, 0xB6, 0xDB, 0x6D};
char pat_default3[]	= {3, 0xDB, 0x6D, 0xB6};
char pat_chex1[]	= {1, 0x55};
char pat_chex2[]	= {1, 0xAA};

char *pats[]	= {		/* pattern pointers */
	pat_zero, pat_ones, pat_random,
	pat_default1, pat_default2, pat_default3,
	pat_chex1, pat_chex2
};



/*
 * fill_pat - repeat a pattern
 */
fill_pat(src, nsrc, dst, ndst)
char *src;
int nsrc;
char *dst;
int ndst;
{
	register char *s = src;
	register char *d = dst;
	register char *sx = s + nsrc;
	register char *dx = d + ndst;

	while (d < dx) {
		*d++ = *s++;
		if (s >= sx)
			s = src;
	}
}

/*
 * mk_pat - make a pattern
 */
mk_pat(nbr, dst, ndst)
int nbr;
char *dst;
int ndst;
{
	register char *p = pats[nbr];
	register char *d = dst;

	fill_pat(p + 1, *p, d, ndst);
}

/*
 * scani - scanf like function
 * returns -1 for null pointer or string
 * else MIN(nargs, WS/DIGITS)
 */
int
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

/*
 * write_min_vtoc
 *	write a minimal VTOC on disk, following the partition information
 */
write_min_vtoc()
{
	struct vtoc *v;
	int where;

	callocrnd(SECTSIZ);
	v = (struct vtoc *)calloc(V_SIZE);

	v->v_sanity = VTOC_SANE;
	v->v_version = V_VERSION_1;
	v->v_size = sizeof(struct vtoc);
	v->v_nparts = partno + 1;
	v->v_secsize = DEV_BSIZE;
	v->v_ntracks = st.ntrak;
	v->v_nsectors = st.nsect;
	v->v_ncylinders = st.ncyl;
	v->v_rpm = DFLT_RPM;
	v->v_nseccyl = v->v_nsectors * v->v_ntracks;
	v->v_capacity = v->v_nseccyl * v->v_ncylinders;
	strcpy(v->v_disktype, "eagle");
	v->v_part[partno] = xp_proto[0];
	v->v_cksum = 0;
	v->v_cksum = vtoc_get_cksum(v);

	where = V_VTOCSEC;
	printf("Writing VTOC to sector %d.\n", where );

	lseek(fd, where<< DEV_BSHIFT, L_SET);

	if (write(fd, v, V_SIZE) != V_SIZE) {
		printf("Ruptured disk - Cannot write VTOC\n");
		return;
	}
	printf("VTOC written.\n");
}
