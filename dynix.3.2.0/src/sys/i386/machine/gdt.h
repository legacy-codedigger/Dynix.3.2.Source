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
 * $Header: gdt.h 2.7 87/11/11 $
 *
 * gdt.h
 *	Global Descriptor Table (GDT) definitions.  Intel 80386 specific.
 *
 * Dynix uses a constant set of segment descriptors (effectively turns
 * segmentation off), using exclusively paging for memory mapping and
 * protection.
 *
 * There is a copy of this table per processor to avoid LOCK# contention
 * during GDT access (see plocal.h).
 *
 * User has no Local Descriptor Table (LDT); system sets LDTR=0.
 *
 * There is a Task State Segments (TSS) for each processor.  This
 * avoids having to re-build the TSS descriptor after turning it on
 * (HW makes it "busy"), since a shared TSS could be concurrently in use.
 * This TSS lists the ring-zero SS and SP for kernel access.  There
 * is no per-process TSS, since HW task switching mechanism is not used.
 */

/* $Log:	gdt.h,v $
 */

/*
 * Definitions give descriptor index in GDT for particular descriptor.
 *
 * Entry zero is undefined (illegal).
 * Higher number entries are made illegal by limiting size of GDT.
 */

#define	GDT_KCS		1		/* kernel code-segment */
#define	GDT_KDS		2		/* kernel data/stack/etc-segment */
#define	GDT_UCS		3		/* user code-segment */
#define	GDT_UDS		4		/* user data/stack/etc-segment */
#define	GDT_TSS		5		/* shared TSS for kernel SS,SP */

#define	GDT_SIZE	6		/* # GDT entries */

/*
 * Interrupt Descriptor Table (IDT) size is fully filled out (256 entries),
 * to better catch bogus hardware vectors; see machine/trap.h for the set
 * of legal interrupt entries.  All other entries vector to a reserved-trap
 * handler (t_res) in machine/locore.s.  This doesn't cost more per-processor
 * memory since have a full 4k page for struct plocal anyhow.
 */

#define	IDT_SIZE	256

/*
 * Convert a descriptor index to GDT selector.
 */

#define	KERNEL_RPL	0x0			/* Kernel RPL for selector */
#define	USER_RPL	0x3			/* User RPL for selector */
#define	RPL_MASK	0x3			/* mask for RPL value */

#define	GDTXTOKSEL(x)	(((x)<<3)|KERNEL_RPL)	/* kernel selector */
#define	GDTXTOUSEL(x)	(((x)<<3)|USER_RPL)	/* user selector */

#define	KERNEL_CS	GDTXTOKSEL(GDT_KCS)	/* Kernel CS selector */
#define	KERNEL_DS	GDTXTOKSEL(GDT_KDS)	/* Kernel DS selector */

#define	USER_CS		GDTXTOUSEL(GDT_UCS)	/* User CS selector */
#define	USER_DS		GDTXTOUSEL(GDT_UDS)	/* User DS selector */

/*
 * Segment and gate descriptor definitions.
 *
 * Defines useful subset for Dynix; see Intel documentation for full set.
 *
 * The descriptor formats reflect their heritage.
 */

struct	segment_desc	{
unsigned short	sd_limit_low,		/* limit bits 15-0 */
		sd_base_low;		/* base bits 15-0 */
unsigned char	sd_base_mid,		/* base bits 23-16 */
		sd_type_etc,		/* various type stuff (P,DPL,type) */
		sd_limit_high,		/* limit bits 19-16 (plus G,B) */
		sd_base_high;		/* base bits 31-24 */
};

struct	gate_desc	{
unsigned short	gd_offset_low,		/* offset bits 15-0 */
		gd_selector;		/* selector */
unsigned char	gd_word_cnt,		/* word count (low 5 bits) */
		gd_type_etc;		/* various type stuff (P,DPL,type) */
unsigned short	gd_offset_high;		/* offset bits 31-16 */
};

struct	tss_seg	{
	u_short	ts_link, ts_fill1;
	u_long	ts_esp0;
	u_short	ts_ss0, ts_fill2;
	u_long	ts_esp1;
	u_short	ts_ss1, ts_fill3;
	u_long	ts_esp2;
	u_short	ts_ss2, ts_fill4;
	u_long	ts_cr3;
	u_long	ts_eip;
	u_long	ts_flags;
	u_long	ts_eax;
	u_long	ts_ecx;
	u_long	ts_edx;
	u_long	ts_ebx;
	u_long	ts_esp;
	u_long	ts_ebp;
	u_long	ts_esi;
	u_long	ts_edi;
	u_short	ts_es, ts_fill5;
	u_short	ts_cs, ts_fill6;
	u_short	ts_ss, ts_fill7;
	u_short	ts_ds, ts_fill8;
	u_short	ts_fs, ts_fill9;
	u_short	ts_gs, ts_fill10;
	u_short	ts_ldt, ts_fill11;
	u_short	ts_avail, ts_bmapoff;
};

/*
 * Type codes and other useful fields.
 */

#define	DATA_SEG_TYPE	0x13		/* Data Seg, ED=0, Writable, Accessed */
#define	CODE_SEG_TYPE	0x1B		/* Exec Seg, C=0, Readable, Accessed */

#define	CALL_GATE_TYPE	0x0C		/* 386 Call Gate */
#define	INTR_GATE_TYPE	0x0E		/* 386 Interrupt Gate */
#define	TRAP_GATE_TYPE	0x0F		/* 386 Trap Gate */

#define	TSS_SEG_TYPE	0x09		/* 386 Task State Segment */

#define	DPL_KERNEL	(KERNEL_RPL<<5)	/* Privilege Level 0 (kernel) */
#define	DPL_USER	(USER_RPL<<5)	/* Privilege Level 3 (user) */
#define	DESC_PRESENT	0x80		/* Present bit */

#define	GRAN_4K		0x80		/* Limit Granularity = 4k, not byte */
#define	BIG_SS		0x40		/* If SS, use ESP instead of SP */

/*
 * Macros to break up values and assign into segment structures.
 * SD_LIMIT_{LOW,HIGH}() handle the "-1" needed for the descriptors.
 *
 * SD_MAX_SEG gives limit argument for full size segment.
 */

#define	SD_LIMIT_LOW(l)		 (((int)(l)-1)&0x0000FFFF)	/* bits 15-0 */
#define	SD_LIMIT_HIGH(l)	((((int)(l)-1)&0x000F0000)>>16)	/* bits 19-16 */

#define	SD_BASE_LOW(b)		 ((u_int)(b)&0x0000FFFF)	/* bits 15-0 */
#define	SD_BASE_MID(b)		(((u_int)(b)&0x00FF0000)>>16)	/* bits 23-16 */
#define	SD_BASE_HIGH(b)		(((u_int)(b)&0xFF000000)>>24)	/* bits 23-16 */

#define	GD_OFFSET_LOW(o)	 ((u_int)(o)&0x0000FFFF)	/* bits 15-0 */
#define	GD_OFFSET_HIGH(o)	(((u_int)(o)&0xFFFF0000)>>16)	/* bits 31-16 */

#define	BUILD_SEG_DESC(desc,base,limit,type,dpl)	{ \
		(desc)->sd_base_low = SD_BASE_LOW(base); \
		(desc)->sd_base_mid = SD_BASE_MID(base); \
		(desc)->sd_base_high = SD_BASE_HIGH(base); \
		(desc)->sd_limit_low = SD_LIMIT_LOW(limit); \
		(desc)->sd_limit_high = SD_LIMIT_HIGH(limit) | GRAN_4K|BIG_SS; \
		(desc)->sd_type_etc = DESC_PRESENT | (dpl) | (type); \
	}

#define	BUILD_TSS_DESC(desc,base,limit,dpl)	{ \
		(desc)->sd_base_low = SD_BASE_LOW(base); \
		(desc)->sd_base_mid = SD_BASE_MID(base); \
		(desc)->sd_base_high = SD_BASE_HIGH(base); \
		(desc)->sd_limit_low = SD_LIMIT_LOW(limit); \
		(desc)->sd_limit_high = SD_LIMIT_HIGH(limit); \
		(desc)->sd_type_etc = DESC_PRESENT | (dpl) | TSS_SEG_TYPE; \
	}

#define	BUILD_GATE_DESC(desc,sel,offset,type,dpl,wc)	{ \
		(desc)->gd_selector = sel; \
		(desc)->gd_offset_low = GD_OFFSET_LOW(offset); \
		(desc)->gd_offset_high = GD_OFFSET_HIGH(offset); \
		(desc)->gd_word_cnt = wc; \
		(desc)->gd_type_etc = DESC_PRESENT | (dpl) | (type); \
	}

#define	SD_MAX_SEG	(KL1PT_PAGES*NPTEPG*NPTEPG*btop(NBPG))

/*
 * GDT and IDT registers are loaded from a desctab structure.
 * The base address is "linear" -- mapped thru page-tables.
 */

struct	desctab	{
	unsigned short	dt_limit;	/* table limit (max offset in bytes) */
	unsigned short	dt_baselow;	/* low 16-bits of table base */
	unsigned short	dt_basehigh;	/* high 16-bits of table base */
};

#define	DT_BASE_LOW(b)		 ((u_int)(b)&0x0000FFFF)	/* bits 15-0 */
#define	DT_BASE_HIGH(b)		(((u_int)(b)&0xFFFF0000)>>16)	/* bits 31-16 */

#define	BUILD_TABLE_DESC(desc, tab, nent) { \
		(desc)->dt_limit = (nent) * sizeof(struct gate_desc) - 1; \
		(desc)->dt_baselow = DT_BASE_LOW(tab); \
		(desc)->dt_basehigh = DT_BASE_HIGH(tab); \
	}
