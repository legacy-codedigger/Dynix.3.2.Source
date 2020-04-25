struct vl	{ long high; long low; };

emul(r,d,a,p)
int r,d,a;
struct vl *p;
{
	register int r7 = 0, r6, r5, r4;

	asm("	movqd	0,r7");
	asm("	cmpd	r7,12(fp)");
	asm("	ble	l10");
	asm("	negd	12(fp),12(fp)");
	asm("	addqd	1,r7");
	asm("l10:	cmpqd	0,8(fp)");
	asm("	ble	l20");
	asm("	negd	8(fp),8(fp)");
	asm("	addqd	-1,r7");

	asm("l20:	movqd	0,r5");
	asm("	movd	12(fp),r4");
	asm("	meid	8(fp),r4");

	asm("	cmpqd	0,r7");
	asm("	beq	l30");
	asm("	comd	r4,r4");
	asm("	comd	r5,r5");
	asm("	addqd	1,r4");
	asm("	addcd	0,r5");

	asm("l30:	movqd	0,r2");
	asm("	cmpd	r2,16(fp)");
	asm("	ble	l40");
	asm("	comd	r2,r2");

	asm("l40:	addd	16(fp),r4");
	asm("	addcd	r2,r5");

	p->low = r4;
	p->high = r5;
}
/*
emul(r,d,a,p)
int r,d,a;
struct vl *p;
{
	register int r7, r6;

	asm("	movd	8(fp),r0");
	asm("	movd	12(fp),r1");

	asm("	absd	r0,r2");
	asm("	movqd	0,r7");
	asm("	absd	r1,r6");
	asm("	meid	r2,r6");

	asm("	xord	r1,r0");
	asm("	cmpqd	0,r0");
	asm("	ble	l30");
	asm("	comd	r6,r6");
	asm("	comd	r7,r7");
	asm("	addqd	1,r6");
	asm("	addcd	0,r7");

asm("l30:	movqd	0,r2");
	asm("	cmpd	r2,16(fp)");
	asm("	ble	l40");
	asm("	comd	r2,r2");

asm("l40:	addd	16(fp),r6");
	asm("	addcd	r2,r7");

	p->low = r6;
	p->high = r7;
}
*/
