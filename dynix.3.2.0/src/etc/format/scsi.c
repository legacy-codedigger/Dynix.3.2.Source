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
 * ident	"$Header: scsi.c 1.6 90/07/23 $
 * scsi.c
 *
 * The scsi subsystem of the online formatter.
 */

/* $Log:	scsi.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#ifdef BSD
#include <sec/scsi.h>
#include <sys/ioctl.h> 
#include <sec/scsiioctl.h> 
#include <zdc/zdc.h>
#else
#include <sys/scsi.h>
#include <sys/scsiioctl.h> 
#include <sys/zdc.h>
#endif
#include <stdio.h>
#include <diskinfo.h>
#include "format.h"
#include "scsiformat.h"

extern caddr_t malloc();

int scsi_setup(), scsi_main(), scsi_usage(), scsi_bootnum();

extern struct scsi_ftype scsi_ftype[];		/* format codes */
extern int num_ftypes;				/* # entries in scsi_ftype */

/*
 * Date global within scsi subsystem
 * 	(these could be static later)
 */
int formcode;			/* format code for SCSI_FORMAT command */
struct scsiinfo *sinfo;		/* pointer to scsiinfo structure for disk */
struct geomtab *sgeom;		/* pointer to geometry info for this disk */
int capacity=0;			/* disk capacity */
caddr_t ioctl_data;		/* pointer to buffer for ioctl data */
int fullpasses;			/* number of verify passes to do */
int startblock, endblock;	/* start and end block # for verify */

/*
 * scsi_usage
 *	Shows which functions and arguments are supported
 *	by scsi subsystem
 */
scsi_usage()
{
	fprintf(stderr, "%s is formatted by the scsi subsystem\n",
		diskname);
	fprintf(stderr, "usage for scsi subsystem:\n");
	fprintf(stderr, 
	"    format [-f|u|a xxx] [-wov] diskname\n");
	return;
}

/*
 * scsi_bootnum
 *	Get the device number used in a bootstring for this
 *	device.
 */
int
scsi_bootnum()
{
	struct scsidev scsidev; 

	/*
	 * Get device-specific info. 
	 */
	if (ioctl(fd, SIOCDEVDATA, (char *)&scsidev) < 0)
		return(-1);
	if (debug) {
		printf("ioctl(fd,SIOCDEVDATA) devno=%d ctlr=%d\n",
			scsidev.scsi_ctlr, scsidev.scsi_devno);
	}
	return(SCSI_BOOTNUM(scsidev.scsi_ctlr, scsidev.scsi_devno));
}

/*
 * scsi_setup
 *	Check argument operands for valid values, and	
 *	do setup which applies to any function for scsi disks
 */
scsi_setup()
{
	int i, data_size;
	struct scsiioctl sioctl;
	struct scinq_dev *inq;
	struct screadcap *rcap;
	char *p, str[80];

	/*
	 * Do checking and any setup needed on command line
	 * arg data.
	 */
	if (args & B_TYPE) {
		formcode = -1;
		for (i = 0; i < num_ftypes; i++) {
			if (strcmp(t_arg, scsi_ftype[i].fstr) == 0)
				formcode = scsi_ftype[i].fvalue;
		}
		if (formcode < 0) {
			fprintf(stderr, "invalid format type: %s\n", t_arg);
			fprintf(stderr, "The following format types are ");
			fprintf(stderr, "supported:\n");
			for (i = 0, p = str; i < num_ftypes; i++) {
				sprintf(p, "%s ", scsi_ftype[i].fstr);
				p += strlen(p);
			}
			fprintf(stderr, "%s\n", str);
			scsi_usage();
			return(-1);
		}
	}
	if (args & B_FULLPASS) {
		fullpasses = p_arg;
		if (fullpasses < 0 || fullpasses > SCSI_PASSMAX) {
			fprintf(stderr, "Warning: fullpass value over maximum ");
			fprintf(stderr, "- setting to default (%d)\n",
				SCSI_PASSDEFAULT);
			fullpasses = SCSI_PASSDEFAULT;
		}
	} else
		fullpasses = SCSI_PASSDEFAULT;

	/*
	 * If the function is one which will wipe out the
	 * disk and the disk has a valid VTOC on it, make
	 * sure the user specified the OVERWRITE option.
	 */
	if ((function & SCSI_OVERWRITES) && !(args & B_OVERWRITE)) {
		if (validvtoc(fd)) {
			fprintf(stderr, "Disk has a valid VTOC on it -- ");
			fprintf(stderr, 
				"specify the -o option to overwrite it\n"); 
			return(-1);
		}
	}
			
	/*
	 * Determine the maximum possible amount of ioctl
	 * data and allocate buffer to be used for ioctl data.
	 */

	data_size = (sizeof(struct reassarg) > sizeof(struct screadcap)) ?
		sizeof(struct reassarg) : sizeof(struct screadcap);
	if (data_size < sizeof(struct scinq_dev))
		data_size = sizeof(struct scinq_dev);
	if (data_size < sizeof(struct defect_hdr))
		data_size = sizeof(struct defect_hdr);
	ioctl_data = MALLOC_ALIGN(data_size, types[disk].align);

	if (debug) printf("ioctl_data = 0x%x\n", ioctl_data);
	
	/*
	 * set up for READ CAPACITY
	 */
	rcap = (struct screadcap *)ioctl_data;
	bzero((caddr_t)rcap, sizeof(struct screadcap));
	bzero((caddr_t)&sioctl, sizeof(struct scsiioctl));
	sioctl.sio_datalength = sizeof(struct screadcap);
	sioctl.sio_addr = (ulong)rcap;
	sioctl.sio_cmd10.cmd_opcode = SCSI_READC;

	if (ioctl(fd, SIOCSCSICMD, (char *)&sioctl) < 0) {
		perror("failed on READ CAPACITY command");
		exit(1);
	}

	capacity = (rcap->rc_nblocks0 << 24) | (rcap->rc_nblocks1 << 16) |
			(rcap->rc_nblocks2 << 8) | (rcap->rc_nblocks3);

	startblock = (args & B_START) ? s_arg : 0;
	if (args & B_END)
		endblock = e_arg;
	if (!(args & B_END) || endblock > capacity)
		endblock = capacity;
	if (startblock > endblock) {
		fprintf(stderr, "invalid start and/or end block - ");
		fprintf(stderr, "drive capacity = %d\n", capacity);
		scsi_usage();
		return(-1);
	}

	/*
	 * set up the inquiry command
	 */
	inq = (struct scinq_dev *)ioctl_data;	
	bzero((caddr_t)inq, sizeof(struct scinq_dev));
	bzero((caddr_t)&sioctl, sizeof(struct scsiioctl));
	sioctl.sio_datalength = sizeof(struct scinq_dev);
	sioctl.sio_addr = (ulong)inq;
	sioctl.sio_cmd6.cmd_opcode = SCSI_INQUIRY;
	sioctl.sio_cmd6.cmd_length = sizeof(struct scinq_dev);

	if (ioctl(fd, SIOCSCSICMD, (char *)&sioctl) < 0) {
		perror("failure on INQUIRY command");
		return(-1);
	}

	/*
	 * Get the bootstring in order to print it out
	 */
	(void)bootstr(str);

	/*
	 * print out the id string from the INQUIRY command.  Need to
	 * count the chars because drive does not null-terminate the
	 * strings.
	 */
	if (function != INFO) {
		printf("\n%s = %s:  ID: vendor ", diskname, str);
		for (i = 0; i < INQ_VEND; i++)
			printf("%c", inq->sc_vendor[i]);
		printf(", product ");
		for (i = 0; i < INQ_PROD; i++)
			printf("%c", inq->sc_product[i]);
		printf(", revision ");
		for (i = 0; i < INQ_REV; i++)
			printf("%c", inq->sc_revision[i]);
		printf("\n\n");
	}

	/*
	 * Get the scsiinfo and geometry info for this
	 * disk type.
	 */
	if ((sinfo = getscsimatch(inq->sc_vendor, inq->sc_product)) == NULL 
	    || inq->sc_hdr.ih_reserved != sinfo->scsi_inqformat) {
		fprintf(stderr, "Drive not a supported CCS disk.\n");
		return(-1);
	}
	if ((sgeom = getgeombyname(sinfo->scsi_diskname)) == NULL) {
		fprintf(stderr, "No geometry file for %s -- exiting\n",
			sinfo->scsi_diskname);
		return(-1);
	}

	return(0);
}

/*
 * scsi_main
 *	This routine calls the various scsi function routines
 *	according to the desired task.
 */
scsi_main()
{
	switch (function) {

	case FORMAT:
		scsi_format();
		break;

	case ADDBAD:
		scsi_addbad();
		break;

	case REPBAD:
		scsi_addbad();
		break;

	case VERIFY:
		scsi_verify();
		break;

	case WRITEDIAG:
		scsi_writediag();
		break;

	case DISPLAY:
		scsi_display();
		break;

	case INFO:
		scsi_info();
		break;

	case USAGE:
		scsi_usage();
		break;

	default:
		fprintf(stderr, "ERROR: scsi_main: internal formatter error\n");
		return;
	
	} /* end switch */

	/*
	 * Now do verify and write diagnostics if appropriate. 
	 */
	if (usep->tasks & VERIFY && !(args & B_NOVERIFY))
		scsi_verify();

	if (usep->tasks & WRITEDIAG && !(args & B_NOWRITEDIAG))
		scsi_writediag();

	return;
}
	
/*
 * scsi_format
 * 	do the actual disk formatting.  This routine does a generic format of
 * 	the P and G lists only.
 */
scsi_format()
{
	struct scsiioctl sioctl;

	if (debug) printf("scsi_format: begin\n");
	
	printf("Beginning disk format ... \n");

	bzero((caddr_t) &sioctl, sizeof(struct scsiioctl));
	sioctl.sio_datalength = 0;
	sioctl.sio_addr = (ulong)0;
	sioctl.sio_cmd6.cmd_opcode = SCSI_FORMAT;
	sioctl.sio_cmd6.cmd_lun = (t_arg) ? formcode : sinfo->scsi_formcode; 
	if (ioctl(fd, SIOCSCSICMD, (char *)&sioctl) < 0) {
		perror("error on FORMAT ioctl");
		fprintf(stderr, "exiting...");
		exit(1);
	} else 
		printf("Format complete\n");
	return;
}

/*
 * scsi_addbad
 *	add a bad block to the devices bad block list
 */
scsi_addbad()
{
	char str[256], *ptr;
	struct scsiioctl sioctl;
	struct reassarg *rarg;
	int block, status;

	if (debug) printf("scsi_addbad: begin\n"); 

	rarg = (struct reassarg *)ioctl_data;
	bzero((caddr_t)rarg, sizeof(struct reassarg));
	bzero((caddr_t)&sioctl, sizeof(struct scsiioctl));
	sioctl.sio_datalength = 4 + sinfo->scsi_reasslen;  /* XXX define in scsi.h */ 
	sioctl.sio_addr = (ulong)rarg;
	sioctl.sio_cmd6.cmd_opcode = SCSI_REASS;
	rarg->length[1] = sinfo->scsi_reasslen; 

	ptr = a_arg;
	if (debug) printf("a_arg string = %s\n", a_arg);

	for (;;) {
		if (a_file) 	/* read list from file */
			status = fscanf(a_file, "%s", str);
		else
			status = sscanf(ptr, "%s", str);
		if (status == EOF)
			break;
		if (!a_file)
			ptr += strlen(str) + 1;

		if ((block = xatoi(str)) < 0) {
			fprintf(stderr, "illegal block number %s - ",
				str);
			fprintf(stderr, "skipping...\n");
			continue;
		} else if (block >= capacity) {
			fprintf(stderr, "block number %d out of range - ",
				 block);
			fprintf(stderr, "skipping...\n");
			continue;
		}
		if (debug) printf("addbad block %d\n", block);
		itob4(block, rarg->defect);
		if (ioctl(fd, SIOCSCSICMD, (char *)&sioctl) < 0) {
			perror("REASSIGN BLOCKS ioctl error");
			fprintf(stderr, "exiting ...\n");
			exit(1);	
		}
	} /* end of for */
}

/*
 * scsi_verify
 *	write verify patterns over disk to uncover any
 *	additional bad blocks.
 */
scsi_verify()
{
	if (debug) printf("scsi_verify: begin\n");
}

/*
 * scsi_writediag
 *	write diagnostic data on the reserved diagnostic
 *	cylinders
 */
scsi_writediag()
{
	register struct csd_db *db, *db_buf;
	register int i, n, count;
	register uint lba, diag_end, diag_start;
	int nspc = sgeom->g_nseccyl;
	static unchar pat_default[] = {	 /* diagnostic track pattern */
		CSD_DIAG_PAT_0, CSD_DIAG_PAT_1, CSD_DIAG_PAT_2,
		CSD_DIAG_PAT_3, CSD_DIAG_PAT_4
	};

	if (debug) printf("scsi_writediag: begin\n"); 


	diag_end = capacity + 1;	/* one beyond the last block */
	/* two cylinders worth of diagnostic data */
	diag_start = diag_end - (nspc * 2);

	/*
	 * get a buffer to hold one cylinder -
	 * align if necessary 
	 */
	db_buf = (struct csd_db *)MALLOC_ALIGN(nspc * sizeof(struct csd_db),
					       types[disk].align);
	if (!db_buf) {
		fprintf(stderr, "scsi_writediag: unable to allocate buffer\n");
		fprintf(stderr, "...not writing diagnostic tracks\n");
		return;	
	}

	/* lay in the patterns; they're identical in all blocks */
	for (db = db_buf, i = 0; i < nspc; db++, i++) {
		fill_pat(pat_default, sizeof(pat_default),
			db->csd_db_pattern, sizeof(db->csd_db_pattern));
	}

	printf("Writing diagnostic tracks (%d..%d)... \n",
		diag_start, diag_end-1);

	if (debug) printf("seeking to 0x%x\n", diag_start * DEV_BSIZE);
	if (lseek(fd, diag_start * DEV_BSIZE, 0) < 0) {
		fprintf(stderr, 
			"scsi_writediag: can't seek to diagnostic block %d :",
			diag_start);
		perror("");
		fprintf(stderr, "...not writing diagnostic tracks\n");
		return;
	}

	n = nspc;
	for (lba = diag_start; lba < diag_end; lba += n) {
		if (lba + n > diag_end)
			n = diag_end - lba;
		for (db = db_buf, i = 0; i < n; db++, i++)
			db->csd_db_blkno = lba + i;
		i *= sizeof(*db);
		if ((count = write(fd, (char *)db_buf, i)) != i) {
			if (count > 0)
				lba += count / sizeof(*db);
			fprintf(stderr, "scsi_writediag: error writing ");
			fprintf(stderr, "diag block %d\n", lba);
			if (count < 0)
				perror("write error");
			fprintf(stderr, 
				" ...unable to write diagnostic tracks\n");
			return;
		}
	}

	printf("...Done\n");
	return;
}

/*
 * scsi_display
 *	display scsi defect lists
 */
scsi_display()
{
	struct scsiioctl sioctl;
	struct defect_hdr *def;
	caddr_t data;

	if (debug) printf("scsi_display: begin\n"); 

	/*
	 * First read defects with length of 0 just to
	 * find out the size of buffer to allocate.
	 */
	def = (struct defect_hdr *)ioctl_data;
	bzero((caddr_t) &sioctl, sizeof(struct scsiioctl));
	bzero((caddr_t)def, sizeof(struct defect_hdr));
	sioctl.sio_datalength = sizeof(struct defect_hdr);
	sioctl.sio_addr = (ulong)def;
	sioctl.sio_cmd10.cmd_opcode = SCSI_READ_DEFECTS;
	sioctl.sio_cmd10.cmd_lba[0] = SCSI_PLIST;
	sioctl.sio_cmd10.cmd_length[1] = sizeof(struct defect_hdr);
	if (ioctl(fd, SIOCSCSICMD, (char *)&sioctl) < 0) {
		perror("SCSI_READ_DEFECTS ioctl error");
		fprintf("...unable to display defects\n");
		return;
	}
	if (debug) printf("def->length = %d\n", def->length);

	if (def->length) {
		def->length += sizeof(struct defect_hdr);
		data = MALLOC_ALIGN(def->length, types[disk].align);
		sioctl.sio_addr = (ulong)data;
		sioctl.sio_cmd10.cmd_length[0] = def->length >> 8;
		sioctl.sio_cmd10.cmd_length[1] = def->length & 0xff;
		if (debug)
			printf("l0 = 0x%x, l1 = 0x%x\n",
				sioctl.sio_cmd10.cmd_length[0],
				sioctl.sio_cmd10.cmd_length[1]);
		/*
		if (ioctl(fd, SIOCSCSICMD, (char *)&sioctl) < 0) {
			perror("SCSI_READ_DEFECTS ioctl error");
			fprintf(stderr, "...unable to display defects\n");
			return;
		}
		printf("defects follow: ");
		for (defect = (int *)(data+sizeof(struct defect_hdr)), i = 0;
		     i < def.length-sizeof(struct defect_hdr); 
		     i += sizeof(int), defect++)
			printf("%d  ", *defect);
		printf("\n");
		*/
	}
	return;
}

/*
 * fill_pat 
 *	repeat a pattern -- used in writing diagnostics
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


scsi_info()
{
	printf("%s\n", sinfo->scsi_diskname);
};
