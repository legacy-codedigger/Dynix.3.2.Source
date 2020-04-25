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
 * ident	"$Header: format_conf.c 1.6 91/01/04 $
 * format_conf.c
 *	The tables and global variables which pertain
 *	to all subsystems of the online formatter.
 */

/* $Log:	format_conf.c,v $
 */

#include <sys/types.h>
#include <stdio.h>
#include "format.h"

/*
 * Global variables
 */
int fd;					/* file descriptor for device */
char *diskname;				/* diskname argument */
int disk;				/* index into types[] array */

int function;				/* function to be performed */
int force;
int args;				/* 'or' of command line args */
struct usage_check *usep;		/* ptr to usage struct for function */

char *a_arg, *t_arg, *b_arg, *show_arg;	/* ptr to -a, -t and -b args */
int s_arg, e_arg, p_arg, d_arg, h_arg;	/* values for -s,-e,-p,-d,-h args */

FILE *a_file;
int verbose;	
int debug;

/*
 * format_type table
 * 	There should be one entry in this table for every
 *	supported disk subsytem.
 */
extern int scsi_main(), scsi_setup(), scsi_usage(), scsi_bootnum();
extern struct usage_check scsi_funcs[]; 
extern int scsi_numfuncs;

extern int zd_main(), zd_setup(), zd_usage(), zd_bootnum();
extern struct usage_check zd_funcs[]; 
extern int zd_numfuncs;

struct format_type types[] = {
	{"wd", 16, scsi_setup, scsi_main, scsi_usage, scsi_bootnum, 
		&scsi_numfuncs, scsi_funcs},
	{"sd", 16, scsi_setup, scsi_main, scsi_usage, scsi_bootnum, 
		&scsi_numfuncs, scsi_funcs},
	{"zd", 16, zd_setup, zd_main, zd_usage, zd_bootnum, 
		&zd_numfuncs, zd_funcs}
};

int nformat_types = sizeof(types)/sizeof(struct format_type);
