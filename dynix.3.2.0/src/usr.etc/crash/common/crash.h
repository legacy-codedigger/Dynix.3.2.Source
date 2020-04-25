/* $Header: crash.h 2.16 1991/07/29 23:26:22 $ */

/* $Copyright: $
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
 * $Log: crash.h,v $
 *
 *
 *
 *
 *
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef BSD
#include <pcc.h>
#else
#define INKERNEL
#include <sys/var.h>
#undef  INKERNEL
#endif

#ifndef _SYS_CFG_H_
#ifdef BSD
#include <machine/cfg.h>	/* needed to pick up arch_type values */
#else
#include <sys/cfg.h>	/* needed to pick up arch_type values */
#endif
#endif

struct	prmode	{
	char	*pr_name;
	int	pr_sw;
};

struct initfuns {
	int	(*i_fun)();
};

struct	tsw	{
	char	*t_snm;		/* uniq name */
	char	*t_nm;		/* full name */
	int	(*t_fun)();	/* function  */
	char	*t_dsc;		/* description */
} ;

/* Structures for internal symbol tables */
struct	sym { 
	char          *sym_name;	/* pointer into string table */
	unsigned long sym_value; 	/* value of symbol itself */
	struct sdb    *sym_sdb;		/* pointer to sdb info (or NULL )*/
	int	      sym_sdbi;		/* index into sdb info (or -1) */
};
extern struct sym *searchbynam();

struct	sdb { 
	char 	      *sdb_name;	/* pointer into string table */
	unsigned char  sdb_type;	/* SDB symbol type */
	unsigned short sdb_desc;	/* PCC generated description */
	unsigned long  sdb_soff; 	/* structure element offset */
        unsigned long  sdb_size; 	/* size of symbol itself */
	unsigned long  sdb_inx;		/* internal index */
#ifdef BSD
	struct symbol *sdb_sym;		/* pointer to symbol table entry */
#else
	unsigned char  sdb_class;       /* SDB class */
	unsigned long  sdb_tag;	        /* tag COFF index */
	unsigned long  sdb_mem;		/* parent COFF index */
	unsigned long  sdb_cinx;	/* original COFF index */
#endif
};
extern struct sdb *searchsdb();

struct val { 
	int v_value;		/* return value */
	struct sdb v_sdb;	/* symbolic information */
};

struct set_val {
	char *sv_name;
	struct set_val *sv_next;
	struct val sv_val;
};
extern struct set_val *searchsetval();

#define	DIRECT	2
#define	OCT2	3
#define	DEC2	4
#define	CHAR	5
#define	WRITE	6
#define	INODE	7
#define	BYTE	8
#define	DEC4	9
#define	OCT4	10
#define	HEX2	11
#define HEX4	12
#define STRING	13
#define STRUCT	14
#define SDB	15

#define	FALSE	0
#define	TRUE	1
#define	MAXSTRLEN	64

#define MINS(a,b)	((a)>(b)?(b):(a))

#define MAX_DEBUG       26
extern int debug[];
extern int xdebug;

extern int err_search;
extern int errno;
extern int kernfd, swapfd;
extern FILE *vmcorefd;
extern char *dynix, *vmcore, *swap;
extern int tok;
extern int live;
extern int paniced;
extern char *panicstr;
extern char Version[], *Date;
extern char _sobuf[];
extern struct tsw t[];
extern struct engine *l_engine;		/* local copy of engine table */
extern struct engine *v_engine;		/* kernel virtual address of engine table */
extern unsigned Nengine;		/* number of engines */
extern unsigned nonline;		/* number of online engines */
extern long Etext;
extern int end_locore;
extern int start_locore;
extern char *err_atoi;

extern struct pte *Sysmap;
extern struct proc *getproc(), *readproc();
extern struct proc *proc, *procmax, *procNPROC;
extern struct config_desc Cd_loc;

/* THIS REALLY WILL BE FIXED */
#define	SWAPPED	(char *)1	/* Returned by getuarea if process is swapped */
#define	BADREAD	(char *)2	/* Returned by getuarea if bad read of u_area */

extern char *getuarea();
extern int readv(), readp();
extern char *nice_char();
extern char *user_cmd();
extern char *addr_str();
extern char *token();
extern char *malloc(), *realloc(), *index(), *rindex();
#ifdef BSD
extern char *sbrk(), *brk();
#endif
extern char *lookbyval();
extern struct sdb sdbinfo;
extern struct val dot;
extern struct set_val set_val;
extern int arch_type;	/* used in readv when reading text */

char *ctime();

#define SANITY(c, m, arg)  (!(c) || (fprintf(stderr, "crash: %s(%d): %s %s\n", __FILE__, __LINE__, m, arg),exit(1)))

#define MAX_CHECK       20
extern char *name_buf[];

extern long text_size;
extern long text_offset;
extern long text_start;
extern long data_size;
extern long data_offset;
extern long data_start;
extern long bss_size;
extern long bss_start;

#ifdef _SEQUENT_
extern struct var v;

#define N_GSYM	0x20
#define N_SSYM	0x60
#define bzero(s,n)	memset(s, 0, n)
#endif

#if defined(BSD) && defined(NOSTR)
#define strchr index
#define strrchr rindex
#define memcmp(A,B,N) bcmp(A,B,N)
#define memcpy(A,B,N) bcopy(B,A,N)
#endif
