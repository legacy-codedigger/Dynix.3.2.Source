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
static char rcsid[] = "$Header: bad144.c 2.1 86/05/18 $";
#endif

/*
 * bad144
 *
 * This program prints and/or initializes a bad block record for a pack,
 * in the format used by the DEC standard 144 as modified by Sequent.
 *
 * It is preferable to write the bad information with a standard formatter,
 * but this program will do in a pinch, e.g. if the bad information is
 * accidentally wiped out this is a much faster way of restoring it than
 * reformatting. 
 */

#include <sys/types.h>
#include <mbad/dkbad.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <mbad/xpioctl.h>

#include <stdio.h>
#include <disktab.h>

int	fflag;
struct  dkbad dkbad[DK_NBADMAX];

main(argc, argv)
	int argc;
	char *argv[];
{
	register union bt_bad *bt;
	register struct disktab *dp;
	char name[BUFSIZ];
	int size, i, f, bad, errs;
	int block, blks;

	argc--, argv++;
	if (argc > 0 && strcmp(*argv, "-f") == 0) {
		argc--, argv++;
		fflag++;
	}
	if (argc < 2) {
		fprintf(stderr,
		  "usage: bad144 [ -f ] type disk [ snum [ bn ... ] ]\n");
		fprintf(stderr, "e.g.: bad144 eagle xp0\n");
		exit(1);
	}
	dp = getdiskbyname(argv[0]);
	if (dp == NULL) {
		fprintf(stderr, "%s: unknown disk type\n", argv[0]);
		exit(1);
	}
	if (strcmp(dp->d_fmt, "dec144") != 0) {
		fprintf(stderr, "%s: not dec144 disk format.\n", dp->d_name);
		exit(1);
	}
	sprintf(name, "/dev/r%sc", argv[1]);
	argc -= 2;
	argv += 2;
	size = dp->d_nsectors * dp->d_ntracks * dp->d_ncylinders; 
	if (argc == 0) {
		bt = dkbad[0].bt_bad;
		for(i=0; i<DK_MAXBAD; i++)
			bt[i].bt_cyl = bt[i].bt_trksec = DK_END;
		f = open(name, O_RDONLY);
		if (f < 0)
			Perror(name);
		printf("bad block information at sector %d in %s:\n",
		    			size - dp->d_nsectors, name);
		block = 0;
		blks = 1;
		readbad(f, dp, size, name, block, blks);
		if((blks = dkbad[0].bt_lastb) > 0) {
			block++;
			readbad(f, dp, size, name, block, blks);
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
			    bt[i].bt_cyl, bt[i].bt_trksec>>8, bt[i].bt_trksec&0xff);
		}
		exit(0);
	}
	f = open(name, 1 + fflag);
	if (f < 0)
		Perror(name);
	dkbad[0].bt_csn = atoi(*argv++);
	bt = dkbad[0].bt_bad;
	for(i=0; i<DK_MAXBAD; i++)
		bt[i].bt_cyl = bt[i].bt_trksec = DK_END;
	argc--;
	if (argc > DK_MAXBAD) {
		printf("bad144: too many bad sectors specified\n");
		printf("limited to %d by information format\n", DK_MAXBAD);
		exit(1);
	}
	errs = 0;
	i = 0;
	while (argc > 0) {
		int sn = atoi(*argv++);

		argc--;
		if (sn <= 0 || sn >= size) {
			printf("%d: out of range 1 to %d for %s\n",
			    sn, size - 1, dp->d_name);
			errs++;
		}
		bt[i].bt_cyl = sn / (dp->d_nsectors*dp->d_ntracks);
		sn %= (dp->d_nsectors*dp->d_ntracks);
		bt[i].bt_trksec =
		    ((sn/dp->d_nsectors) << 8) + (sn%dp->d_nsectors);
		i++;
	}
	if (errs)
		exit(1);

	if(i <= DK_NBAD_0)
		dkbad[0].bt_lastb = 0;
	else
		dkbad[0].bt_lastb = ((i - DK_NBAD_0)/DK_NBAD_N) + 1;

	writebad(f, dp, size, name);

	if (fflag)
		for (i = 0; i < DK_MAXBAD; i++) {
			daddr_t bn;

			bad = (bt[i].bt_cyl<<16) + bt[i].bt_trksec;
			if (bad < 0)
				break;
			bn = (bt[i].bt_cyl * dp->d_ntracks +
			    (bt[i].bt_trksec >> 8)) *
			    dp->d_nsectors + (bt[i].bt_trksec & 0xff);
			format(f, dp, bn);
		}
	exit(0);
}

/*
 * read bad block list
 */

readbad(f, dp, size, name, block, blks)
	int f, size, block;
	register int blks;
	struct disktab *dp;
	char *name;
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
			fprintf(stderr, "bad144: %s: can't read bad block info\n", name);
				exit(1);
		}
		block++;
	} while(--blks);
}

/*
 * write the bad block list
 */

writebad(f, dp, size, name)
	int f, size;
	struct disktab *dp;
	char *name;
{
	int block, copy;

	block = 0;
	do {
		for(copy=0; copy<DK_NBADCOPY; copy++) {
			if(lseek(f, dp->d_secsize * (size - dp->d_nsectors + 
					DK_LOC(block, copy)), L_SET) < 0) {
				fprintf(stderr, "bad144: %s: can't write bad block %d copy %d, seek failed\n", name, block, copy);
				continue;
			}
			if(write(f, (char *)&dkbad[block], sizeof(struct dkbad))
					!= sizeof(struct dkbad))
				fprintf(stderr, "bad144: %s: can't write bad block %d copy %d\n", name, block, copy);
		}
	} while(++block < DK_NBADMAX);
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
		fprintf(stderr, "bad144: can't mark sector %d\n", blk);
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
		fprintf(stderr, "bad144: don't know how to format %s disks\n",
			dp->d_name);
		exit(2);
	}
	(*fp->f_routine)(fd, dp, blk);
}

Perror(op)
	char *op;
{

	fprintf(stderr, "bad144: "); perror(op);
	exit(4);
}
