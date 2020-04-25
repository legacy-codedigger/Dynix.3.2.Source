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
static char rcsid[] = "$Header: sweepd.c 1.7 90/06/08 $";
#endif

/*
 * $Log:	sweepd.c,v $
 */

/*
 * Usermode implementation of Patrol Seek for drives on DCCs
 */
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/time.h>
#define  KERNEL
#include <zdc/zdc.h>
#undef	 KERNEL
#include <zdc/ioconf.h>
#include <sys/file.h>
#include "sweepd.h"

#include <stdio.h>
#include <signal.h>
#include <nlist.h>
#include <ctype.h>

/*
 * Per-unit array
 */
struct dcc_unit dcc_unit[MAX_DCC_UNITS];
int dotrace;
extern int errno;
#define	BIG	  0xFFFFFFFF

/*
 * In the kernel's zdunit[] array, the correct way of
 * identifying a configured drive from an empty slot
 * in the array is to look for ZU_GOOD in the zu_state
 * field (see function getdrives below).  Inside the
 * daemon, bad/disabled/unused units are known by the
 * -1 in the unit type field.
 */
struct dcc_unit badunit = {	/* used for error handling */
    -1,				/* raw partition file descriptor */
    0				/* partition size (in bytes) */
};

/*
 * Sweepd
 */
main(ac, av)
    int ac;
    char **av;
{
    int n_drives;
    int last;
    int i;
    int child;

    getargs (ac, av);
    n_drives = getdrives();
    if (n_drives < 0)		/* some error, message already printed */
	fatal_exit();
    if (n_drives == 0)		/* just no DCC drives or all ignored   */
	exit(0);

    /*
     * This needs to be called after getdrives()
     */
    read_ignorelist();

    /*
     * We can only assume 20 fds per process (we don't want to
     * demand that the customer reconfig the kernel just to run
     * sweepd).  Also don't want to mux the descriptors (takes
     * zillions of opens).  So we fork enough processes to
     * handle all the drives.
     */
    for (i = 0; i < n_drives; i += DRIVES_PER_PROCESS) {
	if (dotrace > 1) {
	    printf ("sweepd parent create child (%d)\n", i);
	    (void) fflush(stdout);
	}
	child = fork();
	if (child < 0)
	    perror ("sweepd: fork");
	else if (child == 0) {
	    last = i + DRIVES_PER_PROCESS;
	    if (last > n_drives)
		last = n_drives;
	    if (dotrace)
		printf ("sweepd pid %d: zd%d - zd%d\n",
				  getpid(), i, last - 1);
	    opendrives(i, last);
	    closedrives(i, last);
	    dosweep(i, last);
	    fprintf (stderr, "sweepd: dosweep() returned?!\n");
	    exit(3);
	}
    }
    exit(0);
}

getargs (ac, av)
    int ac;
    char **av;
{
    int i;
    char *cp;

    for (i = 1; i < ac; ++i) {
	cp = av[i];
	if (cp [0] == '-') {
	    switch (cp[1]) {
		case 'd':
		    dotrace++;
		    break;

		default:
		    fprintf (stderr, "sweepd: unknown option '%c' ignored\n",
					cp[1]);
		    break;
	    }
	}
	else {
	    /*
	     * No non-flag options for now
	     */
	    fprintf (stderr, "sweepd: unknown argument \"%s\" ignored\n", cp);
	}
    }
}

/*
 * read_ignorelist()
 *
 * This routine reads the /etc/sweepd.ignore file and causes
 * the indicated drives to be ignored by setting the unit_fd
 * field in the corresponding array entry to -1.  This tells
 * the opendrives routine to skip it.
 */
read_ignorelist()
{
	FILE *fp = fopen (IGNORE_FILENAME, "r");
	char buf[256];
	int num;

	if (fp == NULL)
		/* no ignore file is not an error */
		return;

	while (fgets(buf, sizeof(buf), fp)) {
		/* Get rid of newline */
		buf[strlen(buf)-1] = '\0';
		if (!buf[0] || buf[0] == '#') 
			/* skip blank lines or comments */
			continue;

		sscanf (buf, "zd%d", &num);
		dcc_unit[num] = badunit;
	}
}

/*
 * Build the drive table, mostly from the
 * zdunit array in /dev/kmem
 */
getdrives()
{
    int kmem_fd;
    int result;

    kmem_fd = open (KMEM_FILENAME, O_RDONLY);
    if (kmem_fd < 0) {
	perror ("sweepd: open kmem");
	return 0;
    }
    result = getdrives_guts (kmem_fd);
    (void) close (kmem_fd);
    return result;
}

getdrives_guts(kmem_fd)
    int kmem_fd;
{
    static struct nlist nl[] = {
#define	X_ZDCONF	0
	{ "_zdc_conf" },
#define	X_ZDUNIT	1
	{ "_zdunit" },
	0
    };

    struct zdc_conf zdc_conf;
    struct zd_unit *zdunit;
    long k_zdunit;
    int drives_found = 0;
    int i;

    (void) nlist (DYNIX_FILENAME, nl);
    if (nl[X_ZDCONF].n_value == 0) {
	fprintf (stderr, "sweepd: no DCCs configured\n");
	return 0;
    }
    if (nl[X_ZDUNIT].n_value == 0) {
	fprintf (stderr, "sweepd: \"zdunit\" in kmem: no such symbol\n");
	return -1;
    }

    if (lseek (kmem_fd, (off_t) nl[X_ZDCONF].n_value, 0) < 0) {
	perror ("sweepd: seek to zdc_conf address on kmem");
	return -1;
    }
    if (read (kmem_fd, (char *) &zdc_conf, sizeof (struct zdc_conf)) < 0) {
	perror ("sweepd: read zdc_conf from kmem");
	return -1;
    }

    if (dotrace > 1)
	printf ("sweepd: zdc_conf at 0x%x ndrives %d\n",
		nl[X_ZDCONF].n_value, zdc_conf.zc_nent);

    zdunit = (struct zd_unit *) calloc ((unsigned) zdc_conf.zc_nent,
					(unsigned) sizeof (struct zd_unit));
    if (zdunit == NULL) {
	fprintf (stderr, "sweepd: allocate zdunit failed\n");
	return -1;
    }
    if (lseek (kmem_fd, (off_t) nl[X_ZDUNIT].n_value, 0) < 0) {
	perror ("sweepd: seek to zdunit address on kmem");
	return -1;
    }
    if (read (kmem_fd, (char *) &k_zdunit, 4) < 0) {
	perror ("sweepd: read zdunit address from kmem");
	return -1;
    }
    if (lseek (kmem_fd, (off_t) k_zdunit, 0) < 0) {
	perror ("sweepd: seek to zdunit array on kmem");
	return -1;
    }
    if (read (kmem_fd, (char *) zdunit,
		zdc_conf.zc_nent*sizeof (struct zd_unit)) < 0) {
	perror ("sweepd: read zdunit array from kmem");
	return -1;
    }

    /*
     * Now we have the in-kernel zdunit array.  Extract the
     * important stuff into our zdunit array.
     */
    for (i = 0; i < zdc_conf.zc_nent; ++i) {
	if (zdunit[i].zu_state != ZU_GOOD) {
	    if (dotrace > 1)
		printf ("zd%d: not bound (state %d)\n", i, zdunit[i].zu_state);
	    dcc_unit[i] = badunit;
	    continue;
	    }
	dcc_unit[i].unit_size = get_size (i);
	dcc_unit[i].unit_pos = SWEEP_OFFSET;
	drives_found++;
	if (dotrace > 1)
	    printf ("zd%d: drive %d on ctlr %d state %d\n",
		i,
		zdunit[i].zu_drive, zdunit[i].zu_ctrlr,
		zdunit[i].zu_state);
    }
    return drives_found;
}

/*
 *  figure out size of disk with binary search
 */


int get_size (drive)
	int drive;
{
	register unsigned int j, fd, u, l;
	char *buf = valloc(512);
	char name[15];

		sprintf (name, "/dev/zd%dc", drive);
		fd = open(name, 0);
		u = BIG; l = 0;
		for(; u > l ;) {
			lseek(fd, j = (u+l)/2, 0);
			if( read(fd, buf, 512) == 512 )
				l = j + 1;
			else
				u = j - 1;
		}
		close(fd);
		return j-1;
}

/*
 * Close the drives back down now that we know which are good
 */
closedrives(first, last)
    int first, last;		/* drive unit numbers */
{
    int i;

    /*
     * Close each open file descriptor
     */
    for (i = first; i < last; ++i) {
	if (dcc_unit[i].unit_fd < 0)
	    continue;
	(void) close(dcc_unit[i].unit_fd);
    }
}

/*
 * Open the "c" partitions for this process
 */
reopen_drive(unit, index)
    struct dcc_unit *unit;
    int index;
{
    char unitname [64];
    int fd, x;

    (void) sprintf (unitname, "/dev/rzd%d", index);

    /*
     * Give up to three tries to get at the disk--we are using the whole-disk
     * interface, which is interlocked and exclusive.
     */
    for (x = 0; x < 3; ++x) {
	if ((fd = open(unitname, O_RDONLY)) >= 0)
	    break;
	sleep(1);
    }
    if (fd < 0)
	return(fd);
    unit->unit_fd = fd;
    return(0);
}

/*
 * Open the "c" partitions for this process
 */
opendrives(first, last)
    int first, last;		/* drive unit numbers */
{
    int i;
    char unitname [64];
    int fd;

    /*
     * Try to open the "c" partition.  On failure, just
     * continue.  Mainline code ignores drives with negative
     * values in the unit_fd field.
     */
    for (i = first; i < last; ++i) {
	if (dcc_unit[i].unit_fd < 0) {
	    if (dotrace > 1)
		printf ("sweepd (%d): zd%d: not present\n", getpid(), i);
	    continue;
	}
	(void) sprintf (unitname, "/dev/rzd%d", i);
	fd = open (unitname, O_RDONLY);
	if (fd < 0) {
	    int foo = errno;	/* fix for "sweepd: not a typewriter" :-) */
	    fprintf (stderr, "sweepd: rzd%d: ", i);
	    errno = foo;
	    perror ("open");
	    dcc_unit[i] = badunit;
	} else {
	    dcc_unit[i].unit_fd = fd;
	    if (dotrace)
		printf ("sweepd: enable patrol seek on zd%d fd %d\n",
				i, fd);
	}
    }
}

/*
 * Do the loop of moving the heads
 */
dosweep(i, last)
    int i, last;
{
    if (dotrace)
	printf ("sweepd: pid %d: enter dosweep (%d, %d)\n",
		 getpid(), i, last);
    for (;;) {
	move_drives (i, last);
    }
}

/*
 * move_drives
 *
 * Seek a (fixed) offset away from the last seek.  If we would
 * go past the end of the disk, seek to zero.  Then read to 
 * force the head to move.
 */
move_drives(first, last)
    int first, last;
{
    int i;
    struct dcc_unit *d;
    static char *junkbuf = 0;
    extern char *valloc();

    /* Get an aligned buffer for ZDs */
    if (!junkbuf) {
	junkbuf = valloc(FORCEREADSIZE);
	if (!junkbuf) {
	    fprintf(stderr, "sweepd: can't get I/O buffer memory\n");
	    fatal_exit();
	}
    }

    /* for each drive this process is responsible for */
    for (i = first; i < last; ++i) {
	sleep (300/(last-first+1));
	d = &dcc_unit[i];
	if (d->unit_fd < 0)
	    continue;

	/*
	 * re-open the file descriptor now that we're going to use it.  If
	 * we couldn't get to the disk this time, just wait until next.
	 */
	if (reopen_drive(d, i) < 0)
	    continue;

	/*
	 * if the next seek would take us past the end of disk, seek to 0
	 * otherwise, seek to a positive offset away from the current
	 * position
	 */
	if (d->unit_pos + SWEEP_OFFSET >= d->unit_size) {
	    if ((d->unit_pos = lseek(d->unit_fd, (off_t)(2*SWEEP_OFFSET), L_SET)) < 0) {
	        int foo = errno;
	        fprintf (stderr, "sweepd: zd%d ", i);
	        errno = foo;
	        perror ("lseek");
	    }
	} else if ((d->unit_pos = lseek (d->unit_fd, (off_t)(d->unit_pos + SWEEP_OFFSET), L_SET)) < 0) {
	        int foo = errno;
	        fprintf (stderr, "sweepd: zd%d ", i);
	        errno = foo;
	        perror ("lseek");
	    }
	if (read (d->unit_fd, (char *) junkbuf, FORCEREADSIZE) < 0) {
	    int foo = errno;
	    fprintf (stderr, "sweepd: zd%d ", i);
	    errno = foo;
	    perror ("read");
	}
	if (dotrace > 1)
	    printf ("sweepd: pid %d: moving zd%d to byte %d of %d\n",
		     getpid(), i, d->unit_pos, d->unit_size);

	/*
	 * Done with the disk for now, so close back down
	 */
	close(d->unit_fd);
    }
}

fatal_exit()
{
    fprintf (stderr, "SWEEPD: INITIALIZATION FAILED: PATROL SEEK DISABLED.\n");
    fprintf (stderr, "Please attempt to resolve the problem.\n");
    fprintf (stderr, "Contact Sequent Customer Service for assistance.\n");
    exit(2);
}

#ifdef lint
/* [To fix the following:]
sweepd.c(595): warning: struct/union fs never defined
sweepd.c(595): warning: struct/union csum never defined
sweepd.c(595): warning: struct/union cg never defined
sweepd.c(595): warning: struct/union dinode never defined
sweepd.c(595): warning: struct/union pte never defined
sweepd.c(595): warning: struct/union vnode never defined
sweepd.c(595): warning: struct/union proc never defined
sweepd.c(595): warning: struct/union zdbad never defined
sweepd.c(595): warning: struct/union dk never defined
*/
struct proc {
    int x;
};
struct zdbad {
    int x;
};
struct dk {
    int x;
};
struct fs {
    int x;
};
struct csum {
    int x;
};
struct cg {
    int x;
};
struct dinode {
    int x;
};
struct pte {
    int x;
};
struct vnode {
    int x;
};
#endif lint
