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

#if !defined(lint)
static char     rcsid[] = "$Id: treen.c,v 1.1 88/09/02 11:48:33 ksb Exp $";
#endif lint

/* 
 *	is there some reason why these aren't #defined?
 */

#include	"0.h"
#include	"tree_ty.h"

struct tnode   *
tree1 (arg1)
int     arg1;
{
	return tree (1, arg1);
}

struct tnode   *
tree2 (arg1, arg2)
int     arg1, arg2;
{
	return tree (2, arg1, arg2);
}

struct tnode   *
tree3 (arg1, arg2, arg3)
int     arg1, arg2;
struct tnode   *arg3;
{
	return tree (3, arg1, arg2, arg3);
}

struct tnode   *
tree4 (arg1, arg2, arg3, arg4)
int     arg1, arg2;
struct tnode   *arg3, *arg4;
{
	return tree (4, arg1, arg2, arg3, arg4);
}

struct tnode   *
tree5 (arg1, arg2, arg3, arg4, arg5)
int     arg1, arg2;
struct tnode   *arg3, *arg4, *arg5;
{
	return tree (5, arg1, arg2, arg3, arg4, arg5);
}
