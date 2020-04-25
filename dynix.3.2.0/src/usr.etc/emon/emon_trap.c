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

#ifndef	lint
static	char	rcsid[] = "$Header: emon_trap.c 2.2 87/04/12 $";
#endif

/*
 * $Log:	emon_trap.c,v $
 */

#include "emon.h"

#define	BLUE 0x1b
#define	GREY 0x44
#define	SEQUENT 0x13
#define	BROADCAST 0xff
#define	CLONE6	 0x6f
#define	SQNT4	 0x6f
#define	CLONE4	 0xd2
#define	CLONE1	 0xba
#define	CLONE7	 0xfb
#define	SEQUENT1 0xc9
#define	SILVER 	 0xbe
#define	BALANCE	 0xba
#define	SEQUENT_E 0x08
#define	SCSI1 0xef
#define	SCSI2 0xad
/*					     190.239 */
#define	IBMPC1 0x66 

u_char trapE[6];

do_do_trap()
{
	char arg[16];
	char xarg[3];
	int l, i;
	struct eentry * ep;

	xarg[2] = '\0';

	for(i=0; i<6; i++) {
		trapE[i] = 0;
	}

	getarg(arg, 13);

	i = 0;
	l = strlen(arg);

	/*
	 * input up to 16 hex nibbles.  If less that 12, then right
	 * justify and zero fill for the address - traps all addresses
	 * that match the bytes provided (low order to high order).
 	 */

	if(l == 0) {
		trapping = OFF;
		trapx = 0;
		printf("trapping = OFF\n");
		return(0);
	}

	trapping = ON;

	if(l == 1) {	/* allow exactly one to be specified */
		xarg[0] = '\0';
		xarg[1] = *arg;
		trapE[5] = atox(xarg);
		goto endit;
	}
	if(l >= 12) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		l--; l--;
		trapE[0] = atox(xarg);
	}
	if(l >= 10) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		trapE[1] = atox(xarg);
		l--; l--;
	}
	if(l >= 8) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		trapE[2] = atox(xarg);
		l--; l--;
	}
	if(l >= 6) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		trapE[3] = atox(xarg);
		l--; l--;
	}
	if(l >= 4) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		trapE[4] = atox(xarg);
		l--; l--;
	}
	if(l >= 2) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		trapE[5] = atox(xarg);
		l--; l--;
	}
endit:

	if(trapx > TRAPEES) {
		printf("trap table full\n");
		return(0);
	}

	for(i = 0; i < 6; i++) 
		trapees[trapx].eaddr[i] = trapE[i];

	ep = (struct eentry *) &trapees[trapx];

	getarg(arg, 16);

	ep->ename = enames[trapx];

	for(i=0; i < TNAMLEN; i++) 
		ep->ename[i] = arg[i];

	for(i = strlen(ep->ename); i < TNAMLEN; i++)
		ep->ename[i] = ' ';

	ep->ename[TNAMLEN] = '\0';

	printf("%x %x %x %x %x %x\n",
		ep->eaddr[0], ep->eaddr[1], ep->eaddr[2],
		ep->eaddr[3], ep->eaddr[4], ep->eaddr[5]);

	printf("Now named %s - %s\n", arg, ep->ename);

	trapx++;
	do_do_trapwho();

	return(0);
}

u_char
atox(xarg)
	char* xarg;
{
	u_char ans;

	ans = (u_char) '\0';
	if(*xarg >= '0' && *xarg <= '9') {
		ans = (*xarg - 0x30);
	}else if (*xarg >= 'a' && *xarg <= 'f') {
		ans = (*xarg - 0x57);
	}
	xarg++;
	ans = (u_char) (ans << 4);
	if(*xarg >= '0' && *xarg <= '9') 
		ans = ans + (*xarg - 0x30);
	else if (*xarg >= 'a' && *xarg <= 'f') 
		ans = (u_char) (ans + (u_char) (*xarg - 0x57));
	return(ans);
}

do_do_trapwho()
{
	int i;

	if(!trapping) {
		printf("not trapping any particular ether address\n");
		return;
	}

	for(i = 0; i < trapx; i++) {
		printf("trapees[%d] = %x %x %x %x %x %x\t", i,
			trapees[i].eaddr[0], trapees[i].eaddr[1],
			trapees[i].eaddr[2], trapees[i].eaddr[3],
			trapees[i].eaddr[4], trapees[i].eaddr[5]);

		printf("%s\n", trapees[i].ename);
	}
	return;
}
