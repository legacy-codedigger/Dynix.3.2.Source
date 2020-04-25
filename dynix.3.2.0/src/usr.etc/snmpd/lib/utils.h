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

#ident	"$Header: utils.h 1.1 1991/07/31 00:06:16 $"

/*
 * utils.h
 *	Definitions used by snmp_config.c 
 */

/* $Log: utils.h,v $
 *
 */


#define streq(s1, s2)	(strcmp((s1), (s2)) == 0)

extern char	*memory(), *mfree();
extern char	*array();
extern char	*copy();

struct list		/* generic linked list type */
{
	char		*data;		/* data pointer */
	struct list	*next;		/* next link in list */
};

typedef struct list	*list_t;	/* linked list pointer */

#define L_NULL	((struct list *) 0)
#define ECHO_TYPE 'E'

extern list_t	mklist();
