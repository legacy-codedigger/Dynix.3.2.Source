/*
 *
 * Copyright (c) 1982, Regents, University of California
 *
 */
#include "global.h"

struct s_dot	{ long I; struct s_dot *CDR; };
struct vl	{ long high; long low; };

struct s_dot *adbig(a,b)
struct s_dot *a, *b;
{
	int la = 1, lb = 1;
	long *sa, *sb, *sc, *base, *alloca();
	struct s_dot *export();
	register struct s_dot *p;
	register int *q, *r, *s;
	register carry = 0;
	Keepxs();

	/* compute lengths */
	
	for(p = a; p->CDR; p = p->CDR) la++;
	for(p = b; p->CDR; p = p->CDR) lb++;
	if(lb > la) la = lb;

	/* allocate storage areas on the stack */

	base = alloca((3*la+1)*sizeof(long));
	sc = base + la +1;
	sb = sc + la;
	sa = sb + la;
	q  = sa;

	/* copy s_dots onto stack */
	p = a;
	do { *--q = p->I; p = p->CDR; } while (p);
	while(q > sb) *--q = 0;
	p = b;
	do { *--q = p->I; p = p->CDR; } while (p);
	while(q > sc) *--q = 0;

	/* perform the addition */
	for(q = sa, r = sb, s = sc; q > sb;)
	{
		carry += *--q + *--r;
		*--s = carry & 0x3fffffff;
		carry >>= 30;
	}
	*--s = carry;

	p = export(sc,base);
	Freexs();
	return(p);
}

long dodiv(top,bottom)
long *top, *bottom; /* top least significant; bottom most */
{
	struct vl work;
	char error;
	long rem = 0, ediv();
	register long *p = bottom;

	for(;p <= top;p++)
	{
		emul(0x40000000,rem,*p,&work);
		*p = ediv(&work,1000000000,&error);
		rem = work.high;
	}
	return(rem);
}

long dsneg(top,bottom)
long *top, *bottom;
{
	register long *p = top;
	register carry = 0;
	register digit;

	while(p >= bottom)
	{
		digit = carry - *p;
		/* carry = digit >> 30; is slow on 68K */
		if(digit < 0) carry = -2;
		if(digit & 0x40000000) carry += 1;
		*p-- = digit & 0x3fffffff;
	}
}

calqhat(uj,v1)
register long *uj, *v1;
{
	struct vl work1, work2;
	register handy, handy2;
	register qhat, rhat;
	char err;
	if(*v1==*uj) {
		/* set qhat to b-1
		 * rhat is easily calculated since if we substite b-1
		 * for qhat in the formula below, one gets (u[j+1] + v[1])
		 */
		 qhat = 0x3fffffff;
		 rhat = uj[1] + *v1;
	} else {
		/* work1 = u[j]b + u[j+1]; */
		handy2 = uj[1];
		handy = *uj;
		if(handy & 1) handy2 |= 0x40000000;
		if(handy & 2) handy2 |= 0x80000000;
		handy >>= 2;
		work1.low = handy2; work1.high = handy;
		qhat = ediv(&work1,*v1,&err);
		/* rhat = work1 - qhat*v[1]; */
		rhat = work1.high;
	}
again:
	/* check if v[2]*qhat > rhat*b+u[j+2] */
	emul(qhat,v1[1],0,&work1);
	/* work2 = rhat*b+u[j+2]; */
	{ handy2 = uj[2]; handy = rhat;
	if(handy & 1) handy2 |= 0x40000000;
	if(handy & 2) handy2 |= 0x80000000;
	handy >>= 2; work2.low = handy2; work2.high = handy; }
	vlsub(&work1,&work2);
	if(work1.high <= 0) return(qhat);
	qhat--; rhat += *v1;
	goto again;
}

long
mlsb(utop,ubot,vtop,nqhat)
register long *utop, *ubot, *vtop;
register nqhat;
{
	register handy, carry;
	struct vl work;
	
	for(carry = 0; utop >= ubot; utop--) {
		emul(nqhat,*--vtop,carry+*utop,&work);
		carry = work.high;
		handy = work.low;
		*utop = handy & 0x3fffffff;
		carry <<= 2;
		if(handy & 0x80000000) carry += 2;
		if(handy & 0x40000000) carry += 1;
	}
	return(carry);
}

long
adback(utop,ubot,vtop)
register long *utop, *ubot, *vtop;
{
	register handy, carry;
	carry = 0;
	for(; utop >= ubot; utop--) {
		carry += *--vtop;
		carry += *utop;
		*utop = carry & 0x3fffffff;
		/* next junk is faster version of  carry >>= 30; */
		handy = carry;
		carry = 0;
		if(handy & 0x80000000) carry -= 2;
		if(handy & 0x40000000) carry += 1;
	}
	return(carry);
}

long dsdiv(top,bot,div)
register long *top, *bot;
register long div;
{
	struct vl work; char err;
	register long handy, carry = 0;
	for(carry = 0;bot <= top; bot++) {
		handy = *bot;
		if(carry & 1) handy |= 0x40000000;
		if(carry & 2) handy |= 0x80000000;
		carry >>= 2;
		work.low = handy;
		work.high = carry;
		*bot = ediv(&work,div,&err);
		carry = work.high;
	}
	return(carry);
}

dsadd1(top,bot)
long *top, *bot;
{
	register long *p, work, carry = 0;

	/*
	 * this assumes canonical inputs
	 */
	for(p = top; p >= bot; p--) {
		work = *p + carry;
		*p = work & 0x3fffffff;
		carry = 0;
		if (work & 0x40000000) carry += 1;
		if (work & 0x80000000) carry -= 2;
	}
	p[1] = work;
}

long
dsrsh(top,bot,ncnt,mask1)
long *top, *bot;
long ncnt, mask1;
{
	register long *p = bot;
	register r = -ncnt, l = 30+ncnt, carry = 0, work, save;
	long mask = -1 ^ mask1;

	while(p <= top) {
		save = work = *p; save &= mask; work >>= r;
		carry <<= l; work |= carry; *p++ = work;
		carry = save;
	}
	return(carry);
}
/*
 *
 * dsmult(top,bot,mul) --
 * multiply an array of longs on the stack, by mul.
 * the element top through bot (inclusive) get changed.
 * if you expect a carry out of the most significant,
 * it is up to you to provide a space for it to overflow.
 */

dsmult(top,bot,mul)
long *top, *bot, mul;
{
	register long *p;
	struct vl work;
	long add = 0;

	for(p = top; p >= bot; p--) {
		emul(*p,mul,add,&work); /* *p has 30 bits of info, mul has 32
					   yielding a 62 bit product. */
		*p = work.low & 0x3fffffff; /* the stack gets the low 30 bits */
		add = work.high;        /* we want add to get the next 32 bits.
					   on a 68k you might better be able to
					   do this by shifts and tests on the
					   carry but I don't know how to do this
					   from C, and the code generated here
					   will not be much worse.  Far less
					   bad than shifting work.low to the
					   right 30 bits just to get the top 2.
					   */
		add <<= 2;
		if(work.low < 0) add += 2;
		if(work.low & 0x40000000) add += 1;
	}
	p[1] = work.low;  /* on the final store want all 32 bits. */
}

extern int Fixzero[];
lispval inewint(n)
{
	register lispval ip;
	lispval newint();

	if(n < 1024 && n >= -1024)
		return ((lispval)(Fixzero+n));
	ip = newint();
	ip->i = n;
	return(ip);
}

blzero(where,howmuch)
{
	bzero(where,howmuch);
}

struct s_dot *mulbig(a,b)
struct s_dot *a, *b;
{
	int la = 1, lb = 1;
	long *sa, *sb, *sc, *base, *alloca();
	struct s_dot *export();
	register struct s_dot *p;
	register int *q, *r, *s;
	long carry = 0, test;
	struct vl work;
	Keepxs();

	/* compute lengths */
	
	for(p = a; p->CDR; p = p->CDR) la++;
	for(p = b; p->CDR; p = p->CDR) lb++;

	/* allocate storage areas on the stack */

	base = alloca((la + la + lb + lb + 1)*sizeof(long));
	sc = base + la + lb + 1;
	sb = sc + lb;
	sa = sb + la;
	q  = sa;

	/* copy s_dots onto stack */
	p = a;
	do { *--q = p->I; p = p->CDR; } while (p);
	p = b;
	do { *--q = p->I; p = p->CDR; } while (p);
	while(q > base) *--q = 0;  /* initialize target */

	/* perform the multiplication */
	for(q = sb; q > sc; *--s = carry)
	    for((r = sa, s = (q--) - lb, carry = 0); r > sb;)
	    {
		    carry += *--s;
		    emul(*q,*--r,carry,&work);
		    test = work.low;
		    carry = work.high << 2;
		    if(test < 0) carry += 2;
		    if(test & 0x40000000) carry +=1;
		    *s = test & 0x3fffffff;
	    }

	p = export(sc,base);
	Freexs();
	return(p);
}
