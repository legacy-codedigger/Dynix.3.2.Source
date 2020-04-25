#	struct vl	{ long high; long low; };
#	
#	ediv(q,d,e)
#	struct vl *q;	/* 64-bit 2's complement number */
#	int d;
#
#		q->high = q%d
#		return q/d;
#
# 8(fp)		q
#12(fp)		d
#16(fp)		e	/* unused */
	.text
	.align	2
	.globl	_ediv
_ediv:
	enter	[],0

	movd	4(8(fp)),r0	# collect dividend into r0, r1
	movd	0(8(fp)),r1
	cmpqd	0,r1
	ble	L16
	comd	r0,r0		# make dividend positive
	comd	r1,r1
	addqd	1,r0
	addcd	0,r1
L16:
	absd	12(fp),r2	# make divisor positive
	deid	r2,r0		# do unsigned division

	movd	0(8(fp)),r2	# if dividend and divisor have oposite
	xord	12(fp),r2	# signs then quotient is negative
	cmpqd	0,r2
	ble	L17
	negd	r1,r1
L17:
	cmpqd	0,0(8(fp))	# if dividend is negative then so
	ble	L18		# is remainder
	negd	r0,r0
L18:
	movd	r0,0(8(fp))	# return remainder

	movd	r1,r0		# return quotient
	exit	[]
	ret	0
