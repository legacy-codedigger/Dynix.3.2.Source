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

/* $Header: config.h 2.2 90/10/09 $ */

/*
 * Config.
 */
#include <sys/types.h>

#define	NODEV	((dev_t)-1)

struct file_list {
	struct	file_list *f_next;	
	char	*f_fn;			/* the name */
	u_char	f_type;			/* see below */
	u_char	f_flags;		/* see below */
	short	f_special;		/* requires special make rule */
	char	*f_needs;
	/*
	 * Random values:
	 *	swap space parameters for swap areas
	 *	root device, etc. for system specifications
	 */
	union {
		struct {		/* when swap specification */
			dev_t	fuw_swapdev;
			int	fuw_swapsize;
		} fuw;
		struct {		/* when system specification */
			dev_t	fus_rootdev;
			dev_t	fus_argdev;
			dev_t	fus_dumpdev;
		} fus;
	} fun;
#define	f_swapdev	fun.fuw.fuw_swapdev
#define	f_swapsize	fun.fuw.fuw_swapsize
#define	f_rootdev	fun.fus.fus_rootdev
#define	f_argdev	fun.fus.fus_argdev
#define	f_dumpdev	fun.fus.fus_dumpdev
};

/*
 * Types.
 */
#define DRIVER		1
#define NORMAL		2
#define	INVISIBLE	3
#define	PROFILING	4
#define	SYSTEMSPEC	5
#define	SWAPSPEC	6

/*
 * Attributes (flags).
 */
#define	CONFIGDEP	1
#define FASTOBJ		2
#define SEDIT		4
#define SRC		8
#define NOPT		16

/*
 * Maximum number of fields that can be initialized in config file
 */
#define	NFIELDS		10

struct	idlst {
	char	*id;
	struct	idlst *id_next;
};

struct device {
	char	*d_name;		/* name of device (e.g. sd) */
	int     d_type;		/* controller type */
	struct	device *d_conn;		/* what it is connected to */
	struct	device *d_next;		/* Next one in list */
	int	d_slave;		/* still used by pseudos? */
	int	d_unit;			/* unit number */
	int	d_bin;			/* interrupt bin */
	struct	idlst *d_vec;		/* interrupt vectors */
	int	d_dk;			/* if init 1 set to number for iostat */
	u_long	d_fields[NFIELDS];	/* list of fields */
};

/*
 *  flags for indicating presence of upper and lower bound values
 */
#define	P_LB	1
#define	P_UB	2

struct p_entry {
	char 	*p_name;			/* name of field */
	long	p_def;				/* default value */
	long 	p_lb;				/* lower bound for field */
	long	p_ub;				/* upper bound of field */ 
	char	p_flags;			/* bound valid flags */
};

struct proto {
	char	*p_name;			/* name of controller type */
	struct  proto *p_next;			/* list of controllers */
	char	p_seen_flag;			/* seen one of these? */
	short	p_nentries;			/* how many fields */
	struct  p_entry	p_fields[NFIELDS];	/* ordered list of fields */
};


#define QUES	-1	/* -1 means '?' */
#define	UNKNOWN -2	/* -2 means not set yet */
#define TO_NEXUS	(struct device *)-1

struct config {
	char	*c_dev;
	char	*s_sysname;
};

/*
 * Config has a global notion of which machine type is
 * being used.  It uses the name of the machine in choosing
 * files and directories.  Thus if the name of the machine is ``vax'',
 * it will build from ``Makefile.vax'' and use ``../vax/asm.sed''
 * in the makerules, etc.
 *
 * The changes for Balance are substantial enough that VAX and SUN are
 * no longer guaranteed to work.
 */
extern	int	machine;
extern	char	*machinename;
#define MACHINE_BALANCE	3

/*
 * For each machine, a set of CPU's may be specified as supported.
 * These and the options (below) are put in the C flags in the Makefile.
 */
struct cputype {
	char	*cpu_name;
	struct	cputype *cpu_next;
} *cputype;

/*
 * A set of options may also be specified which are like CPU types,
 * but which may also specify values for the options.
 */
struct opt {
	char	*op_name;
	char	*op_value;
	struct	opt *op_next;
} *opt;

extern	char	*ident;
char	*ns();
char	*tc();
char	*qu();
char	*get_word();
char	*path();
char	*raise();

extern	int	do_trace;

char	*index();
char	*rindex();
char	*malloc();
char	*strcpy();
char	*strcat();
char	*sprintf();

struct	device *connect();
extern  struct	device *dtab;
extern  struct	proto  *ptab;
struct  proto  *find_proto();
dev_t	nametodev();
char	*devtoname();

extern	char	errbuf[];
int	yyline;

struct	file_list *ftab, *conf_list, **confp;
char	*PREFIX;

int	timezone, hadtz;
int	dst;
int	profiling;
int	srcconfig;
int	oldfw;			/* transition aid to new FW config tables */

int	maxusers;

#define eq(a,b)	(!strcmp(a,b))
