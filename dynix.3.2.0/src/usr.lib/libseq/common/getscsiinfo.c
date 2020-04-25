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


/*
 * "$Header: getscsiinfo.c 1.2 89/09/03 $"
 * getscsiinfo.c
 *	Routines for finding and formatting data for scsi
 *	disks in the /etc/diskinfo/*.scsi files.
 *	These files are used by the online formatter.
 */


/* $Log:	getscsiinfo.c,v $
 */

#define	NULL	0
#include <sys/types.h>
#include <sys/param.h>
#include <sys/dir.h>
#ifdef SYSV
#include <dirent.h>
#endif
#include <sec/scsi.h>
#include <zdc/zdc.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <diskinfo.h>

extern int open(), read(), close();
extern char *strcpy(), *strcat();


static struct scsiinfo scsiinfo;
static char devname[MAXNAMLEN+1];
static char ibuf[4*BUFSIZ];

/*
 * getscsiinfo()
 *
 * This routine finds the next file in the /etc/diskinfo data
 * base which ends with '.scsi', and builds and returns a scsiinfo structure
 * based on the data in that file.  On error or when there are no
 * more '.scsi' files in the directory, NULL is returned.
 */
static DIR *dir = NULL;

struct scsiinfo *
getscsiinfo()
{
	int fd;
	struct scsiinfo *info = &scsiinfo;
#ifdef SYSV
	struct dirent *db;
#else
	struct direct *db;
#endif
	int notscsi;
	int i, base, count;
	int bufoff, offset;

	if (dir == NULL) {
		if ((dir = opendir(INFODIR)) == NULL) {
			fprintf("unable to open directory %s\n", INFODIR);
			return(NULL);
		}
	}
	for (db = readdir(dir); db != NULL; db = readdir(dir)) {
		/*
		 * See if this is a scsi info file by starting
		 * at end of file name.
		 */
		 for (notscsi = 0, i = strlen(SCSISUFF)-1, 
		      base = strlen(db->d_name)-1; i >= 0; i--, base--) {
			if (db->d_name[base] != *(SCSISUFF+i)) {
				notscsi = 1;
				break;
			}
		}
		if (notscsi)
			continue;
		base++;			/* length of base diskname */
		sprintf(devname, "%s/%s", INFODIR, db->d_name);
		if ((fd = open(devname, O_RDONLY)) < 0)
			continue;
		offset = bufoff = 0;
		do {
			if (lseek(fd, offset, 0) < 0) {
				fprintf(stderr, "seek error on file %s: ", devname);
				perror("");
				close(fd);
				continue;
			}
			if ((count = read(fd, ibuf, 4*BUFSIZ)) <= 0) {
				if (count < 0) {
					fprintf(stderr, "read error on file %s: ",
						devname);
					perror("");
				} else
					fprintf(stderr, "file %s empty\n", devname);
				continue;
			}
			bufoff = gfinddata(ibuf, count);	
			offset += bufoff;
		} while (bufoff != 0);
		if (count == 4*BUFSIZ) {
			/*
			 * If we haven't reached EOF, there still might be
			 * valid data in the file, and this routine isn't
			 * prepared to read anymore, so call it an error.
			 * If the valid data size is really greater than
			 * 4*BUFSIZ, just change the read to read more data!
			 */
			fprintf(stderr, "whoops -- too much data in file %s!",
				devname);
			fprintf(stderr, " - can't process\n");
			close(fd);
			continue;
		}
		close(fd);
		info->scsi_diskname = devname;
		strncpy(devname, db->d_name, base);
		devname[base] = '\0';
		ggetstr("vn", info->scsi_vendor);
		ggetstr("pr", info->scsi_product);
		info->scsi_inqformat = ggetnum("iq");
		info->scsi_reasslen = ggetnum("re");
		info->scsi_formcode = ggetnum("fm");
		return(info);
	}
	closedir(dir);
	dir = NULL;
	return(NULL);
}

/*
 * getscsimatch(vendor, product)
 *	char *vendor	- the vendor as returned by a SCSI INQUIRY command
 *	char *product	- the product as returned by a SCSI INQUIRY command
 *
 * This routine searches the '.scsi' files in the /etc/diskinfo data
 * base to find the file which matches the specified vendor and
 * product, then returns a scsiinfo structure built from that file.
 * If no match is found, NULL is returned.
 */
struct scsiinfo *
getscsimatch(vendor, product)
	char *vendor, *product;
{
	struct scsiinfo *info;

	while ((info = getscsiinfo()) != NULL) {
		if (!strncmp(info->scsi_vendor, vendor, INQ_VEND)
		    && !strncmp(info->scsi_product, product, INQ_PROD)) {
			closedir(dir);
			dir = NULL;
			return(info);
		}
	}
	return(NULL);
}

static char *gskip();
static void gdecode();

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
static
ggetnum(id)
	char *id;
{
	register int i, base;
	register char *bp = ibuf;

	for (;;) {
		bp = gskip(bp);
		if (*bp == 0)
			return (-1);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return (-1);
		if (*bp != '#')
			continue;
		bp++;
		base = 10;
		if (*bp == '0')
			base = 8;
		i = 0;
		while (isdigit(*bp))
			i *= base, i += *bp++ - '0';
		return (i);
	}
}

/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
static
ggetstr(id, area)
	char *id, *area;
{
	register char *bp = ibuf;

	for (;;) {
		bp = gskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return (0);
		if (*bp != '=')
			continue;
		bp++;
		gdecode(bp, area);
		return;
	}
}

/*
 * Gdecode does the grung work to decode the
 * string capability escapes.
 */
static void
gdecode(str, area)
	register char *str;
	char *area;
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = area;
	while ((c = *str++) && c != ':') {
		switch (c) {

		case '^':
			c = *str++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (isdigit(c)) {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *str++ - '0';
				while (--i && isdigit(*str));
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	return;
}

/*
 * Return the buffer offset of the beginning of actual
 * data in the file (skip comments and blank lines
 * at front of file).
 */
static int
gfinddata(bp, count)
	register char *bp;
{
	char *sbp, *ebp;
	char *linebp;

	sbp = bp;
	ebp = bp + count;
	while ((bp != ebp) && (*bp == '#' || isspace(*bp))) {
		linebp = bp;
		bp++;
		while (bp != ebp && *bp++ != '\n')
			continue;
	}
	if (bp == ebp)
		return(linebp - sbp);
	return(bp - sbp);
}

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the geomtab file in octal.
 */
static char *
gskip(bp)
	register char *bp;
{

	while (*bp && *bp != ':')
		bp++;
	if (*bp == ':') {
		bp++;
		while (isspace(*bp) || *bp == '\\' || *bp == ':')
			bp++;
	}
	return (bp);
}
