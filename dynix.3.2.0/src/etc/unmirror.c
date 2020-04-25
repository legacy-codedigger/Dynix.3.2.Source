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
static char rcsid[] = "$Header: unmirror.c 1.3 90/11/06 $";
#endif

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <mirror/mirror.h>
#include <sys/param.h>

#define MIRRORNAME "rmr"		  /* base name of raw mirror device */
#define MIRRORMAJOR 20			  /* Conventional raw mirror major */
#define DEV "/dev"			  /* directory scanned for mirrors */
#define MRTAB "/etc/mrtab"

char *rindex(), *raw_dev(), *malloc(), *strcpy(), *strcat();
int alphasort();

int error;				  /* exit code */
static char *dev_t_to_name();
static char *arg0;
static eaccess;				  /* global for scan_mirror err ret. */
static dev_t dev;			  /* global for select_dev */
static char *name;			  /* global for select_name */
static ushort maj;			  /* global for select_major */
static ushort mirrormajor;		  /* real mirror major number */

/*
 * unmirror <mirror> ...
 * unmirror -a
 */

usage()
{
	fprintf( stderr,
		"Usage: %s /dev/rmr? ...\n       %s -a\n",
		arg0, arg0);
	exit(2);
}

main(argc, argv)
	int argc;
	char **argv;
{
	int mr, i;
	char *mirror;

	arg0 = argv[0];
	error = 0;

	if (argc < 2)
		usage();

	for ( i = 1; i < argc; i++ )
		/* Check for options before anything else. */
		if (argv[i][0] == '-')
		{
			if (strcmp(argv[i], "-a") == 0)
			{
				if (argc != 2)
					usage();
				else	unmirrorall();
			}
			else usage();
		}

	for ( i = 1; i < argc; i++ )
	{
		mirror = raw_dev( argv[i] );
		if ( (mr = open( mirror, O_RDWR | O_NDELAY )) < 0 )
		{
			fprintf( stderr, "%s: cannot open mirror ", arg0);
			perror( mirror );
			error = 1;
			continue;
		}

		if ( ioctl( mr, MIOCOFF, (char *)0) )
		{
			fprintf(stderr, "%s: cannot stop mirroring ", arg0 );
			if (errno == ENXIO)
				fprintf(stderr, "%s: mirror not active\n",
					mirror);
			else	perror(mirror);
			error = 1;
		}
		(void)close(mr);
	}
	exit(error);
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
	{
		fprintf(stderr, "%s: out of memory\n", arg0);
		exit(1);
	}
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

	if ( stat(dir->d_name, &statbuf) )
		return 0;		  /* stat failed, don't bother */
	
	return ((statbuf.st_mode & S_IFMT) == S_IFCHR
		&& statbuf.st_rdev == dev);
}

/*
 * dev_t_to_name()
 * Given a dev_t, find a matching name in the DEV directory and return a
 * pointer to it.  Changes the global dev_t dev.
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
 * Given the name of a valid mirror, unmirror all active mirrors in the kernel.
 * Expects to be in the DEV directory (normally "/dev"), and the name to be a
 * path relative to this. 
 */
static
scan_mirrors(name)
	char *name;
{
	int fd, index, maxindex;
	struct stat s;
	struct mioc mioc;

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
	(void)close(fd);
	for ( index = 0; index < maxindex ; index++ )
	{
		mioc.limit = index;
		
		name = dev_t_to_name(makedev(mirrormajor, index));

		if (strlen(name) == 0
		    || (fd = open(name, O_RDWR)) < 0
		    || ioctl(fd, MIOCINFO, (char *)&mioc)
		    || mioc.mirror.active != ACTIVE)
		{
			if (fd >= 0)
				(void)close(fd);
			continue;
		}

		/* We only care about active units */
		if ( ioctl(fd, MIOCOFF, (char *) 0) )
		{
			fprintf( stderr, "%s: cannot stop mirroring %s/",
				arg0, DEV );
			perror( name );
			error = 1;
		}
		(void)close(fd);
	}
	return 1;
}

scanmrtab()
{
	FILE *mrtab;
	char mirror[MAXPATHLEN+1], *name;
	char stdmrname[ sizeof DEV + sizeof "/" + sizeof MIRRORNAME];
	int c, mr;

	(void)strcpy(stdmrname, DEV);
	(void)strcat(stdmrname, "/");
	(void)strcat(stdmrname, MIRRORNAME);
	if ( (mrtab = fopen( MRTAB, "r")) == NULL )
		return;

	mirror[MAXPATHLEN] = '\0';

	/* Initial deblank */
	if (!isspace(c = readss(mrtab)))
		(void)ungetc(c, mrtab);
	if (c == EOF)
		return;

	while ( TRUE )
	{
		switch(readword(mirror, MAXPATHLEN, mrtab))
		{
		case 0:
			/* word too long */
			fprintf(stderr,
				"%s: %s: mirror name >%d bytes: %s",
				arg0, MRTAB, MAXPATHLEN, mirror);
			/* read all of word */
			while(!(c = readword(mirror, MAXPATHLEN, mrtab)))
				fprintf(stderr, "%s", mirror);
			fprintf(stderr,"%s\n", mirror);
			/* read rest of line... */
			if (c < 2)
				while((c=readword(mirror, MAXPATHLEN, mrtab))
				      < 2 )
					;
			if (c == 3)
			{
				/* proper EOF */
				(void)fclose(mrtab);
				return;
			}
			break;
		case 1:
			/* normal word */
			name = raw_dev(mirror);
			if ((mr = open(name, O_RDONLY)) < 0 )
			{
				fprintf(stderr, "%s: cannot open mirror ",
					arg0);
				perror(name);
				error = 1;
			}
			else
			{
				if (ioctl(mr, MIOCOFF, (char *)0) != 0
				    && errno != ENXIO
				    && strncmp(name, stdmrname,
					       strlen(stdmrname)) != 0)
				{
					/*
					 * Couldn't stop mirroring, it wasn't
					 * because mirroring wasn't active, and
					 * the name won't be found again in the
					 * pass to catch all active mirrors--
					 * complain now!
					 */
					fprintf(stderr,
						"%s: cannot stop mirroring ",
						arg0);
					perror(name);
					error = 1;
				}
				(void)close(mr);
			}
			/* read rest of line */
			while ( (c = readword(mirror, MAXPATHLEN, mrtab)) < 2 )
				;
			if (c == 3)
			{
				/* proper EOF */
				(void)fclose(mrtab);
				return;
			}
			break;
		case 2:
			/* EOL */
			fprintf(stderr, "%s: %s: unexpected end of line: %s\n",
				arg0, MRTAB, mirror);
			break;
		case 3:
			/* EOF */
			fprintf(stderr, "%s: %s: unexpected end of file: %s\n",
				arg0, MRTAB, mirror);
			(void)fclose(mrtab);
			return;
			break;
		}
	}
}

/*
 * unmirrorall()
 * cd to DEV (usually "/dev") and find a mirror in that directory, then call
 * scan_mirrors to unmirror them all
 */
unmirrorall()
{
	struct direct **namelist;
	struct stat statbuf;
	int i, imax;

	/* first, we scan /etc/mrtab */
	scanmrtab();

	/* Now, we look for any mirrors we may have missed. */
	if (stat(DEV, &statbuf) || chdir(DEV))
	{
		printf("%s: Cannot cd to %s\n", arg0, DEV);
		exit(1);
	}

	eaccess = 0;
	/* try scanning by name first, major number last */
	name = MIRRORNAME;
	imax = scandir(".", &namelist, select_name, alphasort);
	for ( i = 0; i < imax; i++ )
		if ( scan_mirrors(namelist[i]->d_name) )
			exit(error);

	maj = MIRRORMAJOR;
	imax = scandir(".", &namelist, select_major, alphasort);
	for ( i = 0; i < imax; i++ )
		if ( scan_mirrors(namelist[i]->d_name) )
			exit(error);

	fprintf( stderr, "%s: cannot find any %smirrors in %s\n", arg0,
		eaccess? "openable " : "", DEV);
	exit(1);
}

/*
 * readss(stream)
 * Readss returns a single character, much like getc.  The difference is that
 * all adjacent white space is reduced to a single character. Any white space
 * that includes a newline is returned as newline, all other white space is
 * returned as a space. A '#' is considered to start a comment, which ends at
 * the first newline or EOF.  This is considered  whitespace that includes a
 * newline. 
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
		else 	eof = 1;

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
