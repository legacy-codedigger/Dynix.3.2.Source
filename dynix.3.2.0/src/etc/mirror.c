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
static char rcsid[] = "$Header: mirror.c 1.3 90/11/06 $";
#endif

#include <ctype.h>
#include <errno.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/param.h>
#include <machine/exec.h>
#include <stdio.h>
#include <mirror/mirror.h>

#define MIRRORNAME "rmr"		  /* base name of raw mirror device */
#define MIRRORMAJOR 20			  /* Conventional raw mirror major */
#define DEV "/dev"			  /* directory scanned for mirrors */
#define MRTAB "/etc/mrtab"

#ifdef DEBUG
#define NPROCS nprocs
#define BUFFERSIZE buffersize
#else /* DEBUG */
#define NPROCS 5
#define BUFFERSIZE 192
#endif /* DEBUG */

#define COPY_BUFSIZE (BUFFERSIZE * DEV_BSIZE)
#define DOT_BLOCKS	2048		  /* number of block per dots */
#define XDOTS (DOT_BLOCKS*DEV_BSIZE/COPY_BUFSIZE) /* number of xfers per dot */

#define LABELS_MISMATCHED	(-1)	  /* compare_labels() */
#define LABELS_AGREE		0	  /* compare_labels() */
#define LABELS_DISAGREE		1	  /* compare_labels() */

/*
 * mirror utility
 *
 * usage:
 *
 * mirror
 * With no arguments, this utility will attempt to find a mirror in /dev and,
 * if successful, will printout the status of all mirrors configured intot hte
 * system. 
 *
 *				-OR-
 *
 * mirror -a
 * With the single argument selecting the '-a' option (for all or automatic),
 * mirror makes its way through /etc/mrtab trying to start mirrors.  In
 * /etc/mrtab, the '#' character begins a comment which continues through the
 * end of line. Arguments are separated on a line by spaces and tabs.  All
 * arguments must be fully qualified path names.  The first path is the mirror,
 * the next two are the units that belong to that mirror.  Conceptually, this
 * is like eliminating comments and blank lines from /etc/mrtab and prefixing 
 * each line with 'mirror', except that other options are not allowed and
 * mirror will never copy/update mirrors.
 *
 *				-OR-
 *
 * mirror <mirror> [-l|-b] <unit> [-l|-b] <unit>|-d [-s <size>] [-r] [-f]
 *
 * eg:
 * mirror /dev/mr0 /dev/zd1g -l /dev/zd2g -s 172032
 *
 * This utility checks to see that the mirror (/dev/mr0 in our example) is
 * currently unused, and that the units have valid mirror unit labels, and are
 * from the same mirror.  If so, and the time stamps in the two labels do not 
 * agree, the one with the newer time is considered the reference and is copied
 * over the other before mirroring is started.
 *
 * If one or both of the units are unlabelled (such as when forming a new
 * mirror or replacing one of the elements of an old one), you must specify
 * the '-l'  (label) flag in front of that unit, otherwise an error is prinetd
 * and the operation is aborted.  If a unit has a valid label, but its
 * modification time (from the indoe) is more recent than the label time, the
 * unit is considered to be unlabelled.
 *
 * If both units are unlabelled, the first one specified is the reference and
 * will be copied over the other.
 *
 * If both units have valid mirror unit labels, but they do not match (as in,
 * they are from different mirrors), and you intend that they now be in the
 * same mirror, you must specify '-l' in front of the unit to relabel.  The
 * other becomes the reference and is copied over the relabelled one.
 *
 * If you are making a new mirror, you can specify a smaller size than this
 * utility would ordinarily use with the '-s <size>' option.  The <size> is
 * specified in disk blocks. If the size you specify is larger than this
 * utility would normally use (which is the size of the smaller partition), an
 * advisory message is printed and your size is ignored.
 *
 * If you are making a new mirror and the two units do not have the same size,
 * an advisory message is printed and the smaller size is used.
 * 
 * The size of each partition is determined by attempting reads on the
 * disk in a binary search pattern.
 * 
 * Before it does any copying, this utility verifies that the units involved in
 * the copy are not involved in other mirrors.
 *
 * NOTE:  if you are labelling a partition that has a bootstrap program on it,
 * mirror will complain unless you use '-b' instead of '-l'.  Mirror will not
 * allow '-b' to be specified except where it is needed.
 *
 * The -r option creates a mirror that has a block zero that is accessable.
 * It does this by offsetting requests.  This is useful for applications, such
 * as databases, where one wants to use a raw partition for I/O.  The -f option
 * restores the default operation of an inaccessable block zero.  This has the
 * advantage that the layout of the mirror units is the same whether mirrored
 * or not, and is useful when file systems are mirrored (because the units can
 * be checked independently). The -d option is useful if an offset/raw type of
 * mirror has one of its units rendered unusable.  Specifying -d instead of the
 * second mirror unit brings the mirror up with only one unit, as a Dmamged
 * mirror .  It DOES NOT PROVIDE ANY OF THE USUAL BENEFITS OF MIRRORING.  It
 * should only be used when the data is otherwise inaccessible.
 */

u_long get_size(), atoul();
char *valloc(), *raw_dev(), *block_dev(), *malloc();
int alphasort();
long time();

static char *dev_t_to_name();

void read_label(), write_label(), copy_unit(), print_mirrors(), check_units();
void pcopy();
dev_t device();
static char *arg0 = (char *)0, *mirror, *unit0, *unit1;
static eaccess;				  /* global for scan_mirror err ret. */
static dev_t dev;			  /* global for select_dev */
static char *name;			  /* global for select_name */
static ushort maj;			  /* global for select_major */
static ushort mirrormajor;		  /* real mirror major number */

shared int next_read, next_write, error;  /* for parallel copy */

#ifdef DEBUG
int nprocs = 5, buffersize = 192;
#endif /* DEBUG */

#ifdef DEBUG
int debug = 1;
#else	/* DEBUG */
int debug = 0;
#endif /* DEBUG */

#define dmsg if (debug) fprintf
#define msg(x) (void)write(1,x,sizeof x - 1) /* for constant messages */

usage()
{
	fprintf( stderr, "Usage: %s\n", arg0);
	fprintf( stderr, "Usage: %s -a\n", arg0);
	fprintf( stderr,
		"Usage: %s /dev/rmr? [-l|-b] unit0 [-l|-b] unit1 [-s size]",
		arg0);
	fprintf( stderr, " [-r] [-f]\nUsage: %s /dev/rmr? unit0 -d\n", arg0);
	exit(1);
}

#define pexit(msg) pexit2(msg, (char *)0)
	
pexit2( msg1, msg2 )
	char *msg1, *msg2;
{
	if ( msg2 )
	{
		fprintf( stderr, "%s: %s ", arg0, msg1);
		perror( msg2 );
	} else {
		fprintf( stderr, "%s: ", arg0);
		perror( msg1 );
	}
	exit(1);
}

main( argc, argv)
	int argc;
	char **argv;
{
	int mr, u0, u1;			  /* file descriptors */
	int l0, l1;			  /* flag memory */
	int ok0 = 0, ok1 = 0;		  /* unit ok to use */
	dev_t ioctl_dev[4];		  /* raw and block for u0, u1 */
	unsigned long size=0, s0, s1, s;  /* sizes */
	dev_t mminor;
	int i;
	struct mirror_label *label0, *label1; /* unit labels */
	struct timeval tp;		  /* time, for labelling. */
	unsigned first_block = 1;	  /* first usable block of mirror */
	int r_or_f = 0;			  /* -r or -f specified */
	int damaged = 0;		  /* damaged operation */
	int nocopy = 0;			  /* don't even copy--fail instead */

	if (arg0)
		nocopy++;		  /* can only happen with -a */

	arg0 = argv[0];

	if (argc == 1)
	{
		/* No args means print all mirrors */
		print_mirrors();
		exit( 0 );
	}

	if (argc == 2 && strcmp("-a", argv[1]) == 0)
		mirrorall();		  /* which calls exit */
	
	if ( ((label0=(struct mirror_label *)valloc(DEV_BSIZE)) == 0 )
	    || ((label1=(struct mirror_label *)valloc(DEV_BSIZE)) == 0 ) )
		pexit( "Not enough memory to read mirror unit labels!" );

	if (argc < 3)
		usage();

	mirror = raw_dev( argv[1] );
	mr = get_mirror( mirror );
	
	for ( i=2, unit0=unit1=NULL, l0=l1=0; i < argc; i++ )
		if ( argv[i][0] == '-' )
		{
			if (strcmp( "-l", argv[i] ) == 0)
			{
				/*  We can force a label on this unit */
				if ( !unit0 )
				{
					if (!l0)
						l0++;
					else	usage();
				}
				else if ( !unit1 )
				{
					if (!l1)
						l1++;
					else	usage();
				}
				else usage();
			}
			else if (strcmp( "-b", argv[i] ) == 0)
			{
				/*  force a label on this bootable unit */
				if ( !unit0 )
				{
					if(!l0)
						l0--;
					else	usage();
				}
				else if ( !unit1 )
				{
					if (!l1)
						l1--;
					else	usage();
				}
				else usage();
			}
			else if (strcmp( "-s", argv[i] ) == 0)
			{
				i++;
				if ( i <argc )
					size = atoul( argv[i] );
				else	size = 0;

				if (size == 0)
					usage();
			}
			else if (strcmp( "-r", argv[i] ) == 0)
			{
				first_block = 0;
				r_or_f = 1;
			}
			else if (strcmp( "-f", argv[i] ) == 0)
			{
				first_block = 1;
				r_or_f = 1;
			}
			else if (strcmp( "-d", argv[i] ) == 0)
				damaged++;
			else usage();
		}
		else if ( !unit0 )
			unit0 = raw_dev( argv[i] );
		else if ( !unit1 )
			unit1 = raw_dev( argv[i] );
		else usage();

	if ( damaged )
	{
		if ( unit1 || !unit0 || l0 != 0 || l1 != 0 || r_or_f
		    || damaged > 1)
			usage();
		/* damaged usage is restricted to existing mirrors */
		fprintf( stderr,
			"%s: WARNING: %s IS NOT BEING MIRRORED\n",
			arg0, unit0);
		ioctl_dev[U1BLOCK] = ioctl_dev[U1RAW] = -1;
	}
	else if ( !unit1 )		  /* check we have both units! */
		usage();

	u0 = get_unit( unit0, ioctl_dev );
	if (!damaged)
		u1 = get_unit( unit1, ioctl_dev+2 );

	if ( ioctl_dev[U0BLOCK] == ioctl_dev[U1BLOCK]
	    || ioctl_dev[U0RAW] == ioctl_dev[U1RAW] )
	{
		fprintf( stderr, "%s: cannot mirror a unit to itself!\n",
			arg0);
 		exit( 1 );
	}

	/*
	 * Check_units checks for any of the intended mirror units being used
	 * in another mirror.  If it finds a problem, it exits, so if it
	 * returns, everything is o.k.. A file descriptor that points to an
	 * open mirror is necessary for this (mr).
	 */
	check_units(mr, ioctl_dev);

       	/*
	 * At this point, mr is a file descriptor to an unused mirror device,
	 * u0 and u1 are file descriptors to disk units, and l0 and l1 are
	 * either -1, 1 or 0, depending if the -b flag, the -l flag, or neither
	 * was specified (respectively) for u0 or u1 (respectively), and size
	 * is set to the size the user requested, if any.  
	 */
	
	mminor = device(mirror, S_IFCHR); /* still a full dev_t */
	mirrormajor = major(mminor);
	mminor = minor(mminor);		  /* now, just a minor */
	
	if (major(ioctl_dev[U0RAW]) == mirrormajor
	    || major(ioctl_dev[U1RAW]) == mirrormajor)
	{
		fprintf( stderr, "%s: cannot mirror a mirror!\n", arg0);
		exit( 1 );
	}

	s0 = get_size( u0 );
	s1 = damaged? s0 : get_size( u1 );
	
	/* read and verify labels */
	
	if ( verify_label( u0, label0, s0, l0, unit0 ) )
	{
		ok0++;
		s0 = label0->blk_break - label0->first_blk
			+ label0->label_size; 
	}

	if ( damaged && !ok0 )
	{
		fprintf( stderr, "%s: ERROR: mirror unit %s is BAD\n",
			arg0, unit0);
		exit( 1 );
	}

	if ( damaged )
		ok1++;			  /* damaged gives false positive */
	else if ( verify_label( u1, label1, s1, l1, unit1) )
	{
		ok1++;
		s1 = label1->blk_break - label1->first_blk
			+ label1->label_size;
	}

	/* set sizes we will use */
	s = s0<s1 ? s0 : s1;
	if (size)
		s = size<s? size : s;

	dmsg( stderr,
	     "%s: debug: mr=%d u0=%d u1=%d, mminor=%d, s=%d s0=%d s1=%d\n",
	     arg0, mr, u0, u1, mminor, s, s0, s1);
	dmsg( stderr, "%s: debug: u0r 0x%x u0b 0x%x u1r 0x%x u1b 0x%x\n",
	     arg0, ioctl_dev[U0RAW], ioctl_dev[U0BLOCK], ioctl_dev[U1RAW],
	     ioctl_dev[U1BLOCK]); 

	/*
	 * We now know what each disk thinks of itself.  We need to know if one
	 * of them is out of date, what the current unit thinks of it
	 */
	if (!damaged && compare_labels(label0, label1) == LABELS_DISAGREE)
	{
		/* Labels are for same mirror, but one is out of date. */
		if ( label0->label_time > label1->label_time )
			ok1 = 0;	  /* Trust label0 */

		else	ok0 = 0;	  /* trust label1 */

	}

	if ( ok0+ok1 == 0 )
	{
		if ( l0==0 || l1==0 )
		{
			/*
			 * This can happen if both units are out of date by
			 * virtue of direct writes to the raw devices of a
			 * unit, bypassing mirroring, or both units failing
			 * with mirroring on 
			 */
			fprintf( stderr,
				"%s: %s and %s are bad/out-of-date-- use -l\n",
				arg0, unit0, unit1);
			exit( 1 );
		}

		/*
		 * Don't need to check for nocopy, as that is only set when we
		 * call ourself to process "-a", and both l0 and l1 will be
		 * zero 
		 */

		/* neither unit is o.k. as is, write label */
		if ( s != s0    ||    s != s1)
			fprintf( stderr,
				"%s: Warning: size %s=%d %s=%d, using %d\n",
				arg0, unit0, s0, unit1, s1, s);

		if ( s < size )
		{
			fprintf( stderr,
				"%s: Warning: -size %d too large, using %d\n",
				arg0, size, s);
		}

		/* Both units bad--label unit0 and pretend it's "good" */
		if ( gettimeofday( &tp, (struct timezone *)0 ) )
			pexit( "Can't get current time" );

		label0->mirror_magic = MIRROR_MAGIC;
		label0->label_time = tp.tv_sec;
		label0->mminor = mminor;
		label0->unit = 0;
		label0->label_size = 1;
		label0->first_blk = first_block;
		label0->blk_break = s - (label0->label_size - first_block);
		label0->bmap_density = M_BMAP_DENSITY;
		label0->nunits = 2;
		label0->bmap0 = 0;
		label0->bmap1 = 0xff;

		write_label( u0, label0, unit0 );
		s0 = s;
		ok0++;
	} else  {
		if ( size )
			fprintf( stderr,
				"%s: Warning: -size only useful on new %s",
				arg0, "mirrors, using size from label\n");
		if ( r_or_f )
			fprintf( stderr,
				"%s: Warning: -%c only useful on new %s",
				arg0, first_block? 'f' : 'r',
				"mirrors, mirror type set per label.\n");
	}

	/* At this point, at most one unit is "bad" */
	if ( ok0 == 0 )
	{
		if ( s1 > s0 )
		{
			fprintf( stderr,
				"%s: can't copy %s=%d to %s=%d (unit=size)\n",
				arg0, unit1, s1, unit0, s0);
			exit( 1 );
		}
		if (nocopy)
		{
			fprintf( stderr,
				"%s: can't copy %s to %s with \"-a\"\n",
				arg0, unit1, unit0);
			exit( 1 );
		}
		copy_unit( u1, u0, unit1, unit0 );
	}
 	if ( ok1 == 0 )
	{
		if ( s0 > s1 )
		{
			fprintf( stderr,
				"%s: can't copy %s=%d to %s=%d (unit=size)\n",
				arg0, unit0, s0, unit1, s1);
			exit( 1 );
		}
		if (nocopy)
		{
			fprintf( stderr,
				"%s: can't copy %s to %s with \"-a\"\n",
				arg0, unit0, unit1);
			exit( 1 );
		}
		copy_unit( u0, u1, unit0, unit1 );
	}

	read_label( u0, label0, unit0 );
	if (!damaged)
		read_label( u1, label1, unit1 );

	/* cross verify labels */
	if (!damaged) switch ( compare_labels( label0, label1 ) )
	{
	case LABELS_MISMATCHED:		  /* units from different mirrors */
		if ( l0+l1 != 1 )
		{
			/* Needed exactly one unit labeled */
			fprintf(stderr, "%s: Mirror unit mismatch\n", arg0);
			exit( 1 );
		}

		/* get physical size of unit we're over-writing */
		if ( l0 )
			s0 = get_size( u0 );
		else	s1 = get_size( u1 );

		/* Now, check to be sure copy can happen! */
		if (l0  && s1>s0)
		{
			fprintf(stderr,
				"%s: Can't copy %s=%d to %s=%d (unit=size)\n",
				arg0, unit1, s1, unit0, s0);
			exit( 1 );
		}
		else	if (l1  && s0>s1)
		{
			fprintf(stderr,
				"%s: Can't copy %s=%d to %s=%d (unit=size)\n",
				arg0, unit0, s0, unit1, s1);
			exit( 1 );
		}
		else	if (nocopy)
		{
			fprintf(stderr,
				"%s: Can't copy %s to %s with \"-a\"\n",
				arg0, l0? unit1 : unit0, l0? unit0 : unit1);
			exit( 1 );
		}
		else	fprintf(stderr,
				"%s: Warning: copying %s over %s\n",
				arg0, l0? unit1 : unit0, l0? unit0 : unit1);

		if ( l0 )
			copy_unit( u1, u0, unit1, unit0 );
		else	copy_unit( u0, u1, unit0, unit1 );
		break;
	case LABELS_DISAGREE:		  /* unit out of date, same mirror */
		if (nocopy)
		{
			fprintf(stderr,
				"%s: can't update %s with \"-a\"\n",
				arg0,
				label0->label_time > label1->label_time?
				unit1 : unit0);
			exit( 1 );
		}
		if ( label0->label_time > label1->label_time )
			copy_unit( u0, u1, unit0, unit1 );
		else	copy_unit( u1, u0, unit1, unit0 );
		break;
	case LABELS_AGREE:
		/* If we get here, we know that label0 and label1 agree */
		if (label0->bmap0 && !label0->bmap1)
		{
			if (nocopy)
			{
				fprintf( stderr,
					"%s: can't update %s with \"-a\"\n",
					arg0, unit0);
				exit( 1 );
			}
			copy_unit( u1, u0, unit1, unit0);
		}
		if (!label0->bmap0 && label0->bmap1)
		{
			if (nocopy)
			{
				fprintf( stderr,
					"%s: can't update %s with \"-a\"\n",
					arg0, unit1);
				exit( 1 );
			}
			copy_unit( u0, u1, unit0, unit1);
		}
		break;
	}

	/* Now, all units are o.k., so start up the mirror! */
	if ( ioctl( mr, MIOCON, (char *)ioctl_dev) )
		pexit( "cannot start mirroring" );

	exit(0);			  /* success */
}

/*
 * Given a character string consisting of whitespace and decimal digits only,
 * return an unsigned long integer.
 */
u_long
atoul(c)
	char *c;
{
	unsigned long result = 0;

	/* Skip white space */
	while ( *c == '\t'   ||   *c == ' ' )
		c++;

	while ( *c >= '0'    &&    *c <= '9' )
		result = 10*result + *c++ - '0';

	return result;
}

/*
 * Given a file descriptor, use check_size to find the size of the partition.
 */
u_long
get_size( fd )
{
	unsigned long min, max, trial;
	int result;
	
	min = 1;
	max = trial = 1<<18;
	while ( (result = check_size( fd, trial )) != 0 )
	{
		if ( result < 0 )
		{
			if ( min == trial )
				return 0;
			max = trial;
			trial = (max + min)/2;
		}
		else
		{
			min = trial;
			if ( max == trial )
				trial = max = 2 * max;
			else 	trial = (max + min + 1)/2;
		}
	}
	return trial;
}

/*
 * Given a file descriptor to an open file and a size, determines if the file
 * (typically a raw device) has a greater, lesser or equal number of blocks in
 * it as specified by size.  The file should be an exact number of blocks long.
 * Returns 0 if size is correct, -1 if the actual size is smaller than the
 * given size, +1 if the actual size is greater than the given size.
*/
check_size( fd, size )
	int fd;
	unsigned long size;
{
	int readsize;
	static char *buffer;

	if ( ! buffer && ((buffer = valloc( 2 * DEV_BSIZE)) == 0 ) )
		return -1;

	if (  lseek( fd, (off_t)((size-1)*DEV_BSIZE), L_SET ) == -1 )
		return -1;

	readsize = read( fd, buffer, 2*DEV_BSIZE );
	if ( readsize < DEV_BSIZE )
		return -1;
	else if ( readsize < 2*DEV_BSIZE )
		return 0;
	else	return 1;
}

/*
 * Open the mirror unit with the given name.  If the open is successful, and
 * the mirror is not currently in use, return the file descriptor of the open.
 * Otherwise, return -1.  
 */

get_mirror( name )
	char *name;
{
	int fd;
	struct mioc info;

	fd = open( name, O_RDWR | O_NDELAY );

	if (fd < 0)
		pexit2( "Cannot open mirror", name );

	if ( ioctl(fd, MIOCINFO, (char *)&info) == -1 )
	{
		fprintf( stderr, "%s: %s is not a mirror\n", arg0, name);
		exit(1);
	}	

	/*
	 * We no long check for active mirrors here--that's done in
	 * check_units. The check is there because if the mirror is in use but
	 * is mirroring the two partitions that we are requesting it start now,
	 * it is only a warning, not an error.
	 */ 
	errno = 0;
	return fd;
}

get_unit( name, unit )
	char *name;
	dev_t unit[2];
{
	int fd;

	unit[0] = device(name, S_IFCHR);
		
	if ( (fd = open( name, O_RDWR)) < 0 )
		pexit2( "Cannot open unit", name );

	name = block_dev( name );

	unit[1] = device(name, S_IFBLK);

	return fd;
}

/*
 * verify_label()
 * read in and verify the label on this presumed unit of a mirror. If
 * this is a valid mirror label that Dynix/3.0 mirroring can handle,
 * returns 1. Otherwise, if silent is non-zero, return 0, otherwise
 * produce error message and exit.
 */
verify_label( fd, label, size, silent, name )
	int fd;
	struct mirror_label *label;
	unsigned long size;
	int silent;
	char *name;
{
	struct stat info;

	/* first, we need to read the label. */
	read_label( fd, label, name );

	if ( silent<0 || ((struct exec *)label)->a_magic==SMAGIC )
	{
		if ( silent >= 0 )
		{
			fprintf( stderr,
				"%s: Error: Cannot mirror %s, %s\n",
				arg0, name, "unit is bootable.");
			exit( 1 );
		}
		
		if ( ((struct exec *)label)->a_magic != SMAGIC )
		{
			fprintf( stderr,
				"%s: Error: -b used on %s, %s\n",
				arg0, name, "which is not bootable.");
			exit( 1 );
		}
	}

	if ( fstat( fd, &info) < 0 )
		pexit2( "Cannot stat", name );

	if ( label->mirror_magic != MIRROR_MAGIC
	    || label->unit > 1
	    || label->label_size != 1
	    || label->first_blk > 1
	    || label->blk_break-label->first_blk+label->label_size  > size
	    || label->bmap_density != M_BMAP_DENSITY
	    || label->nunits != 2 )
	{
		if ( !silent )
		{
			fprintf( stderr, "%s: unit %s, bad mirror label\n",
				arg0, name);
			exit( 1 );
		}
		else	return 0;
	}
	/*
	 * This is a real mirror unit.  Check if its good.  If the unit is bad
	 * at this point, it doesn't need the -l flag to fix it.
	 */
	if (label->unit == 0  && label->bmap0)
		return 0;
	if (label->unit == 1  && label->bmap1)
		return 0;
	if ( label->label_time < info.st_mtime )
		return 0;

 	return 1;
}

/*
 * Compare the two labels.  If they are units of the same mirror, and they are
 * equally up-to-date, return LABELS_AGREE (0).  If they are *not* from the
 * same mirror, return LABELS_MISMATCHED (-1).  If they are from the same
 * mirror, but one is newer, return LABELS_DISAGREE (1).
*/
compare_labels ( a, b )
	struct mirror_label *a, *b;
{
	if ( a->mminor != b->mminor
	    || a->unit == b->unit
	    || a->first_blk != b->first_blk
	    || a->blk_break != b->blk_break )
		return( LABELS_MISMATCHED );
	/*
	 * If they are units of the same mirror, but one is out of date, a
	 * simple compare of the label times will tell which one is the better
	 * one to keep.
	 */
	if ( a->label_time != b->label_time )
		return( LABELS_DISAGREE );

	/*
	 * If the bitmaps don't match but the timestamps do, they are not units
	 * of the same mirror after all.
	 */
	if ( a->bmap0 != b->bmap0
	    || a->bmap1 != b->bmap1 )
		return( LABELS_MISMATCHED );

	return( LABELS_AGREE );
}

/*
 * Copy unit a to unit b.  Makes label changes in the process.
 */
void
copy_unit( from, to, fromname, toname )
	char *fromname, *toname;
{
	static char *buffer = NULL;
	static struct mirror_label *l;
	unsigned long size;
	struct timeval tp;		  /* time, for labelling. */
			

	if ( !buffer )
	{
		if ( (buffer = valloc( (unsigned)COPY_BUFSIZE )) == 0  )
			pexit( "No memory for copy");
		l = (struct mirror_label *)buffer;
	}

	printf( "Updating out-of-date unit %s\n", toname);

	read_label(from, l, fromname);
	size = l->blk_break - l->first_blk + l->label_size;

	/*
	 * First, copy as is.  We correct the labels at the very end, in case
	 * we don't make it.  Change unit, though, otherwise it will look like
	 * it's from a different mirror.  Also, zero time of "to" unit, so if
	 * we're interrupted in the middle, it will be properly updated next
	 * time. UPM needs to be 2 for this to always work.
	 */
	l->unit = (l->unit + 1)%UPM;

	l->label_time = 0;

	write_label( to, l, toname);

	printf( "Copying %s to %s--please wait...", fromname, toname);
	(void)fflush( stdout );

	pcopy(fromname, toname, 1, size); /* copy from block 1 to size */

	/* correct both labels */
	read_label( from, l, fromname);

	if ( gettimeofday( &tp, (struct timezone *)0 ) )
		pexit( "Can't get current time" );

	l->label_time = tp.tv_sec;

	if ( l->unit == 0 )
		l->bmap1 = l->bmap0;
	else	l->bmap0 = l->bmap1;

	write_label( from, l, fromname );
	
	/* UPM needs to be 2 for this to always work. */
	l->unit = (l->unit + 1)%UPM;

	write_label( to, l, toname );
}

char *
raw_dev( dev )
	char *dev;
{
	char *result, *r, last, next;

	/*
	 * alloc memory for new string, which may be one longer than the old.
	 * Remember that strlen doesn't count the null at the end, but we must.
	 * Thus, we allocate a chunk that is two larger than the string length.
	 */
	if ( (result = malloc((unsigned)(strlen(dev) + 2))) == NULL )
		pexit( "Out of memory");
	(void)strcpy(result, dev);

	if ( (r=rindex(result, '/')) == NULL )
		r = result-1;
	r++;

	if ( *r != 'r' )
	{
		for ( last='r', next= *r; last; *r=last, last=next, next= *++r)
			;
		*r = last;
	}
	return result;
}

char *
block_dev( name )
	char *name;
{
	char *result, *r;


	/* Alloc a chunk as long as the string + 1 for the terminating null */
	if ( (result = malloc((unsigned)(strlen(name) + 1))) == NULL )
		pexit( "Out of memory");

	(void)strcpy(result, name);

	if ( (r=rindex(result, '/')) == NULL )
		r = result-1;
	r++;

	if ( *r == 'r' )
		for ( ; *r; r++ )
			*r = *(r+1);

	return result;
}

dev_t
device( name, type )
	char *name;
	int type;
{
	struct stat info;

	if ( stat( name, &info ) < 0 )
		pexit2( "Cannot stat", name );

	if ( (info.st_mode & S_IFMT) != type )
	{
		fprintf( stderr, "%s: %s wrong device type\n",
			arg0, name);
		usage();
	}
	
	return info.st_rdev;
}

/*
 * read_label()
 * Reads the label from the unit that the file descriptor fd references into
 * the buffer "label" supplied.  Exit on failure.
 */
void
read_label( fd, label, name )
	int fd;
	struct mirror_label *label;
	char *name;
{
	if ( lseek(fd, (off_t)0, L_SET) == -1
	    || read(fd, (char *)label, DEV_BSIZE) < ML_SIZE )
		pexit2( "Cannot read label from", name );
}

/*
 * write_label()
 * write label writes the given label to the unit that the file descriptor fd
 * references. It makes sure that the label time matches the modified time in
 * the inode of the unit.
 */
void
write_label( fd, label, name )
	int fd;
	struct mirror_label *label;
	char *name;
{
	struct timeval times[2];

	times[0].tv_sec = label->label_time;
	times[1].tv_sec = label->label_time;
	times[0].tv_usec = 0;
	times[1].tv_usec = 0;

	if ( lseek(fd, (off_t)0, L_SET) == -1
	    || write(fd, (char *)label, DEV_BSIZE) < ML_SIZE
	    || utimes( name, times ) == -1  ) {
		(void)utimes( name, (struct timeval *)0 ); /* touch */
		pexit2( "Cannot write label to", name );
	}
}

/*
 * check_units()
 * takes an open file descriptor to a mirror and the array of devs that will be
 * used in the MIOCON ioctl and tests that the units are free for mirroring
 * a la MIOCVRFY in the kernel. The mirror scanning is done here (although it
 * is also done in MIOCVRFY) in order to provide more informative error
 * messages. 
 */
void
check_units(fd, devs)
	int fd;
	dev_t *devs;
{
	struct mioc mioc;
	struct stat info;
	dev_t mrdev;
	int i, imax, j;
	int damaged = 0;
	
	if (fstat(fd, &info) < 0)
		pexit("cannot stat open mirror");
	mrdev = info.st_rdev;

	if (devs[U1RAW] == -1 && devs[U1BLOCK] == -1)
		damaged = 1;

	if (ioctl(fd, MIOCINFO, (caddr_t)&mioc))
		pexit( "ioctl failed on mirror");
	
	if (mioc.mirror.active == ACTIVE
	    && ((mioc.unit[0].block == devs[U0BLOCK]
		 && mioc.unit[0].raw == devs[U0RAW]
		 && mioc.unit[1].block == devs[U1BLOCK]
		 && mioc.unit[1].raw == devs[U1RAW])
		|| (mioc.unit[1].block == devs[U0BLOCK]
		    && mioc.unit[1].raw == devs[U0RAW]
		    && mioc.unit[0].block == devs[U1BLOCK]
		    && mioc.unit[0].raw == devs[U1RAW])))
	{
		fprintf(stderr,
			"%s: Warning: %s/%s already mirroring %s/%s & %s/%s\n",
			arg0, DEV, dev_t_to_name(mrdev),
			DEV, dev_t_to_name(devs[U0RAW]),
			DEV, dev_t_to_name(devs[U1RAW]));
		exit(0);		  /* not an error */

	}

	if (mioc.mirror.active != INACTIVE || mioc.mirror.open != 1) {
		pexit( "mirror busy");
	}
	imax = mioc.limit;
	for ( i = 0; i < imax; i++ )
	{
		mioc.limit = i;
		if (ioctl(fd, MIOCOINFO, (caddr_t)&mioc))
			pexit( "ioctl failed on mirror");

		for ( j = 0; j < UPM; j++ )
		{
			if ( mioc.mirror.active != ACTIVE )
				continue;

			if ( devs[U0RAW] == mioc.unit[j].raw
			    || devs[U0BLOCK] == mioc.unit[j].block)
			{
				(void)chdir(DEV);
				fprintf(stderr,
					"%s: %s/%s busy in another mirror\n",
					arg0, DEV, dev_t_to_name(devs[U0RAW]));
				exit( 1 );
			}

			if ( !damaged
			    && (devs[U1RAW] == mioc.unit[j].raw
				|| devs[U1BLOCK] == mioc.unit[j].block))
			{
				(void)chdir(DEV);
				fprintf(stderr,
					"%s: %s/%s busy in another mirror\n",
					arg0, DEV, dev_t_to_name(devs[U1RAW]));
				exit( 1 );
			}
		}
		if (ioctl(fd, MIOCVRFY, (caddr_t)devs))
		{
			if (errno == EBUSY)
				pexit("unit is mounted");
			else	if (errno == EPERM)
				pexit2("Must be super user to mirror:",
				       mirror); 
			else	pexit(mirror);
		}
	}
	return;
}

/*
 * select_name()
 * select_name is used for scandir.  As such, it can only take one arg.  It
 * uses the global char *name as the prefix of the name for which we search.
 */
static
select_name(dir)
	struct direct *dir;
{
	return (strncmp( name, dir->d_name, strlen(name) ) == 0);
}

/*
 * select_major()
 * This routine is also for scadir, and also can only take a single arg.  It
 * searches for character special files with a major device number matching the
 * one in the global maj.
 */
static
select_major(dir)
	struct direct *dir;
{
	static struct stat statbuf;

	if ( stat(dir->d_name, &statbuf) )
		return 0;		  /* stat failed, don't bother */
	
	return ((statbuf.st_mode & S_IFMT) == S_IFCHR
		&& major(statbuf.st_rdev) == maj);
}

/*
 * select_dev()
 * Another scan_dir special, requiring the global dev_t dev to match the entry
 * for which we are scanning.
 */
static
select_dev(dir)
	struct direct *dir;
{
	static struct stat statbuf;
	char *name;

	if ((name = malloc(sizeof DEV + sizeof "/" + dir->d_namlen)) == NULL)
		return 0;		  /* out of memeory */

	(void)strcpy(name, DEV);
	(void)strcat(name, "/");
	(void)strcat(name, dir->d_name);

	if ( stat(name, &statbuf) )
	{
		/* stat failed, don't bother */
		free(name);
		return 0;
	}
	free(name);
	return ((statbuf.st_mode & S_IFMT) == S_IFCHR
		&& statbuf.st_rdev == dev);
}

/*
 * dev_t_to_name()
 * Given a dev_t, find a matching name in the DEV directory and return a
 * pointer to it.  Changes the global dev_t dev. DEV is ordinarily "/dev".
 */
static char *
dev_t_to_name(raw)
	dev_t raw;
{
	struct direct **namelist;
	int imax;

	dev = raw;

	imax = scandir(DEV, &namelist, select_dev, alphasort);
	return((imax>0)? namelist[0]->d_name : "");
}

/*
 * scan_mirrors()
 * Given the name of a valid mirror, print out the usual mirror info for all
 * mirrors configured into the kernel.  Expects to be in the DEV directory
 * (normally "/dev"), and the name to be a path relative to this.
 */
static
scan_mirrors(name)
	char *name;
{
	int fd, index, maxindex;
	struct stat s;
	struct mioc mioc;
	static char *active[] = {
		"inactive", "changing", "active", "shutdown",
	};
		

	if ((fd = open(name, O_RDONLY)) < 0 ) {
		if (errno == EACCES)
		{
			fprintf(stderr, "%s: ", arg0);
			perror(name);
			eaccess = 1;	  /* access problem, global return. */
		}

		return  0;
	}
	if ( ioctl(fd, MIOCINFO, (char *)&mioc) )
	{
		(void)close(fd);
		return 0;
	}

	if (fstat(fd, &s))
	{
		fprintf(stderr, "%s: Cannot fstat open mirror %s/%s", arg0,
			DEV, name);
		mirrormajor = -1;
	} else	mirrormajor = major(s.st_rdev);
	
	maxindex = mioc.limit;
	for ( index = 0; index < maxindex ; index++ )
	{
		mioc.limit = index;
		
		if ( ioctl(fd, MIOCOINFO, (char *)&mioc) )
		{
			printf("%s: Cannot get info on mirror %d\n", arg0,
			       index); 
			continue;
		}
		
		name = dev_t_to_name(makedev(mirrormajor, index));
		
		if ( strlen(name) )
			printf("Mirror%d %s/%s %s", index, DEV, name,
			       active[mioc.mirror.active]);
		else	printf("Mirror%d (no inode) %s", index,
			       active[mioc.mirror.active]);
		
		if (mioc.mirror.active == ACTIVE)
		{
			printf( " (%s) on %s/%s %s",
			       mioc.label.first_blk? "fs" : "raw", DEV,
			       dev_t_to_name(mioc.unit[0].raw), 
			       mioc.unit[0].ok? "(OK)" : "(BAD)");
			printf( " %s/%s %s", DEV,
			       dev_t_to_name(mioc.unit[1].raw), 
			       mioc.unit[1].ok? "(OK)" : "(BAD)");
		}
		printf(" %d open%s %s\n", mioc.mirror.open,
		       mioc.mirror.open==1? "" : "s" );
	}
	(void)close(fd);
	return 1;
}

/*
 * print_mirrors()
 * cd to DEV (usually "/dev") and find a mirror in that directory, then call
 * scan_mirrors to print the usual mirror statistics.
 */
static void
print_mirrors()
{
	struct direct **namelist;
	struct stat statbuf;
	int i, imax;

	if (stat(DEV, &statbuf) || chdir(DEV))
	{
		printf("%s: Cannot cd to %s\n", arg0, DEV);
		return;
	}

	eaccess = 0;
	/* try scanning by name first, major number last */
	name = MIRRORNAME;
	imax = scandir(".", &namelist, select_name, alphasort);
	for ( i = 0; i < imax; i++ )
		if ( scan_mirrors(namelist[i]->d_name) )
			return;

	maj = MIRRORMAJOR;
	imax = scandir(".", &namelist, select_major, alphasort);
	for ( i = 0; i < imax; i++ )
		if ( scan_mirrors(namelist[i]->d_name) )
			return;

	fprintf( stderr, "%s: cannot find any %smirrors in %s\n", arg0,
		eaccess? "openable " : "", DEV);
}

mirrorall()
{
	FILE *mrtab;
	char name[MAXPATHLEN+1], *argv[4];
	int c, word, errors = 0;

	if ( (mrtab = fopen( MRTAB, "r")) == NULL )
		pexit2(MRTAB, "cannot open");

	name[MAXPATHLEN] = '\0';
	word = 0;
	argv[0] = arg0;			  /* local argv! */

	/* Initial deblank */
	if (!isspace(c = readss(mrtab)))
		(void)ungetc(c, mrtab);

	while ( TRUE )
		switch(readword(name, MAXPATHLEN, mrtab))
		{
		case 0:
			/* word too long */
			errors++;
			fprintf(stderr,
				"%s: %s: %s name >%d bytes, entry ignored: %s",
				arg0, MRTAB, word? "unit" : "mirror",
				MAXPATHLEN, name);
			/* read all of word */
			while(!(c = readword(name, MAXPATHLEN, mrtab)))
				fprintf(stderr, "%s", name);
			fprintf(stderr,"%s\n", name);
			/* read rest of line... */
			if (c < 2)
				while( readword(name, MAXPATHLEN, mrtab) < 2)
					;
			word = 0;
			break;
		case 1:
			/* normal word */
			argv[word+1] = raw_dev(name); /* get it in any case */
			if (word < 2)
				word++;
			else
			{
				errors++;
				/* extra args--word 2 should end the line */
				fprintf(stderr,
					"%s: %s: %s extra args ignored.\n",
					arg0, MRTAB, argv[1]);
				/* read rest of line */
				while (readword(name, MAXPATHLEN, mrtab) < 2)
					;
				/* now, do it */
				errors += mirrorone(errors, argv);
				word = 0;
			}
			break;
		case 2:
			/* EOL */
			argv[word+1] = raw_dev(name);
			if (word == 2)
				errors += mirrorone(errors, argv);
			else
			{
				errors++;
				fprintf(stderr,
					"%s: %s: %s unexpected end of line\n",
					arg0, MRTAB, argv[1]);
			}
			word = 0;
			break;
		case 3:
			/* EOF */
			if (word == 0)
			{
				if (errors & ~0xff)
					exit(0xff);
				else	exit(errors);
			}

			if (word == 2)
			{
				argv[3] = raw_dev(name);
				errors += mirrorone(errors, argv);
			}
			else
			{
				errors++;
				fprintf(stderr,
					"%s: %s: unexpected end of file\n",
					arg0, MRTAB);
			}
			(void)fclose(mrtab);
			if (errors & ~0xff)
				exit(0xff);
			else	exit(errors);
			break;
		}
}

/*
 * mirrorone(errors, argv)
 * This routine forks and calls main again with the new argv passed.  This argv
 * always has 4 args--0 = arg0, 1 = mirror, 2 = unit0 and 3 = unit1.  This
 * routine will exit if it can't fork or can't wait for the child.  A non-zero
 * return code indicates that the child failed, and when we exit, we should
 * have a non-zero exit status. A return code of 0 indicates success. 
 */
mirrorone(errors, argv)
	char **argv;
{
	union wait status;

	(void)fflush(stdout);
	(void)fflush(stderr);
	dmsg(stdout, "\t%s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
	switch(fork())
	{
	case -1:
		fprintf(stderr, "%s: ", arg0);
		perror("fork");
		++errors;
		if (errors & ~0xff)
			exit(0xff);
		else	exit(errors);
		break;
	case 0:
		main(4, argv);		  /* child, will exit */
		break;
	default:
		if (wait(&status) == -1)
		{
			fprintf(stderr, "%s: ", arg0);
			perror("wait");
			++errors;
			if (errors & ~0xff)
				exit(0xff);
			else	exit(errors);
		}
		dmsg(stdout, "\texit(%d)\n", status.w_retcode);
		return status.w_status? 1 : 0;
		break;
	}

	/*
	 * This next statement keeps lint happy.  Lint doesn't know that you
	 * can't ever get to this statement.
	 */
	return errors;
}

/*
 * readss(stream)
 * Readss returns a single character, much like getc.  The difference is that
 * all adjacent white space is reduced to a single character. EOF is considered
 * "whitespace". Any white space including an eof is returned as eof, any not
 * including eof but including a new line is returned as newline, and all other
 * white space is returned as a space. A '#' is considered to start a comment,
 * which ends at the first newline or EOF.  This is considered  whitespace.
 */
static
readss(stream)
       FILE *stream;
{
	int c, newline = 0, comment = 0, eof = 0;

	c = getc(stream);
	if (c == '#')
		comment = 1;

	if (isspace(c) || comment)
	{
		do
		{
			switch(c)
			{
			case '\n':
				newline = 1;
				comment = 0;
				break;
			case EOF:
				eof = 1;
				comment = 0;
				break;
			}

			c = getc(stream);
			if (c == '#')
				comment = 1;
			
		}
		while (isspace(c) || comment);
	
		if (c != EOF)
			(void)ungetc(c, stream);
		else	eof = 1;

		if (eof) 		return EOF;
		else if (newline)	return '\n';
		else			return ' ';
	}
	return c;
}

/*
 * readword(buffer, size, stream)
 * readword reads a word (delimited by white space or comments) into the buffer
 * specified.  If the size of the word exceeds size, it returns 0.  If the word
 * ends the line, it returns 2.  If the word ends the file, it returns 3.
 * Otherwise, it returns 1. 
 */
static
readword(buffer, size, stream)
	char *buffer;
	FILE *stream;
{
	int done = 0, c;

	while(size--)
		switch(*buffer = readss(stream))
		{
		case EOF:
			done++;
			/*FALLTHROUGH*/
		case '\n':
			done++;
			/*FALLTHROUGH*/
		case ' ':
			done++;
			*buffer = '\0';
			return done;
			break;
		default:
			buffer++;
		}

	/* out of buffer space, check for end of word */
	switch (c = readss(stream))
	{
	case EOF:
		return 3;
	case '\n':
		return 2;
	case ' ':
		return 1;
	default:
		(void)ungetc(c, stream);
		return 0;
	}
}

void
nthcopy(fromname, toname, start_blk, nblks, nth)
	char *fromname, *toname;
	unsigned long nblks;
	int start_blk, nth;
{
	int next_proc, fdi, fdo, blk, bufsize;
	char *buffer;

	next_proc = (nth + 1)%NPROCS;
	bufsize = COPY_BUFSIZE/DEV_BSIZE; /* bufsize in blocks */

	if (!error && (buffer = valloc((unsigned)COPY_BUFSIZE)) == NULL)
	{
		error = 1;
		fprintf(stderr, "%s(%d): Unable to allocate buffer\n", arg0,
			nth);
		return;
	}

	if (!error && (fdi = open(fromname, O_RDONLY)) < 0)
	{
		error = 1;
		fprintf(stderr, "%s(%d): %s: Open failed.\n", arg0, nth,
			fromname);
		return;
	}

	if (!error &&(fdo = open(toname, O_WRONLY)) < 0)
	{
		error = 1;
		fprintf(stderr, "%s(%d): %s: Open failed.\n", arg0, nth,
			toname);
		return;
	}

	for (blk = start_blk + nth*bufsize; blk < nblks; blk += NPROCS*bufsize)
	{
		if (error)
			break;

		if (!error && lseek(fdi, blk * 512, 0) < 0)
		{
			error++;
			fprintf(stderr,
				"%s(%d): lseek error block %d ",
				arg0, nth, blk);
			perror( fromname );
		}

		if (!error && lseek(fdo, blk * 512, 0) < 0)
		{
			error++;
			fprintf(stderr,
				"%s(%d): lseek error block %d ",
				arg0, nth, blk);
			perror( toname );
		}

		if ( blk + bufsize > nblks )
			bufsize = nblks - blk; /* correction for last read */

		while( next_read != nth && !error)
			;		  /* wait my turn */
		
		next_read = next_proc;	  /* let next guy go */
		
		if (!error && read(fdi, buffer, bufsize*512) != bufsize*512)
		{
			error++;
			fprintf(stderr, "%s(%d): read error on block %d ",
				arg0, nth, blk);
			perror(fromname);
		}

		while( next_write != nth && !error)
			;		  /* wait my turn */

		next_write = next_proc;	  /* let next guy go */

		if (!error && write(fdo, buffer, bufsize*512) != bufsize*512)
		{
			error++;
			fprintf(stderr, "%s(%d): write error on block %d ",
				arg0, nth, blk);
			perror(toname);
		}

		if (blk/DOT_BLOCKS != (blk + bufsize)/DOT_BLOCKS)
		{
			/* crossed a DOT_BLOCKS boundary */
			if ((blk/DOT_BLOCKS)%25 == 0)
				msg("\n.");
			else	msg(".");
		}
	}
	(void)close(fdi);
	(void)close(fdo);
	return;
}

void
pcopy(fromname, toname, startblk, nblks)
	char *fromname, *toname;
	unsigned long nblks;
{
	int i, child;
	long t, t2;

#ifdef DEBUG
	int fd;
	char buf[81];
	buf[81] = '\0';
	
	if ( (fd = open(".n", O_RDONLY)) >= 0
	    && read(fd, buf, 80) > 0
	    && atoi(buf) > 0)
	{
		nprocs = atoi(buf);
		(void)close(fd);
	}

	if ( (fd = open(".s", O_RDONLY)) >= 0
	    && read(fd, buf, 80) > 0
	    && atoi(buf) > 0)
	{
		buffersize = atoi(buf);
		(void)close(fd);
	}
#endif DEBUG
	next_read = next_write = error = 0;

	(void)fflush(stdout);
	(void)fflush(stderr);
	(void) time(&t);
	for (i = 0; i < NPROCS; i++)
		switch(child=fork()) {
		case 0:
			nthcopy(fromname, toname, startblk, nblks, i);
			exit(0);
			break;
		case -1:
			error = 1;
			pexit("fork for copy");
			break;
		default:
			continue;
		}

	child = 0;
	for ( child = 0; wait((union wait *)0) != -1; child++ )
		;

	if (error)			  /* some child got a fatal error */
		exit(1);

	msg("\n");

	dmsg(stderr, "%s: copy %s to %s, %d blocks from block %d took %d s.\n",
	     arg0, fromname, toname, nblks, startblk, time(&t2)-t);
	dmsg(stderr, "%d procs, buffersize = %d\n", NPROCS, BUFFERSIZE);
	if ( child != NPROCS )
	{
		fprintf(stderr,
			"%s: parallel copy: %d (out of %d) processes failed\n",
			NPROCS - child, child);
		exit(1);
	}
}
