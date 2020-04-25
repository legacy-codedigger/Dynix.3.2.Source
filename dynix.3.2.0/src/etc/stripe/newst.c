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
static char rcsid[] = "$Header: newst.c 1.3 1991/07/03 23:26:21 $";

/* static char *rcsid = "$Header: newst.c 1.3 1991/07/03 23:26:21 $"; */
#endif  lint

/*
 * newst
 *	``friendly'' interface to for constructing striped filesystems.
 *	Will attempt to determine a good configuration for the stripe,
 *	making its stripecap entry, load that configuration into the
 * 	kernel's stripe driver (ds), and use mkfs to generate a filesystem
 * 	for that stripe.
 */

/* $Log: newst.c,v $
 *
 *
 */

#include	<stdio.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/ioctl.h>
#include	<ufs/fs.h>
#define		STRIPEMACS
#include	<sys/buf.h>
#include	<stripe/stripe.h>
#include	<disktab.h>
#include	<ctype.h>
#include	<mntent.h>

#define DEFMINFREE	10		  /* default minfree argument */
#define MINNCG	3			  /* minimum # cyl groups */
#define MINCPG	1			  /* minimum # cyl per group */
#define MAXANS  32			  /* maximum size of answer buf */

#define	MAXIPG		2048	/* max number inodes/cyl group */
#define	MAXCPG		32	/* maximum fs_cpg */
/*
 * The following table is used to accumulate
 * information about each disk partition in
 * the stripe.  This info table will originally
 * be in the same order as the devices were
 * specified on the command line.  Later it 
 * will be sorted based on partition sizes in
 * descending order.
 */
struct info
{
    char   *special;			  /* character special device name */
    char   type[25];			  /* device type e.g. eagle */
    struct disktab  disktab;		  /* the disktab info from
					     /etc/disktab */
    struct stat stat;			  /* the stat table */
    int     size;
    int    ndev_index;			  /* Associated ndev table entry */
}           info[MAX_STRIPE_PARTITIONS];

/* 
 * The ndev table counts the number of partitions
 * that happen to be on the same physical device.
 * Each entry is associated with the physical device
 * from the same indexed entry of the original info
 * table.  The info entries contain an index back into
 * this table so that they can still refer to the same
 * ndev entry after being sorted.
 * When a partition is added to the info table, the
 * ndev entry corresponding to the same physical device
 * with the lowest index is incremented.  During sectioning
 * that same ndev entry is decremented when the use of that
 * info entry is exhausted.  In this way, we can tell how
 * many real devices are used in a section and weather 
 * interleaved striping will do much good.
 */
int     ndev[MAX_STRIPE_PARTITIONS];     /* table of the number of partitions 
					     used per device */
#define BASEDEV(x) ((info[ (x) ].stat.st_rdev) / N_PARTITIONS)

int     npart;				  /* number of partitions involved in
					     the stripe */

/*
 * Misc. variables for tracking command
 * line options and other stripe geometry.
 */
int     verbose = 0,			 /* verbose output */
        nochange = 0,			 /* don't acutally do anything */
        entry = 0;			 /* only update stripecap */
int     multi_part_disks;		 /* Non-zero when there are multiple 
					  * partitions on a physical disk in
					  * the current stripe section. */
int     bsize,				 /* Filesystem block size for mkfs */
        fsize,				 /* Filesystem frag size for mkfs */
        dev,				  /* # devices in a section */
        secsize,			  /* section size */
        stbsize;			  /* stripe block size */
	stb_arg = 0;			  /* Set if stripe block size included
					   * on the command line. */
int     minfree = DEFMINFREE,		  /* min % space reserved for suser */
        revpersec = 60,			  /* revolutions pers second of volume*/
        nsectors,
        ntracks,			  /* mkfs parameters */
	cpg,			          /* cylinders per group */
	nbpi = 0,			  /* # bytes per inode for mkfs */
	ninodes;			  /* desired number of inodes */
char   *stname;				  /* Name of this stripe device */
struct stat ststat;			  /* the stripe stat */
char    indexstr[] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

char 	ans[MAXANS];			  /* buffer for answer to prompt */

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
    fprintf (stderr, a, b, c, d, e, f);
    exit (1);
}

missingarg (c)
{
    fatal ("Option -%c missing argument.\n", c);
}

infocmp (a, b)
struct info *a,
           *b;
{
    /* for sorting table into descending order */
    return (b -> size - a -> size);
}

usage ()
{
    p ("usage: newst [options] ds? disk_dev1 [type1] disk_dev2 [type2] ...\n");
    p ("	where options are:\n");
    p ("	-v	Verbose- prints out everything.\n");
    p ("	-n	Nochange- only prints stripe table.\n");
    p ("	-e	Entry- only updates /etc/stripecap.\n");
    p ("	-s arg	Specify number of sectors / track.\n");
    p ("	-t arg	Specify number of tracks / cylinder.\n");
    p ("	-b arg	Specify file system block size e.g. 8k or 8192.\n");
    p ("	-f arg	Specify file system frag size e.g. 1k or 1024.\n");
    p ("	-c arg	Specify number of cylinders / group.\n");
    p ("	-m arg	Specify file system minfree parameter.\n");
    p ("	-r arg	Specify file system revolutions/second parameter.\n");
    p ("	-i arg	Specify number of bytes per inode mkfs should use.\n");
    p ("	-I arg	Specify number of inodes desired.\n");
    p ("	-B arg	Specify stripe block size in sectors.\n");
}

main (argc, argv)			  /* newst - make a new st system */
char  **argv;
{
    register int i, j;
    int k, l, used = 0, totalsize = 0, rem, code;
    char buf[DEV_BSIZE], tmpbuf[20], stbuf[BUFSIZ];
    struct disktab *disktabp;
    FILE * stf;				  /* points to the stripe file */
    dev_t   basedev;
    char c;

    argc--, argv++;
    while (argc > 0 && **argv == '-')
    {
	(*argv)++;
	while (**argv) {
	    switch (*(*argv)++)
	    {
		case 'n': 
		    nochange++;		  /* Don't do anything */
		    break;
		case 'v': 
		    verbose++;
		    break;
		case 'e': 
		    entry++;		  /* Make stripecap entry only */
		    break;
		case 'b': 
		    if (*++argv == NULL)
			missingarg ('b');
		    argc--;
		    bsize = atoi (*argv);
		    while (isdigit (**argv))
			++* argv;
		    if (**argv == 'k') {
			bsize *= 1024;
			++* argv;
		    }
		    break;
		case 'f': 
		    if (*++argv == NULL)
			missingarg ('f');
		    argc--;
		    fsize = atoi (*argv);
		    while (isdigit (**argv))
			++* argv;
		    if (**argv == 'k') {
			fsize *= 1024;
			++* argv;
		    }
		    break;
		case 'm': 
		    if (*++argv == NULL)
			missingarg ('m');
		    argc--;
		    minfree = atoi (*argv);
		    if (minfree < 0)
			fatal ("%s: minfree must be non-negative", *argv);
		    while (isdigit (**argv))
			++* argv;
		    break;
		case 'r': 
		    if (*++argv == NULL)
			missingarg ('r');
		    argc--;
		    revpersec = atoi (*argv);
		    if (revpersec < 0)
			fatal ("%s: revpersec must be non-negative", *argv);
		    while (isdigit (**argv))
			++* argv;
		    break;
		case 's': 
		    if (*++argv == NULL)
			missingarg ('s');
		    argc--;
		    nsectors = atoi (*argv);
		    while (isdigit (**argv))
			++* argv;
		    break;
		case 't': 
		    if (*++argv == NULL)
			missingarg ('t');
		    argc--;
		    ntracks = atoi (*argv);
		    while (isdigit (**argv))
			++* argv;
		    break;

		/* added option to allow user to specify the desired *
		 * # of cylinders per group            -JDK 11/13/86 */
		case 'c':
		    if (*++argv == NULL)
			missingarg ('c');
		    argc--;
		    cpg = atoi(*argv);
		    if (cpg < 0)
			fatal("%s: bad cylinders/group", *argv);
		    while (isdigit (**argv))
			++* argv;
		    break;

		/* added option so users can pass number of bytes per *
		 * inode directly through to mkfs.  shouldn't use     *
		 * along with the -I option.			      */
		case 'i':
		    if (*++argv == NULL)
			missingarg ('i');
		    argc--;
		    nbpi = atoi(*argv);
	    	    while (isdigit (**argv))
		        ++* argv;
		    if (**argv == 'k' || **argv == 'K'){
		        nbpi *= 1024;
		        ++* argv;
	    	    }
		    if (nbpi < 1)
			fatal("%s: bad number of bytes per inode\n",
				*argv);
		    break;

		/* added option to help users specify desired # of *
		 * inodes they want in file system   -JDK 11/13/86 */
		case 'I':
		    if (*++argv == NULL)
			missingarg ('I');
		    argc--;
		    ninodes = atoi(*argv);
	    	    while (isdigit (**argv))
		        ++* argv;
		    if (**argv == 'k' || **argv == 'K'){
		        ninodes *= 1024;
		        ++* argv;
	    	    }
		    if (ninodes < 0)
			fatal("%s: bad number of inodes\n",
				*argv);
		    break;
		case 'B': 
		    if (*++argv == NULL)
			missingarg ('B');
		    argc--;
		    stb_arg = atoi (*argv);
		    if (stb_arg < 1 || stb_arg % MIN_SBLK)
			fatal ("%s: stripe block size must be a positive multipe of %d\n", 
				*argv, MIN_SBLK);
		    while (isdigit (**argv))
			++* argv;
		    break;

		default: 
		    p ("Unknown option -%c\n", *--*argv);
		    usage ();
		    exit (1);
	    }
	}
	argc--, argv++;
    }

    if (*argv == NULL)
    {
	p ("Missing required stripe device name.\n");
	usage ();
	exit (1);
    }

    /* 
     * Validate the specified stripe device.
     * Verify both its raw and block interfaces.
     */
    stname = *argv++;
    if (strncmp (stname, "/dev/", 5) == 0)
	stname = &stname[5];
    if (stname[0] == 'r')
	stname++;
    (void) sprintf (buf, "/dev/r%s", stname);
    if (stat (buf, &ststat) < 0)
	fatal ("Can't stat stripe device %s.\n", buf);
    if ((ststat.st_mode & S_IFCHR) == 0)
	fatal ("%s is not a raw device.\n", buf);
    if (major (ststat.st_rdev) != STRIPECHAR)
	fatal ("%s is not a raw stripe device.\n", buf);

    (void) sprintf (buf, "/dev/%s", stname);
    if (stat (buf, &ststat) < 0)
	fatal ("Can't stat stripe device %s.\n", buf);
    if ((ststat.st_mode & S_IFBLK) == 0)
	fatal ("%s is not a block device.\n", buf);
    if (major (ststat.st_rdev) != STRIPEBLOCK)
	fatal ("%s is not a block stripe device.\n", buf);

    /* make sure the stripe device is not already mounted (X-6809) -- 12/87 */
    if (mounted (buf)) {
	if (nochange)	/* let them continue and see stripecap entry, etc */
	    fprintf (stderr, "warning: %s is already mounted.\n", buf);
	else
	    fatal ("%s is already mounted.\n", buf);
    }

    /* Make certain the argument list is not too long. */
    if (--argc > 2 * MAX_STRIPE_PARTITIONS)
	fatal("Too many diskdev/type arguments - limit is %d devices.\n", 
		MAX_STRIPE_PARTITIONS);

    /* validate each component partition */
    for (npart = 0; *argv && npart < MAX_STRIPE_PARTITIONS; npart++)
    {
	struct vtctodisktab *vp;

	info[npart].special = *argv++;
	if (strncmp (info[npart].special, "/dev/", 5) == 0)
	    info[npart].special = &info[npart].special[5];
	if (info[npart].special[0] == 'r')
	    info[npart].special++;
        vp = tryvtoc(info[npart].special);
        if (vp != (struct vtctodisktab *) -1) {
	    int index = 0;

	    printf("You obviously have a vtoc\n");
	    strcpy(info[npart].type,vp->v_disktype);
	    info[npart].disktab.d_secsize = vp->v_secsize;
	    info[npart].disktab.d_ntracks = vp->v_ntracks;
	    info[npart].disktab.d_nsectors = vp->v_nsectors;
	    info[npart].disktab.d_ncylinders = vp->v_ncylinders;
	    info[npart].disktab.d_rpm = vp->v_rpm;
	    while (index < 8) {
	    	info[npart].disktab.d_partitions[index].p_size = vp->v_part[index].p_size;
	    	info[npart].disktab.d_partitions[index].p_bsize = vp->v_part[index].p_bsize;
	    	info[npart].disktab.d_partitions[index].p_fsize = vp->v_part[index].p_fsize;
		index++;
	    }
	    /* Check if the next argument is a disk type, then skip it */
	    if(getdiskbyname(*argv) != NULL) {
		printf("Skipping dev type %s\n",*argv);
		*argv++;
	    }
	} else {
	    printf("You obviously don't have a vtoc\n");
	    if (*argv == NULL)
	        fatal ("Missing disk type for %s.\n", info[npart].special);
	    strcpy(info[npart].type,*argv++);
	    for (disktabp = NULL, i = 0; i < npart; i++) {
	        if (strcmp (info[i].type, info[npart].type) == 0) {
		    disktabp = &info[i].disktab;
		}
	    }
	    if (disktabp == NULL &&
		    (disktabp = getdiskbyname (info[npart].type)) == NULL) {
	        fatal ("No entry in /etc/disktab for %s.\n",info[npart].type);
	    }
	    info[npart].disktab = *disktabp;
	}
	(void) sprintf (buf, "/dev/r%s", info[npart].special);
	if (stat (buf, &info[npart].stat) < 0)	/* stat character device */
	    fatal ("Can't stat %s.\n", buf);
	if ((info[npart].stat.st_mode & S_IFCHR) == 0)
	    fatal ("%s is not a raw device.\n", buf);
	/* make sure components are not mounted file systems in themselves */
	(void) sprintf (buf, "/dev/%s", info[npart].special);
	if (mounted (buf)) {	/* buf contains name of the block device */
	    if (nochange)	/* just issue a warning -- don't abort */
	    	fprintf (stderr,"warning: %s is a mounted file system.\n",buf);
	    else
	    	fatal ("%s is a mounted file system.\n", buf);
	}
	/*
	 * We still need to stat block device to determine the
	 * major/minor numbers to put in the stripecap file entry.
	 * We can't assume block and char devices have the same 
	 * major numbers.
	 */
	if (stat (buf, &info[npart].stat) < 0)	/* stat block device */
	    fatal ("Can't stat %s.\n", buf);
	if ((info[npart].stat.st_mode & S_IFBLK) == 0)
	    fatal ("%s is not a block device.\n", buf);
	info[npart].size = info[npart].disktab.d_partitions
	    [minor (info[npart].stat.st_rdev & (N_PARTITIONS - 1))]
	    .p_size;
	printf("info[%d].size is %d\n",npart,info[npart].size);
	/*
	 * Count this device in the 'ndev' table
 	 * entry pertaining to this partition's real
	 * physical device.
	 */
	for (basedev = BASEDEV(npart), i = 0; basedev != BASEDEV(i); i++)
		;  		/* Find lowest info index of base device */ 
	ndev[i]++;		/* Indicates how many partitions are on it */
	info[npart].ndev_index = i; 	/* Save for future references */

	/* 
	 * While we are at it, lets make certain none
	 * of the other partitions are the same exact
	 * device, although with overlapping partitions,
	 * there still may be a problem.  We can't detect
	 * this since we can't locate the start block
	 * of the partitions on the actual devices.
	 */
	for (; i < npart; i++) 
	    if (info[i].stat.st_rdev == info[npart].stat.st_rdev)
		fatal("Partitions %s and %s match - they must be distinct.\n",
			info[i].special, info[npart].special);
    }

    if (npart < 2)
	fatal ("Striping only makes sense with 2 or more devices.\n");

    for (i = 0; i < npart; i++) 
	if (ndev[i] > 1 && !multi_part_disks++) {
	    p("Warning: More than one partition used per disk drive.\n");
	    p("         this implies more seeking (and poorer performance).\n");
	}

    /* set up default parameters */
    if (bsize == 0)
	bsize = info[0].disktab.d_partitions
	    [minor (info[0].stat.st_rdev & (N_PARTITIONS - 1))].p_bsize;
    if (fsize == 0)
	fsize = info[0].disktab.d_partitions
	    [minor (info[0].stat.st_rdev & (N_PARTITIONS - 1))].p_fsize;
    if (ntracks == 0)
	ntracks = info[0].disktab.d_ntracks;
    if (nsectors == 0)
	nsectors = info[0].disktab.d_nsectors *
	    (npart - multi_part_disks);
    if (bsize < MINBSIZE || bsize > MAXBSIZE)
	fatal("Block size must be between %d and %d.\n",
		MINBSIZE, MAXBSIZE);


    /* sort info table by partition size	 */
    qsort ((char *) info, npart, sizeof (struct info), infocmp);

    if (verbose)
	for (i = 0; i < npart ; i++)
	   printf("Partition %d: special device %s, type %s, size %d sectors\n",
		    i, info[i].special, info[i].type, info[i].size);


    /* open the stripe data base file */
    if (!nochange)
    {
	/* open the stripe data base file */
	stf = fopen (STRIPECAP, "a");
	if (stf == NULL)
	    fatal ("Can't open stripe database file %s.\n",
		    STRIPECAP);
	if (stgetent (stbuf, stname) == 1)
	{
	    /*
	    fatal ("An entry for \"%s\" already exists.  Edit %s to remove the old one.\n",
		    stname, STRIPECAP);
	     *
	     * change this to just be a warning and ask the user if he wishes to
	     * overwrite the existing entry
	     * 					- JDK 11/10/86
	     */
	    
    	    fprintf (stdout, "An entry for \"%s\" already exists.\n", stname);
	    fprintf (stdout, "Do you wish to continue and overwrite the existing entry? [y/n](n):");
	    if ((fgets (ans, MAXANS, stdin) != 0) && ans[0] == 'y') {
		if (stf != NULL) (void)fclose (stf); /* close stripecap file */
		(void)strment (stname); 	/* remove previous entry */
		if ((stf = fopen (STRIPECAP, "a")) == NULL)
	    	    fatal ("Can't open stripe database file %s.\n",
		   	     STRIPECAP);
	    }
	    else 				/* don't write over old one */
		fatal ("newst: stripecap file unchanged\n");
	}
    }

    /* put the info in the file about the devices */
    if (verbose)
	printf ("\nStripe database file %s:\n", STRIPECAP);
    if (!nochange)
	fprintf (stf, "%s:\\\n", stname);
    if (verbose)
	printf ("%s:\\\n", stname);

    if (!nochange)			  /* # of partitions striped */
	fprintf (stf, "\t:np#%d:", npart);
    if (verbose)
	printf ("\t:np=%d:", npart);

    for (i = 0; i < npart; i++){ /* output maj/min numbers for each partition */
	if (!nochange)
	    fprintf (stf, "\\\n\t:M%c#%d:m%c#%d:",
		    indexstr[i], major (info[i].stat.st_rdev),
		    indexstr[i], minor (info[i].stat.st_rdev));
	if (verbose)
	    printf ("\\\n\t:M%c#%d:m%c#%d:",
		    indexstr[i], major (info[i].stat.st_rdev),
		    indexstr[i], minor (info[i].stat.st_rdev));
    }

    /* 
     * Output the stuff for each section.
     * Section sizes are limited by the length
     * of the shortest not yet fully used partition
     * in the partition table, which is in order
     * of largest to smallest, so start at the end.
     */
    for (i = npart, k = 0; i-- > 0;
	 i = j, k++, used += secsize, totalsize += dev * secsize) {

	/* Find all partitions of the same size */
	for (j = i; j > 0 && info[i].size == info[j - 1].size; j--);
		/* 'i' is index of last partition of given size, 
	         * j is index of first of same size. */

	dev = i + 1;			/* #partitions in the section */
	secsize = info[i].size - used;	/* #blocks from each one */

	/* 
	 * Set the stripe block size (interleave factor).
 	 * If more than one partition is on the same
	 * device than use the remainder for it.  Otherwise,
	 * use the size from the command line or the filesystem
	 * block size as the interleave factor.  Note that
	 * this value must be a multiple of the system memory
	 * cluster size.  Also don't interleave with less than
	 * two stripe blocks on each device. 
	 */
	if (stb_arg)
		stbsize = stb_arg;
	else 
		stbsize = MIN_SBLK;

	if (dev == 1 || multi_part_disks || secsize / stbsize < 2) {
		stbsize = secsize - secsize % MIN_SBLK;
		if (stbsize < MIN_SBLK)
			stbsize = MIN_SBLK;  /* can't be less */
	} 

	/* 
	 * Adjust the section size as necessary
	 * so that it will be a multiple of the
	 * stripe block size and so the total
	 * size with this section does not
	 * exceed the system imposed limit.
	 */
	rem = (MAX_DEV_BLKS - totalsize) / dev;
	rem -= rem % MIN_SBLK;
	if (rem < secsize) {
		if (rem / stbsize < 2)
			stbsize = rem;	/* Not worth interleaving */
		else
		 	rem -= rem % stbsize;
	    	p("Warning: Stripe section %d has been adjusted so the ", k);
		p("\n\tstripe size does not exceed the system imposed");
		p("\n\tmaximum of %d sectors.  At least %d sectors are", 
			MAX_DEV_BLKS, secsize - rem);
		p("\n\tbeing used on each of %d devices.\n", dev);
		secsize = rem;
	} else if ((rem = secsize % stbsize) != 0) {
	    secsize -= rem;
	    p("Warning: The final %d sectors of device%s", rem, i==j ? "" : "s");
	    for (l = j; l <= i; l++)
		p(" %s", info[l].special);
	    if (secsize == stbsize) {
	    	p("\n\tare unused so the section and stripe section\n");
		p("\tblock size %d are multiples of %d.\n", 
			secsize, MIN_SBLK);
	    } else {
	    	p("\n\tare unused so the section size %d is a multiple\n", 
			secsize);
	    	p("\tof the stripe section block size %d.\n", stbsize);
	    }
	}

	if (!nochange)
	    fprintf (stf, "\\\n\t:D%c#%d:B%c#%d:S%c#%d:",
		    indexstr[k], dev,
		    indexstr[k], secsize,
		    indexstr[k], stbsize);
	if (verbose)
	    printf ("\\\n\t:D%c#%d:B%c#%d:S%c#%d:",
		    indexstr[k], dev,
		    indexstr[k], secsize,
		    indexstr[k], stbsize);

	/* 
	 * Since the partitions described
	 * by info table entries i thru j
	 * have been exhausted, no longer
	 * count their reference to their
	 * base device.
	 */
	for (l = j; l <= i; l++)
	    ndev[info[l].ndev_index]--;

	/* 
	 * Now determine if the any of the base devices
	 * are used in the next section for more than
	 * one of its partitions.  This factors into
	 * the section's stripe block size.  Note that
	 * only devices zero through 'j - 1' are in it,
	 * but all of the first 'npart' entries of ndev
	 * must be checked, since the info table order
	 * may have changed during the sort.
	 */
	for (multi_part_disks = l = 0; l < npart; l++)
	    if (ndev[l] > 1) { 
		multi_part_disks++;
		break;  	/* Short circuit this for loop */
	    }
    }

    if (verbose)
	printf("\nStripe %s contains a total of %d sectors.\n", 
		stname, totalsize);


    /* added code to set default value of cpg if not given  -JDK 11/13/86 */
    /*                  (this code came from newfs.c)                     */

    if (cpg == 0) {
	/*
	 * Can't just pick 16, because this fails for bsize > 32KB.
	 * Note that this change works for bsize = 64KB, but perhaps
	 * fails for bsize > 64KB.
         *				- KAF 6/5/85
	cpg = (bsize > 32768) ? 32 : 16;
	 */

	/*
	 *   This is still arbitrary -- try to come up with a better 
	 * heuristic: The maximum number of files that can be created
	 * for a file system is set by the file system size.  There
	 * is a minimun file size of one fragment, so find out how
	 * many frags there are in system.  Then allow for some overhead
	 * knowing that *all* of the file system does not consist of
	 * simply data blocks.   Also have a percentage that makes
	 * this not as wasteful as a "worst-case" situation. (ie,
	 * some files in the file system will be > 1 frag)  We will
	 * define USAGE_PCT as this somewhat arbitrarily-selected
	 * percentage of space used on data.
	 *   Then we figure out how many cylinder groups we will need
	 * based on MAXIPG inodes per group.
	 *   This figure is then converted to cylinders per group and
	 * passed on to mkfs.
	 *				- JDK 11/7/86
	 */

/*  Take this out and use a "table", set below dependent upon fsize...
#define	USAGE_PCT	0.35	* estimate of how much space used for data *
*/

long fssize_in_frags, 	/* file system size in fragments,not segments */
	  spb,			/* sectors per block */
	  spc = 0,		/* sectors per cylinder */
	  cpc,			/* cylinders per cycle */
	  ncyl,			/* number of cylinders in given partition */
	  ncg;			/* number of cylinder groups */
double    max_nfiles,		/* max # files in system==max # inodes needed */
	  USAGE_PCT;		/* estimate of how many frags used for data */

	/* set the USAGE_PCT value relative to the frag size... */
	if (fsize <= 1024)		/* 512 or 1k */
	    USAGE_PCT = .35;
	else if (fsize <= (2*1024))	/* 2k */
	    USAGE_PCT = .45;
	else if (fsize <= (4*1024))	/* 4k */
	    USAGE_PCT = .60;
	else if (fsize <= (8*1024))	/* 8k */
	    USAGE_PCT = .70;
	else if (fsize <= (16*1024))	/* 16k */
	    USAGE_PCT = .80;
	else if (fsize <= (32*1024))	/* 32k */
	    USAGE_PCT = .90;
	else			/* 64k -- most files with be only one frag */
	    USAGE_PCT = .95;	

	/* first calculate the # cylinders per cycle, because cpg *
	 * must be a multiple of cpc 		- JDK 11/12/86    */
	spb = bsize / DEV_BSIZE;
	/*
	 * Calculate spc the same way mkfs does
	 * DGS 1/15/88
	 */
	spc = nsectors * ntracks;
	for (cpc = spb, i = spc;
	     cpc > 1 && (i & 1) == 0;
	     cpc >>= 1, i >>= 1)
		/* void */;
	if (cpc > MAXCPG) {
	    printf("maximum block size with nsect %d and ntrak %d is %lld\n",
		    nsectors, ntracks, bsize / (cpc / MAXCPG));
	    exit(1);
	}

 	fssize_in_frags = totalsize * DEV_BSIZE / fsize;
		/* must convert the sectors to fragments */

	max_nfiles = (double)fssize_in_frags * USAGE_PCT;
		/* minimum size for a file is 1 fragment */
	if (ninodes)
	    max_nfiles = ninodes;
	ncg = (long)max_nfiles / MAXIPG;
	if (ncg < MINNCG)	/* have a min # of cyl groups */
	    ncg = MINNCG;
	ncyl = totalsize / spc;	/* want # cyl in partition only */
   	cpg = ncyl / ncg;
	if (cpg % cpc != 0)	/* if cpg not multiple of cpc */
	    cpg = roundup(cpg, cpc);   /* make it one */
	while (cpg < MINCPG)	/* if figure comes out weird, */
	    cpg += cpc;		/*    make it acceptable      */
  	while (cpg > MAXCPG)
	    cpg -= cpc;		/* keep it a multiple of cpg */
#ifdef DEBUG
    printf ("\n** DEBUG PRINTS **\n");
    printf ("fssize (frag) = %lld\n", fssize_in_frags);
    printf ("ncyl = %lld\n", ncyl);
    printf ("max_nfiles  = %lld\n", (long)max_nfiles);
    printf ("ncg = %lld\n", ncg);
    printf ("spb = %lld\n", spb);
    printf ("spc = %lld\n", spc);
    printf ("cpc = %lld\n", cpc);
    printf ("cpg = %d\n", cpg);
    printf ("** END OF DEBUG PRINTS **\n");
#endif
	/*** END OF CODE FOR cpg ASSIGNMENT     -- kucera 10/31/86  ***/

    }

    if (!nochange)
	fprintf (stf, "\n\n");		  /* dummy end section */
    if (verbose)
	printf ("\n");
    if (!nochange)
	(void) fclose (stf);

    if (entry)
	exit (0);

    /* put stripe table on the disk using stput */
    (void) sprintf (buf, "/etc/putst /dev/r%s", stname);
    printf ("%s\n", buf);
    if (!nochange)
    {
	code = system (buf);
	if (code)
	    fatal ("putst exited abnormally\n");
    }

    /* invoke mkfs */
    (void) sprintf(tmpbuf, "%d", nbpi);
    (void) sprintf (buf, "/etc/mkfs /dev/r%s %d %d %d %d %d %d %d %d %s",
	    stname, totalsize, nsectors, ntracks, bsize, fsize, cpg, minfree,
	    revpersec, nbpi > 0 ? tmpbuf : "");
    printf ("%s\n", buf);
    if (!nochange)
    {
	code = system (buf);
	if (code)
	    fatal ("mkfs exited abnormally\n");
    }
    /* invoke fsirand to randomize the inode generation counts */
    (void) sprintf (buf, "/etc/fsirand /dev/r%s", stname);
    printf ("%s\n", buf);
    if (!nochange)
    {
	code = system (buf);
	if (code)
	    fatal ("fsirand exited abnormally\n");
    }
    exit (0);
}


/*
 * Check to see if file system with given name is already mounted.
 * We have to be careful because getmntent modifies its static struct.
 *	( this was stolen [& slightly modified] from mount.c )
 */
mounted(fsname)
	char *fsname;
{
	int found = 0;
	struct mntent *mnt;
	FILE *mnttab;

	mnttab = setmntent(MOUNTED, "r");
	if (mnttab == NULL) {
		return(0);	/* assume not mounted */
	}
	while ((mnt = getmntent(mnttab)) != NULL) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) {
			continue;
		}
		if (strcmp(fsname, mnt->mnt_fsname) == 0) {
			found = 1;
			break;
		}
	}
	endmntent(mnttab);
	return (found);
}
