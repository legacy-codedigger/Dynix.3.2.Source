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

#ifndef	lint
static	char	rcsid[] = "$Header: kpop.c 1.1 86/10/07 $";
#endif

/* $Log:	kpop.c,v $
 */

/*
 * NAME
 *	kpop.c -- operate on profiling log files
 *
 * SYNOPSIS
 *	kpop -o operation src_file1 [src_file2] [dest_file]
 *
 *	The last file on the command line is always the destination file.
 *	Cases where the destination is one of the sources are handled
 *	properly.
 *
 *	dyadic operations: (two files required)
 *
 *	+	add the two files and place result in destination
 *	-	add the two files and place result in destination
 *
 *	monadic operatons: (one file required)
 *
 * DESCRIPTION
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include "kp_util.h"

#define MAXFILES	3
#define	SRC1		0
#define	SRC2		1
#define DEST		2

struct v_ops {
	int op;
	int nf_req;
} valid_ops[] = {
	{ '+', 2 },
	{ '-', 2 },
	{ '\0', 0 }
};

extern char *malloc();
extern char *optarg;
extern int optind;
extern struct timeval todop();

main(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	register u_long *s1, *s2, *d;
	struct kp_hdr fhdr[MAXFILES];
	char *filename[MAXFILES];
	u_long *kp_data[MAXFILES];	/* data pointers */
	int fd[MAXFILES];
	int nfiles, rfiles;
	int operation = 0;
	int recsize;

	if (getargs(argc, argv, &operation, &nfiles, &rfiles, filename)) {
		fprintf(stderr,
		   "Usage: %s -o operation src_file1 [src_file2] [dest_file]\n",
		   argv[0]);

		exit(1);
	}
	/*
	 * Open each of the input files and, read and check their headers
	 */
	for (i = 0; i < rfiles; i++) {
		if ((fd[i] = open(filename[i], O_RDONLY, 0)) < 0) {
			printf("%s: Can't open `%s': ", argv[0], filename[i]);
			perror("");
			exit(1);
		}
		if (read(fd[i], (char *)&fhdr[i], sizeof(struct kp_hdr)) !=
				sizeof(struct kp_hdr)) {
			printf("%s: Read error `%s': ", argv[0], filename[i]);
			perror("");
			exit(1);
		}
	}
	if (rfiles == 2) {	/* dyadic operation */
		if (fhdr[SRC1].bins != fhdr[SRC2].bins) {
			fprintf(stderr, "Number of bins mismatch %d and %d\n",
				fhdr[SRC1].bins, fhdr[SRC2].bins);
			exit(1);
		}
		if (fhdr[SRC1].engines != fhdr[SRC2].engines) {
			fprintf(stderr,
				"Number of engines mismatch %d and %d\n",
				fhdr[SRC1].engines, fhdr[SRC2].engines);
			exit(1);
		}
		if ((fhdr[SRC1].b_text != fhdr[SRC2].b_text) ||
		     (fhdr[SRC1].e_text != fhdr[SRC2].e_text)) {
			fprintf(stderr, "Kernel text section mismatch\n");
			exit(1);
		}
		if (fhdr[SRC1].tod_flag != fhdr[SRC2].tod_flag) {
			fprintf(stderr, "File flags don't agree\n");
			exit(1);
		}
		if (operation == '-' &&
		     fhdr[SRC1].tod.tv_sec <= fhdr[SRC2].tod.tv_sec) {
/* STILL NEEDS FIXING */
			fprintf(stderr, "Source 1 created before Source 2\n");
			fprintf(stderr, "Subtraction not valid\n");
			exit(1);
		}
	}
	if (fhdr[SRC1].bins <= 0) {
		fprintf(stderr, "Invalid number of bins: %d\n",
			fhdr[SRC1].bins);
		exit(1);
	}
	if (fhdr[SRC1].engines <= 0) {
		fprintf(stderr, "Invalid number of engines: %d\n",
			fhdr[SRC1].engines);
		exit(1);
	}
	/*
	 * Read data in each file and close it.
	 */
	recsize = fhdr[SRC1].bins * sizeof(u_long);
	for (i = 0; i < rfiles; i++) {
		kp_data[i] = (u_long *) malloc((unsigned)recsize);
		if (kp_data[i] == NULL) {
			fprintf(stderr, "Malloc failed for file #%d\n", i);
			exit(1);
		}
		if (read(fd[i], (char *)kp_data[i], recsize) != recsize) {
			printf("%s: Read error `%s': ", argv[0], filename[i]);
			perror("");
			exit(1);
		}
		(void)close(fd[i]);
	}
	/*
	 * Open the destination file
	 */
	if ((fd[DEST] = open(filename[DEST], O_WRONLY|O_CREAT, 0640)) < 0) {
		printf("%s: Can't open `%s' \n", argv[0], filename[DEST]);
		perror("");
		exit(1);
	}
	/*
	 * Fill out the destination header
	 */
	fhdr[DEST].tod_flag = ELAPSED;	/* really only true for dyadic ops */
	fhdr[DEST].tod = todop(operation, fhdr[SRC1].tod, fhdr[SRC2].tod);
	fhdr[DEST].bins = fhdr[SRC1].bins;
	fhdr[DEST].binshift = fhdr[SRC1].binshift;
	fhdr[DEST].engines = fhdr[SRC1].engines;
	fhdr[DEST].b_text = fhdr[SRC1].b_text;
	fhdr[DEST].e_text = fhdr[SRC1].e_text;

	s1 = kp_data[SRC1];
	s2 = kp_data[SRC2];
	d = kp_data[DEST] = (u_long *) malloc((unsigned)recsize);
	for (i = 0; i < fhdr[DEST].bins; i++) {
		switch (operation) {
		case '+':
			*d++ = *s1++ + *s2++;
			break;
		case '-':
			*d++ = *s1++ - *s2++;
			break;
		}
	}
	if (write(fd[DEST], (char *)&fhdr[DEST], sizeof(struct kp_hdr)) != sizeof(struct kp_hdr)) {
		perror("Header write error: ");
		exit(1);
	}
	if (write(fd[DEST], (char *)kp_data[DEST], recsize) != recsize) {
		perror("Write error: ");
		exit(1);
	}
	(void)close(fd[DEST]);
	exit(0);
}


/*
 * Process the command line arguments
 * Return 0 if ok, nonzero if not
 */

getargs(argc, argv, op, nf, rf, filename)
	int argc;
	char *argv[];
	register int *op;
	int *nf;
	int *rf;
	char *filename[];
{
	register struct v_ops *p;
	int i, c, err = 0, valid = 0;

	*op = 0;

	while ((c = getopt(argc, argv, "o:")) != EOF) {
		switch (c) {
		case 'o':
			if (strlen(optarg) != 1) {
				fprintf(stderr, "%s: funny operaton `%s'\n",
					argv[0], optarg);
				err++;
				break;
			}
			*op = optarg[0];
			p = valid_ops;
			while (p->op) {
				if (p->op == *op) {
					valid = 1;
					break;
				}
				p++;
			}
			if (!valid) {
				fprintf(stderr, "%s: invalid operaton `%c'\n",
					argv[0], *op);
				err++;
			}
			break;
		case '?':
			err++;
			break;
		}
	}
	if (err || *op == 0)
		return(1);

	if (optind + p->nf_req >= argc)
		return(1);

	*rf = p->nf_req;
	*nf = 0;
	for (i = 0; i < p->nf_req; i++) {
		filename[i] = argv[optind];
		(*nf)++;
		optind++;
	}
	/*
	 * destination file explicitly specified?
	 */
	if (optind < argc) {
		filename[DEST] = argv[optind];
		(*nf)++;
	} else {
		filename[DEST] = filename[p->nf_req - 1];
	}
	return(0);
}

/*
 * Perform operations on struct timeval's
 */
struct timeval
todop(op, s1, s2)
	int op;
	struct timeval s1, s2; 
{
	struct timeval result;

	switch (op) {
	case '+':
		result.tv_sec = s1.tv_sec + s2.tv_sec;
		result.tv_usec = s1.tv_usec + s2.tv_usec;
		if (result.tv_usec >= 1000000) {
			result.tv_sec++;
			result.tv_usec -= 1000000;
		}
		break;
	case '-':
		result.tv_sec = s1.tv_sec - s2.tv_sec;
		if (s1.tv_usec >= s2.tv_usec) {
			result.tv_usec = s1.tv_usec - s2.tv_usec;
		} else {
			result.tv_sec--;
			result.tv_usec = 1000000 + s1.tv_usec - s2.tv_usec;
		}
		break;
	}
	return(result);
}
