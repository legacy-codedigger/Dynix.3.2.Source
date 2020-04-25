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
 * $Header: cmc.h 1.24 89/06/28 $
 *
 * Cache Memory Controller (CMC) sub-registers and bits.
 */

/* $Log:	cmc.h,v $
 */


/*
 * CMC ID Register: Read Only.
 */

#define	CMC_ID	0x00
#define	    CMCI_VAL	0x03		/* what the CMC says it is */

/*
 * CMC Version Control Register: Read Only.
 */

#define	CMC_VERSION	0x01		/* version control register */
#define	    CMCV_VER_MASK	0xF0	/* version number mask */
#define	    CMCV_REV_MASK	0x0F	/* revision number */

#define	    CMCV_REVISION(x)	((x) & CMCV_REV_MASK)
#define	    CMCV_VERSION(x)	(((x) & CMCV_VER_MASK) >> 4)


/*
 * Mode Register: Read/Write.
 *
 * This register resets itself to all 1's; hence enables are 0's.
 * This is inconsistent with the rest of the Symmetry VLSI.
 *
 * Notice that invalidating the cache doesn't affect the
 * bus side state; flushing does, of course.
 */

#define	CMC_MODE	0x02		/* mode control sub-register */
#ifdef CMC_REV1
#define	    CMCM_MASK	0xFF		/* useful bits */
#else
#define	    CMCM_MASK	0xF7		/* useful bits */
#endif CMC_REV1
#define	    CMCM_SLAVE		0x80	/* this CMC is the slave */
#define	    CMCM_DISA_INV	0x40	/* disable invalidate */
#define	    CMCM_DISA_FLUSH	0x20	/* disable flush */
#define	    CMCM_DISA_SET	0x10	/* set disabled */
#ifdef CMC_REV1
#define	    CMCM_ASYNC		0x08	/* asynchronous operation */
#endif CMC_REV1
#define	    CMCM_NOCACHE_2G	0x04	/* 2nd gigabyte is non-cached */
#define	    CMCM_NOCACHE_1G	0x02	/* first gigabyte is non-cached */
#define	    CMCM_WRITE_THRU	0x01	/* write-thru mode (else copyback) */

/*
 * Cache Parameters Register: Read/Write.
 */

#define	CMC_PARAM	0x03		/* cache parameters sub-register */
#define	    CMCP_MASK	0x7F		/* useful bits */
#define	    CMCP_NARROW	0x40		/* narrow transfer mode */
#define	    CMCP_SSIZ_MASK	0x30	/* set size select... */
#define		CMCP_SSIZ_32K	0x00	/* 32K bytes */
#define		CMCP_SSIZ_64K	0x10	/* 64K bytes */
#define		CMCP_SSIZ_128K	0x20	/* 128K bytes */
#define		CMCP_SSIZ_256K	0x30	/* 256K bytes */
#define	    CMCP_TSIZ_MASK	0x0C	/* transfer size select... */
#define		CMCP_TSIZ_8	0x04	/* 8 bytes */
#define		CMCP_TSIZ_16	0x08	/* 16 bytes */
#define		CMCP_TSIZ_32	0x0C	/* 32 bytes */
#define	    CMCP_BSIZ_MASK	0x01	/* block size select... */
#define		CMCP_BSIZ_16	0x00	/* 16 bytes */
#define		CMCP_BSIZ_32	0x01	/* 32 bytes */

/*
 * Interface register: Read/Write.
 * On Reset all bits are ones.
 */

#define	CMC_INTRFACE	0x04		/* interface modes sub-register */
#define	    CMCI_MASK	0xFF		/* useful bits */
#define	    CMCI_ASYNC_MASK	0xF0	/* Asynchronous Select bits */
#define		CMCI_ASYNC_BUS3	0x80	/* 0=sync, 1=async */
#define		CMCI_ASYNC_BUS2	0x40
#define		CMCI_ASYNC_BUS1	0x20
#define		CMCI_ASYNC_SLIC	0x10	
#define	    CMCI_FLOAT_ENABLE	0x08	/* 0=float disabled, 1=enabled */
#define	    CMCI_UNUSED		0x04	/* not used */
#define	    CMCI_SLOW_MODE	0x02	/* bdp or cmc slow transfer mode */
#define	    CMCI_ENABLE_LRU	0x01	/* LRU input enabled */

/*
 * Interface Register 2: Read/Write.
 * On reset, all bits are one.
 *
 * The values defined here are used to program the CMC interface register 2.
 * The values depend on processor model, CMC speed, bus transfer size, and
 * bus width.  The Central Systems group is responsible for providing valid
 * values.  (Refer to the CMC Theory of Operation Document for more
 * information on the fields within the interface register 2)
 */

#define	CMC_INTRFACE_2	0x05		/* REQUEST/RESPOND timing control */

#define     CMCI2_MODB_SLOW_16B_NARR    0xFF    /* Model B */
#define     CMCI2_MODB_SLOW_16B_WIDE    0xFF
#define     CMCI2_MODB_SLOW_32B_NARR    0x9D
#define     CMCI2_MODB_SLOW_32B_WIDE    0x7A
#define     CMCI2_MODB_FAST_16B_NARR    0xFF
#define     CMCI2_MODB_FAST_16B_WIDE    0xFF
#define     CMCI2_MODB_FAST_32B_NARR    0xFF
#define     CMCI2_MODB_FAST_32B_WIDE    0xFF

#define     CMCI2_MODC_SLOW_16B_NARR    0xAE    /* Model C (20MHz Model B) */
#define     CMCI2_MODC_SLOW_16B_WIDE    0xAE
#define     CMCI2_MODC_SLOW_32B_NARR    0x8C
#define     CMCI2_MODC_SLOW_32B_WIDE    0x60
#define     CMCI2_MODC_FAST_16B_NARR    0xFF
#define     CMCI2_MODC_FAST_16B_WIDE    0xFF
#define     CMCI2_MODC_FAST_32B_NARR    0xFF
#define     CMCI2_MODC_FAST_32B_WIDE    0xFF

#define     CMCI2_MOD3_SLOW_16B_NARR    0x30    /* Model 3 (ns32532)  */
#define     CMCI2_MOD3_SLOW_16B_WIDE    0x30
#define     CMCI2_MOD3_SLOW_32B_NARR    0x30
#define     CMCI2_MOD3_SLOW_32B_WIDE    0x30
#define     CMCI2_MOD3_FAST_16B_NARR    0x30
#define     CMCI2_MOD3_FAST_16B_WIDE    0x30
#define     CMCI2_MOD3_FAST_32B_NARR    0x30
#define     CMCI2_MOD3_FAST_32B_WIDE    0x30

/*
 * Cache Status Register: Read Only.
 */
#define	CMC_STATUS	0x06		/* cache status register */
#define	    CMCS_MASK	0x03		/* useful bits */
#define	    CMCS_INVAL	0x02		/* invalidate operation is active */
#define	    CMCS_FLUSH	0x01		/* flush operation is active */

/*
 * Cache flush counter: Read/Write.
 */
#define	CMC_FLUSH_LO	0x08
#define	CMC_FLUSH_HI	0x09

/*
 * Processor-side Address Tags: Read/Write.
 */

#define	CMC_PROC_TAG_LO	0x10		/* bits 7:0 of proc-side addr tag */
#define	CMC_PROC_TAG_HI	0x11		/* bits 15:8 of proc-side addr tag */

/*
 * Bus-side Address Tags: Read/Write.
 */

#define	CMC_BUS_TAG_LO	0x12		/* bits 7:0 of bus-side addr tag */
#define	CMC_BUS_TAG_HI	0x13		/* bits 15:8 of bus-side addr tag */

/*
 * Holding Register: Read/Write.
 */

#define	CMC_HOLD	0x14		/* holding register for cache rams */
#define	CMCH_MASK	0xFF		/* useful bits */

/*
 * Processor-side State Tag: Read/Write.
 */

#define	CMC_PROC_STATE	0x16		/* bits 1:0 of proc-side state tag */
#define	    CMCPS_MASK		0x03	/* useful bits */
#define	    CMCPS_INVALID	0x00	/* invalid state */
#define	    CMCPS_PRIVATE	0x01	/* private state */
#define	    CMCPS_MODIFIED	0x02	/* modified state */
#define	    CMCPS_UNK		0x03	/* Not possible */

/*
 * Bus-side State Tag: Read/Write.
 */

#ifdef CMC_REV1
#define	CMC_BUS_STATE	0x17		/* bits 1:0 of bus-side state tag */
#else
#define	CMC_BUS_STATE	0x18		/* bits 1:0 of bus-side state tag */
#endif CMC_REV1
#define	    CMCBS_MASK		0x03	/* useful bits */
#define	    CMCBS_INVALID	0x00	/* invalid state */
#define	    CMCBS_PRIVATE	0x01	/* private state */
#define	    CMCBS_MODIFIED	0x02	/* modified state */
#define	    CMCBS_SHARED	0x03	/* shared state */

/*
 * Bus side state tag register we read from.
 * For CMC Rev 1, this is not the same as the
 * write register.
 */

#define	CMC_BUS_STATE_R	0x18

/*
 * Registers for the performance counters
 */

#define	CMC_P_CNTR1_B0	0x20		/* performance counter1 byte 0 */
#define	CMC_P_CNTR1_B1	0x21		/* performance counter1 byte 1 */
#define	CMC_P_CNTR1_B2	0x22		/* performance counter1 byte 2 */
#define	CMC_P_CNTR1_B3	0x23		/* performance counter1 byte 3 */
#define	CMC_P_SELECT1	0x24		/* performance select reg 1 */
#define	CMC_P_DISABLE_BOTH	0x25	/* disable both counters */
#define	CMC_P_DISABLE1	0x26		/* read=disable, write=clear+enable */
#define	CMC_P_REENABLE1	0x27		/* read=enable+no_clear */
#define	CMC_P_MASK1_B0	0x28		/* mask reg 1 byte 0 */
#define	CMC_P_MASK1_B1	0x29		/* mask reg 1 byte 1 */
#define	CMC_P_MASK1_B2	0x2A		/* mask reg 1 byte 2 */

#define	CMC_P_CNTR2_B0	0x30		/* performance counter2 byte 0 */
#define	CMC_P_CNTR2_B1	0x31		/* performance counter2 byte 1 */
#define	CMC_P_CNTR2_B2	0x32		/* performance counter2 byte 2 */
#define	CMC_P_CNTR2_B3	0x33		/* performance counter2 byte 3 */
#define	CMC_P_SELECT2	0x34		/* performance select reg 2 */
#define	CMC_P_DISABLE2	0x36		/* read=disable, write=clear+enable */
#define	CMC_P_REENABLE2	0x37		/* read=enable+no_clear */
#define	CMC_P_MASK2_B0	0x38		/* mask reg 1 byte 0 */
#define	CMC_P_MASK2_B1	0x39		/* mask reg 1 byte 1 */
#define	CMC_P_MASK2_B2	0x3A		/* mask reg 1 byte 2 */

/*
 * CMC_P_SELECT[12] performance select register options.
 */

#define	    CMCPS_MEMREFS	0	/* select memory references */
#define	    CMCPS_CACHE_M	1	/* select cache misses */
#define	    CMCPS_MEMREQS	2	/* select memory requests */
#define	    CMCPS_BUSOPS	3	/* select bus operations */
#define	    CMCPS_WAITS		4	/* select wait states */
#define	    CMCPS_CPUS		5	/* select cpu stalls */
#define	    CMCPS_TEST32	6	/* select test mode 32 bit */
#define	    CMCPS_TEST4		7	/* select test mode 4 - 8 bit gang */
