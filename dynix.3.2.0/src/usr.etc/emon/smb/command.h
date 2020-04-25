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
 * $Header: command.h 1.1 86/05/20 $
 */

/*
 * $Log:	command.h,v $
 */

/* CORE File Sharing Protocol Commands */

#define	SMBmkdir	0x00		/* create directory */
#define	SMBrmdir	0x01		/* delete directory */
#define	SMBopen		0x02		/* open file */
#define	SMBcreate	0x03		/* create file */
#define	SMBclose	0x04		/* close file */
#define	SMBflush	0x05		/* flush file */
#define	SMBunlink	0x06		/* unlink file */
#define	SMBmv		0x07		/* rename file */
#define	SMBgetatr	0x08		/* get file attributes */
#define	SMBsetatr	0x09		/* set file attributes */
#define	SMBread		0x0A		/* read from file */
#define	SMBwrite	0x0B		/* write to file */
#define	SMBlock		0x0C		/* lock byte range */
#define	SMBunlock	0x0D		/* unlock byte range */
#define	SMBctemp	0x0E		/* create temporary file */
#define	SMBmknew	0x0F		/* make new file */
#define	SMBchkpth	0x10		/* check directory path */
#define	SMBexit		0x11		/* process exit */
#define	SMBlseek	0x12		/* seek */
#define	SMBtcon		0x70		/* tree connect */
#define	SMBtdis		0x71		/* tree disconnect */
#define	SMBnegprot	0x72		/* negotiate protocol */
#define	SMBdskattr	0x80		/* get disk attributes */
#define	SMBsearch	0x81		/* search directory */
#define	SMBsplopen	0xC0		/* open print spool file */
#define	SMBsplwr	0xC1		/* write to print spool file */
#define	SMBsplclose	0xC2		/* close print spool file */
#define	SMBsplretq	0xC3		/* return print queue */

/*
 * SMBcommand_wct == the appropriate *command* wct (not response)
 */

#define	SMBmkdir_wct	0		/* create directory */
#define	SMBrmdir_wct	0		/* delete directory */
#define	SMBopen_wct	2		/* open file */
#define	SMBcreate_wct	3		/* create file */
#define	SMBclose_wct	3		/* close file */
#define	SMBflush_wct	1		/* flush file */
#define	SMBunlink_wct	1		/* unlink file */
#define	SMBmv_wct	1		/* rename file */
#define	SMBgetatr_wct	0		/* get file attributes */
#define	SMBsetatr_wct	8		/* set file attributes */
#define	SMBread_wct	5		/* read from file */
#define	SMBwrite_wct	5		/* write to file */
#define	SMBlock_wct	5		/* lock byte range */
#define	SMBunlock_wct	5		/* unlock byte range */
#define	SMBctemp_wct	3		/* create temporary file */
#define	SMBmknew_wct	3		/* make new file */
#define	SMBchkpth_wct	0		/* check directory path */
#define	SMBexit_wct	0		/* process exit */
#define	SMBlseek_wct	4		/* seek */
#define	SMBtcon_wct	0		/* tree connect */
#define	SMBtdis_wct	0		/* tree disconnect */
#define	SMBnegprot_wct	0		/* negotiate protocol */
#define	SMBdskattr_wct	0		/* get disk attributes */
#define	SMBsearch_wct	2		/* search directory */
#define	SMBsplopen_wct	2		/* open print spool file */
#define	SMBsplwr_wct	1		/* write to print spool file */
#define	SMBsplclose_wct	1		/* close print spool file */
#define	SMBsplretq_wct	2		/* return print queue */


/*
 * SMBresponse_rwct == the appropriate *response* wct 
 */

#define	SMBmkdir_rwct	0		/* create directory */
#define	SMBrmdir_rwct	0		/* delete directory */
#define	SMBopen_rwct	7		/* open file */
#define	SMBcreate_rwct	1		/* create file */
#define	SMBclose_rwct	0		/* close file */
#define	SMBflush_rwct	0		/* flush file */
#define	SMBunlink_rwct	0		/* unlink file */
#define	SMBmv_rwct	0		/* rename file */
#define	SMBgetatr_rwct	10		/* get file attributes */
#define	SMBsetatr_rwct	0		/* set file attributes */
#define	SMBread_rwct	5		/* read from file */
#define	SMBwrite_rwct	1		/* write to file */
#define	SMBlock_rwct	0		/* lock byte range */
#define	SMBunlock_rwct	0		/* unlock byte range */
#define	SMBctemp_rwct	1		/* create temporary file */
#define	SMBmknew_rwct	1		/* make new file */
#define	SMBchkpth_rwct	0		/* check directory path */
#define	SMBexit_rwct	0		/* process exit */
#define	SMBlseek_rwct	2		/* seek */
#define	SMBtcon_rwct	2		/* tree connect */
#define	SMBtdis_rwct	0		/* tree disconnect */
#define	SMBnegprot_rwct	1		/* negotiate protocol */
#define	SMBdskattr_rwct	5		/* get disk attributes */
#define	SMBsearch_rwct	1		/* search directory */
#define	SMBsplopen_rwct	1		/* open print spool file */
#define	SMBsplwr_rwct	0		/* write to print spool file */
#define	SMBsplclose_rwct 0	/* close print spool file */
#define	SMBsplretq_rwct	2		/* return print queue */

