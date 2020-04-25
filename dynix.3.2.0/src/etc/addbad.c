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

#ifndef lint
static char rcsid[] = "$Header: addbad.c 2.3 91/02/07 $";
#endif

/*
 * addbad
 *
 * This program prints and/or changes a bad block record for a pack,
 * in the format used by the DEC standard 144 as modified by Sequent.
 * To initialize a bad block record, use the formatter or bad144(8).
 *
 * When bad blocks are added to the list the
 * data in replacement sectors are moved to avoid corrupting files.
 * Thus, this program can be used on a disk with usable file systems
 * on it.  The disk should not be active.  A reboot is necessary after
 * changing the bad block records.
 */
#include <sys/types.h>
#include <mbad/dkbad.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <mbad/xpioctl.h>

#include <stdio.h>
#include <disktab.h>

#define SECSIZE 512

int	count;
struct	dkbad dkbad[DK_NBADMAX];
char	rbuf[DK_MAXBAD][SECSIZE];
int	snlist[DK_MAXBAD], n;
char name[BUFSIZ];

main(argc, argv)
	int argc;
	char *argv[];
{
	register union bt_bad *bt;
	register struct disktab *dp;
	int size, i, j, f, bad, errs;
	int block, blks;

	argc--, argv++;
	if (argc < 2) {
		fprintf(stderr,
		  "usage: addbad type disk [ bn ... ],\n");
		fprintf(stderr,
		  "or addbad type disk [ -m ] for mfg defects list\n");
		fprintf(stderr, "e.g.: addbad eagle xp0\n");
		exit(1);
	}
	dp = getdiskbyname(argv[0]);
	if (dp == NULL) {
		fprintf(stderr, "%s: unknown disk type\n", argv[0]);
		exit(1);
	}
	if (strcmp(dp->d_fmt, "dec144") != 0) {
		fprintf(stderr, "%s: not dec144 disk format.\n", dp->d_name);
		fprintf(stderr, "addbad supports only dec144 disk format.\n");
		exit(1);
	}
	sprintf(name, "/dev/r%sc", argv[1]);
	argc -= 2;
	argv += 2;
	size = dp->d_nsectors * dp->d_ntracks * dp->d_ncylinders; 
	if (argc == 0) {
		f = open(name, O_RDONLY);
		if (f < 0)
			Perror(name);
		bt = dkbad[0].bt_bad;
		for(i=0; i<DK_MAXBAD; i++)
			bt[i].bt_cyl = bt[i].bt_trksec = DK_END;
		printf("bad block information at sector %d in %s:\n",
		    size - dp->d_nsectors, name);
		block = 0;
		blks = 1;
		readbad(f, dp, size, block, blks);
		if((blks = dkbad[0].bt_lastb) > 0) {
			block++;
			readbad(f, dp, size, block, blks);
		}
		printf("cartidge serial number: %d(10)\n", dkbad[0].bt_csn);
		switch (dkbad[0].bt_flag) {

		case -1:
			printf("alignment cartridge\n");
			break;

		case 0:
			break;

		default:
			printf("bt_flag=%x(16)?\n", dkbad[0].bt_flag);
			break;
		}
		for (i = 0; i < DK_MAXBAD; i++) {
			bad = (bt[i].bt_cyl<<16) + bt[i].bt_trksec;
			if (bad < 0)
				break;
			printf("sn=%d, cn=%d, tn=%d, sn=%d\n",
			    (bt[i].bt_cyl*dp->d_ntracks + (bt[i].bt_trksec>>8))
				* dp->d_nsectors + (bt[i].bt_trksec&0xff),
			    		bt[i].bt_cyl, bt[i].bt_trksec>>8,
						bt[i].bt_trksec&0xff);
		}
		exit(0);
	}
	if(argc == 1 && strcmp(argv[0], "-m") == 0) {
		mfg_defects(size, dp);
		exit(0);
	}
	f = open(name, O_RDWR);
	if (f < 0)
		Perror(name);
	bt = dkbad[0].bt_bad;
	for(i=0; i<DK_MAXBAD; i++)
		bt[i].bt_cyl = bt[i].bt_trksec = DK_END;
	block = 0;
	blks = 1;
	readbad(f, dp, size, block, blks);
	if((blks = dkbad[0].bt_lastb) > 0) {
		block++;
		readbad(f, dp, size, block, blks);
	}

	for (count = 0; count < DK_MAXBAD; count++)
		if (bt[count].bt_cyl == DK_END)
			break;
	if ( argc + count > DK_MAXBAD ) {
		fprintf(stderr, "addbad: too many bad sectors specified\n");
		fprintf(stderr, "limited to %d by information format\n",
								DK_MAXBAD);
		fprintf(stderr, "current list has %d entries\n", count);
		exit(1);
	}
	if(argc + count <= DK_NBAD_0)
		dkbad[0].bt_lastb = 0;
	else
		dkbad[0].bt_lastb = ((count + argc - DK_NBAD_0)/DK_NBAD_N) + 1;
	for (i = 0; i < count; i++) {
		if (lseek(f, dp->d_secsize*(size - dp->d_nsectors - i - 1), L_SET) < 0)
			Perror("lseek");
		if (read(f, rbuf[i], sizeof rbuf[i]) != sizeof rbuf[i]) {
			fprintf(stderr, "addbad: %s: can't read replacement block %d\n", name, i);
			exit(1);
		}
	}
	errs = 0;
	n = 0;
	while (argc > 0) {
		int sn = atoi(*argv++);
		int temp;
		u_short cyl;
		u_short trksec;

		argc--;
		if (sn <= 0 || sn >= size) {
			fprintf(stderr, "%d: out of range 1 to %d for %s\n",
			    sn, size - 1, dp->d_name);
			errs++;
			continue;
		}
		cyl = sn / (dp->d_nsectors*dp->d_ntracks);
		temp = sn % (dp->d_nsectors*dp->d_ntracks);
		trksec = ((temp/dp->d_nsectors) << 8) + (temp%dp->d_nsectors);

		for (i=0; bt[i].bt_cyl < cyl ||
			  (bt[i].bt_cyl == cyl &&
			   bt[i].bt_trksec < trksec); i++);
		if (bt[i].bt_cyl == cyl &&
		    bt[i].bt_trksec == trksec) {
			fprintf(stderr, "addbad: sector %d is already in list\n", sn);
			errs++;
		}

		count++;
		for (j = count-1; j > i; j--) {
			bt[j].bt_cyl = bt[j-1].bt_cyl;
			bt[j].bt_trksec = bt[j-1].bt_trksec;
			bcopy(rbuf[j-1], rbuf[j], SECSIZE);
		}
		bt[j].bt_cyl = cyl;
		bt[j].bt_trksec = trksec;
		if (lseek(f, dp->d_secsize*sn, L_SET) < 0) {
			Perror("lseek");
			errs++;
		}
		if (read(f, rbuf[i], SECSIZE) != SECSIZE)
			fprintf(stderr, "addbad: can't read bad block %d\n", sn);

		snlist[n++] = sn;
	}
	if (errs)
		exit(1);

	for (i = 0; i < n; i++)
		format(f, dp, snlist[i]);

	writebad(f, dp, size);

	for (i = 0; i < count; i++) {
		if (lseek(f, dp->d_secsize*(size - dp->d_nsectors - i - 1), L_SET) < 0)
			Perror("lseek");
		if (write(f, (caddr_t)rbuf[i], sizeof rbuf[i]) != sizeof rbuf[i]) {
			fprintf(stderr, "addbad: %s: can't write replacement block %d\n", name, i);
		}
	}
	exit(0);
}

/*
 * read bad block list
 */

readbad(f, dp, size, block, blks)
	int f, size, block;
	register int blks;
	struct disktab *dp;
{
	register int copy;

	do {
		for(copy=0; copy<DK_NBADCOPY; copy++) {
			if(lseek(f, dp->d_secsize * (size - dp->d_nsectors + 
					DK_LOC(block, copy)), L_SET) < 0)
				Perror("lseek");
			if(read(f, (char *)&dkbad[block], sizeof(struct dkbad))
					== sizeof(struct dkbad))
				break;
		}
		if(copy == DK_NBADCOPY) {
			fprintf(stderr, "addbad: %s: can't read bad block info\n", name);
				exit(1);
		}
		block++;
	} while(--blks);
}

/*
 * write the bad block list
 */

writebad(f, dp, size)
	int f, size;
	struct disktab *dp;
{
	int block, copy;

	block = 0;
	do {
		for(copy=0; copy<DK_NBADCOPY; copy++) {
			if(lseek(f, dp->d_secsize * (size - dp->d_nsectors + 
					DK_LOC(block, copy)), L_SET) < 0) {
				fprintf(stderr, "addbad: %s: can't write bad block %d copy %d, seek failed\n", name, block, copy);
				continue;
			}
			if(write(f, (char *)&dkbad[block], sizeof(struct dkbad))
					!= sizeof(struct dkbad))
				fprintf(stderr, "addbad: %s: can't write bad block %d copy %d\n", name, block, copy);
		}
	} while(++block < DK_NBADMAX);
}

/*
 *	Routine to print mfg's defects list
 */

 mfg_defects(size, dp)
	int size;
	register struct disktab *dp;
 {
	register int i;
	struct dkbad_mfg dkbad_mfg[DK_NBADMAX];
	register union bt_mfgbad *bt_mfg;
	long sum;
	int fd, val_dfct_lst = 1;

	fd = open(name, O_RDONLY);
	if (fd < 0)
		Perror(name);
	bt_mfg = dkbad_mfg[0].bt_mfgbad;
	for(i=0; i<DK_MAXBAD; i++)
		bt_mfg[i].bt_sect = DK_END;
	if(lseek(fd, dp->d_secsize * DK_MDBSF(size - dp->d_nsectors),
			 			L_SET) < 0)
		Perror("lseek");
	if(read(fd, (char *)dkbad_mfg, sizeof(struct dkbad_mfg) * DK_NBADMAX)
				!= sizeof(struct dkbad_mfg) * DK_NBADMAX) {
		fprintf(stderr, "addbad: %s: can't read mfg defects list\n", name);
		exit(1);
	}
	if(dkbad_mfg[0].bt_flag != DK_FLAG_MDBSF) {
		val_dfct_lst = 0;
		fprintf(stderr, "addbad: %s: mfg defects list flag field has incorrect value\n", name);
		fprintf(stderr, "\t\t\texpected %d received %d\n", DK_FLAG_MDBSF, dkbad_mfg[0].bt_flag);
	}

	sum = 0;
	for (i = 0; i < DK_MAXBAD; i++) {
		if (bt_mfg[i].bt_sect == DK_END)
			break;
		sum += bt_mfg[i].bt_sect;
	}
	if(sum != dkbad_mfg[0].bt_csn) {
		val_dfct_lst = 0;
		fprintf(stderr, "addbad: %s: mfg defects list failed check sum test\n", name);
	}
	if( !val_dfct_lst ) {
		fprintf(stderr, "addbad: %s: mfg defects list considered invalid\n", name);
		fprintf(stderr, "addbad: disks formatted prior to DYNIX 2.0 do not have valid mfg defects list\n");
		exit(4);
	}

	printf("mfg defects list at sector %d in %s:\n",
		    DK_MDBSF(size - dp->d_nsectors), name);
	for (i = 0; i < DK_MAXBAD; i++) {
		if (bt_mfg[i].bt_sect == DK_END)
			break;
		printf("cn=%d, tn=%d, byte=%d\n", bt_mfg[i].bt_mfgcyl,
			bt_mfg[i].bt_mfghead, bt_mfg[i].bt_mfgbyte);
	}
}

/*
 *	Routine to zap a header on xp
 */

zapxp(fd, dp, blk)
	int fd;
	struct disktab *dp;
	daddr_t blk;
{
	struct xp_ioctl s;
	daddr_t t;

	s.x_cyl = blk / (dp->d_nsectors*dp->d_ntracks);
	t = blk % (dp->d_nsectors*dp->d_ntracks);
	s.x_head = t / dp->d_nsectors;
	s.x_sector = t % dp->d_nsectors;
	/* Allow for adaptive format */
	s.x_sector = (s.x_sector + s.x_head) % dp->d_nsectors;

	if (ioctl(fd, XPIOCZAPHEADER, &s) < 0)
		fprintf(stderr, "addbad: can't mark sector %d\n", blk);
}


struct	formats {
	char	*f_name;		/* disk name */
	int	(*f_routine)();		/* routine for special handling */
} formats[] = {
	{ "eagle", zapxp },
	{ 0, 0 }
};

format(fd, dp, blk)
	int fd;
	struct disktab *dp;
	daddr_t blk;
{
	register struct formats *fp;

	for (fp = formats; fp->f_name; fp++)
		if (strcmp(dp->d_name, fp->f_name) == 0)
			break;
	if (fp->f_name == 0) {
		fprintf(stderr, "addbad: don't know how to format %s disks\n",
			dp->d_name);
		exit(2);
	}
	(*fp->f_routine)(fd, dp, blk);
}

Perror(op)
	char *op;
{
	fprintf(stderr, "addbad: "); perror(op);
	exit(4);
}
