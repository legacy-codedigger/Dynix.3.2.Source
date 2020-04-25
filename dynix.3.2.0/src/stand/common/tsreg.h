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

/* $Header: tsreg.h 2.0 86/01/28 $ */

/*
 * Ts.c stand alone device driver header file.
 * 
 * Contains useful macros and driver structures used
 * to operate the device.
 */
struct ts_cmd {
	u_char	ts_command;		/* the SCSI command goes HERE 	*/
	u_char	ts_unit;		/* unit number in top 3 bits  	*/
	u_char	ts_bytes[8];		/* variable; from 6 to 8 used 	*/
};

/*
 * Sense byte handling 
 */
#define TSFM	0x80		/* EOF EOM bits */
#define TSEOM	0x40		/* EOM bit */
#define TSKEY	0x0F		/* Filemarks detected */

struct tstypes {
	u_char *setmodecmd;
};

/*
 * some useful macros
 */

	/* set block count */
#define SETBC(c,n)				\
	(c)->ts_unit     |= 1;			\
	(c)->ts_bytes[0]  =   (n) >>  16; 	\
	(c)->ts_bytes[1]  =   (n) >>  8; 	\
	(c)->ts_bytes[2]  =   (n)

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
