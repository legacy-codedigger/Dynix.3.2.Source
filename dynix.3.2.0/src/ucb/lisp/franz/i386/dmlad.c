#include "config.h"

	.text

	.globl	_dmlad
/*
	routine for destructive multiplication and addition to a bignum by
	two fixnums.

	from C, the invocation is dmlad(sdot,mul,add);
	where sdot is the address of the first special cell of the bignum
	mul is the multiplier, add is the fixnum to be added (The latter
	being passed by value, as is the usual case.

	Register assignments:

	r7 = current sdot
	r6 = carry
	r5  = previous sdot, for relinking.
*/
	.align	2
_dmlad:
	enter	[r2,r3,r4,r5,r6,r7],0
	movd	8(fp),r7		#initialize cell pointer
	movd	16(fp),r6		#initialize carry
loop:
 #	emul	12(fp),0(r7),r6,r0	#r0 gets cell->car times mul + carry

	movd	0(r7),r0
	movd	12(fp),r1
	absd	r0,r5
	movqd	0,r3
	absd	r1,r2
	meid	r5,r2			# abs( 0(r7) ) * abs( 12(fp) )

	xord	r1,r0
	cmpqd	0,r0
	ble	l30
	comd	r2,r2
	comd	r3,r3
	addqd	1,r2
	addcd	0,r3
l30:					# 0(r7) * 12(fp)
	movqd	0,r5
	cmpd	r5,r6
	ble	l40
	comd	r5,r5
l40:	addd	r6,r2
	addcd	r5,r3			# 0(r7) * 12(fp) + r6

 #	extzv	$0,$30,r0,0(r7)
 #	extv	$30,$32,r0,r6
	movd	r2,r4			# just in case an integer result
	deid	0x40000000,r2
	movd	r2,0(r7)
	movd	r3,r6

	movd	r7,r5			#save last cell for fixup at end.
	movd	4(r7),r7		#move to next cell
	cmpqd	0,r7
	bne	loop			#done indicated by 0 for next sdot
	cmpqd	0,r6			#if carry zero no need to allocate
	beq	done			#new bigit
	cmpqd	-1,r6
	bne	alloc			#if not must allocate new cell.
	cmpqd	0,0(r5)			#make sure product isn't -2**30
	beq	alloc
	movd	r4,0(r5)		#save old lower half of product.
	br	done

alloc:	jsr	_qnewdot		#otherwise allocate new bigit
	movd	r6,0(r0)		#store carry
	movd	r0,4(r5)		#save new link cell
done:	movd	8(fp),r0
	exit	[r2,r3,r4,r5,r6,r7]
	ret	0
