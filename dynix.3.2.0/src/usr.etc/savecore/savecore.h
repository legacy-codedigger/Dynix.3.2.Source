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

#ident "$Header: savecore.h 1.2 1991/04/12 22:22:39 $"

/*
 * savecore.h--definitions for data structures used to implement
 * parallel compress.
 */

#define NUMBUFS 32	/* number of buffers avlbl. to compressors */

/* description of the buffer header */
struct bufhdr {
	int status;		/* Status of buffer (FREE, DONE, INUSE) */
	unsigned length;	/* Length of compressed data */
	unsigned char *buf_p;   /* ...pointer to data */
};

#define FREE 1
#define DONE 2
#define INUSE 3

/*
 * Data structure describing /etc/dumplist list of partitions
 * dump resides on
 */

struct dumplist {
	char **ifname;		/* Array of partition names */
	int *offset;		/* Array of offsets within those partitions */
	int *n_segs;		/* Array of segments which fit in partition */
};

/*
 * Pointers to shared data
 */

unsigned int *next_buf_p;	/* Next buffer to be compressed into*/
unsigned int *next_mem_p;	/* Next mem segment to be compressed */
unsigned int *next_pos_p;	/* Position in file, correponding to 
				 * next_mem_p.
				 */	
unsigned next_write;		/* Next buffer to be written out */

