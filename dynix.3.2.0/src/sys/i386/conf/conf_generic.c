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
 * $Header: conf_generic.c 2.3 90/07/03 $
 */

/* $Log:	conf_generic.c,v $
 */

#include "../h/param.h"
#include "../h/conf.h"

dev_t	rootdev, argdev;
struct	swdevt swdevt[] = {
	{ -1,	1,	0 },
	{ 0,	0,	0 },
};
struct	genericconf {
	char	*gc_name;
	dev_t	gc_root;
} genericconf[] = {
	/* name, major,minor */
	{ "sd", makedev(1,0), },
	{ "wd", makedev(2,0), },
	{ "zd", makedev(7,0), },
	{ 0 },
};
