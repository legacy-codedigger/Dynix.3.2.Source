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

/* $Header: sdreg.h 2.5 90/09/13 $ */

/*
 * Useful macros and driver structures used
 * to operate the SCSI disk device.
 */
struct sd_cmd {
	u_char	sd_command;		/* the SCSI command goes HERE 	*/
	u_char	sd_unit;		/* unit number in top 3 bits  	*/
	u_char	sd_bytes[8];		/* variable; from 6 to 8 used 	*/
};

/*
 * some useful macros
 */

#define	SD_ADDRALIGN	8		/* align I/O to 8-byte boundaries */
#define SDSENSEKEY	0x0f		/* extended sense key mask */
#define SDRECOVERED	0x01		/* drive recovered from error */

	/* set block address */
#define SETBA(c,n)				\
	(c)->sd_unit     |= (((n) >> 16)&0x1f); \
	(c)->sd_bytes[0]  =   (n) >>  8; 	\
	(c)->sd_bytes[1]  =   (n)

/*
 * The following macro fills out an iat entry(s)
 * based on whether the <4k transfer will cross
 * a hardware imposed dma address boundary of 64K bytes.
 *
 * Requires a pointer to a two entry sec_iat array.
 *
 */	
#define SETMA(m, count, iatp)		\
	if(((m) & ~0xFFFF) != (((m)+(count)) & ~0xFFFF)) { 		  \
		int	x = (((m)+(count)) & ~0xFFFF);		  	  \
		((struct sec_iat *)(iatp))[0].iat_data = (u_char *)(m);	  \
		((struct sec_iat *)(iatp))[0].iat_count = x-(m);	  \
		((struct sec_iat *)(iatp))[1].iat_data = (u_char *)x;	  \
		((struct sec_iat *)(iatp))[1].iat_count = (((m)+(count))&0xFFFF); \
	}else{								  \
		((struct sec_iat *)(iatp))[0].iat_data = (u_char *)(m);	  \
		((struct sec_iat *)(iatp))[0].iat_count = (count);	  \
	}
