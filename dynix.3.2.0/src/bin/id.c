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
static 	char rcsid[] = "$Header: id.c 2.1 1991/05/16 20:39:53 $";
#endif

/*
 * Id - Print a checksum, size, owner, group, permissions, and pathname(s).
 * 	    Sorts files in directories for consistant comparisons.
 * Options
 * 	-r  recursive on directories
 *	-l  verbose output
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/universe.h>
#include <ar.h>

usage(){ fprintf(stderr, "usage: id [ -rl ] absolute-path(s)\n"); exit(1); }

extern	int alphasort(), dselect(), fselect();
extern	char *perm();
extern	unsigned short sum();
char 	errbuf[ 20 * BUFSIZ ];
struct	stat statbuf;
int	rflag = 0;
int	lflag = 0;
int	dash = 0;
int	errs = 0;

unsigned short CRCtab[256] = {
	0x0000,	0xc0c1,	0xc181,	0x0140,	0xc301,	0x03c0,	0x0280,	0xc241,
	0xc601,	0x06c0,	0x0780,	0xc741,	0x0500,	0xc5c1,	0xc481,	0x0440,
	0xcc01,	0x0cc0,	0x0d80,	0xcd41,	0x0f00,	0xcfc1,	0xce81,	0x0e40,
	0x0a00,	0xcac1,	0xcb81,	0x0b40,	0xc901,	0x09c0,	0x0880,	0xc841,
	0xd801,	0x18c0,	0x1980,	0xd941,	0x1b00,	0xdbc1,	0xda81,	0x1a40,
	0x1e00,	0xdec1,	0xdf81,	0x1f40,	0xdd01,	0x1dc0,	0x1c80,	0xdc41,
	0x1400,	0xd4c1,	0xd581,	0x1540,	0xd701,	0x17c0,	0x1680,	0xd641,
	0xd201,	0x12c0,	0x1380,	0xd341,	0x1100,	0xd1c1,	0xd081,	0x1040,
	0xf001,	0x30c0,	0x3180,	0xf141,	0x3300,	0xf3c1,	0xf281,	0x3240,
	0x3600,	0xf6c1,	0xf781,	0x3740,	0xf501,	0x35c0,	0x3480,	0xf441,
	0x3c00,	0xfcc1,	0xfd81,	0x3d40,	0xff01,	0x3fc0,	0x3e80,	0xfe41,
	0xfa01,	0x3ac0,	0x3b80,	0xfb41,	0x3900,	0xf9c1,	0xf881,	0x3840,
	0x2800,	0xe8c1,	0xe981,	0x2940,	0xeb01,	0x2bc0,	0x2a80,	0xea41,
	0xee01,	0x2ec0,	0x2f80,	0xef41,	0x2d00,	0xedc1,	0xec81,	0x2c40,
	0xe401,	0x24c0,	0x2580,	0xe541,	0x2700,	0xe7c1,	0xe681,	0x2640,
	0x2200,	0xe2c1,	0xe381,	0x2340,	0xe101,	0x21c0,	0x2080,	0xe041,
	0xa001,	0x60c0,	0x6180,	0xa141,	0x6300,	0xa3c1,	0xa281,	0x6240,
	0x6600,	0xa6c1,	0xa781,	0x6740,	0xa501,	0x65c0,	0x6480,	0xa441,
	0x6c00,	0xacc1,	0xad81,	0x6d40,	0xaf01,	0x6fc0,	0x6e80,	0xae41,
	0xaa01,	0x6ac0,	0x6b80,	0xab41,	0x6900,	0xa9c1,	0xa881,	0x6840,
	0x7800,	0xb8c1,	0xb981,	0x7940,	0xbb01,	0x7bc0,	0x7a80,	0xba41,
	0xbe01,	0x7ec0,	0x7f80,	0xbf41,	0x7d00,	0xbdc1,	0xbc81,	0x7c40,
	0xb401,	0x74c0,	0x7580,	0xb541,	0x7700,	0xb7c1,	0xb681,	0x7640,
	0x7200,	0xb2c1,	0xb381,	0x7340,	0xb101,	0x71c0,	0x7080,	0xb041,
	0x5000,	0x90c1,	0x9181,	0x5140,	0x9301,	0x53c0,	0x5280,	0x9241,
	0x9601,	0x56c0,	0x5780,	0x9741,	0x5500,	0x95c1,	0x9481,	0x5440,
	0x9c01,	0x5cc0,	0x5d80,	0x9d41,	0x5f00,	0x9fc1,	0x9e81,	0x5e40,
	0x5a00,	0x9ac1,	0x9b81,	0x5b40,	0x9901,	0x59c0,	0x5880,	0x9841,
	0x8801,	0x48c0,	0x4980,	0x8941,	0x4b00,	0x8bc1,	0x8a81,	0x4a40,
	0x4e00,	0x8ec1,	0x8f81,	0x4f40,	0x8d01,	0x4dc0,	0x4c80,	0x8c41,
	0x4400,	0x84c1,	0x8581,	0x4540,	0x8701,	0x47c0,	0x4680,	0x8641,
	0x8201,	0x42c0,	0x4380,	0x8341,	0x4100,	0x81c1,	0x8081,	0x4040};

main(argc, argv)
int argc;
char *argv[];
{
	register char *options;
	register int arg;

	if (argc < 2)
		usage();

	for (arg = 1; arg < argc; arg++)
		if (*argv[arg] == '-')
			for (options = argv[arg]; *++options; )
				switch (*options) {

				  case 'r':
					rflag++;
					break;

				  case 'l':
				  case 'f':
					lflag++;
					break;

				  default:
					fprintf(stderr, "id: %c: unknown option\n", *options);
					usage();
				}
		else {
			if(*argv[arg] != '/') {
				fflush(stdout);
				fprintf(stderr,"id: %s not absolute path, ignored.\n", argv[arg]);
				++errs;
				continue;
			}
			errs += verify(argv[arg]);
		}
	exit(errs);
}

verify(name)
register char *name;
{
	register status = 0;
	register i, dcnt, fcnt;
	struct direct **d, **f;
	char dirbuf[ 10 * BUFSIZ ];


	if (lstat(name, &statbuf)) {
		fflush(stdout);
		(void) sprintf(errbuf, "id: lstat: %s", name);
		perror(errbuf);
		return (1);
	}

	switch (statbuf.st_mode & S_IFMT) {
	case S_IFCHR:	/* char device */
	case S_IFBLK:	/* block device */
	case S_IFSOCK:	/* socket */
	case S_IFLNK:	/* symbolic link */
	case S_IFREG:	/* regular file */
	case S_IFIFO:	/* fifo */
		status += line(name);
		break;

	case S_IFDIR:	/* directory */
		status += line(name);
		if (rflag == 0)
			break;
		if (chdir(name)) {
			fflush(stdout);
			(void) sprintf(errbuf, "id: chdir: %s", name);
			perror(errbuf);
			++status;
			break;
		}
		if (name[1] == NULL) 
			name[0] = NULL;

		/* scan "." for files and directories */
		dcnt = scandir(".", &d, dselect, alphasort);
		fcnt = scandir(".", &f, fselect, alphasort);

		/*
		 * Generate lines for all files and dirs in current directory
		 */
		for (i = 0; i < fcnt; i++) {
			(void) sprintf(dirbuf, "%s/%s", name, f[i]->d_name);
			status += line(dirbuf);
		}

		for (i = 0; i < dcnt; i++) {
			(void) sprintf(dirbuf, "%s/%s", name, d[i]->d_name);
			status += verify(dirbuf);
		}
		break;

	default:	/* unknown */
		fflush(stdout);
		fprintf(stderr, "id: %s: bad file type\n", name);
		++status;
		break;
	}
	return(status);
}

unsigned short
sum(file)
	char *file;
{
	register unsigned short Sum = 0;
	register FILE *f;
	char	 buffer[BUFSIZ];
	struct	 ar_hdr arbuf;
	register int arcv_flag = 0;
	register int count;
	long	 remain;
	
	if ((f = fopen(file, "r")) == NULL) {
		fflush(stdout);
		fprintf(stderr, "id: can't open %s\n", file);
		return (0);
	}

	if ((count=fread(buffer, 1, SARMAG, f)) > 0) {
		if ((count == SARMAG) && (!strncmp(buffer, ARMAG, SARMAG)))
			arcv_flag++;
		Sum = summer(Sum, buffer, count);
	}
	else goto fin;

	if (!arcv_flag)		/* normal file */

		while ((count=fread(buffer, 1, BUFSIZ, f)) > 0)
			Sum = summer(Sum, buffer, count);

	else {			/* archive file */
		while((count=fread((char *)&arbuf, 1, sizeof(arbuf), f)) > 0) {
			if (count != sizeof(arbuf) || 
				strncmp(arbuf.ar_fmag, ARFMAG, sizeof(arbuf.ar_fmag))) {
				fprintf(stderr, "malformed archive file\n");
				exit(1);
			} else {
				Sum = summer(Sum, arbuf.ar_name, sizeof(arbuf.ar_name));
				Sum = summer(Sum, arbuf.ar_uid, sizeof(arbuf.ar_uid));
				Sum = summer(Sum, arbuf.ar_gid, sizeof(arbuf.ar_gid));
				Sum = summer(Sum, arbuf.ar_mode, sizeof(arbuf.ar_mode));
				Sum = summer(Sum, arbuf.ar_size, sizeof(arbuf.ar_size));
				Sum = summer(Sum, arbuf.ar_fmag, sizeof(arbuf.ar_fmag));
	
				sscanf(arbuf.ar_size, "%ld", &remain);
				if (remain & 1)  remain++;
	
				while (remain > 0) {
					count=fread(buffer, 1, remain>BUFSIZ?BUFSIZ:remain, f);
					remain -= count;
					if (count > 0)
						Sum = summer(Sum, buffer, count);
				}
			}
		}
	}

fin:	if (ferror(f)) {
		fflush(stdout);
		fprintf(stderr, "id: read error on %s\n", file);
		return (0);
	}
	(void) fclose(f);
	return ((unsigned short)Sum);
}

summer(Sum, bufp, nchars)
	register unsigned short Sum;
	register char *bufp;
	register int nchars;
{
	register int ch;

	for (ch=0; ch<nchars; ch++) 
		Sum = CRCtab[(Sum^bufp[ch])&0xFF] ^ Sum>>8;
	return((unsigned short)Sum);
}

/*
 * Return 1 if entry is a file (not a directory)
 */
int
fselect(d)
	register struct direct *d;
{
	register char *p;

	p = d -> d_name;
	if (lstat(p, &statbuf)) {
		fflush(stdout);
		(void) sprintf(errbuf, "id: lstat: %s", p);
		perror(errbuf);
		return(0);
	}
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) 
		return (0);
	return(1);
}

/*
 * Return 1 if entry is a DESIRED directory
 */
int
dselect(d)
	register struct direct *d;
{
	register char *p;

	p = d -> d_name;
	if (lstat(p, &statbuf) != 0) {
		fflush(stdout);
		(void) sprintf(errbuf, "id: lstat: %s", p);
		perror(errbuf);
		return (0);
	}
	if ((statbuf.st_mode&S_IFMT) != S_IFDIR)
		return (0);

	/* handle "." and ".." */
	if (p[0] == '.')  {
		if (p[1] == 0 || (p[1] == '.' && p[2] == 0) )
			return (0);
	}
	return(1);
}

line(name)
	char *name;
{
	register i;
	char lbuf[ 5 * BUFSIZ ];

	if (lstat(name, &statbuf) != 0) {
		fflush(stdout);
		(void) sprintf(errbuf, "id: lstat: %s", name);
		perror(errbuf);
		return (1);
	}
	switch (statbuf.st_mode & S_IFMT) {
	case S_IFCHR:	/* char device */
		printf("%05u %-6d", 0, 0);
		if (lflag) {
			printf(" %3d/%-3d c%s", statbuf.st_uid, statbuf.st_gid, perm());
			if (statbuf.st_nlink > 1)
				printf("-%d", statbuf.st_nlink);
		}
		printf(" %s\n", name);
		break;

	case S_IFBLK:	/* block device */
		printf("%05u %-6d", 0, 0);
		if (lflag) {
			printf(" %3d/%-3d b%s", statbuf.st_uid, statbuf.st_gid, perm());
			if (statbuf.st_nlink > 1)
				printf("-%d", statbuf.st_nlink);
		}
		printf(" %s\n", name);
		break;

	case S_IFSOCK:	/* socket */
		printf("%05u %-6d", 0, 0);
		if (lflag) {
			printf(" %3d/%-3d s%s", statbuf.st_uid, statbuf.st_gid, perm());
			if (statbuf.st_nlink > 1)
				printf("-%d", statbuf.st_nlink);
		}
		printf(" %s\n", name);
		break;

	case S_IFLNK:	/* symbolic link */
		/* 
		 * Permissions on symbolic links have no meaning so in order that
		 * comparisons will always work just print "--777".
		 * Also, symbolic links *CANNOT* have a link count > 1.
		 */
		printf("%05u %-6d", 0, 0);
		if (lflag) {
			printf(" %3d/%-3d l%s", statbuf.st_uid, statbuf.st_gid, "--777");
		}
		if (statbuf.st_spare4[0] == 1) {
			i = readclink(name, lbuf, sizeof (lbuf), U_UCB);
			if (i > 0) 
				lbuf[i] = 0;
			printf(" %s -> ucb=%s", name, lbuf);
			i = readclink(name, lbuf, sizeof (lbuf), U_ATT);
			if (i > 0) 
				lbuf[i] = 0;
			printf(" att=%s\n", lbuf);
			break;
		} else {
			i = readlink(name, lbuf, sizeof (lbuf));
			if (i > 0) 
				lbuf[i] = 0;
			printf(" %s -> %s\n", name, lbuf);
			break;
		}

	case S_IFIFO:
		printf("fifo %s\n", name);
		return;

	case S_IFREG:	/* regular file */
		printf("%05u %-6d", sum(name), statbuf.st_size);
		if (lflag) {
			printf(" %3d/%-3d %c%s", statbuf.st_uid, statbuf.st_gid,
			(statbuf.st_mode & S_ISVTX) ? 't' : '-', perm());
			if (statbuf.st_nlink > 1)
				printf("-%d", statbuf.st_nlink);
		}
		printf(" %s\n", name);
		break;

	case S_IFDIR:	/* directory */
		printf("%05u %-6d", 0, 0);
		if (lflag) {
			printf(" %3d/%-3d d%s", statbuf.st_uid, statbuf.st_gid, perm());
			if (statbuf.st_nlink > 1)
				printf("-%d", statbuf.st_nlink);
		}
		printf(" %s\n", name);
		break;
	}
	return (0);
}

char *
perm()
{
	static char buf[sizeof("--777")];
	unsigned short mode = statbuf.st_mode;

	(void) sprintf(buf, "%c%c%d%d%d", 
		(mode & S_ISUID) ? 'u' : '-',
		(mode & S_ISGID) ? 'g' : '-',
		(mode & 0700) >> 6,
		(mode & 0070) >> 3,
		(mode & 0007) >> 0);
	return(buf);
}
