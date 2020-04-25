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

/* $Header: zdc_ucode.h 1.2 86/03/20 $ */

/*
 * Structure that represents batches ZDC microcode embedded within C programs.
 * The u_loaded field indicates if this batch is currently in WCS. The u_size
 * is the batch's size, in bytes.  The struct uword declaration is intended
 * to fake out the compiler; in practice, the u_word structure array will be
 * quite long - one array member per microinstruction word.
 *
 * A translation program converts the microassembler output into .s files that
 * have constant definitions that map into this structure type.
 */

#define MAX_UCODESIZE	4096	/* Maximum number of microcode words. */
#define	UWORDSZ	8

struct ucode {
	u_char	u_loaded;
	u_char	u_chksum;
	u_short	u_size;
	struct uword {
		u_char	uw_bytes[UWORDSZ];
	} u_word[1];	
};

/*
 * The address bits of the sequencer's micro-pc and the contents
 * of the WREG are split across two locations.  These macros reduce the
 * effort required to read and write this information.
 */
#define	READ_UPC(slic)	((rdslave(slic,SL_Z_UPC0_3) & 0x0f) | \
			 ((rdslave(slic,SL_Z_UPC4_A) & 0xff) << 4))
#define READ_WREG(slic)	((rdslave(slic,SL_Z_WREG0_3) & 0x0f) | \
			 ((rdslave(slic, SL_Z_WREG4_A) & 0xff) << 4))
#define WRITE_WREG(slic, addr)	{ \
	wrslave(slic, SL_Z_WREG0_3, (u_char)((addr) & 0x0f)); \
	wrslave(slic, SL_Z_WREG4_A, (u_char)((addr) >> 4 & 0xff)); \
	}
