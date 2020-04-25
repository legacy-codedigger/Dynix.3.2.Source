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
static char rcsid[] = "$Header: getst.c 1.2 1991/07/03 23:26:05 $";
#endif  lint

/*
 * getst
 *	Print information about stripe psuedo device(s)
 * 	from the kernel's stripe configuration table.
 */

/* $Log: getst.c,v $
 *
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <stripe/stripe.h>
#include <sys/stat.h>
#include <sys/conf.h>

char   *ProgramName;

/*VARARGS1*/
p (a, b, c, d, e, f)
char   *a;
{
    fprintf (stderr, a, b, c, d, e, f);
}

/*VARARGS1*/
fatal (a, b, c, d, e, f)
char   *a;
{
    fprintf (stderr, "%s: ", ProgramName);
    fprintf (stderr, a, b, c, d, e, f);
    exit (1);
}

usage ()
{
    p ("usage: %s /dev/rds* ...\n", ProgramName);
    exit (1);
}

stripe_t    t;
	
main (argc, argv)
char  **argv;
{
    register    i;
    int     unit,
            part;
    int     dev;
    struct stat sbuf;
    char    *stname;
    char    buf[BUFSIZ];

    ProgramName = argv[0];

    argc--, argv++;
    while (argc > 0 && **argv == '-') {
	(*argv)++;
	while (**argv) switch (*(*argv)++) {
	    default: 
		p ("Unknown option -%c\n", *--*argv);
		usage ();
	}
	argc--, argv++;
    }

    if (argc == 0) usage();

    while (*argv) {
    stname = *argv++;
    if ( strncmp(stname, "/dev/", 5)  == 0 )
	stname = &stname[5];
    if ( stname[0] == 'r' )
	stname++;
    (void) sprintf (buf, "/dev/r%s", stname);
    dev = open (buf, 0);		  /* open stripe device */
    if (dev < 0)
	fatal ("Can't open stripe device %s.\n", buf);

    if (stat (buf, &sbuf) == -1)
	fatal ("Can't stat stripe device %s.\n", buf);

    if ((sbuf.st_mode & S_IFCHR) == 0)
	fatal ("%s is not a raw device.\n", buf);

    if (major (sbuf.st_rdev) != STRIPECHAR)
	fatal ("%s is not a raw stripe device.\n", buf);

    (void) ioctl (dev, STGETTABLE, (char *) &t);
    (void) close (dev);

    printf("%s:\n",buf);
    for (i = 0; i < t.st_s[0].st_ndev; i++)
    {
	printf ("\tPartition %d:\t", i + 1);

	switch(major (t.st_dev[i])) {
	case ZDBLOCK:
	    unit = minor (t.st_dev[i]) / N_PARTITIONS;
	    part = minor (t.st_dev[i]) % N_PARTITIONS;
	    printf ("/dev/zd%d%c\n", unit, 'a' + part);
	    break;
	case SDBLOCK:
	    unit = minor (t.st_dev[i]) / N_PARTITIONS;
	    part = minor (t.st_dev[i]) % N_PARTITIONS;
	    printf ("/dev/sd%d%c\n", unit, 'a' + part);
	    break;
	default:
	    printf ("(device major %d minor %d)\n",
		    major (t.st_dev[i]),
		    minor (t.st_dev[i]));
	    break;
	}
    }
    for (i = 0; i < MAX_STRIPE_PARTITIONS; i++)
    {
	if (t.st_s[i].st_size == 0)
	    break;
	printf ("\tSection %d:",
		i + 1);
	printf ("\tStarts with logical block %d.\n",
		t.st_s[i].st_start);
	if (t.st_s[i].st_ndev == 1)
		printf ("\t\t\tUses space on partition 1.\n");
	else
		printf ("\t\t\tUses space on partitions 1 through %d.\n",
		t.st_s[i].st_ndev);
	printf ("\t\t\tUses %d blocks on each partition.\n",
		t.st_s[i].st_size);
	printf ("\t\t\tUses stripe blocking factor of %d blocks.\n",
		t.st_s[i].st_block);
    }
    printf ("\tTotal size: %d blocks.\n", t.st_total_size);

    }
    exit (0);
}
