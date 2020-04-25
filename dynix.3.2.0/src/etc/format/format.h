/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
 * ident	"$Header: format.h 1.9 91/03/26 $
 * format.h
 *	defines and data structures used at top level
 *	of online formatter.
 */

/* $Log:	format.h,v $
 */

#define FORMAT_VERSION		4

/*
 * This structure contains information for one disk subsystem.
 */
struct format_type {
	char 	disk_name[32];
	int	align;
	int	(*setup)();
	int	(*dev_main)();
	int	(*usage)();
	int	(*bootnum)();
	int 	*numfuncs;
	struct	usage_check *use;
};

#ifdef BSD

typedef unsigned int uint;
#undef unchar
typedef unsigned char unchar;

#endif /* BSD */

/*
 * Each disk subsystem uses this structure to describe the functions
 * it supports.
 */
struct usage_check  {
	uint	function;
	uint	opts_supported;
	uint	opts_required;
	uint	tasks;
	int	open_flags;
	char 	*usage_string;
};

/*
 * Defines for all possible functions the formatter recognizes.
 * These correspond to the input arguments which specify a specific
 * task and of which one and only one must be specified on the
 * command line.
 */
#define	FORMAT		0x0001	
#define REFORMAT	0x0002
#define REFORMAT_MFG	0x0004
#define ADDBAD		0x0008
#define VERIFY		0x0010
#define WRITEDIAG	0x0020
#define DISPLAY		0x0040
#define USAGE		0x0080
#define ADD_MFG		0x0100
#define INFO		0x0200
#define REPBAD		0x0400
#define SHOW		0x0800
#define	VFORMAT		0x1000	

/*
 * Defines describing all other possible command line arguments.
 * These must be bit defines.
 */
#define B_VERBOSE	0x0001		/* -v : verbose */
#define B_NOWRITEDIAG	0x0002		/* -w : don't write diags */
#define B_NOVERIFY	0x0004		/* -n : don't verify */
#define B_BADFILE	0x0008		/* -b : defects file */
#define B_START 	0x0010		/* -s :	start for verify */ 
#define B_END		0x0020		/* -e : end for verify */
#define B_HDRPASS	0x0040		/* -h : # header passes */
#define B_FULLPASS	0x0080		/* -p : # full passes */
#define B_DEFECTPASS	0x0100		/* -d : # defect passes */
#define B_TYPE		0x0200		/* -t : subsystem interprets */
#define B_OVERWRITE	0x0400		/* -o : write over VTOCS */
#define B_CHECKDATA	0x0800		/* -c : check data on verify */

/* Path to append to fron of specified disk name */
#ifdef BSD
#define DEVPATH		"/dev/r"
#else
#define DEVPATH		"/dev/rdsk/"
#endif

/*
 * Miscellaneous defines
 */
#define ADDLENGTH	256	/* max. length of addbad arg */

#ifndef BSD
/*
 * Macros used across all disk subsystems
 */
#define	bcopy(s1,s2,n)	memcpy(s2, s1, n)
#define bzero(s,n)	memset(s, (char)0, n)
#define bcmp(s1,s2,n)	memcmp(s1, s2, n)
#endif

/* align allocated memory according to specified value */
#define MALLOC_ALIGN(size, align) \
		((align) ? (char *)(((uint)(malloc((size)+(align))) \
			   + (align-1)) & ~(align-1)) \
			: malloc((size)))


/*
 * Global variables
 */
extern int function;		/* function to be performed */
extern int args;		/* 'or' of command line args */ 
extern int force;
extern int verbose;
extern int debug;

extern char *a_arg, *t_arg, *b_arg, *show_arg;
extern int s_arg, e_arg, h_arg, p_arg, d_arg;

extern FILE *a_file;

extern struct format_type types[];
extern int nformat_types;
extern int disk;
extern char *diskname;
extern struct usage_check *usep;
extern int fd;

#define BZ_BADPHYS 0x4
