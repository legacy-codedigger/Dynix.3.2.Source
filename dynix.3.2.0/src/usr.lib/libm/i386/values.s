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

/* $Header: values.s 1.5 1991/06/03 21:52:51 $ */

	.text
	.align 2

	.globl	libm_quarter_pi
	.globl	libm_half_pi
	.globl	libm_maxexp
	.globl	libm_minexp
	.globl	libm_epsilon
	.globl	libm_log_tot
	.globl	libm_lnovfl
	.globl	libm_hi_range
	.globl	libm_lo_range
	.globl  _maxfloat
	.globl  _minfloat
	.globl  _maxdouble
	.globl  _mindouble
	.globl	libm_half_tloss

libm_quarter_pi:.double	0Dx3FE921FB54442D18   #PI/4
libm_half_pi:	.double	0Dx3FF921FB54442D18   #PI/2
libm_maxexp:	.double	0Dx40862E42FEFA39EF   #709.782712893384(max arg)
libm_minexp:	.double	0DxC086232BDD7AB3BF   #-708.396418532(min arg)
libm_epsilon:	.double	0Dx3FD2BEC33301883A   #1 - sqrt(2)/2 (0.29289321881345)
libm_log_tot:	.double 0Dx3FD62E42FEAD449C
libm_lnovfl:	.double 0Dx40862E42FEFA39EF
libm_hi_range:	.double 0Dx4046800000000000   #45
libm_lo_range:	.double 0Dx3DDB7CDFD9D7BDBA   #1e-10
libm_half_tloss:	.double 0Dx433921fb54442d18   # X_TLOSS/2 in values.h
_maxfloat:      .float  0Fx7f7fffff
_minfloat:      .float  0Fx00800000
_maxdouble:     .double 0Dx7fefffffffffffff
_mindouble:     .double 0Dx0010000000000000
