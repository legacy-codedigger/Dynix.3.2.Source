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

#ifdef RCS
static char rcsid[] = "@(#)$Header: formatscsi.c 2.5 90/09/13 $";
#endif

/*
 * Formatscsi:
 *	Format disk on Adaptec SCSI Target Adapter
 *
 * Design Criteria:
 *	1) Must be a standalone program, using the standalone library.
 *	2) Allow any scsi disk that conforms to minimal Adaptec requirements
 *		to be formatted in any manner.  The minimum commands to a
 *		non-UNIX disk should only be Set Modes and the Format.
 *	3) Make formatting UNIX disks easy.  The defaults should be for UNIX.
 *	4) Be able to re-use any diagnostics data that exist on the disk
 *		prior to its being formatted (for re-formats).
 *	5) Be able to read a list of bad blocks from a file.
 *	6) If the disk is formatted as a UNIX disk, write the diagnostic
 *		data onto the diagnostics tracks after the format.
 *		(A UNIX disk is taken as any disk that has 512-byte blocks.)
 *	7) Avoid the Read Capacity command as much as possible.
 *		Some target adapters fail on the partial Read Capacity
 *		command, which is essential to locating the placement of
 *		the diagnostic data.  First an attempt is made to use
 *		the partial Read Capacity command.  If this fails,
 *		an attempt is made to calculate the locations.
 *	8) Be user-friendly.
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vtoc.h>
#include "saio.h"
#include "scsi.h"
#include "scsidisk.h"
#include "sec_diag.h"
#include "sdreg.h"

typedef	unsigned	int	uint;
typedef	unsigned	char	unchar;
#define DFLT_RPM	3600

extern struct drive_type drive_table[];

int	fd;			/* file descriptor of device to be formatted */
char	filename[80];		/* file name of device to be formatted */
uint	cap_lba, cap_size;	/* results from do_cap() */
uint	bytes_blk, bytes_trk, byte_offset;
struct  vtoc	*vtoc = (struct vtoc *)0;	/* pointer to vtoc structure */
char	*prompt();
struct	st st;
struct	z z;
struct	bb bb[MAX_DEFECTS];
union {
	struct	db db;
	unchar	buf[DBSIZE];
} ud;

#define ud_pat	ud.db.db_pat
#define ud_mds	ud.db.db_mds
#define ud_fmt	ud.db.db_fmt
#define ud_z	ud.db.db_z

#define YES	'y'
#define NO	'n'
#define ADD	'a'
#define CHG	'c'
#define DEL	'd'
#define FALSE	0
#define TRUE	1
#define BB_TYPE_DEL	255	/* fake bb type - for signaling deletes */
#define M1	(1024 * 1024)

/*
 * Inquiry command returned data
 */

struct inqarg {
	unchar	opcode;
	unchar	pad1[3];
	unchar	length;
	unchar	pad2;
	unchar	devtype[3];
	unchar	format;				/* 0 for adaptec, 1 for CCS */
} inq;

unchar pat_default[] = {0x6D, 0xB6, 0xDB};	/* diagnostic track pattern */

char Done[] = "Done\n";
char Error[] = "Error. ";
char Formatting[] = "Formatting...";
char Writing[] = "Writing diagnostic data at %u...";
char acd[] = "nacd";
char fmt_blk_trk[] = "\nMaximum physical blocks per track = %u\n";
char fmt_blk_size[] = "\nBlock size = %u\n";
char fmt_cyls[] = "\nNumber of cylinders = %u\n";
char fmt_heads[] = "\nNumber of heads = %u\n";
char fmt_defect[] = "defect %u:\n";
char fmt_ileave[] = "\nInterleave factor = %u\n";
char fmt_lzone[] = "\nLanding zone position = %d\n";
char fmt_nbr_defs[] = "\n%u defects\n";
char fmt_rwcc[] = "\nReduced write current cyl = %u\n";
char fmt_show_bbl[] = "Display current list of defects";
char fmt_spares[] = "\nSpare blocks/track = %u\n";
char fmt_srate[] = "\nSeek speed = %u\n";
char fmt_use_bbl[] = "Use current list of defects";
char fmt_wpc[] = "\nWrite precompensation cyl = %u\n";
char hd_defects[] = "\n #   cyl  head  bytes\n";
char i_ileave[] = "\nWarning: Only %u blocks per track\n";
char i_sizes[] = "\n%u blks/trk, %u hds, %u cyls, %u blks (%uMB)\n";
char newline[] = "\n";
char no_yes[] = "ny";
char yes_no[] = "yn";
char pr_bbl[] = "\nEnter list of defects (MAX %u)\n\
A blank entry indicates the end\n\n";
char pr_fmt_blk_trk[] = "Enter maximum physical blocks per track";
char pr_fmt_cyls[] = "Enter number of cylinders";
char pr_fmt_heads[] = "Enter number of heads";
char pr_blk_size[] = "Enter block size";
char pr_fmt_acd[] = "Enter add, change, delete";
char pr_fmt_cyl[]	= "  cyl  ";
char pr_fmt_head[]	= "  head ";
char pr_fmt_bytes[]	= "  bytes";
char pr_fmt_def_nbr[] = "Enter defect number";
char pr_fmt_ileave[] = "Enter interleave factor";
char pr_fmt_lzone[] = "Enter landing zone position";
char pr_fmt_rwcc[] = "Enter reduced write current cyl";
char pr_fmt_show_sort[] = "Display list of sorted defects";
char pr_fmt_spares[] = "Enter spare blocks/track";
char pr_fmt_srate[] = "Enter seek speed (0=3ms, 1=28us, 2=12us)";
char pr_fmt_wpc[] = "Enter write precompensation cyl";
char pr_last_chance[] = "Last chance before formatting the disk... Proceed";
char pr_wr_anyway[] = "Write the diagnostic tracks anyway";
char pr_write_vtoc[] = "Write VTOC?";

#ifdef DEBUG
int debug;
#endif /* DEBUG */

main()
{
	printf("\nAdaptec Disk Formatter $Revision: 2.5 $\n");
	getdevice();

#ifdef DEBUG
	debug = number("\nEnable debugging (0=none, 1=more, 2=most)", 0);
	(void)ioctl(fd, SAIODEBUG, (caddr_t)debug);
#endif /* DEBUG */

	inq.opcode = SCSI_INQUIRY;
	inq.length = SIZE_INQ;
	if (ioctl(fd, SAIOSCSICMD, &inq) != 0)
	    printf("formatscsi: assuming drive is an UNFORMATTED 72Mb drive\n");
	if (inq.format != 0) {
		printf("%s: not an adaptec target adaptor.  Try sdformat.\n",
			filename);
		exit(1);
	}

	ioctl(fd, SAIODEVDATA, (caddr_t)&st);
	bzero(ud.buf, DBSIZE);
	ld_bb();
	get_fmt_info();
	if (answer(pr_last_chance, no_yes) == YES) {
		for (;;) {
			/*
			 * ioctl to set the modes before formatting
			 */
			if (ioctl(fd, SAIOSCSICMD, (caddr_t)&ud_mds)) {
				if (answer("Set Modes failed.  Retry", no_yes)
					== YES)
					continue;
				goto quit;
			}
			/*
			 * ioctl to do the format
			 */
			printf(Formatting);
			if (ioctl(fd, SAIOSCSICMD, (caddr_t)&ud_fmt)) {
				printf("Failed\n");
				if (answer("Format failed.  Retry", no_yes)
					== YES)
					continue;
				goto quit;
			}
			printf(Done);
			/*
			 * write the diagnostic data
			 */
			put_diag();
			/*
			 * write a minimal VTOC
			 */
			write_min_vtoc();
			break;
		}
	} else {
		/*
		 * No format.  Maybe we should write the diagnostic data anyway
		 */
		if (z.blk_size == UNIX_BLK_SIZE)
			if (answer(pr_wr_anyway, no_yes) == YES)
				put_diag();
		if (answer(pr_write_vtoc, no_yes) == YES) {
			if (vtoc == NULL)
				setup_min_vtoc();
			write_min_vtoc();
		}
	}
quit:
	close(fd);
	exit(0);
}

/*
 * Prompt and verify a device name from the user.
 */
getdevice()
{
	register char *cp;

top:
	cp = prompt("\nDevice to format? ==> ");
	if ((fd = open(cp, 2)) < 0) {		/* 2 === O_RDWR */
		register struct devsw *dp;

		printf("Known devices are: ");
		for (dp = devsw; dp->dv_name; dp++)
			printf("%s ", dp->dv_name);
		printf(newline);
		goto top;
	}
	/* save the filename */
	{	register char *fn;

		for (fn = filename; *fn = *cp; fn++, cp++);
	}
}

/*******************************************************************************
 *
 *		Routines form scsi disk test file a5500_st/data.c
 *
 ******************************************************************************/

/*
 * answer - get a user's 1-char response
 *		(return 0 on no input)
 */
int
answer(m, a)
char *m;	/* prompt string */
char *a;	/* response chars */
{
	char buff[32];
	register c;
	register char *p;

	printf(newline);
	for (;;) {
		printf("%s? [%s] ==> ", m, a);
		gets(buff);
		c = buff[0];
		if (!c)
			return(0);
		for (p = a; *p; p++)
			if (c == *p)
				return(c);
		printf(Error);
	}
}

/*
 * number - get a user's numeric response
 *		(return def on no input)
 *		*** DIFFERS FROM ORIGINAL ***
 */
int
number(m, def)
char *m;	/* prompt string */
{
	char buffer[32];
	register char *p;
	register n, i;

	for (;;) {
		printf("%s ==> ", m);
		gets(p = buffer);
		if (!*p)
			return(def);		/* no response */
		if (*p == '-') {
			i = -1;
			p++;
		} else
			i = 1;
		for (n = 0; *p; p++) {
			if (*p < '0' || *p > '9')
				break;
			n *= 10;
			n += *p;
			n -= '0';
		}
		n *= i;
		if (*p) {
			printf(Error);
			continue;
		}
		return(n);
	}
}

/*
 * itob4 - insert int i into a 4-byte field of a scsi buffer
 */
itob4(i, p)
register uint i;
register unchar *p;
{
	p[3] = (unchar) i;
	i >>= 8;
	p[2] = (unchar) i;
	i >>= 8;
	p[1] = (unchar) i;
	i >>= 8;
	p[0] = (unchar) i;
}

/*
 * itob3 - insert int i into a 3-byte field of a scsi buffer
 */
itob3(i, p)
register uint i;
register unchar *p;
{
	p[2] = (unchar) i;
	i >>= 8;
	p[1] = (unchar) i;
	i >>= 8;
	p[0] = (unchar) i;
}

/*
 * itob2 - insert int i into a 2-byte field of a scsi buffer
 */
itob2(i, p)
register uint i;
register unchar *p;
{
	p[1] = (unchar) i;
	i >>= 8;
	p[0] = (unchar) i;
}

/*
 * b4toi - get int i from a 4-byte field of a scsi buffer
 */
uint
b4toi(p)
register unchar *p;
{
	return (((((p[0] << 8) | p[1]) << 8) | p[2]) << 8) | p[3];
}

/*
 * b3toi - get int i from a 3-byte field of a scsi buffer
 */
uint
b3toi(p)
register unchar *p;
{
	return (((p[0]<< 8) | p[1]) << 8) | p[2];
}

/*
 * b2toi - get int i from a 2-byte field of a scsi buffer
 */
uint
b2toi(p)
register unchar *p;
{
	return (p[0]<< 8) | p[1];
}

/*
 * fill_pat - repeat a pattern
 */
fill_pat(src, nsrc, dst, ndst)
unchar *src;
uint nsrc;
unchar *dst;
uint ndst;
{
	register unchar *s = src;
	register unchar *d = dst;
	register unchar *sx = s + nsrc;
	register unchar *dx = d + ndst;

	while (d < dx) {
		*d++ = *s++;
		if (s >= sx)
			s = src;
	}
}

/*
 * blk_chk - check if arg is a valid block size
 *		*** DIFFERS FROM ORIGINAL ***
 */
int
blk_chk(siz)
uint siz;
{
	register uint u;

	/* see if it's generally valid */
	for (u = MINBLK; u; u += u)
		if (u == siz)
			return(FALSE);
	return(TRUE);
}

#ifdef DEBUG
/*
 * dump - print a hex byte dump
 *		*** DIFFERS FROM ORIGINAL ***
 */
dump(p, n)
register unchar *p;
{
	register i, j, k;

#define NBYTES 16
	for (i = 0; i < n; i++, p++) {
		if (!(i & (NBYTES - 1))) {
			/* starting a new line. look for duplicates */
			k = (i == 0) || (i + NBYTES >= n);
			if (!k) {
				for (j = 0; j < NBYTES && !k; j++) {
					k  = p[j] ^ p[j-NBYTES];
					k |= p[j] ^ p[j+NBYTES];
				}
			}
			if (!k) {
				/* got one. now skip up to a change */
				printf("\n...");
				do {
					i += NBYTES;
					p += NBYTES;
					for (j = 0; j < NBYTES && !k; j++)
						k |= p[j] ^ p[j+NBYTES];
				} while (!k && i + NBYTES < n);
				i--;
				p--;
			} else {
				printf("\n%x", i);
				hex_byte(":\t", *p);
			}
		} else if (!(i & ((NBYTES/2) - 1)))
			hex_byte("  ", *p);
		else
			hex_byte(" ", *p);
	}
	printf("\n\n");
}

hex_byte(s, c)	/* standalone printf can't handle %02x */
char *s;
unchar c;
{	printf("%s%x%x", s, (c >> 4) &0xf, c & 0xf);
}
#endif /* DEBUG */

/*******************************************************************************
 *
 *		Routines form scsi disk test file a5500_st/fmt.c
 *
 ******************************************************************************/

/*
 * chksum - compute the checksum for a diagnostic block
 */
ushort
chksum(buf, nbytes)
unchar *buf;
uint nbytes;
{
	register ushort sum = 0;
	register ushort *p = (ushort *) buf;
	register n = (int) nbytes;
	ushort old_cs = ((struct db *)p)->db_chksum;

	((struct db *)p)->db_chksum = CHECKSUM;
	for (; n > 0; n -= sizeof(*p))
		sum += *p++;
	((struct db *)buf)->db_chksum = old_cs;
	return sum;
}

/*
 * bbl_chk - check the blocks holding the bad block list
 */
int
bbl_chk(bbl)
register struct db *bbl;
{
	return	bbl->db_magic  != MAGIC_BBL ||
		bbl->db_chksum != chksum((unchar *)bbl, z.bbl_size);
}

/*
 * get_bbl - read the bad block list from the disk
 *		*** DIFFERS FROM ORIGINAL ***
 */
int
get_bbl()
{
	register struct bb *bp;
	register struct dlist *dl;
	register i;

	/* locate the last lba on the disk */
	if (get_last())
		return(TRUE);
#ifdef DEBUG
	if (debug)
		printf("diag_bbl=%u, diag_stop=%u\n", z.diag_bbl, z.diag_stop);
#endif /* DEBUG */
	z.bbl_size = DBSIZE;		/* nbr bytes in bbl blocks */
	if (lseek(fd, z.diag_bbl * UNIX_BLK_SIZE, 0))
		printf("Error. Can't seek to diagnostic data\n");
	else if (alread(fd, (char *)ud.buf, DBSIZE) != DBSIZE)
		printf("Error. Can't read diagnostic data\n");
	else if (bbl_chk(&ud.db)) {
		printf("Error. Invalid diagnostic data\n");
#ifdef DEBUG
		if (debug)
			dump(ud.buf, DBSIZE);
#endif /* DEBUG */
	} else {
#ifdef DEBUG
		if (debug > 1)
			dump(ud.buf, DBSIZE);
#endif /* DEBUG */
		z = ud.db.db_z;
		z.db_in = TRUE;
		/* load bb buffer */
		bp = bb;
		dl = ud_fmt.dlist;
		for (i = z.bb_nbr; i != 0; i--, bp++, dl++) {
			bp->bb_cyl = b3toi(dl->d_cyls);
			bp->bb_head = dl->d_heads;
			bp->bb_bytes = b4toi(dl->d_bytes);
		}
		return(FALSE);
	}
	bzero(ud.buf, DBSIZE);
	return(TRUE);
}

/*
 * Do a read into a 16 byte-aligned space.  This is done to satisfy the
 * requirement of the driver that all I/O be to space aligned to
 * 16 byte boundaries.  This routine is a bit wasteful of memory since
 * it does a calloc for every transfer.  Should be rewritten if it
 * is ever used more than once in formatscsi
 */

alread(fd, buf, n)

int fd;
char *buf;
int n;
{
	char *p;
	int nread;

	callocrnd(16);
	p = (char *)calloc(n);

	nread = read(fd, p, n);
	if (nread < 0)
		return(nread);

	bcopy(p, buf, nread);
	return(nread);
}

/*
 * Get mode select and format info
 *		*** DIFFERS FROM ORIGINAL ***
 */
get_fmt_info()
{
	register struct db *db = &ud.db;
	register struct bb *bp;
	register struct dlist *dl;
	register i, n;

	db->db_mds.m_type = SCSI_MODES;		/* mode select command */
	db->db_mds.m_ilen = SIZE_MODES_2;	/* size of mode select data */
	db->db_mds.m_dlen = SCSI_MODES_DLEN;	/* size of extent decriptors */
	db->db_mds.m_fcode = FMT_FCODE;
	db->db_fmt.f_type = SCSI_FORMAT;	/* format command */
	db->db_fmt.f_misc = FMT_ALL;	/* data set, cmplt set, use head/cyl */
	db->db_fmt.f_data = FMT_PAT;		/* data byte to format with */
	db->db_fmt.f_full = FMT_FULL;		/* full disk format  */
	db->db_fmt.f_pad1 = 0;

	/* Block Size */
	i = b3toi(db->db_mds.m_bsize);
	if (blk_chk((uint)i)) {
		i = z.blk_size;
		if (blk_chk((uint)i))
			i = UNIX_BLK_SIZE;	/* unix default */
	}
	printf(fmt_blk_size, i);
	do {
		i = number(pr_blk_size, i);
	} while (blk_chk((uint)i));
	z.blk_size = i;
	itob3((uint)i, db->db_mds.m_bsize);

	/* try to guess the max blks/trk */
	if (i == 256)
		z.blk_trk = NBTI256;
	else if (i == 512)
		z.blk_trk = NBTI512;
	else if (i == 1024 || !z.blk_trk)
		z.blk_trk = NBTI1024;

	/* Nbr Heads */
	n = db->db_mds.m_heads;
	if (n <= 0) {
		n = st.ntrak;
		if (n == 0)
			n = z.nbr_heads;
	}
	printf(fmt_heads, n);
	n = number(pr_fmt_heads, n);
	z.nbr_heads = n;
	db->db_mds.m_heads = n;

	/* Nbr Cyls */
	n = b2toi(db->db_mds.m_cyls);
	if (n <= 0) {
		n = st.ncyl;
		if (n == 0)
			n = z.nbr_cyls;
	}
	printf(fmt_cyls, n);
	n = number(pr_fmt_cyls, n);
	z.nbr_cyls = n;
	itob2((uint)n, db->db_mds.m_cyls);

	/* Nbr blocks/track */
	n = z.blk_trk;
	if (n <= 0)
		n = st.nsect + 1;
	printf(fmt_blk_trk, n);
	n = number(pr_fmt_blk_trk, n);
	z.blk_trk = n;

	/* Interleave */
	n = b2toi(db->db_fmt.f_ileave);
	if (n < 1 || n >= z.blk_trk) {
		n = z.interleave;
		if (n < 1 || n >= z.blk_trk)
			n = UNIX_INTERLEAVE;
	}
	printf(fmt_ileave, n);
	n = number(pr_fmt_ileave, n);
	z.interleave = n;
	itob2((uint)n, db->db_fmt.f_ileave);
	if (n == 1 && z.blk_size < MAXBLK)
		printf(i_ileave, --z.blk_trk);

	/* Spares */
	n = db->db_fmt.f_spares;
	if (n >= z.blk_trk) {
		n = z.nbr_spares;
		if (n >= z.blk_trk)
			n = 0;
	}
	printf(fmt_spares, n);
	n = number(pr_fmt_spares, n);
	db->db_fmt.f_spares = n;
	z.nbr_spares = n;
	z.blk_trk -= n;

	/* Report what's been specified */
	n = z.blk_trk * z.nbr_heads;
	z.blk_cyl = n;
	n *= z.nbr_cyls;
	printf(i_sizes, z.blk_trk, z.nbr_heads, z.nbr_cyls, n,
		n * z.blk_size / M1);

	/* Reduced Write Current Cylinder */
	n = b2toi(db->db_mds.m_rwcc);
	if (n <= 0 || n > z.nbr_cyls + 1)
		n = z.nbr_cyls + 1;
	printf(fmt_rwcc, n);
	n = number(pr_fmt_rwcc, n);
	itob2((uint)n, db->db_mds.m_rwcc);

	/* Write Precompensation Cylinder */
	printf(fmt_wpc, n);
	n = number(pr_fmt_wpc, n);
	itob2((uint)n, db->db_mds.m_wpc);

	/* Landing Zone Position */
	n = db->db_mds.m_lzone;
	printf(fmt_lzone, n);
	db->db_mds.m_lzone = (char)number(pr_fmt_lzone, n);

	/* Step Pulse Output Rate */
	printf(fmt_srate, SCSI_MODES_SRATE);
	db->db_mds.m_srate = (unchar)number(pr_fmt_srate, SCSI_MODES_SRATE);

	i = NO;
	n = 0;				/* non-neg ==> ask about display */
	if (z.bb_nbr) {
		if (answer(fmt_show_bbl, no_yes) == YES) {
			pr_defects();
			n = -1;		/* neg ==> don't ask about display */
		}
		i = answer(fmt_use_bbl, yes_no);
		if (!i)
			i = YES;
	}
	if (i != YES) {
		n = 0;			/* non-neg ==> ask about display */
		printf(pr_bbl, MAX_DEFECTS);
		for (z.bb_nbr = 0; z.bb_nbr < MAX_DEFECTS; z.bb_nbr++) {
		add_bb:
			if (!bb_input(z.bb_nbr))
				break;
		}
	}
	/* check for add/change/delete */
	for (;;) {
		bb_sort();
		/* ask if user wants display, and, if so, do the display */
		if (n >= 0) {
			printf(fmt_nbr_defs, z.bb_nbr);
			if (z.bb_nbr && (answer(pr_fmt_show_sort, no_yes) == YES))
				pr_defects();
		}
		n = 0;		/* always ask about display from now on */
		i = answer(pr_fmt_acd, acd);
		if (!i || i == NO)
			break;
		printf(newline);
		if (i == ADD)			/* add */
			goto add_bb;
		while (z.bb_nbr && (n = number(pr_fmt_def_nbr, 0))) {
			n--;
			if (i == CHG) {		/* change */
				if (!bb_input((uint)n))
					break;
				continue;
			}
			/* delete */
			if (z.bb_nbr == 1)
				z.bb_nbr = 0;
			/* force delete when sort occurs */
			else
				bb[n].bb_type = BB_TYPE_DEL;
		}
	}

	/* length of defect list */
	itob2((uint)(z.bb_nbr * sizeof(struct dlist)), db->db_fmt.f_dlen);

	/* load fmt buffer */
	bp = bb;
	dl = db->db_fmt.dlist;
	for (i = 0; i < z.bb_nbr ; i++, bp++, dl++) {
		itob3(bp->bb_cyl, dl->d_cyls);
		dl->d_heads = (unchar)bp->bb_head;
		itob4(bp->bb_bytes, dl->d_bytes);
	}
	bzero((unchar *)dl, sizeof(struct dlist) * (MAX_DEFECTS - i));

	/* set up the block fields */
	mk_bbl(&ud.db);

	/* display standard VTOC and allow user to change */
	setup_min_vtoc();
}


/*
 * pr_defects - print internal defects list
 */
pr_defects()
{
	register struct bb *bp = bb;
	register i;

	printf(hd_defects);
	for (i=1; i <= z.bb_nbr; i++, bp++) {
		/*
		 * The standalone printf doesn't support width specification.
		 * The code below is to implement:
		 * char d_defect[] = "%2u %5u %5u %6u\n";
		 * printf(d_defect, i, bp->bb_cyl, bp->bb_head, bp->bb_bytes);
		 */
		pr_unsigned(i, 2);
		pr_unsigned(bp->bb_cyl, 6);
		pr_unsigned(bp->bb_head, 6);
		pr_unsigned(bp->bb_bytes, 7);
		putchar('\n');
	}
}

/*
 * pr_unsigned - print the unsigned int "x" as a decimal,
 *		right-justified in "n" chars
 */
pr_unsigned(x, n)
register uint x;
register n;
{
	char nbr[8];
	register char *p = nbr;

	do {
		*p++ = '0' + x % 10;
		n--;
		x /= 10;
	} while (x);
	/* print the leading spaces */
	for (; n > 0; n--)
		putchar(' ');
	/* print the number */
	while (p > nbr)
		putchar(*--p);
}

/*
 * bb_input - input one bad block
 */
int
bb_input(n)
uint n;
{
	register struct bb *bp;
	register i, j, k;

	printf(fmt_defect, n + 1);
	i = number(pr_fmt_cyl, -1);
	if (i < 0)
		return(FALSE);
	j = number(pr_fmt_head, -1);
	if (j < 0)
		return(FALSE);
	k = number(pr_fmt_bytes, -1);
	if (k < 0)
		return(FALSE);
	bp = &bb[n];
	bp->bb_cyl = i;
	bp->bb_head = j;
	bp->bb_bytes = k;
	bb->bb_type = BB_TYPE_CHB;
	return(TRUE);
}

/*
 * bb_sort - sort the bad block list; discard any duplicates
 */
bb_sort()
{
	register struct bb *b;
	register i, n;
	register count = z.bb_nbr;
	struct bb t;

	/* first delete and records tagged for deletion */
	for (i = 0, b = bb; i < count;)
		if (b->bb_type == BB_TYPE_DEL) {
			--z.bb_nbr;
			--count;
			*b = bb[count];
		} else {
			i++;
			b++;
		}
	/* now sort them */
	count--;
	for (n = 0; n < count; n++) {
		for (i = 0, b = bb; i < count;) {
			if (b[0].bb_cyl < b[1].bb_cyl)
				goto next;
			if (b[0].bb_cyl > b[1].bb_cyl)
				goto swap;
			if (b[0].bb_head < b[1].bb_head)
				goto next;
			if (b[0].bb_head > b[1].bb_head)
				goto swap;
			if (b[0].bb_bytes < b[1].bb_bytes)
				goto next;
			if (b[0].bb_bytes > b[1].bb_bytes)
				goto swap;
			/* equal: delete the second */
			b[1] = bb[count];
			--z.bb_nbr;
			--count;
			continue;
		swap:
			t = b[0]; b[0] = b[1]; b[1] = t;
		next:
			i++;
			b++;
		}
	}
}

/*
 * mk_bbl - make the bbl constants in bbl
 */
mk_bbl(bbl)
register struct db *bbl;
{
	bbl->db_magic = MAGIC_BBL;
	bbl->db_blkno = z.diag_bbl;
	bbl->db_z = z;
	bbl->db_chksum = chksum((unchar *)bbl, z.bbl_size);
}

/*
 * mk_diag - make a diag block in the buffer db
 *		*** DIFFERS FROM ORIGINAL ***
 */
mk_diag(db, lba)
register struct db *db;
uint lba;
{
	fill_pat(pat_default, sizeof pat_default, (unchar *)db, z.blk_size);
	db->db_magic = MAGIC_DIAG;
	db->db_blkno = lba;
	db->db_chksum = chksum((unchar *)db, z.blk_size);
}

/*
 * wrt-diag - write the diagnostics blocks (other than the bad block list)
 *		*** DIFFERS FROM ORIGINAL ***
 */
wrt_diag()
{
	register unchar *p;
	register uint lba, i, n;

	n = DBSIZE / z.blk_size;
	for (lba = z.diag_start; lba < z.diag_bbl; lba += n) {
		if (lba + n > z.diag_bbl)
			n = z.diag_bbl - lba;
		for (p = ud.buf, i = 0; i < n; p += z.blk_size, i++)
			mk_diag((struct db *)p, lba + i);
		i *= z.blk_size;
		if (alwrite(fd, ud.buf, i) != i) {
			printf("\nError. Writing diagnostic block %u\n", lba);
			return;
		}
	}
	z.diag_hit = FALSE;
}

/*
 * Do a write from a 16 byte-aligned space.  This is done to satisfy the
 * requirement of the driver that all I/O be to space aligned to
 * 16 byte boundaries.  This routine assumes that the I/O will
 * never be greater than DBSIZE.
 */

char *alwp;

alwrite(fd, buf, n)
int fd;
char *buf;
int n;
{
	if (alwp == 0) {
		callocrnd(16);
		alwp = (char *)calloc(DBSIZE);
	}
	bcopy(buf, alwp, n);

	return(write(fd, alwp, n));
}

/*
 * get_last - locate the last block on the disk
 */
int
get_last()
{
	/* Read Capacity to locate the last lba on the disk */
	if (do_cap(0, SCSI_FULL_CAP))
		return(TRUE);
	if (cap_size != UNIX_BLK_SIZE) {
		printf("Error. Block size = %u, expected %u\n",
			cap_size, UNIX_BLK_SIZE);
		return(TRUE);
	}
	z.blk_size = UNIX_BLK_SIZE;
	z.diag_stop = cap_lba;
	z.diag_bbl = cap_lba + 1 - DBSIZE / UNIX_BLK_SIZE;
	return(FALSE);
}

/*
 * set_z - locate diag_start, diag_bbl & diag_stop on the newly formatted disk.
 *		don't use Read Capacity, which may be broken.
 *		set up as much of z as possible for final write.
 */
set_z()
{
	struct bb btemp;
	register uint lba;

	/* locate diag_stop & diag_bbl */
	if (get_last())
		return(TRUE);

	/* first Read Capacity to locate diag_start */
	lba = z.diag_stop - 5 * z.blk_cyl / 2;
	if (do_cap(lba, SCSI_PART_CAP) ||
		cap_lba == 0 || cap_lba < lba || cap_lba >= z.diag_bbl) {

		/* figure diag_start */
		set_bb();
		btemp.bb_cyl = z.nbr_cyls - 2;	/* 2nd-to-last cyl nbr */
		btemp.bb_head = 0;		/* head 0 */
		btemp.bb_bytes = BO_512;	/* bytes = common offset */
		chbtolba(&btemp);
		z.diag_start = btemp.bb_lba;	/* start of diag tracks */
	} else
		z.diag_start = cap_lba + 1;	/* start of diag tracks */

#ifdef DEBUG
	if (debug)
		printf("diag_start=%u, diag_bbl=%u, diag_stop=%u\n",
			z.diag_start, z.diag_bbl, z.diag_stop);
#endif /* DEBUG */
	z.blk_disk = z.diag_stop + 1;	/* disk's actual blocks/disk */
	z.bbl_size = DBSIZE;		/* nbr bytes in bbl blocks */
	z.blk_clust = 0;		/* block cluster for this disk */
	z.no_format = FALSE;		/* TRUE ==> disk's format is blown */
	z.diag_hit = FALSE;		/* TRUE ==> diag tracks need re-write */
	z.db_in = TRUE;			/* TRUE ==> db is in memory ok */
	return(FALSE);
}

/*
 * write the diagnostic tracks after a test
 *		*** DIFFERS FROM ORIGINAL ***
 */
put_diag()
{
	if (z.blk_size != UNIX_BLK_SIZE)
		return;
	if (set_z())
		return;
	mk_bbl(&ud.db);
#ifdef DEBUG
	if (debug > 1)
		dump(ud.buf, DBSIZE);
#endif /* DEBUG */
	if (lseek(fd, z.diag_bbl * UNIX_BLK_SIZE, 0)) {
		printf("\nError. Can't seek to block %u\n", z.diag_bbl);
		return;
	}
	printf(Writing, z.diag_start);
	if (alwrite(fd, ud.buf, DBSIZE) != DBSIZE) {
		printf("\nError. Can't write diagnostic data at block %u\n",
			z.diag_bbl);
		return;
	}
	if (lseek(fd, z.diag_start * UNIX_BLK_SIZE, 0)) {
		printf("\nError. Can't seek to block %u\n", z.diag_start);
		return;
	}
	wrt_diag();
	printf(Done);
}

/*******************************************************************************
 *
 *		New Routines
 *
 ******************************************************************************/

/*
 * do_cap() - do a Read Capacity command
 *		*** DIFFERS FROM ORIGINAL in a5500_st/nio.c ***
 */
int
do_cap(lba, flg)
uint lba;
unchar flg;		/* full or partial media indicator */
{
	unchar cap[C01SIZE + SIZE_CAP];

	bzero(cap, sizeof cap);
	cap[0] = SCSI_READC;
	itob4(lba, cap + 2);
	cap[8] = flg;
	/* ioctl to read capacity to locate the last lba on the disk */
	if (ioctl(fd, SAIOSCSICMD, (caddr_t)cap)) {
		printf("Error. Can't read capacity. lba = %u, flg = %u\n",
			lba, flg);
		return(TRUE);
	}
	cap_lba  = b4toi(cap + C01SIZE);	/* last lba */
	cap_size = b4toi(cap + C01SIZE + 4);	/* block size */
	if (cap_size != UNIX_BLK_SIZE) {
		printf("Error. Read Capacity block size = %u, expected %u\n",
			cap_size, UNIX_BLK_SIZE);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * scani - scan input for a number
 */
char *
scani(p, nbr)
register char *p;
int *nbr;
{
	register n = 0;

	/* leading whitespace */
	for (; *p == ' ' || *p == '\t'; p++);
	for (; *p >= '0' && *p <= '9'; p++) {
		n *= 10;
		n += *p;
		n -= '0';
	}
	*nbr = n;
	/* trailing whitespace */
	for (; *p == ' ' || *p == '\t'; p++);
	return(p);
}

/*******************************************************************************
 *
 *		Routines form scsi disk test file a5500_st/bad.c
 *
 ******************************************************************************/

/*
 * lba_calc - calculate the absolute lba, assuming no bad blocks
 */
uint
lba_calc(bp)
register struct bb *bp;
{
	register uint lba;

	lba = bp->bb_cyl;
	lba *= z.nbr_heads;
	lba += bp->bb_head;
	lba *= z.blk_trk;
	lba += bp->bb_bytes / bytes_blk;
	return bp->bb_lba = lba;
}

/*
 * set_bb - set up the bad block list for chbtolba()
 *		*** NEW ***
 */
set_bb()
{
	register struct bb *bp;
	register n, nbad;

	/*
	 * set the bytes/block, etc
	 */
	n = z.blk_size;
	if (n == 256) {
		bytes_blk = BB_256;
		byte_offset = BO_256;
	} else if (n == 512) {
		bytes_blk = BB_512;
		byte_offset = BO_512;
	} else if (n == 1024) {
		bytes_blk = BB_1024;
		byte_offset = BO_1024;
	} else {	/* ??? */
		bytes_blk = n + BB_512 - 512;
		byte_offset = n / 2;
	}
	bytes_trk = bytes_blk * z.blk_trk;

	/*
	 * compute the "lba" for the bad block entries
	 */
	for (n = 0, bp = bb; n < z.bb_nbr; n++, bp++)
		bp->bb_lba = lba_calc(bp);
	/*
	 * now adjust the lba's according to the preceeding bad blocks
	 */
	for (nbad = n = 0, bp = bb; n < z.bb_nbr; n++, bp++) {
		bp->bb_lba -= nbad;
		if (bp->bb_bytes >= bytes_trk)
			continue;
		if (n + 1 < z.bb_nbr && bp[0].bb_lba + nbad == bp[1].bb_lba)
			continue;
		nbad++;
	}
#ifdef DEBUG
	if (debug) {
		printf("bytes_blk = %u, bytes_trk = %u, byte_offset = %u\n",
			bytes_blk,	bytes_trk,	byte_offset);
		if (debug > 1)
			for (n = 1, bp = bb; n <= z.bb_nbr; n++, bp++)
				printf("bb[%u]\t%u\t%u\t%u\t%u\n", n,bp->bb_cyl,
					bp->bb_head, bp->bb_bytes, bp->bb_lba);
	}
#endif
}

/*
 * chbtolba - translate a cyl/head/byte reference to an lba
 *		*** DIFFERS FROM ORIGINAL ***
 */
int
chbtolba(bp)
register struct bb *bp;
{
	register struct bb *bbp;
	register uint n, k;

	/* get the un-adjusted lba */
	k = lba_calc(bp);

	/*
	 * decrement the lba for every unique bad block <= lba
	 *	and whose bytes lie within the used portion of a track
	 */
	for (bbp = bb, n = 0; n < z.bb_nbr; n++, bbp++) {
		if (bbp->bb_lba > k)
			break;
		if (bbp->bb_bytes >= bytes_trk)
			continue;
		if (n && bbp[0].bb_lba == bbp[-1].bb_lba)
			continue;
		k--;
	}
	bp->bb_lba = k;
	return((int)k);
}

/*
 * ld_bb - load bb from either a host file or the disk's diag tracks
 *		*** DIFFERS FROM ORIGINAL ***
 */
ld_bb()
{
#define N	128	/* size of line buffer */
	char *buf;
	register struct bb *bp;
	register char *p;
	register i, n;
	register df;	/* file descriptor */
	int cyl, head, bytes;

	callocrnd(16);
	buf = (char *)calloc(N + 1);
	z.bb_nbr = 0;
	if (answer("Does disk have diagnostics data already", no_yes) == YES) {
		/*
		 * read the disk's diagnostics tracks
		 */
		if(!get_bbl())
			return;
	}
	if (answer("Does defect list exist in a file", no_yes) != YES)
		return;

	/*
	 * read the bad block list from a file
	 */
	for (;;) {
		p = prompt("Filename with defect list? ==> ");
		if (!*p)
			return;
		if ((df = open(p, 0)) >= 0)	/* 0 == O_RDONLY */
			break;
	}
	bp = bb;
	for (n = 0, p = buf; z.bb_nbr < MAX_DEFECTS; p++, n -= p - buf) {
		for (i = 0; i < n; i++)
			buf[i] = p[i];
		i = read(df, buf + n, N - n);
		if (i > 0)
			n += i;
		if (n == 0)
			break;
		p = buf;
		p[n] = 0;
		p = scani(p, &cyl);
		if (*p == '\n')
			continue;
		p = scani(p, &head);
		if (*p == '\n')
			continue;
		p = scani(p, &bytes);
		if (*p != '\n')
			break;
		bp->bb_cyl = cyl;
		bp->bb_head = head;
		bp->bb_bytes = bytes;
		bb->bb_type = BB_TYPE_CHB;
		bp++;
		z.bb_nbr++;
	}
	close(df);
	bb_sort();
}

/*
 * setup_min_vtoc
 *	prompt for VTOC values	
 */
setup_min_vtoc()
{
	char *cp;
	struct drive_type *drive = drive_table;
	int partno = 1;
	struct partition part; 

	part = drive_table[0].dt_part;
	callocrnd(16);		/* align a la SCED alignment requirements */
	vtoc = (struct vtoc *)calloc(V_SIZE);

	printf("Standard minimal VTOC for this disk:\n\n");
	do {
		printf("Partition: %d\n", partno);
		printf("Offset in sectors: %d\n", part.p_start);
		printf("Size in sectors: %d\n", part.p_size);
		cp = prompt("Use this layout [y/n]? ");
		if (*cp != 'y' && *cp != 'Y') {
			partno = atoi(prompt("Partition number (0-254)? "));
			part.p_start = atoi(prompt("Offset (sectors)? "));
			part.p_size = atoi(prompt("Size (sectors)? "));
		}
	} while (*cp != 'y' && *cp != 'Y');

	if (do_cap(0, SCSI_FULL_CAP)) {
		printf("Unable to read capacity - not writing VTOC\n");
		vtoc = NULL;
		return;
	}

	vtoc->v_sanity = VTOC_SANE;
	vtoc->v_version = V_VERSION_1;
	vtoc->v_size = sizeof(struct vtoc);
	vtoc->v_nparts = partno + 1;
	vtoc->v_secsize = DEV_BSIZE;
	vtoc->v_ntracks = drive->dt_st.ntrak;
	vtoc->v_nsectors = drive->dt_st.nsect;
	vtoc->v_ncylinders = drive->dt_st.ncyl;
	vtoc->v_rpm = DFLT_RPM;
	vtoc->v_capacity = cap_lba + 1;
	vtoc->v_nseccyl = drive->dt_st.nspc;
	(void)strcpy(vtoc->v_disktype, drive->dt_diskname); 
	vtoc->v_part[partno] = part;
	vtoc->v_cksum = 0;
	vtoc->v_cksum = vtoc_get_cksum(vtoc);
}

/*
 * write_min_vtoc
 *	write the VTOC
 */
write_min_vtoc()
{
	int where;

	if (vtoc == NULL)		/* vtoc not set up */
		return;
	printf("Writing VTOC to sector %d...", V_VTOCSEC);
	where = V_VTOCSEC << DEV_BSHIFT;
	lseek(fd, where, 0);
	if (write(fd, vtoc, V_SIZE) != V_SIZE) {
		printf("Unable to write VTOC\n");
		return;
	}
	printf("Done\n");
}
