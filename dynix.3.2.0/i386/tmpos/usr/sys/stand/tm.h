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

/* $Header: tm.h 1.1 90/08/15 $ */

/*
 * Definitions for the SSM standalone scsi tape driver.
 */

/* $Log:	tm.h,v $
 */

/*
 * Macros for the size of a flag byte array for bit 
 * manipulation, computing the index into it for a 
 * particular unit, and the bit mask within that byte.
 */
#define TM_NUM_FLAG_BYTES	32		/* 4 ssm's x 8 ta's */
#define TM_INDEX(unit)	(SCSI_BOARD((unit)) * 8 + SCSI_TARGET((unit)))
#define TM_FLAG(unit)	(1 << SCSI_UNIT((unit)))

/*
 * The following defines the number of data 
 * bytes for a Mode Select.
 */
#define TM_MODESEL_LEN		12		

/* 
 * Return value when scsi command fails.
 */
#define TM_CMD_FAIL		(-1)

/* 
 * Control byte value to use for SCSI 
 * request/sense commands.
 */
#define TM_CONTROL		0

#ifndef BOOTXX
/*
 * The following value is passed to tm_docmd
 * to execute an an ioctl operation.
 */
#define TM_IOCTL		(-1)

extern tmclose(), tmioctl();
#endif BOOTXX

extern tmopen(), tmstrategy();
