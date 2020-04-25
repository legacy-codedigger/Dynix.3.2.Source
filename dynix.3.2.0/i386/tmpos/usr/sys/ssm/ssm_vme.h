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

/* $Header: ssm_vme.h 1.3 90/08/01 $ */

/*
 * ssm_vme.h
 *      Systems Services Module (SSM) VMEbus definitions.
 */

/* $Log:	ssm_vme.h,v $
 */

/* defines for programming the SSM's I/O PIC */
#define SSM_IO_PIC_ADDR 0x68
#define PIC_IO_ADDR_REG 3

#define SSM_VME_PIC_MASK	0xfe000000 /* bits that the pic decodes */
#define PIC_IO_ADDR_SHIFT 24

#define PIC_REGISTER(base,reg) (*(long *)((long)(base) | 0x7f<<13 | (reg)))
#define PIC_REGISTER_ADDR(base,reg) ((long*)(((long)base) | 0x7f<<13 | (reg)))
#define PIC_BCR_NARROW          0x4c      /* use with PIC_REGISTER as reg */
#define PIC_BCR_WIDE            0x6c      /* wide PIC address for BCR */
#define PIC_BCR_EMPTY		0x10	  /* PIC buffers don't need flushing */
#define PIC_BCR_FLUSH		2	  /* Flush buffers */
#define PIC_BCR_FLUSH_ACK	1	  /* Buffers just flushed */

#define PIC_FLUSH_LOOPS		1000	  /* cycles to wait before panicing */

#define SSM_VME_COMP_MASK	0x7ff00000 /* comparator bits */

#define SSM_MAP_HIT 		1	  /* All maps--hit (valid) bit */

#define SSM_S2V_1ST_MAP 	64	  /* First usable sequent to vme map */
#define SSM_S2V_NMAPS		2048	  /* number of sequent to vme maps */
#define SSM_S2V_MAP_SIZE	16384	  /* size covered by a s2v map */
#define SSM_S2V_MAP_MASK	(-(long)SSM_S2V_MAP_SIZE)

#define SSM_VME_NMAPS		512	   /* # of maps per map set */
#define SSM_VME_A16_MAPPINGS	32	  /* # of distinct mappings */
#define SSM_VME_MAP_SIZE 	2048
#define SSM_VME_BYTES_PER_MAP 	SSM_VME_MAP_SIZE
#define SSM_VME_MAP_ADDR_MASK 	(-(long)SSM_VME_MAP_SIZE)

/* VME space configuration from address modifier configuration */
#define VME_SPACE(x)    ((x) & 0x30)
#define VME_A16_SPACE   0x20            /* Configured for A16 space */
#define VME_A24_SPACE   0x30            /* Configured for A24 space */
#define VME_A32_SPACE   0x00            /* Configured for A32 space */
#define VME_USER_SPACE  0x10            /* User defined - not supported */

/* values for mapset */
#define SSM_VME_A16_MAPS        0       /* DMA map using A16 registers */
#define SSM_VME_A24_MAPS        1       /* DMA map using A24 registers */
#define SSM_VME_A32_LO_MAPS     2       /* DMA map using low A32 registers */
#define SSM_VME_A32_HI_MAPS     3       /* DMA map using high A32 registers */

#ifdef notyet
long ssm_vme_map_setup();
void ssm_vme_pic_flush();
#endif notyet
long ssm_allocate_maps();
long ssm_s2v_map();
