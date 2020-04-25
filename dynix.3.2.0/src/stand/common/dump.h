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

/* $Header: dump.h 2.1 1991/04/12 20:34:22 $ */

/*
 *	dump.h
 *
 *	Contains definitions shared between etc/savecore and stand/dump
 */

#define DUMP_MAGIC	0xdeadbabe	/* magic number of some sort */
#define NEW_MAGIC      0xbeadface      /* magic number of some sort */
#define VER1	101
#define UNIT_SZ        (8*1024)
#define BITS  		13

#define EOF_MAP         -2
#define SEG_MISSING     -1


struct dump_info {
	unsigned dump_magic;            /*magic number */
	int dump_size;                  /* size of dump */
	int compressed_size;            /* size of dump */
	int revision;                   /*version of compression algorithm */
        unsigned seg_size;              /*unit of compression */
	unsigned comp_size;             /*size of compression map*/
	unsigned data_end;              /*offset to end of data in file */
	unsigned dmesg_end;             /*offset to end of dmesg*/
};
