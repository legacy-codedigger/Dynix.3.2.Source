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

/* $Header: fpvalues.c 1.1 86/02/24 $
 *
 * define minimum, maximum floating values.
 *	really
 *		float minfloat, maxfloat;
 *		double mindouble, maxdouble;
 *
 */

int	_minfloat = 0x00800000,
	_maxfloat = 0x7f7fffff;

int	_mindouble[2] = { 0x00000000, 0x00100000 },
	_maxdouble[2] = { 0xffffffff, 0x7fefffff };

/*
 * define powers on 1.25 .. used by atof()
 *	really
 *		double pow1_25[];
 */

int pow1_25[] = {
	0x00000000, 0x3ff40000, /* 1.250000000000000000e+0   1.25 ^ 1 */
	0x00000000, 0x3ff90000, /* 1.562500000000000000e+0   1.25 ^ 2 */
	0x00000000, 0x40038800, /* 2.441406250000000000e+0   1.25 ^ 4 */
	0x00000000, 0x4017d784, /* 5.960464477539062500e+0   1.25 ^ 8 */
	0x37e08000, 0x4041c379, /* 3.552713678800500929e+1   1.25 ^ 16 */
	0xb5056e17, 0x4093b8b5, /* 1.262177448353618888e+3   1.25 ^ 32 */
	0xe93ff9f5, 0x41384f03, /* 1.593091911132452277e+6   1.25 ^ 64 */
	0xf9301d32, 0x42827748, /* 2.537941837315649223e+12  1.25 ^ 128 */
	0x7f73bf3b, 0x45154fdd, /* 6.441148769597133308e+24  1.25 ^ 256 */
	0x15d4c1d2, 0x4a3c6334, /* 4.148839747208266430e+49  1.25 ^ 512 */
	0xb0d02ea2, 0x54892ece, /* 1.721287124801515210e+99  1.25 ^ 1024 */
	0x6bb8a7ac, 0x6923d167, /* 2.962829366007466998e+198 1.25 ^ 2048 */
};
