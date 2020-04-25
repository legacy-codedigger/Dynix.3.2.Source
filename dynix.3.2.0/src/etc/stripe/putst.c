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

#ifndef lint
static char rcsid[] = "$Header: putst.c 1.2 1991/07/03 23:26:31 $";

/* static char *rcsid = "$Header: putst.c 1.2 1991/07/03 23:26:31 $";  */
#endif  lint

/*
 * putst
 *	load stripe configurations into the kernel's
 *	stripe psuedo device(s).
 */

/* $Log: putst.c,v $
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/file.h>
#define	STRIPEMACS
#include <stripe/stripe.h>


#define	ARB		128
#define	MAX_MAJOR	255
#define	MAX_MINOR	255

#define	TRUE		1
#define	FALSE		0

/* Stripecap keywords */
#define ST_NUMPART	"np"
#define ST_MAJOR	"M"
#define ST_MINOR	"m"
#define ST_NUMDEV	"D"
#define ST_NUMBLOCK	"B"
#define ST_BLOCKSIZE	"S"


char		*ProgramName;
int		 aflag		= FALSE;	/* "-a" flag */
int		 arguments	= FALSE;	/* arguments on command line */
int		 nflag		= FALSE;	/* "-n" flag */
char		 stname[BUFSIZ];		/* stripe device name */
char		 stcapent[BUFSIZ];		/* stripe cap entry */
int		 vflag		= FALSE;	/* "-v" flag */


void		 put_entry();			/* internal procedure */
char		*sprintf();			/* printf(3S) */
int		 stgetent();


/* main --- main routine for putst */
main(argc, argv)
int argc;
char **argv;
{
	int		 i,j;			/* temporary */


	/* preliminary setup */

	ProgramName = argv[0];			/* save program name */


	/* process the command line */

	for (i = 1; i < argc; ++i) {
	    if (argv[i][0] == '-') {	/* got an option */
		while (*++argv[i]) {
		    switch (*argv[i]) {
		    case 'a':		/* do all devices */
			aflag = TRUE;
			break;
		    case 'n':		/* no change */
			nflag = TRUE;
			break;
		    case 'v':		/* verbose */
			vflag = TRUE;
			break;
		    default:		/* oops */
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			exit(EX_USAGE);
		    }
		}
	    } else {			/* got an argument */
		arguments = TRUE;
		break;
	    }
	}


	/* check for command line error */

	if (aflag == arguments) {		/* must have "-a" OR args */
		fprintf(stderr,
			"Usage: %s [-vna] [device ...]\n", ProgramName);
		exit(EX_USAGE);
	}


	/* process the stripes */

	if (aflag) {				/* do all stripes */
	    for (i = 0; i < MAX_STRIPE_DEVICES; ++i) {
		    /*
		       we don't want to play with the files that aren't in the
		       stripecap data base. that's what all of this avoids.
		    */
		(void)sprintf(stname, "ds%d", i);
		j = stgetent( stcapent, stname );
		if (j == -1) {
		    fprintf (stderr, "Cannot open %s\n", STRIPECAP);
		    exit(EX_OSFILE);
		} else
		    if (j == 0) {		  /* No stripecap entry found */
			if (vflag)
			    fprintf (stderr,"Warning: No stripe descriptor for %s in %s\n", stname, STRIPECAP);
		    } else {
			(void)sprintf(stname, "/dev/rds%d", i);
			put_entry(stname);
		    }
	    }
	} else {				/* do specified stripes */
		for (; i < argc; ++i)
			put_entry(argv[i]);
	}

	exit(EX_OK);				/* successful completion */
}


/* put_entry --- load an entry for a specific stripe device */
void
put_entry(sname)
char *sname;
{
	char		 bsbuf[ARB];		/* blocksize entry name */
	int		 i;			/* temporary */
	char		 mabuf[ARB];		/* name of major device */
	int		 maj;			/* major device # */
	char		 mibuf[ARB];		/* name of minor device */
	int		 min;			/* minor device # */
	char		 name[ARB];		/* name of stripe ("st?") */
	char		 nbbuf[ARB];		/* # of blocks entry name */
	char		 ndbuf[ARB];		/* # of devices entry name */
	int		 ndevs;			/* # of partitions in stripe */
	int		 nstripes;		/* # of stripe partitions */
	int		 ratio;			/* temporary */
	int		 rem;			/* temporary */
	int		 sfd;			/* stripe file descriptor */
	int		 sb;			/* temporary */
	struct stat	 sbuf;			/* stat buffer */
	stripe_t	 t;


	/* get information about the device */

	if (stat(sname, &sbuf) != 0) {
	    fprintf(stderr, "Cannot stat stripe device %s\n", sname);
	    return;
	}


	/* open the device */

	sfd = open(sname, 0);
	if (sfd < 0) {				/* open failed */
	    fprintf(stderr, "Cannot open stripe device %s\n", sname);
	    goto finish;
	}


	/* perform some simple checks on the device */

	if ((sbuf.st_mode & S_IFCHR) == 0) {
	    fprintf(stderr, "%s is not a character device\n", sname);
	    goto finish;
	}

	if (major(sbuf.st_rdev) != STRIPECHAR) {
	    fprintf(stderr, "%s is not a raw stripe device\n", sname);
	    goto finish;
	}


	/* get the stripe description */

	(void)sprintf(name, "ds%d", minor(sbuf.st_rdev));
	i = stgetent(stcapent, name);
	if (i < 0) {				/* cannot open file */
		fprintf(stderr, "Cannot open %s\n", STRIPECAP);
		exit(EX_OSFILE);
	}

	if (i == 0) {				/* entry not found */
	    fprintf(stderr, "Stripe description for %s not found\n", name);
	    goto finish;
	}


	/* get the number of partitions in the stripe */

	ndevs = stgetnum(ST_NUMPART);
	if (ndevs < 0) {
	    fprintf(stderr, "Missing %s field in %s description\n", ST_NUMPART, name);
	    goto finish;
	}


	/* Clear the stripe configuration data structure */
	bzero((char *)&t, sizeof(stripe_t));

	/* get and figure the device numbers for the partitions */

	for (i = 0; i < ndevs; i++) {
		(void)sprintf(mabuf, "%s%d", ST_MAJOR, i);
		maj = stgetnum(mabuf);
		if (maj < 0) {
			fprintf(stderr,
				"Missing %s field in %s description\n",
				mabuf, name);
			goto finish;
		}
		if (maj > MAX_MAJOR) {
			fprintf(stderr,
				"Invalid %s field in %s description\n",
				mabuf, name);
			goto finish;
		}

		(void)sprintf(mibuf, "%s%d", ST_MINOR, i);
		min = stgetnum(mibuf);
		if (min < 0) {
			fprintf(stderr,
				"Missing %s field in %s description\n",
				mibuf, name);
			goto finish;
		}
		if (min > MAX_MINOR) {
			fprintf(stderr,
				"Invalid %s field in %s description\n",
				mibuf, name);
			goto finish;
		}
		t.st_dev[i] = makedev(maj, min);
	}


	/* process the specific information for each partition in the stripe */

	t.st_total_size = 0;

	for (i = 0; i < ndevs; i++) {
		(void)sprintf(ndbuf, "%s%d", ST_NUMDEV, i);
		t.st_s[i].st_ndev = stgetnum(ndbuf);
		if (t.st_s[i].st_ndev == -1) {
			t.st_s[i].st_ndev = 0;	/* Leave zeroed for ioctl */
			if (i > 0)		/* no more sections */
				break;
			fprintf(stderr,
				"Missing %s field in %s description\n",
				ndbuf, name);
			goto finish;
		}
		if ((t.st_s[i].st_ndev < 1) || (t.st_s[i].st_ndev > ndevs)) {
			fprintf(stderr,
				"Invalid %s field in %s description\n",
				ndbuf, name);
			goto finish;
		}

		(void)sprintf(nbbuf, "%s%d", ST_NUMBLOCK, i);
		t.st_s[i].st_size = stgetnum(nbbuf);
		if (t.st_s[i].st_size < 0) {
			fprintf(stderr,
				"Missing %s field in %s description\n",
				nbbuf, name);
			goto finish;
		}

		(void)sprintf(bsbuf, "%s%d", ST_BLOCKSIZE, i);
		t.st_s[i].st_block = stgetnum(bsbuf);
		if (t.st_s[i].st_block < 0) {
			fprintf(stderr,
				"Missing %s field in %s description\n",
				bsbuf, name);
			goto finish;
		} else if (t.st_s[i].st_block % MIN_SBLK) {
			fprintf(stderr, "Stripe block size %d must be a ",
				t.st_s[i].st_block);
			fprintf(stderr, "multiple of %d in %s description.\n",
				MIN_SBLK, name);
			goto finish;
		}

		/* 
		 * Try to adjust this section if it would 
		 * exceed the system maximum for this device.
		 * Also make the section size a multiple of
		 * the stripe block size.
		 */
		rem = (MAX_DEV_BLKS - t.st_total_size) / t.st_s[i].st_ndev;
		if (t.st_s[i].st_size > rem) {
		    	sb = t.st_s[i].st_block;
		    	ratio = rem / sb;
			if (ratio < 2) {  /* not much sense interleaving */
				ratio = 1;
				sb = rem;
			}
		    	if (vflag) {
				rem = t.st_s[i].st_size - ratio * sb;
				printf("Warning: stripe section size must ");
				printf("be reduced so the total stripe size\n");
				printf("   does not exceed the system maximum");
				printf(" of %d blocks.\n", MAX_DEV_BLKS);

				printf("   The maximum usable space is ");
				printf("%d blocks per device ", (ratio * sb));
				printf("(%d blocks total).\n",
					ratio * sb * t.st_s[i].st_ndev);

				printf("   Losing %d blocks per device ", rem);
				printf("(%d blocks total) at the end of this\n",
					rem * t.st_s[i].st_ndev);
				printf("   stripe section.\n");
			}
		    	t.st_s[i].st_size = ratio * sb;

		} else if ((rem = t.st_s[i].st_size % t.st_s[i].st_block) != 0) {
			sb = t.st_s[i].st_block;
			ratio = t.st_s[i].st_size / sb;
			if (vflag) {
				printf("Warning: stripe section size not ");
				printf("divisible by stripe section block ");
				printf("size.\n");
				printf("         maximum usable space is ");
				printf("%d blocks per device ", (ratio * sb));
				printf("(%d blocks total)\n",
					ratio * sb * t.st_s[i].st_ndev);
				printf("Losing %d blocks per device ", rem);
				printf("(%d blocks total) ",
					rem * t.st_s[i].st_ndev);
				printf("at the end of the stripe section.\n");
			}
			t.st_s[i].st_size = ratio * sb;
		}

		if (i == 0) {
			if (t.st_s[i].st_ndev != ndevs) {
				fprintf(stderr, "Not every device is in ");
				fprintf(stderr, "first stripe section in ");
				fprintf(stderr, "%s description\n", name);
				goto finish;
			}
			t.st_s[0].st_start = 0;
		} else {
			t.st_s[i].st_start = t.st_total_size;
			if (t.st_s[i].st_ndev >= t.st_s[i - 1].st_ndev) {
				fprintf(stderr, "Succeeding stripe sections ");
				fprintf(stderr, "in %s description ", name);
				fprintf(stderr, "do not have fewer devices\n");
				goto finish;
			}
		}
		t.st_total_size += t.st_s[i].st_size * t.st_s[i].st_ndev;
	}

	t.st_s[i].st_size = 0;
	nstripes = i;


	/* print out stripe summary if verbose option was selected */

	if (vflag) {
		for (i = 0; i < ndevs; i++)
			printf ("Partition %d: device <%d, %d>\n",
				i, major(t.st_dev[i]), minor(t.st_dev[i]));
		for (i = 0; i < nstripes; i++) {
			printf("Section %d: size %d ndev %d ",
				i, t.st_s[i].st_size, t.st_s[i].st_ndev);
			printf("blksize %d start %d\n",
				t.st_s[i].st_block, t.st_s[i].st_start);
		}
		printf ("Total size: %d\n", t.st_total_size);
	}


	/* if we're really supposed to do it - do it */

	if (nflag == FALSE) {
		if (ioctl(sfd, STPUTTABLE, (char *) &t) < 0) {
			perror("ioctl");
			exit(EX_OSERR);
		}
	}


	/* finish things up */

finish:
	(void)close(sfd);
	return;
}
