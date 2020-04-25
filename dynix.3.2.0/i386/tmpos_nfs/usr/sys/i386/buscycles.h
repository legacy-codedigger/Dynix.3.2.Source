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
 * $Header: buscycles.h 1.1 87/01/08 $
 *
 * Definitions and macros for SB8000 Bus cycles.
 */

/* $Log:	buscycles.h,v $
 */

/*
 * cycle types for sgs are defined below as a 7 bit field.
 * (re: SGS h/w func. spec. "Cycle Type section").
 *
 * Note: 'x' means don't care and will be set to a one to be
 * consistent with the Mentor simulations (aka floats high).
 */
#define CT_MASK		0x7F		/* Usable bits */
#define CT_WA4			0x78	/* 111 1000 */
#define CT_WA4_N		0x38	/* 011 1000 */
#define CT_WA8			0x7A	/* 111 1010 */
#define CT_WA8_N		0x3A	/* 011 1010 */
#define CT_WA8_W		0x58	/* 101 1000 */
#define CT_WA8_NW		0x18	/* 001 1000 */
#define CT_WA16			0x68	/* 110 1000 */
#define CT_WA16_N		0x28	/* 010 1000 */
#define CT_WA16_W		0x5A	/* 101 1010 */
#define CT_WA16_NW		0x1A	/* 001 1010 */
#define CT_WA32_W		0x08	/* 000 1000 */
#define CT_WA32_IW		0x48	/* 100 1000 */
	
#define CT_WDF			0x7C	/* x1x 1100 */
#define CT_WDL			0x7D	/* x1x 1101 */
#define CT_WDF_W		0x5C	/* x0x 1100 */
#define CT_WDL_W		0x5D	/* x0x 1101 */

#define CT_RA4			0x71	/* 111 0001 */
#define CT_RA4_I		0x31	/* 011 0001 */
#define CT_RA8			0x73	/* 111 0011 */
#define CT_RA8_I		0x33	/* 011 0011 */
#define CT_RA8_W		0x51	/* 101 0001 */
#define CT_RA8_IW		0x11	/* 001 0001 */
#define CT_RA16			0x61	/* 110 0001 */
#define CT_RA16_I		0x21	/* 010 0001 */
#define CT_RA16_W		0x53	/* 101 0011 */
#define CT_RA16_IW		0x13	/* 001 0011 */
#define CT_RA32_W		0x41	/* 100 0001 */
#define CT_RA32_IW		0x01	/* 000 0001 */

#define CT_RDF			0x74	/* x1x 0100 */
#define CT_RDF_W		0x54	/* x0x 0100 */
#define CT_RDL			0x75	/* x1x 0101 */
#define CT_RDL_W		0x55	/* x0x 0101 */

#define CT_IA			0x3F	/* 0xx 1111 */
#define CT_ULA			0x5F	/* 10x 1111 */
#define CT_IDLE			0x7F	/* 11x 1111 */

/*
 * Cycle type macros.
 * (re: BDP func. spec. "Slic Interface section, data byte latches").
 */
#define CYCLE63(type)	((type) & 0x78)	/* bits 6:3 of the cycle type */
#define CYCLE21(type)	((type) & 0x06)	/* bits 2:1 of the cycle type */
#define CYCLE0 (type)	((type) & 0x01)	/* bit 0 of the cycle type */
#define CYCLE1 (type)	((type) & 0x02)	/* bit 1 of the cycle type */
#define CYCLE2 (type)	((type) & 0x04)	/* bit 2 of the cycle type */
#define CYCLE3 (type)	((type) & 0x08)	/* bit 3 of the cycle type */
#define CYCLE4 (type)	((type) & 0x10)	/* bit 4 of the cycle type */
#define CYCLE5 (type)	((type) & 0x20)	/* bit 5 of the cycle type */
#define CYCLE6 (type)	((type) & 0x40) /* bit 6 of the cycle type */

/*
 * Byte Mark definitions	(some but not all combinations)
 * (re: none specific, mentions in bdp spec.)
 */
#define BM_MASK		0x0F	/* Usable bits */
#define BM_FIRST_4		0x01	/* first 4 bytes valid */
#define	BM_SECOND_4		0x02	/* second 4 bytes valid */
#define BM_THIRD_4		0x04	/* third 4 bytes valid */
#define BM_FOURTH_4		0x08	/* fourth 4 bytes valid */
#define BM_FIRST_8		0x03	/* first 8 bytes valid */
#define BM_SECOND_8		0x0C	/* Second 8 bytes valid */
#define	BM_FIRST_16		0x0F	/* 16 bytes valid */

/*
 * RdResp(0:3)
 * (re: BDP func. spec. "System Bus Interface section").
 */
#define RR_MASK		0x0F	/* Usable bits */
#define RR_NO_ERROR		0x0C	/* no error */
#define RR_NON_FATAL		0x0A	/* non-fatal error */
#define RR_FATAL		0x06	/* fatal error */
#define RR_TIMEOUT		0x00	/* bus timeout occurred */
#define RR_NOT_RR		0x0F	/* not a read response */

/*
 * RdResp bit masks
 */
#define RR_BITS31	0x0E		/* 3:1 Read Response bits */
