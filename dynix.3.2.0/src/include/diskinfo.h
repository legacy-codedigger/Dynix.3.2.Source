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
 * $Header: diskinfo.h 1.1 89/08/23 $
 *
 * $Log:	diskinfo.h,v $
 */

#ifndef	_DISKINFO_H_

/*
 * Disk information description tables
 * from psx diskinfo.h 1.7 89/05/03 "
 */
#define	INFODIR		"/etc/diskinfo"

#define	GEOMSUFF	".geom"

struct	geomtab {
	char	*g_name;		/* drive name */
	int	g_secsize;		/* sector size in bytes */
	int	g_ntracks;		/* # tracks/cylinder */
	int	g_nsectors;		/* # sectors/track */
	int	g_ncylinders;		/* # cylinders */
	int	g_rpm;			/* revolutions/minute */
	int	g_capacity;		/* # sectors/disk */
	int	g_nseccyl;		/* # sectors/cylinder */
	int	g_mincap;		/* Minimum capacity, rarely used */
};

extern	struct	geomtab *getgeombyname();

#ifndef INQ_VEND
#define INQ_VEND 8
#define INQ_PROD 16
#endif /* INQ_VEND */

#define SCSISUFF	".scsi"

struct scsiinfo {
	char	*scsi_diskname;		   /* disk type */
	char	scsi_vendor[INQ_VEND+1];   /* vendor (from INQUIRY)  */
	char	scsi_product[INQ_PROD+1];  /* product (from INQUIRY) */
	unsigned char	scsi_inqformat;    /* format of INQUIRY data */
	unsigned char	scsi_reasslen;     /* # bytes for REASSIGN command */ 
	unsigned	scsi_formcode;     /* default format, CDB byte 1 */
};

extern struct scsiinfo *getscsimatch();
extern struct scsiinfo *getscsiinfo();

#define ZDSUFF	".zd"

struct zdinfo {
	char	*zi_name;
	struct zdcdd	zi_zdcdd;
};

extern struct zdinfo *getzdinfobyname();
extern struct zdinfo *getzdinfobydtype();
extern struct zdinfo *getzdinfo();

#define	_DISKINFO_H_
#endif	/* _DISKINFO_H_ */
