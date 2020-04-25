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
 * $Header: cntrlblock.h 1.2 90/07/05 $
 */

/* $Log:	cntrlblock.h,v $
 */

/*
 * The  descriptor  cb_desc  contains   four   fields
 * described  below.  After the cb_desc is filled out
 * it should contain all the information needed about
 * a drivers command blocks.
 */
struct cb_desc {
	char *cb_cbs;			/* pointer to block of cbs */
	short cb_state_offset;		/* offset of state field */
	short cb_count;			/* number of cbs */
	short cb_size;			/* size in bytes of a command block */
};

/*
 * macro to compute state field offset
 */
#define FLD_OFFSET(struct, field) (int) &(((struct *) 0)->field)

/*
 * command block is in use if bit 31 is set
 */
#define CB_BUSY 0x80000000

/*
 * Functions available in cntrlblock.c.
 */
#ifdef KERNEL

#ifdef notyet
extern struct cb_desc *alloc_cb_desc_cbs();
#endif notyet

extern struct cb_desc *alloc_cb_desc();
extern caddr_t get_cb();
extern free_cb();

#endif KERNEL
