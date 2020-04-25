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
 * $Header: zdformat.h 1.4 90/07/23 $
 *
 * zdformat.h
 */

/* $Log:	zdformat.h,v $
 */

#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define	ROUNDUP(x, y)	((((x)+((y)-1))/(y))*(y))

#define TRUE		1
#define FALSE		0

#define	SUCCESS		1
#define	FAIL		0


/*
 * A reasonable default to get things bootstraped
 */
#define DFLT_RPM	3600

/*
 * Tasks performed by this program.
 */
#define	FORMAT		1	/* Format disk */
#define REFORMAT	2	/* Reformat disk */
#define VERIFY		3	/* Verify (only) disk */
#define ADDBAD		4	/* add bad block(s) to bad block list */
#define	DISPLAY		5	/* print bad block list a/o mfg defect list */
#define	CLONEFMT	6	/* reformat cylinders as per bad block list */
#define WRITEVTOC	7	/* write minimal VTOC */
#define EXIT		8	/* exit menu */

/*
 * Display options
 */
#define	MFG_DEFECT	0x1	/* Print out mfg defect list */
#define	BAD_BLOCK	0x2	/* Print out bad block list */

/*
 * ZDBYTESPT - total number of bytes per track
 * ZDMAXBAD  - maximum number of bad blocks allowed on disk
 */
#define ZDBYTESPT(c)    ((((c)->zdd_sectors + (c)->zdd_spare) \
                        * (c)->zdd_sector_bc) + (c)->zdd_runt)
#define ZDMAXBAD(c)     ((((c)->zdd_sectors - ZDD_NDDSECTORS) / 2) \
                         << DEV_BSHIFT)

/*
 * Format/verify defaults
 */
#define NHDRPASS        10      /* No. of verify passes on tracks with
                                 * potentially bad headers */
#define	NFULLPASS	3	/* No. of verify passes on entire range */
#define	NDEFECTPASS	45	/* no. of verify passes on tracks with spots */

/*
 * Header format.
 */
struct	hdr {
	u_char	h_type;		/* type of sector */
	u_char	h_flag;		/* see stand/zdc.h */
	u_short	h_cyl;		/* cylinder number */
	u_char	h_head;		/* track number */
	u_char	h_sect;		/* sector number */
};
#define HDR_CRC_BC      2       /* The number of hdr correction bytes */

/* Compare headers to see if all fields match */
#define HDR_CMPR(hp1,hp2)                       \
        ( (hp1)->h_cyl  == (hp2)->h_cyl  &&     \
          (hp1)->h_head == (hp2)->h_head &&     \
          (hp1)->h_sect == (hp2)->h_sect &&     \
          (hp1)->h_type == (hp2)->h_type &&     \
          (hp1)->h_flag == (hp2)->h_flag   )

/* Determine if a ZD_BADUNUSED header is acceptable */
#define BUHDR_CMPR(hp)  ((hp)->h_sect == ZD_INVALSECT)

/*
 * 1 per track in cylinder.
 */
struct track_hdr {
	int	t_free;			/* No. of unused spares on track */
	int	t_bad;			/* No. of defects to resolve on track */
	struct	hdr *t_hdr;		/* array of headers for track */
};

/*
 * 2 cylinder window.
 */
struct cyl_hdr {
	int	c_free;			/* No. of unused spares on cylinder */
	int	c_bad;			/* No. of defects to resolve on cyl */
	struct	track_hdr *c_trk;	/* array of track_hdrs */
};
extern struct cyl_hdr cyls[];

/*
 * Structure to hold bad blocks not resolved via the first pass.
 * These BZ_SNF entries are usually resolved in non-adjacent cylinders.
 * This is used by ADDBAD and to format disks where there can be more
 * errors in small cylinder group than spares to resolve.
 */
struct snf_list {
	struct	diskaddr snf_addr;
	caddr_t	snf_data;
};
extern struct snf_list snf_list[];
extern int snftogo;

#define	SNF_LIST_SIZE	256

/*
 * Addlist definition and size
 */
struct addlist {
	u_char		al_type;
	struct diskaddr al_addr;
};
#define	MAXADDBAD	64		/* Max no. to addbad at one time */
extern struct addlist addlist[];	/* Addlist for addbad */

/*
 * The following are used when attempting to correct
 * a BAD_UNUSED sector header so that it won't appear
 * as a bogus header while trying to read another
 * (the sector zero problem).  Try adjusting it up to
 * 8 times, each time shifting it over 64 bytes.
 */
#define ZDHDRBSHFT      64      /* Bytes for sector header adjustment */
#define ZDNHDRSHFT      8       /* # retries for adjusting header */
#define ZDNBEATS        16      /* no. times to beat up a track after error */
#define ZDSYNCPAT       0x0c    /* Primary sync pattern for track reformat */
#define ZDALTPAT        0x01    /* Alternate sync pattern for track reformat */

extern int      hdrpasses;      /* no. of sector hdr verification passes
                                 * (suspected tracks only) */
extern int      fullpasses;     /* no. of full passes (all cylinders) */
extern int      defectpasses;   /* no. of defect passes (tracks with spots) */
extern bool_t   checkdata;      /* check data? */
extern bool_t   doverify;       /* perform normal verification ? */

extern int	startcyl;	/* starting cylinder */
extern int	lastcyl;	/* last cylinder */
extern int	verbose;	/* print interesting things */
extern int	fd;		/* file descripter for disk */
extern int	totspt;		/* total sectors per track (incl spare(s)) */
extern struct zdcdd *chancfg;	/* Channel configuration - drive descriptor */
extern struct bad_mfg	*mfg;	/* mfg defect list */
extern struct zdbad  *bbl;	/* bad block list */
extern struct zdbad  *newbbl;	/* new bad block list - built during format */
extern struct bz_bad *tobzp;	/* ptr to new list entry */
struct bz_bad *find_bbl_entry();
struct bz_bad *newbad();
int	bblcomp();
caddr_t	calloc();
char	*prompt();
int	scani();
char    *index();
long    getsum();

#define UMFG_DPT        4       /* Max # reported UMFG defects per track */

/*
 * This is the format of the headers returned by the the
 * ZDC_READ_HEADERS command.
 */

union headers{
	struct hdr	hdr;
	char		pad[8];
};
extern union headers *headers;          /* Header list for READ_HDRS ioctl */

/*
 * This data structure is used for tracking
 * tracks suspected of having defective headers
 * and fixes made to them
 */
struct hdr_suspect {
        struct diskaddr addr;           /* Suspected cylinder and head */
        short  *adjustment;             /* Most recently attempted adjustment */
};
extern struct hdr_suspect *suspect;	/* Array of tracks containing potential
				   	 * header defects resulting in hdr-ecc
				   	 * errors.  Verified during hdr-pass */
extern int max_suspects;		/* Static size of header suspect list */
extern int num_suspects;		/* Number of elements in suspect list */

extern int   n_hdrs;			/* Number of hdrs per track w/ runts */
extern int   task;			/* Program task */
extern int   adjust_header();		/* Compensate for header defects */
extern int   readjust_headers();	/* Re-compensate for header defects */
