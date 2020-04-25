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

/* $Header: pow.c 2.0 86/01/28 $ */

#include <mp.h>
pow(a,b,c,d) MINT *a,*b,*c,*d;
{	int i,j,n;
	MINT x,y;
	x.len=y.len=0;
	xfree(d);
	d->len=1;
	d->val=xalloc(1,"pow");
	*d->val=1;
	for(j=0;j<b->len;j++)
	{	n=b->val[b->len-j-1];
		for(i=0;i<15;i++)
		{	mult(d,d,&x);
			mdiv(&x,c,&y,d);
			if((n=n<<1)&0100000)
			{	mult(a,d,&x);
				mdiv(&x,c,&y,d);
			}
		}
	}
	xfree(&x);
	xfree(&y);
	return;
}
rpow(a,n,b) MINT *a,*b;
{	MINT x,y;
	int i;
	x.len=1;
	x.val=xalloc(1,"rpow");
	*x.val=n;
	y.len=n*a->len+4;
	y.val=xalloc(y.len,"rpow2");
	for(i=0;i<y.len;i++) y.val[i]=0;
	y.val[y.len-1]=010000;
	xfree(b);
	pow(a,&x,&y,b);
	xfree(&x);
	xfree(&y);
	return;
}
