  .asciz "$Header: qfuncl.c 1.2 87/04/08 $"

/*					-[Mon Mar 21 17:04:58 1983 by jkf]-
 * 	qfuncl.c				$Locker:  $
 * lisp to C interface
 *
 * (c) copyright 1982, Regents of the University of California
 */

/* 
 * This is written in assembler but must be passed through the C preprocessor
 * before being assembled.
 */

#include "ltypes.h"
#include "config.h"

/* important offsets within data types for atoms */
#define Atomfnbnd 8

/*  for arrays */
#define Arrayaccfun 0

#ifdef PROF
	.set	indx,0
#define Profile \
	addr	prbuf+indx,r0 \
	.set 	indx,indx+4 \
	jsr 	mcount
#define Profile2 \
	movd   r0,r5 \
	Profile	\
	movd   r5,r0 
#else
#define Profile
#define Profile2
#endif

#ifdef PORTABLE
#define NIL	_nilatom
#define NP	_np
#define LBOT	_lbot
#else
#define NIL	0
#define NP	r6
#define LBOT	r7
#endif


/*   transfer  table linkage routine  */

	.globl	_qlinker
	.align	2
_qlinker:
	enter 	[r0,r1,r2,r3,r4,r5,r6,r7],0	# save all possible registers
	Profile
	cmpqd	0,_exception	        # any pending exceptions
	beq	noexc
	cmpqd	0,_sigintcnt		# is it because of SIGINT
	beq	noexc			# if not, just leave
	movqd	2,tos			# else push SIGINT
	jsr	_sigcall
	adjspb	-4
noexc:
	movd	16(fp),r0		# get return pc
	addd	-4(r0),r0		# get pointer to table
	movd	4(r0),r1		# get atom pointer
retry:					# come here after undef func error
	movd	Atomfnbnd(r1),r2	# get function binding
	ble	nonex			# if none, leave
	cmpqd	0,_stattab+2*4		# see if linking possible (Strans)
	beq	nolink			# no, it isn't
	movd	r2,r3
	ashd	-9,r3			# check type of function
	cmpb	BCD,_typetable+1(r3)	
	beq	linkin			# bcd, link it in!
	cmpb	ARRAY,_typetable+1(r3) # how about array?
	beq	doarray			# yep


nolink:
	movd	r1,tos			# non, bcd, call interpreter
	jsr	_Ifuncal
	adjspb	-4
	exit	[r0,r1,r2,r3,r4,r5,r6,r7]
	ret	0

/*
 * handle arrays by pushing the array descriptor on the table and checking
 * for a bcd array handler
 */
doarray:
	movd	Arrayaccfun(r2),r3	# get access function addr shifted
	ashd	-9,r3			# get access function addr shifted
	cmpb	BCD,_typetable+1(r3)	# bcd??
	bne	nolink			# no, let funcal handle it
#ifdef PORTABLE
	movd	NP,r4
	movd	r2,0(r4)		# store array header on stack
	addqd	4,r4
	movd	r4,NP
#else
	movd	r2,0(r6)		# store array header on stack
	addqd	4,r6
#endif
	movd	0(r2),r2
	movd	0(r2),r2		# get in func addr
	jump	2(r2)			# jump in beyond calls header
	
	
linkin:	
	movd	4(r2),r3		# check type of function discipline
	ashd	-9,r3			# check type of function discipline
	cmpqb	0,_typetable+1(r3)	# is it string?
	beq	nolink			# yes, it is a c call, so dont link in
	movd	0(r2),r2			# get function addr
	movd	r2,0(r0)			# put fcn addr in table
	jump	2(r2)			# enter fcn after mask

nonex:	movd	r0,tos			# preserve table address
	movd	r1,tos			# non existant fcn
	jsr	_Undeff			# call processor
	adjspb	-8
	movd	r0,r1			# back in r1
	movd	tos,r0		# restore table address
	br	retry			# for the retry.


	.globl	__erthrow		# errmessage for uncaught throws
__erthrow: 
	.asciz	"Uncaught throw from compiled code"

	.globl _tynames
_tynames:
	.long	NIL				# nothing here
	.long	_lispsys+20*4			# str_name
	.long	_lispsys+21*4			# atom_name
	.long	_lispsys+19*4			# int_name
	.long	_lispsys+23*4			# dtpr_name
	.long	_lispsys+22*4			# doub_name
	.long	_lispsys+58*4			# funct_name
	.long	_lispsys+103*4			# port_name
	.long	_lispsys+47*4			# array_name
	.long	NIL				# nothing here
	.long	_lispsys+50*4			# sdot_name
	.long	_lispsys+53*4			# val_nam
	.long	NIL				# hunk2_nam
	.long	NIL				# hunk4_nam
	.long	NIL				# hunk8_nam
	.long	NIL				# hunk16_nam
	.long	NIL				# hunk32_nam
	.long	NIL				# hunk64_nam
	.long	NIL				# hunk128_nam
	.long	_lispsys+124*4			# vector_nam
	.long	_lispsys+125*4			# vectori_nam

/*	Quickly allocate small fixnums  */

	.globl	_qnewint
	.align	2
_qnewint:
	Profile
	cmpd	r5,1024
	bge	alloc
	cmpd	r5,-1024
	ble	alloc
	addr	_Fixzero(r5),r0
	ret	0
alloc:
	movd	_int_str,r0			# move next cell addr to r0
	cmpqd	0,r0
	ble	callnewi			# if no space, allocate
	movd	_lispsys+24*4,r1
	addqd	1,0(r1)				# inc count of ints
	movd	0(r0),_int_str			# advance free list
	movd	r5,0(r0)			# put baby to bed.
	ret	0
callnewi:
	movd	r5,tos
	jsr	_newint
	adjspb	-4
	movd	tos,0(r0)
	ret	0


/*  _qoneplus adds one to the boxed fixnum in r0
 * and returns a boxed fixnum.
 */

	.globl	_qoneplus
	.align	2
_qoneplus:
	Profile2
	movqd	1,r5
	addd	0(r0),r5
#ifdef PORTABLE
	movd	r6,NP
	movd	r6,LBOT
#endif
	jump	_qnewint

/* _qoneminus  subtracts one from the boxes fixnum in r0 and returns a
 * boxed fixnum
 */
	.globl	_qoneminus
	.align	2
_qoneminus:
	Profile2
	movd	0(r0),r5
	addqd	-1,r5
#ifdef PORTABLE
	movd	r6,NP
	movd	r6,LBOT
#endif
	jump	_qnewint

/*
 *	_qnewdoub quick allocation of a initialized double (float) cell.
 *	This entry point is required by the compiler for symmetry reasons.
 *	Passed to _qnewdoub in r4,r5 is a double precision floating point
 *	number.  This routine allocates a new cell, initializes it with
 *	the given value and then returns the cell.
 */

	.globl	_qnewdoub
	.align	2
_qnewdoub:
	Profile
	movd	_doub_str,r0			# move next cell addr to r0
	ble	callnewd			# if no space, allocate
	movd	_lispsys+30*4,r1
	addqd	1,0(r1)			# inc count of doubs
	movd	0(r0),_doub_str			# advance free list
	movd	r4,0(r0)				# put baby to bed.
	movd	r4,4(r0)				# put baby to bed.
	ret	0

callnewd:
	save	[r4,r5]				# stack initial value
	jsr	_newdoub
	restore	[r4,r5]				# restore initial value
	ret	0

	.globl	_qcons
	.align	2

/*
 * quick cons call, the car and cdr are stacked on the namestack
 * and this function is jsb'ed to.
 */

_qcons:
	Profile
	movd	_dtpr_str,r0			# move next cell addr to r0
	ble	getnew				# if ran out of space jump
	movd	_lispsys+28*4,r1
	addqd	1,0(r1)				# inc count of dtprs
	movd	0(r0),_dtpr_str			# advance free list
storit:
	addqd	-4,r6
	movd	0(r6),0(r0)			# store in cdr
	addqd	-4,r6
	movd	0(r6),4(r0)			# store in car
	ret	0

getnew:
#ifdef PORTABLE
	movd	r6,NP
	addr	-8(r6),LBOT
#endif
	jsr	_newdot			# must gc to get one
	br	storit			# now initialize it.

/*
 * Fast equivalent of newdot, entered by jsb
 */

	.globl	_qnewdot
	.align	2
_qnewdot:
	Profile
	movd	_dtpr_str,r0			# mov next cell addr t0 r0
	cmpqd	0,_dtpr_str
	bge	mustallo			# if ran out of space
	movd	_lispsys+28*4,r1
	addqd	1,0(r1)				# inc count of dtprs
	movd	0(r0),_dtpr_str			# advance free list
	movqd	0,0(r0)
	movqd	0,4(r0)
	ret	0
mustallo:
	jsr	_newdot
	movqd	0,0(r0)
	movqd	0,4(r0)
	ret	0

/*  prunel  - return a list of dtpr cells to the free list
 * this is called by the pruneb after it has discarded the top bignum 
 * the dtpr cells are linked through their cars not their cdrs.
 * this returns with an rsb
 *
 * method of operation: the dtpr list we get is linked by car's so we
 * go through the list and link it by cdr's, then have the last dtpr
 * point to the free list and then make the free list begin at the
 * first dtpr.
 */
qprunel:
	movd	r0,r2				# remember first dtpr location
rep:	movd	_lispsys+28*4,r1
	addqd	-1,0(r1)			# decrement used dtpr count
	movd	4(r0),r1			# put link value into r1
	beq	endoflist			# if nil, then end of list
	movd	r1,0(r0)				# repl cdr w/ save val as car
	movd	r1,r0				# advance to next dtpr
	br	rep				# and loop around
endoflist:
	movd	_dtpr_str,0(r0)			# make last 1 pnt to free list
	movd	r2,_dtpr_str			# & free list begin at 1st 1
	ret	0

/*
 * qpruneb - called by the arithmetic routines to free an sdot and the dtprs
 * which hang on it.
 * called by
 *	movd	sdotaddr,tos
 *	jsr	_qpruneb
 */
	.globl	_qpruneb
	.align	2
_qpruneb:
	Profile
	movd	4(sp),r0			# get address
	movd	_lispsys+48*4,r1
	addqd	-1,0(r1)			# decr count of used sdots
	movd	_sdot_str,0(r0)		# have new sdot point to free list
	movd	r0,_sdot_str		# start free list at new sdot
	movd	4(r0),r0		# get address of first dtpr
	bne	qprunel			# if exists, prune it
	ret	0			# else return.


/*
 * _qprunei 	 
 *	called by the arithmetic routines to free a fixnum cell
 * calling sequence
 *	movd	fixnumaddr,tos
 *	jsr	_qprunei
 */

	.globl	_qprunei
	.align	2
_qprunei:
	Profile
	movd	4(sp),r0		# get address of fixnum
	addr	_Lastfix,r1
	cmpd	r1,r0			# is it a small fixnum
	bhs	skipit			# if so, leave
	movd	_lispsys+24*4,r1
	addqd	-1,0(r1)		# decr count of used ints
	movd	_int_str,0(r0)		# link the fixnum into the free list
	movd	r0,_int_str
skipit:
	ret	0


	.globl	_qpopnames
	.align	2
_qpopnames:			# equivalent of C-code popnames, entered by jsb.
	movd	4(sp),r1	# Lower limit
	movd	_bnp,r2		# pointer to bind stack entry
qploop:
	addqd	-8,r2		# for(; (--r2) > r1;) {
	cmpd	r2,r1		# test for done
	ble	qpdone		
	movd	4(r2),r0
	movd	0(r2),0(r0)	# r2->atm->a.clb = r2 -> val;
	br	qploop		# }
qpdone:
	movd	r1,_bnp		# restore bnp
	ret	0

/*
 * _qget : fast get subroutine
 *  (get 'atom 'ind)
 * called with -8(r6) equal to the atom
 *	      -4(r6) equal to the indicator
 * no assumption is made about LBOT
 * unfortunately, the atom may not in fact be an atom, it may
 * be a list or nil, which are special cases.
 * For nil, we grab the nil property list (stored in a special place)
 * and for lists we punt and call the C routine since it is  most likely
 * and error and we havent put in error checks yet.
 */

	.globl	_qget
	.align	2
_qget:
	Profile
	movd	-4(r6),r1	# put indicator in r1
	movd	-8(r6),r0	# and atom into r0
	beq	nilpli		# jump if atom is nil
	movd	r0,r2
	ashd	-9,r2		# check type
	cmpqb	1,_typetable+1(r2)	# is it a symbol??
	bne	notsymb		# nope
	movd	4(r0),r0	# yes, put prop list in r1 to begin scan
	beq	fail		# if no prop list, we lose right away
lp:	cmpd	r1,4(r0)	# is car of list eq to indicator?
	beq	good		# jump if so
	movd	0(r0),r0
	movd	0(r0),r0	# else cddr down list
	bne	lp		# and jump if more list to go.

fail:	addqd	-8,NP		# unstack args
	ret	0		# return with r0 eq to nil

good:	movd	0(r0),r0		# return cadr of list
	movd	4(r0),r0
	addqd	-8,NP		#unstack args
	ret	0

nilpli:	movd	_lispsys+64*4,r0 # want nil prop list, get it specially
	bne	lp		# and process if anything there
	addqd	-8,NP		#unstack args
	ret	0		# else fail
	
notsymb:
#ifdef PORTABLE
	movd	r6,NP
	addr	-8(r6),LBOT	# must set up LBOT before calling
#else
	addr	-8(r6),LBOT	# must set up LBOT before calling
#endif
	jsr	_Lget		# not a symbol, call C routine to error check
	addqd	-8,NP		#unstack args
	ret	0		# and return what it returned.


/*
 * pushframe : stack a frame 
 * When this is called, the optional arguments and class have already been
 * pushed on the stack as well as the return address (by virtue of the jsr)
 * , we push on the rest of the stuff (see h/frame.h)
 * for a picture of the save frame
 */
	.globl	_qpushframe
	.align	2

_qpushframe:
	Profile
	movd	_errp,tos
	movd	_bnp,tos
	movd	NP,tos
	movd	LBOT,tos
	addr	0(sp),r0	# return addr of lbot on stack
	addr	0(fp),tos
	movd	r7,tos
	movd	r6,tos
	movd	r5,tos
	movd	r4,tos
	movd	r3,tos
	movqd	0,_retval	# set retval to C_INITIAL
	adjspb	12
	jump	0(40+12(sp))

	.globl	_qpopframe
	.align	2

_qpopframe:
	Profile
	movd	40(sp),r0	# olderrp
	ret	60

/*
 * qretfromfr
 * called with frame to ret to in r7.  The popnames has already been done.
 * we must restore all registers, and jump to the ret addr. the popping
 * must be done without reducing the stack pointer since an interrupt
 * could come in at any time and this frame must remain on the stack.
 * thus we can't use popr.
 */

	.globl	_qretfromfr
	.align	2

_qretfromfr:
	Profile
	movd	r7,r0		# return error frame location
	subd	24,r7
	lprd	sp,r7
	movd	0(sp),r3
	movd	4(sp),r4
	movd	8(sp),r5
	movd	12(sp),r6
	movd	16(sp),r7
	lprd	fp,20(sp)
	movd	24(sp),LBOT
	movd	28(sp),NP
	adjspb	12
	jump	0(40+12(sp))

/*
 * this routine finishes setting things up for dothunk
 * it is code shared to keep the size of c-callable thunks
 * for lisp functions, small.
 */
	.globl	_thcpy
	.align	2
_thcpy:
	jsr	_abort
	/*
	movd	0(sp),r0
	movd	ap,tos
	movd	0(r0),tos
	addqd	4,r0
	movd	0(r0),tos
	addqd	4,r0
	jsr	_dothunk
	adjspb	-16
	*/
	ret	0
/*
 * This routine gets the name of the inital entry point
 * It is here so it can be under ifdef control.
 */
	.globl	_gstart
	.align	2
_gstart:
	addr	start,r0
	ret	0
	
	.globl	_proflush
	.align	2
_proflush:
	ret	0

/*
 * The definition of mcount must be present even when the C code
 * isn't being profiled, since lisp code may reference it.
 */
	.align	2

/*
.globl	mcount
mcount:
*/

.globl _mcount
_mcount:

#ifdef PROF
	movd	0(r0),r1
	bne	incr
	movd	_countbase,r1
	beq	return
	addqd	8,_countbase
	movd	0(sp),0(r1)
	addqd	4,r1
	movd	r1,0(r0)
incr:
	addqd	1,0(r1)
return:
#endif
	ret	0

	
/* This must be at the end of the file.  If we are profiling, allocate
 * space for the profile buffer
 */
#ifdef PROF
	.data
	.comm	_countbase,4
	.lcomm	prbuf,indx+4
	.text
#endif

	.globl	_vlsub
	.align	2
_vlsub:
	subd	4(8(sp)),4(4(sp))
	subcd	0(8(sp)),0(4(sp))
	ret	0
