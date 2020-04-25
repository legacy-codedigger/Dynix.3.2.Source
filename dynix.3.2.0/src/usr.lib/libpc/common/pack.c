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

/* $Header: pack.c 1.1 89/03/12 $ */
/*
 * pack(a,i,z)
 *
 * with:	a: array[m..n] of t
 *	z: packed array[u..v] of t
 *
 * semantics:	for j := u to v do
 *			z[j] := a[j-u+i];
 *
 * need to check:
 *	1. i >= m
 *	2. i+(v-u) <= n		(i.e. i-m <= (n-m)-(v-u))
 *
 * on stack:	lv(z), lv(a), rv(i) (len 4)
 *
 * move w(t)*(v-u+1) bytes from lv(a)+w(t)*(i-m) to lv(z)
 */

PACK(i, a, z, size_a, lb_a, ub_a, size_z)
long i;		/* subscript into a to begin packing */
char *a;	/* pointer to structure a */
char *z;	/* pointer to structure z */
long size_a;	/* sizeof(a_type) */
long lb_a;	/* lower bound of structure a */
long ub_a;	/* (upper bound of a) - (lb_a + sizeof(z_type)) */
long size_z;	/* sizeof(z_type) */
{
	register char *cp, *zp = z, *limit;
	int subscr;

	subscr = i - lb_a;
	if (subscr < 0 || subscr > ub_a) {
		ERROR("i = %D: Bad i to pack(a,i,z)\n", i);
		/*NOTREACHED*/
		return;
	}
	cp = &a[subscr * size_a];
	limit = cp + size_z;
	do	{
		*zp++ = *cp++;
	} while (cp < limit);
}
