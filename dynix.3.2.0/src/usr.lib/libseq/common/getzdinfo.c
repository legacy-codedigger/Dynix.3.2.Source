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
 * "$Header: getzdinfo.c 1.2 89/09/03 $"
 * getzdinfo.c
 *	routines for extracting the zdinfo structure data
 *	from the *.zd files in /etc/diskinfo.
 */

/* $Log:	getzdinfo.c,v $
 */
 
#include <sys/types.h>
#include <sys/param.h>
#include <sys/dir.h>
#ifdef SYSV
#include <dirent.h>
#endif
#include <zdc/zdc.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <diskinfo.h>

extern int open(), read(), close();
extern char *strcpy(), *strcat();

static readzdinfo();

static struct zdinfo zdinfo;
static char devname[MAXNAMLEN+1];
static char ibuf[4*BUFSIZ];

/*
 * getzdinfo(name)
 *	char *name		- disk type name corresponding to file name
 *
 * This routine matches 'name' to a file in /etc/diskinfo by appending
 * the '.zd' suffix to name.  It then reads the file and creates and
 * returns a zdinfo structure based on the data in that file.
 */
struct zdinfo *
getzdinfobyname(name)
	char *name;
{
	int fd;
	struct zdinfo *info = &zdinfo;
	int i, count;
	int bufoff, offset;

	devname[0] = '\0';	
	sprintf(devname, "%s/%s%s", INFODIR, name, ZDSUFF);
	if ((fd = open(devname, O_RDONLY)) < 0) {
		fprintf(stderr, "unable to open file %s: ", devname);
		perror("");
		return((struct zdinfo *)0);
	}
	offset = bufoff = 0;
	do {
		if (lseek(fd, offset, 0) < 0) {
			fprintf(stderr, "seek error on file %s: ", devname);
			perror("");
			close(fd);
			return((struct zdinfo *)0);
		}
		if ((count = read(fd, ibuf, 4*BUFSIZ)) <= 0) {
			if (count < 0) {
				fprintf(stderr, "read error on file %s: ",
					devname);
				perror("");
			} else
				fprintf(stderr, "file %s empty\n", devname);
			return((struct zdinfo *)0);
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
		return((struct zdinfo *)0);
	}
	close(fd);
	info->zi_name = devname;
	strcpy(devname, name);
	readzdinfo(info);
	return(info);
}

static DIR *dir = NULL;

/*
 * getzdinfo
 *
 * This routine finds the next '.zd' file in the /etc/diskinfo
 * directory and returns a zdinfo structure built from the data
 * in that file.  On error or on finding no more '.zd' files,
 * NULL is returned
 */
struct zdinfo *
getzdinfo()
{
	int fd;
	struct zdinfo *info = &zdinfo;
#ifdef SYSV
	struct dirent *db;
#else
	struct direct *db;
#endif
	int notzd;
	int i, base, count;
	int bufoff, offset;

	if (dir == NULL) {
		if ((dir = opendir(INFODIR)) == NULL) {
			fprintf("unable to open directory %s\n", INFODIR);
			return(NULL);
		}
	}
#ifdef SYSV
	memset(info, (char)0, sizeof(struct zdinfo));
#else
	bzero(info, sizeof(struct zdinfo));
#endif
	for (db = readdir(dir); db != NULL; db = readdir(dir)) {
		/*
		 * See if this is a scsi info file by starting
		 * at end of file name.
		 */
		 for (notzd = 0, i = strlen(ZDSUFF)-1, 
		      base = strlen(db->d_name)-1; i >= 0; i--, base--) {
			if (db->d_name[base] != *(ZDSUFF+i)) { 
				notzd = 1;
				break;
			}
		}
		if (notzd)
			continue;
		base++;			/* length of base diskname */
		devname[0] = '\0';
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
		info->zi_name = devname;
		strncpy(devname, db->d_name, base);
		devname[base] = '\0';
		readzdinfo(info);
		return(info);
	}
	closedir(dir);
	dir = NULL;
	return(NULL);
}

/*
 * readzdinfo(info)
 *	struct zdinfo *info	- the structure to put the zd data in
 *
 * This routine fills in each field of the zdinfo structure by calling
 * the appropriate parsing routine.
 */
static
readzdinfo(info)
	struct zdinfo *info;
{
	info->zi_zdcdd.zdd_magic = ggetnum("mg");
	info->zi_zdcdd.zdd_ecc_bytes = ggetnum("ec");
	info->zi_zdcdd.zdd_spare = ggetnum("sp");
	info->zi_zdcdd.zdd_sectors = ggetnum("se");
	info->zi_zdcdd.zdd_tracks = ggetnum("tr");
	info->zi_zdcdd.zdd_cyls = ggetnum("cy");
	info->zi_zdcdd.zdd_drive_type = ggetnum("dt");
	info->zi_zdcdd.zdd_xfer_rate = ggetnum("xf");
	info->zi_zdcdd.zdd_runt = ggetnum("ru");
	info->zi_zdcdd.zdd_chdelay = ggetnum("ch");
	info->zi_zdcdd.zdd_hsdelay = ggetnum("hs");
	info->zi_zdcdd.zdd_hpo_rd_bc = ggetnum("hr");
	info->zi_zdcdd.zdd_hpo_fmt_bc = ggetnum("hf");
	info->zi_zdcdd.zdd_cskew = ggetnum("ck");
	info->zi_zdcdd.zdd_tskew = ggetnum("tk");
	info->zi_zdcdd.zdd_hdr_bc = ggetnum("hb");
	info->zi_zdcdd.zdd_sector_bc = ggetnum("sb");
	info->zi_zdcdd.zdd_strt_ign_bc = ggetnum("si");
	info->zi_zdcdd.zdd_end_ign_bc = ggetnum("ei");
	info->zi_zdcdd.zdd_ddc_regs.dr_status = ggetnum("ds");
	info->zi_zdcdd.zdd_ddc_regs.dr_error = ggetnum("de");
	info->zi_zdcdd.zdd_ddc_regs.dr_ppb0 = ggetnum("p0");
	info->zi_zdcdd.zdd_ddc_regs.dr_ppb1 = ggetnum("p1");
	info->zi_zdcdd.zdd_ddc_regs.dr_ppb2 = ggetnum("p2");
	info->zi_zdcdd.zdd_ddc_regs.dr_ppb3 = ggetnum("p3");
	info->zi_zdcdd.zdd_ddc_regs.dr_ppb4 = ggetnum("p4");
	info->zi_zdcdd.zdd_ddc_regs.dr_ppb5 = ggetnum("p5");
	info->zi_zdcdd.zdd_ddc_regs.dr_ptb0 = ggetnum("t0");
	info->zi_zdcdd.zdd_ddc_regs.dr_ptb1 = ggetnum("t1");
	info->zi_zdcdd.zdd_ddc_regs.dr_ptb2 = ggetnum("t2");
	info->zi_zdcdd.zdd_ddc_regs.dr_ptb3 = ggetnum("t3");
	info->zi_zdcdd.zdd_ddc_regs.dr_ptb4 = ggetnum("t4");
	info->zi_zdcdd.zdd_ddc_regs.dr_ptb5 = ggetnum("t5");
	info->zi_zdcdd.zdd_ddc_regs.dr_ec_ctrl = ggetnum("er");
	info->zi_zdcdd.zdd_ddc_regs.dr_hbc = ggetnum("hc");
	info->zi_zdcdd.zdd_ddc_regs.dr_dc = ggetnum("dc");
	info->zi_zdcdd.zdd_ddc_regs.dr_oc = ggetnum("oc");
	info->zi_zdcdd.zdd_ddc_regs.dr_sc = ggetnum("sc");
	info->zi_zdcdd.zdd_ddc_regs.dr_nso = ggetnum("ns");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb0_pat = ggetnum("h0");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb1_pat = ggetnum("h1");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb2_pat = ggetnum("h2");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb3_pat = ggetnum("h3");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb4_pat = ggetnum("h4");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb5_pat = ggetnum("h5");
	info->zi_zdcdd.zdd_ddc_regs.dr_rdbc = ggetnum("rd");
	info->zi_zdcdd.zdd_ddc_regs.dr_dma_addr = ggetnum("dm");
	info->zi_zdcdd.zdd_ddc_regs.dr_dpo_bc = ggetnum("do");
	info->zi_zdcdd.zdd_ddc_regs.dr_hpr_bc = ggetnum("rh");
	info->zi_zdcdd.zdd_ddc_regs.dr_hs1_bc = ggetnum("s1");
	info->zi_zdcdd.zdd_ddc_regs.dr_hs2_bc = ggetnum("s2");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb0_ctrl = ggetnum("c0");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb1_ctrl = ggetnum("c1");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb2_ctrl = ggetnum("c2");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb3_ctrl = ggetnum("c3");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb4_ctrl = ggetnum("c4");
	info->zi_zdcdd.zdd_ddc_regs.dr_hb5_ctrl = ggetnum("c5");
	info->zi_zdcdd.zdd_ddc_regs.dr_extdecc_bc = ggetnum("xd");
	info->zi_zdcdd.zdd_ddc_regs.dr_exthecc_bc = ggetnum("xh");
	info->zi_zdcdd.zdd_ddc_regs.dr_hpo_wr_bc = ggetnum("po");
	info->zi_zdcdd.zdd_ddc_regs.dr_dpr_wr_bc = ggetnum("pr");
	info->zi_zdcdd.zdd_ddc_regs.dr_ds1_bc = ggetnum("d1");
	info->zi_zdcdd.zdd_ddc_regs.dr_ds2_bc = ggetnum("d2");
	info->zi_zdcdd.zdd_ddc_regs.dr_dpo_pat = ggetnum("op");
	info->zi_zdcdd.zdd_ddc_regs.dr_hpr_pat = ggetnum("or");
	info->zi_zdcdd.zdd_ddc_regs.dr_hs1_pat = ggetnum("a1");
	info->zi_zdcdd.zdd_ddc_regs.dr_hs2_pat = ggetnum("a2");
	info->zi_zdcdd.zdd_ddc_regs.dr_gap_bc = ggetnum("gp");
	info->zi_zdcdd.zdd_ddc_regs.dr_df = ggetnum("df");
	info->zi_zdcdd.zdd_ddc_regs.dr_ltr = ggetnum("lt");
	info->zi_zdcdd.zdd_ddc_regs.dr_rtr = ggetnum("rt");
	info->zi_zdcdd.zdd_ddc_regs.dr_sector_bc = ggetnum("et");
	info->zi_zdcdd.zdd_ddc_regs.dr_gap_pat = ggetnum("gt");
	info->zi_zdcdd.zdd_ddc_regs.dr_dfmt_pat = ggetnum("fd");
	info->zi_zdcdd.zdd_ddc_regs.dr_hpo_pat = ggetnum("ah");
	info->zi_zdcdd.zdd_ddc_regs.dr_dpr_pat = ggetnum("ad");
	info->zi_zdcdd.zdd_ddc_regs.dr_ds1_pat = ggetnum("r1");
	info->zi_zdcdd.zdd_ddc_regs.dr_ds2_pat = ggetnum("r2");
	return;
}

/*
 * getzdinfobydtype(type)
 *	int type		- corresponds to the numbered disk types
 *
 * This routine finds the file for the specified disk type based on
 * a number and returns a zdinfo structure for that type.
 */
struct zdinfo *
getzdinfobydtype(type)
	int type;
{
	struct zdinfo *info;

	while ((info = getzdinfo()) != NULL) {
		if (info->zi_zdcdd.zdd_drive_type == type) {
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
		while (bp != ebp && *bp++ != '\n') {
			continue;
		}
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
