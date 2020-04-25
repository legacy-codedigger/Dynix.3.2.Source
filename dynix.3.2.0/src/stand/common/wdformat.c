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


#include <sys/file.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vtoc.h>
#include "saio.h"
#include "scsi.h"
#include "ccs.h"
#include "scsiioctl.h"
#include "sdformat.h"
#include "scsidisk.h"

#define LINELIMIT	128
#define NIL		-999

char *prompt();

extern	struct	drive_type wddrive_table[];
extern	int	maxbadlist;		/* maximum number of bad blocks */
extern	int	list[];			/* list of bad blocks to be added */

struct	modearg modearg;
int	fd = -1;			/* fd for disk we are working on */
char	filename[LINELIMIT];		/* name of disk */
int	addindex, getindex;		/* ptrs into bad block list */
struct	drive_type *drive;		/* drive-specific information */
int	capacity = 0;

struct task_type task_table[] = {

	{ EXIT,		"Exit from wdformat",				},
	{ FORMAT,	"Format disk using existing bad block lists",	},
	{ ADDBAD,	"Addbad - add blocks to existing badlists",	},
	{ WRITEVTOC,    "Write a new minimal VTOC",                     },
	{ -1,		},
};


/*
 * wdformat - standalone formatter for Common Command Set (CCS) SCSI disks
 *
 */

main()
{
	int token;
	char *cp;

	printf("\nSSM SCSI Disk Formatter\n");

	get_disk_name();
	if (! is_CCS_disk())
		exit(1);
	
	get_disk_type();

	/*
	 * main command loop
	 */
	
	display_menu();

	for(;;) {
		cp = prompt("task? ");
		if (!*cp) {
			display_menu();
			continue;
		}
		token = xatoi(cp);
		switch (token) {

		case FORMAT:
			format();
			cp = prompt("Write VTOC? [n/y] ");
			if (*cp == 'y' || *cp == 'Y')
				write_min_vtoc();
			cp = prompt("Write diagnostic patterns? [n/y] ");
			if (*cp == 'y' || *cp == 'Y')
				write_diag();
			break;

		case ADDBAD:
			addbad();
			break;

		case DISPLAY:
			display_defects();
			break;

		case WRITEVTOC:
			write_min_vtoc();
			break;

		case EXIT:
			exit(0);

		default:
			printf("wdformat: bad selection\n");
			display_menu();
			break;
		}
	}
}

/*
 * determine the name of the disk to be formatted and open it, putting
 * the descriptor in fd.
 */

get_disk_name()
{
	extern char *index();
	register char *pp;
	char partbuf[20];
	char *dp, *cp, *fn;
	int n = V_NUMPAR;

	/*
	 * Load up partbuf with the character translation of V_NUMPAR.
	 */
	bzero(partbuf, 20);
	pp = partbuf + sizeof(partbuf) - 1;
	*pp-- = '\0';
	do {
		*pp-- = "0123456789"[n%10];
		n /= 10;
	} while (n);
	pp++;

	for (fn = filename; *fn = *cp; fn++, cp++);

	while (fd < 0) {
		cp = prompt("\nDevice to format? (or q to quit) ");

		if (cp[0] == 'q' && cp[1] == '\0')
			exit(0);

		if (cp[0] == 'w' && cp[1] == 'd') {
			/* 
			 * Copy name to filename
			 */
			for (fn = filename; *fn = *cp; fn++, cp++);
			/* 
			 * Replace partition number with V_NUMPAR
			 */
			dp = index(filename, ',');
			dp++;
			strcpy(dp, pp);
			dp = filename + strlen(filename);
			strcpy(dp, ")");
			/*
			 * Now open
			 */
			if ((fd = open(filename, O_RDWR)) < 0) {
				printf("Open failed. Check input, cables ");
				printf("and drive switch settings.\n");
				printf("Example for target adaptor 5: ");
				printf("wd(40,0)\n");
			}
		} else {
			printf("Example for target adaptor 5: wd(40,0)\n");
			continue;
		}
	}
}

/*
 * Decide if this is a supported CCS disk or not.  This is currently
 * implemented by doing an INQUIRY command and looking at the
 * format byte.
 */

is_CCS_disk()
{
	struct inqarg inqarg;

	/*
	 * set up the command block.  A short (4 byte) INQUIRY will be done.
	 */

	bzero((caddr_t) &inqarg, sizeof(struct inqarg));
	inqarg.cmd.cmd_opcode = SCSI_INQUIRY;
	inqarg.cmd.cmd_length = SIZE_INQ;

	if (ioctl(fd, SAIOSCSICMD, (char *)&inqarg) < 0) {
		printf("wdformat: problems doing INQUIRY\n");
		exit(1);
	}

	if (inqarg.inq.sdq_format == 0) {
		printf("wdformat: This is not a supported CCS disk.  ");
		printf(" Try formatscsi.\n");
		return(0);
	}

	return(1);
}


/*
 * Do an INQUIRY command on the disk, and look up the vendor and
 * product IDs in the wddrive_table to determine the geometry.  Also,
 * do a READ CAPACITY to sanity-check addbads against.
 */

get_disk_type()
{
	int i;
	struct inqarg inqarg;
	struct drive_type *dp;
	struct readcarg readcarg;

	/*
	 * set up the inquiry command
	 */
	
	bzero((caddr_t) &inqarg, sizeof(struct inqarg));
	inqarg.cmd.cmd_opcode = SCSI_INQUIRY;
	inqarg.cmd.cmd_length = SIZE_INQ_XTND;

	if (ioctl(fd, SAIOSCSICMD, (char *)&inqarg) < 0) {
		printf("wdformat: failed on INQUIRY command\n");
		exit(1);
	}

	/*
	 * print out the id string from the INQUIRY command.  Need to
	 * count the chars because drive does not null-terminate the
	 * strings.
	 */

	printf("\n%s ID string: vendor ", filename);
	for (i = 0; i < SDQ_VEND; i++)
		printf("%c", inqarg.inq.sdq_vendor[i]);
	printf(", product ");
	for (i = 0; i < SDQ_PROD; i++)
		printf("%c", inqarg.inq.sdq_product[i]);
	printf(", revision ");
	for (i = 0; i < SDQ_REV; i++)
		printf("%c", inqarg.inq.sdq_revision[i]);
	printf("\n\n");

	/*
	 * search the drive table for the ID string
	 */

	for (dp = wddrive_table; dp->dt_vendor; dp++)
		if (strncmp(inqarg.inq.sdq_vendor, dp->dt_vendor, SDQ_VEND) == 0
		&& strncmp(inqarg.inq.sdq_product, dp->dt_product, SDQ_PROD) == 0)
			break;

	if (!dp) {
		printf("wdformat: could not find ID string in drive table\n");
		exit(1);
	}

	drive = dp;

	if (inqarg.inq.sdq_format != drive->dt_inqformat) {
		printf("%s: Not a supported CCS Disk.\n", filename);
		exit(1);
	}

	/*
	 * set up for READ CAPACITY
	 */
	
	bzero((caddr_t) &readcarg, sizeof(struct readcarg));
	readcarg.cmd.cmd_opcode = SCSI_READC;

	if (ioctl(fd, SAIOSCSICMD, (char *)&readcarg) < 0) {
		printf("wdformat: failed on READ CAPACITY command\n");
		exit(1);
	}

	capacity = (readcarg.nblocks[0] << 24) |
			(readcarg.nblocks[1] << 16) |
			(readcarg.nblocks[2] << 8) |
			(readcarg.nblocks[3]);

}
		

display_menu()
{
	struct task_type *tp;

	printf("Task Menu:\n\n");
	for (tp = task_table; tp->tt_desc; tp++)
		printf("(%d)       %s\n", tp->tt_cmd, tp->tt_desc);

	printf("\n");
}


/*
 * add bad blocks to the disk's G-list
 */

addbad()

{
	int valid = 0;
	char *cp;
	char *file;
	int finished;
	int n;
	int spot;
	int bfd;


	while (! valid) {
		printf("Enter defect list manually (%d) or from a file (%d)? ",
			MANUALLY, FILE);
		cp = prompt("");

		switch (xatoi(cp)) {

		case MANUALLY:
			printf("Enter list of defects, %d maximum\n",
				maxbadlist);
			printf("Entries should be positive integers between ");
			printf("0 and %d inclusive\n\n", capacity);
			printf("(type RETURN to terminate list)\n");

			initlist();
			init_get_spot();
			finished = n = 0;

			while (! finished) {
				printf("Enter logical address of bad spot %d: ",
					n++);
				spot = get_spot(-1);

				if (spot == -1) {
					printf("illegal input\n");
					n--;
					continue;
				}
						
				if (spot > capacity) {
					printf("%d is greater than disk capacity (%d)\n",
						spot, capacity);
					n--;
					continue;
				}
				if (addlist(spot) < 0 || spot == NIL)
					finished = 1;
			}

			cp = prompt("Proceed with addbad? [n/y] ");
			if (*cp == 'y' || *cp == 'Y')
				while ((spot = getlist()) != NIL)
					if (reassign_blocks(spot) < 0) {
						printf("aborting addbad\n");
						break;
					}
			valid = 1;
			break;

		case FILE:

			file = prompt("File with bad spot information? ");
			if ((bfd = open(file, O_RDONLY)) < 0) {
				printf("wdformat: cannot open %s\n", file);
				continue;
			}

			/*
			 * get bad spots from the file, adding them to
			 * the list.  list is NIL terminated.
			 */
			
			initlist();
			init_get_spot();

			while ((spot = get_spot(bfd)) != NIL) {

				if (spot == -1)
					continue;
					
				if (spot > capacity) {
					printf("%d is greater than disk capacity (%d)\n",
						spot, capacity);
					continue;
				}

				if (addlist(spot) < 0)
					break;
			}

			(void) addlist(NIL);

			cp = prompt("Proceed with addbad? [n/y] ");
			if (*cp == 'y' || *cp == 'Y')
				while ((spot = getlist()) != NIL)
					if (reassign_blocks(spot) < 0) {
						printf("aborting addbad\n");
						break;
					}
			close(bfd);

			valid = 1;
			break;

		default:
			printf("wdformat: bad option\n");
			break;
		}
	}
}

/*
 * badlist management - initialize the internal badlist
 */

initlist()
{
	int i;

	for (i = 0; i < maxbadlist; i++)
		list[i] = NIL;

	addindex = getindex = 0;

}

/*
 * add an item to the badlist
 */

addlist(spot)

int spot;
{
	
	if (addindex == maxbadlist) {
		printf("No more bad blocks.  Add the existing list first\n");
		return(-1);
	}

	list[addindex++] = spot;
	if (spot != NIL)
		printf(" -- added %d to list\n", list[addindex - 1]);
	return(0);
}

/*
 * retreive the next bad spot from the list.  Termination condition
 * is a NIL entry or maxbadlist elements reached.
 */

getlist()
{
	if ((getindex == maxbadlist) || (list[getindex] == NIL))
		return(NIL);
	else
		return(list[getindex++]);
}

int  count;
char addbuf[1024];
char *cp;
char *lastline;

init_get_spot()
{
	count = 0;
	cp = lastline = addbuf;
}

/*
 * input the address of a bad spot.  If fd is non-negative, input
 * from that file.  Otherwise, input from the console.
 */

get_spot(ifd)

int ifd;
{
	int nread;
	int lba;

	if (ifd < 0) {				/* console input */

		cp = prompt("");
		if (!*cp)
			return(NIL);
		return(xatoi(cp));

	} else {
		if (cp == lastline) {
			count = count - (lastline - addbuf);
			bcopy(lastline, addbuf, count);
			lastline = &addbuf[count];
			/* note kludge for ts tape */
			nread = read(ifd, lastline,
				((sizeof addbuf) - count) & ~511);
			count += nread;

			/*
			 * replace newlines with null
			 */

			for (cp = addbuf; cp < addbuf + count; cp++)
				if (*cp == '\n') {
					*cp = 0;
					lastline = cp + 1;
				}
			cp = addbuf;
		}
		if (cp == lastline) {
			/*
			 * empty buffer
			 */
			 cp = addbuf;
			 addbuf[0] = 0;
		}

		lba = xatoi(cp);
		if (lba < 0) {
			printf("illegal data in input file:\n");
			printf("%s\n", cp);
			lba = -1;
		}

		if (!*cp)
			return(NIL);
		for (; *cp++; )
			continue;
		return (lba);
	}
}

/*
 * tell the disk to revector a bad block
 */

reassign_blocks(block)

int block;
{
	struct reassarg reassarg;

	bzero((caddr_t) &reassarg, sizeof (struct reassarg));
	reassarg.cmd.cmd_opcode = SCSI_REASS;
	reassarg.length[1] = drive->dt_reasslen;
	itob4(block, reassarg.defect);

#ifdef DEBUG
	{
		u_char *bptr = (u_char *)&reassarg;
		int i;

		printf("Bytes being sent to REASSIGN BLOCKS:");
		for (i = 0; i < sizeof(struct reassarg); i++)
			printf(" 0x%x", *bptr++);
		printf("\n");
	}
#endif
	printf(" -- revectoring %d --\n", block);
	if (ioctl(fd, SAIOSCSICMD, (char *)&reassarg) < 0) {
		printf("wdformat: problem with REASSIGN BLOCKS\n");
		return(-1);
	}
	return(0);
}

/*
 * do the actual disk formatting.  This routine does a generic format of
 * the P and G lists only.
 */

format()
{
	register char *chp;
	struct formarg formarg;

	bzero((caddr_t) &formarg, sizeof(struct formarg));

	chp = prompt("Last chance before format ... proceed? [n/y] ");
	if (*chp == 'y' || *chp == 'Y') {
		
		printf("Beginning disk format ... \n");
		formarg.cmd.cmd_opcode = SCSI_FORMAT;
		formarg.cmd.cmd_lun = drive->dt_formcode;

#ifdef DEBUG
		{
			u_char *bptr = (u_char *)&formarg;
			int i;

			printf("Bytes being sent to FORMAT UNIT:");
			for (i = 0; i < sizeof(struct formarg); i++)
				printf(" 0x%x", *bptr++);
			printf("\n");
		}
#endif
		if (ioctl(fd, SAIOSCSICMD, (char *)&formarg) < 0)
			printf("wdformat: format failed\n");
		else 
			printf("Format complete\n");
	}
}

write_diag()
{
	register struct csd_db *db, *db_buf;
	register int i, n, count;
	register u_int lba, diag_end, diag_start;
	struct st st;			/* device data */
	struct readcarg readcarg;	/* read capacity data */
	static u_char pat_default[] = {	 /* diagnostic track pattern */
		CSD_DIAG_PAT_0, CSD_DIAG_PAT_1, CSD_DIAG_PAT_2,
		CSD_DIAG_PAT_3, CSD_DIAG_PAT_4
	};

	/* get the device data to determine where the diag blocks start */
	if (ioctl(fd, SAIODEVDATA, (char *)&st) < 0) {
		printf("wdformat: can't get device data\n");
		exit(1);
	}

	/* read the capacity to determine where the diag blocks stop */
	bzero((caddr_t) &readcarg, sizeof(struct readcarg));
	readcarg.cmd.cmd_opcode = SCSI_READC;
	if (ioctl(fd, SAIOSCSICMD, (char *)&readcarg) < 0) {
		printf("wdformat: failed on READ CAPACITY command\n");
		exit(1);
	}
	diag_end = (readcarg.nblocks[0] << 24) | (readcarg.nblocks[1] << 16)
		 | (readcarg.nblocks[2] << 8)  | (readcarg.nblocks[3]);
	diag_end++;	/* one beyond the last block */

	/* two cylinders worth of diagnostic data */
	diag_start = diag_end - (st.nspc * 2);

	/* get a buffer to hold one cylinder, making sure it's aligned */
	callocrnd (SCSI_XFER_ALIGN);
	db_buf = (struct csd_db *)calloc(st.ncyl * sizeof(*db));
	if (!db_buf) {
		printf("wdformat: calloc failed.  No buffer for patterns.\n");
		exit(1);
	}

	/* lay in the patterns; they're identical in all blocks */
	for (db = db_buf, i = 0; i < st.nspc; db++, i++)
		fill_pat(pat_default, sizeof pat_default,
			db->csd_db_pattern, sizeof(db->csd_db_pattern));

	printf("Writing diagnostic tracks (%d..%d)... \n",
		diag_start, diag_end-1);

	if (lseek(fd, diag_start * 512, 0)) {
		printf("wdformat: Can't seek to diagnostic block %d\n",
			diag_start);
		exit(1);
	}

	/*
	 * Set base to sector 0 of disk (since partition
	 * opened may not start at beginning of disk)
	 */
	ioctl(fd, SAIOZSETBASE, (char *)NULL);

	n = st.nspc;
	for (lba = diag_start; lba < diag_end; lba += n) {
		if (lba + n > diag_end)
			n = diag_end - lba;
		for (db = db_buf, i = 0; i < n; db++, i++)
			db->csd_db_blkno = lba + i;
		i *= sizeof(*db);
		if ((count = write(fd, (char *)db_buf, i)) != i) {
			if (count > 0)
				lba += count / sizeof(*db);
			printf("wdformat: Error writing diagnostic block %d\n",
				lba);
			break;
		}
	}

	printf("...Done\n");
}

/*
 * fill_pat - repeat a pattern
 */
fill_pat(src, nsrc, dst, ndst)
u_char *src;
u_int nsrc;
u_char *dst;
u_int ndst;
{
	register u_char *s = src;
	register u_char *d = dst;
	register u_char *sx = s + nsrc;
	register u_char *dx = d + ndst;

	while (d < dx) {
		*d++ = *s++;
		if (s >= sx)
			s = src;
	}
}

display_defects()
{
	printf("Displaying defects\n");
}


/*
 * write_min_vtoc
 *	write a minimal VTOC on disk.
 */
write_min_vtoc()
{
	struct vtoc *v;
	int where;
	int partno = 1;
	struct partition part; 
	
	part = drive->dt_part;
	callocrnd (SCSI_XFER_ALIGN);
	v = (struct vtoc *)calloc(V_SIZE);

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

	v->v_sanity = VTOC_SANE;
	v->v_version = V_VERSION_1;
	v->v_size = sizeof(struct vtoc);
	v->v_nparts = partno + 1;
	v->v_secsize = DEV_BSIZE;
	v->v_ntracks = drive->dt_st.ntrak;
	v->v_nsectors = drive->dt_st.nsect;
	v->v_ncylinders = drive->dt_st.ncyl;
	v->v_rpm = DFLT_RPM;
	v->v_capacity = capacity + 1;   /* last lba + 1 */
	v->v_nseccyl = drive->dt_st.nspc;
	(void)strcpy(v->v_disktype, drive->dt_diskname); 
	v->v_part[partno] = part;
	v->v_cksum = 0;
	v->v_cksum = vtoc_get_cksum(v);

	/*
	 * Set base to sector 0 of disk (since partition
	 * opened may not start at beginning of disk)
	 */
	ioctl(fd, SAIOZSETBASE, (char *)NULL);

	printf("Writing VTOC to sector %d.\n", V_VTOCSEC);

	where = V_VTOCSEC << DEV_BSHIFT;
	lseek(fd, where, 0);
	if (write(fd, v, V_SIZE) != V_SIZE) {
		printf("Unable to write VTOC\n");
		return;
	}
	printf("VTOC written.\n");
}
/*
 * internal version of atoi: check to be sure all of the input is
 * numeric first.
 */

int
xatoi(s)

char *s;
{
	register char *chp;

	for (chp = s; *chp; chp++)
		if (*chp < '0' || *chp > '9')
			return(-1);
	return(atoi(s));
}
