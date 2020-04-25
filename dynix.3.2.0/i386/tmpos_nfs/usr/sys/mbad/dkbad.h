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
 * $Header: dkbad.h 2.0 86/01/28 $
 */

/*
 * Definitions needed to perform bad sector
 * revectoring ala DEC STD 144 as extended by Sequent.
 *
 * The bad sector data is located in the last track of the disk.  There are 5
 * identical copies of each block of the data.  The 1st block, relative block
 * 0, is located in the 1st 5 even-numbered sectors of the track.  The 2nd block
 * (if any) is located in the 1st 5 odd-numbered sectors of the track.  The 3rd
 * block (if any) is located in sectors 10-14 of the track.  Subsequent blocks
 * (if any) are located in subsequent 5-block contiguous groups.
 *
 * The format of the data in block 0 is described by the dkbad structure.
 * Block 0 conatins a header and 126 bad sector entries.  Subsequent blocks
 * contain 128 bad sector entries.
 *
 * Replacement sectors are allocated starting with the 1st sector before
 * the last track, and work backwards towards the beginning of the disk.
 * The position of the bad sector in the bad sector list
 * determines which replacement sector it corresponds to.
 *
 * The bad sector data and replacement sectors are conventionally only
 * accessible through the 'c' file system partition of the disk.  If that
 * partition is used for a file system, the user is responsible for making
 * sure that it does not overlap the bad sector data or any replacement sector.
 */

#define DK_NBADCOPY	5	/* nbr of copies on disk */
#define DK_NBADMAX	9	/* max nbr of blocks */
#define DK_NBAD_0	126	/* nbr of entries, block 0 */
#define DK_NBAD_N	128	/* nbr of entries, block > 0 */

/* max nbr of defects a full list can contain */
#define DK_MAXBAD	( (DK_NBADMAX - 1 ) * DK_NBAD_N + DK_NBAD_0 )

/* macro for locating the blocks of the defect list relative to the origin */
#define DK_LOC(block, copy)	\
	((block) < 2 ? 2 * (copy) + (block) : DK_NBADCOPY * (block) + (copy))

/* macro for locating (based on the orgin of Sequent's bad block list)
   the Manufacturer's Detected Bad Sector File */
#define DK_MDBSF(x)	( x - DK_MAXBAD - DK_NBADMAX )

#define DK_END	0xFFFF	/* value for end of bad block list */
#define DK_INVAL -2	/* bad block list entry is invalid */

/* structure of defect list block 0 */
struct dkbad {
	long	bt_csn;		/* cartridge serial nbr, or the
				   mfg's defect list checksum */
	u_short	bt_lastb;	/* nbr of last block of list */
	u_short	bt_flag;	/* see below */
	union bt_bad {
		struct bt_chs {
			u_short	bt_chs_cyl;	/* cylinder nbr of bad sector */
			u_short	bt_chs_ts;	/* track and sector nbr */
		} bt_chs;
		u_int	bt_sect;		/* physical sector number */
	} bt_bad[DK_NBAD_0];
};

#define bt_cyl		bt_chs.bt_chs_cyl
#define bt_trksec	bt_chs.bt_chs_ts

/* structure of mfg defect list block 0 */
struct dkbad_mfg {
	long	bt_csn;		/* cartridge serial nbr, or the
				   mfg's defect list checksum */
	u_short	bt_lastb;	/* nbr of last block of list */
	u_short	bt_flag;	/* see below */
	union bt_mfgbad {
		struct bt_chb {
		 	unsigned bt_chb_cyl : 10;	/* cylinder nbr */
		 	unsigned bt_chb_head : 5;	/* track nbr */
		 	unsigned bt_chb_byte : 17;	/* byte nbr */
		} bt_chb;
		u_int	bt_sect;		/* physical sector number */
	} bt_mfgbad[DK_NBAD_0];
};

#define bt_mfgcyl	bt_chb.bt_chb_cyl
#define bt_mfghead	bt_chb.bt_chb_head
#define bt_mfgbyte	bt_chb.bt_chb_byte

/* bt_flag values */
#define DK_FLAG_DATA	0x0000	/* defect list - data disk */
#define DK_FLAG_MDBSF	0x0001	/* manufacturer's defect list */
#define DK_FLAG_SECT	0x0002	/* defects in sector form (not cyl-trk-sct) */

#define	ECC	0
#define	SSE	1
#define	BSE	2
#define	CONT	3
