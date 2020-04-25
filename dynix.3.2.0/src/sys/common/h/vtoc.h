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
 * #ident	"$Header: vtoc.h 1.8 91/01/01 $"
 */

#ifndef	_SYS_VTOC_H_

/*
 * VTOC driver data structures and definitions.
 */

/* $Log:	vtoc.h,v $
 */

#define V_NUMPAR 		255		/* max number of partitions */
#define V_VTOCSEC		16		/* where VTOC starts */
#define LABELSECTOR		V_VTOCSEC
#define	V_SIZE			8192		/* VTOC size */
#define VTYPCHR			14		/* max length of type name */

/*
 * structure of a single partition description in the VTOC
 */
struct partition	{
	daddr_t p_start;	/* start sector no of partition*/
	long	p_size;		/* # of blocks in partition*/
	int	p_type;		/* type of this partition */
	short	p_bsize;	/* block size in bytes */
	short	p_fsize;	/* frag size in bytes */
};

/*
 * on-disk VTOC
 */
struct vtoc {
	ulong	v_sanity;	/* magic number */
	ushort	v_version;
	ulong	v_cksum;	/* checksum after v_cksum has been zeroed */
	long	v_size;		/* # of bytes in this vtoc */
	ushort	v_nparts;	/* number of partitions */
	int	v_secsize;	/* sector size in bytes */
	int	v_ntracks;	/* # tracks/cylinder */
	int	v_nsectors;	/* # sectors/track */
	int	v_ncylinders;	/* # cylinders */
	int	v_rpm;		/* revolutions/minute */
	int	v_capacity;	/* # of sectors per disk */
	int	v_nseccyl;	/* # sectors/cylinder */
	long	v_reserved[8];
	char	v_disktype[VTYPCHR]; /* type of disk this is */
	struct partition v_part[V_NUMPAR];
};

/*
 * driver-specific part of a vtoc dev.  We share the same structure
 * between "normal" vtoc devs and the diagnostic vtoc dev, to make
 * memory allocation easier.  The driver_id should be first for
 * conformance to other partitioning drivers.
 */
struct vtoc_private {
	int		vp_driver_id;	/* major number of vtoc driver */
	daddr_t		vp_start;	/* start of physical partition */
	long		vp_size;	/* size of this device */
	struct	dev	*vp_physdev;	/* handle on physical driver */
	struct	devops	*vp_devops;	/* copy of devops used by other devs */
	int		vp_flags;	/* see below */
};

/* vp_flags */

#define	VP_WRPROTECT		0x01		/* No raw writes allowed */

#define VTOC_SANE		0x061160	/* Indicates a sane VTOC */
#define V_VERSION_1		0x01		/* layout version number */

/* Partition types */
#define V_MIN_PART_TYPE		0x00
#define V_NOPART		0x00		/* empty slot */
#define	V_RAW			0x01		/* regular partition */
#define	V_BOOT			0x02		/* bootstrap */
#define V_RESERVED		0x03		/* reserved disk area */
#define	V_FW			0x04		/* firmware */
#define V_DIAG                  0x05            /* diagnostic/error log */
#define V_MAX_PART_TYPE		0x05


/* Do partition structs a and b overlap? */

#define V_OVLAP(A, B)	   	((A)->p_start < (B)->p_start + (B)->p_size && \
				 (B)->p_start < (A)->p_start + (A)->p_size)

#define NUMPARTS	8

struct cmptsize {
	daddr_t	cmpt_start; 	/* start of physical partition in sectors */
	long	cmpt_size;	/* size of physical partition in sectors */
};


/*
 * Special minor numbers for disks
 *
 * Disks devices use the following semantics for their minor numbers.
 */

#define V_PARTSHIFT    3               /* bit to shift to get unit */
#define V_NEWPARTMASK  0xf800          /* upper 5 bits of partition */
#define V_NEWPARTSHIFT (11-3)          /* bit to shift to get rest of partition */
#define V_PART_MASK     0x7             /* mask to get partition entry */
#define V_UNIT_MASK     0xff            /* mask to get unit entry */
#define VUNIT(dev)    ((minor((dev)) >> V_PARTSHIFT) & V_UNIT_MASK)
#define VPART(dev)    ((minor((dev)) & V_PART_MASK)|((minor(dev)& V_NEWPARTMASK)>>V_NEWPARTSHIFT))
#define V_ALL(dev)     ((minor(dev) & 0xf807) == 0xf807)

#ifdef KERNEL
/*
 * To implement a protection mechanism for partitions, each driver will need
 * to keep a count of the number of opens active on a device.  The declarations
 * below provide such a structure, and a super-structure containing both the
 * VTOC and open-count structures.  This can be used to dynamically allocate
 * both together, potentially improving storage efficiency.
 */
struct v_vo {
	unsigned short v_opens[V_NUMPAR];
	unsigned int   v_modes[V_NUMPAR];
};

struct v_open {
	struct vtoc v_v;	/* The VTOC */
	struct v_vo v_vo;	/* Run-time data per VTOC */
};

#endif /* KERNEL */

#define	_SYS_VTOC_H_
#endif	/* _SYS_VTOC_H_ */
