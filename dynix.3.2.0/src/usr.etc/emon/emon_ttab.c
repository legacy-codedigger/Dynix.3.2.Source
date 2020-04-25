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
static	char	rcsid[] = "$Header: emon_ttab.c 2.3 87/04/11 $";
#endif

/*
 * $Log:	emon_ttab.c,v $
 */

#include "emon.h"

struct eentry etab[] = {
	{"CRG2        ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x6d }},
	{"SEQUENT     ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x81 }},
	{"CRG0        ", { 0x02, 0x07, 0x01, 0x00, 0x17, 0xf4 }},
	{"SW1         ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x06 }},
	{"SW2[CRG_GW] ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x1a }},
	{"SW3         ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x03 }},
	{"SW4[SILVER] ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x0e }},
	{"SW5         ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x21 }},
	{"SW6         ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x08 }},
	{"SW7         ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x04 }},
	{"SW8         ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x37 }},
	{"SE4         ", { 0x08, 0x00, 0x47, 0x00, 0x00, 0x07 }},
	{"MENTOR      ", { 0x02, 0x07, 0x01, 0x00, 0x40, 0x70 }},
	{"BROADCAST   ", { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }},
#ifdef oldaddrs
	{"BLUE        ", { 0x02, 0x07, 0x01, 0x00, 0x0d, 0x1b }},
	{"GREY        ", { 0x02, 0x07, 0x01, 0x00, 0x12, 0x44 }},
	{"CRAN        ", { 0x02, 0x07, 0x01, 0x00, 0x17, 0xf4 }},
	{"SEQUENT     ", { 0x02, 0x07, 0x01, 0x00, 0x0b, 0x13 }},
	{"CLONE6(JAX)", { 0x02, 0x07, 0x01, 0x00, 0x0e, 0x6f }},
	{"CLONE4(RBAK)", { 0x02, 0x07, 0x01, 0x00, 0x14, 0xd2 }},
	{"CLONE1[GIZ]", { 0x02, 0x07, 0x01, 0x00, 0x0c, 0xba }},
	{"CLONE1[GARYK]", { 0x02, 0x07, 0x01, 0x00, 0x18, 0x1d }},
	{"CLONE7[PHIL]", { 0x02, 0x07, 0x01, 0x00, 0x10, 0xfb }},
	{"SEQUENT1", { 0x02, 0x07, 0x01, 0x00, 0x14, 0xc9 }},
	{"SILVER", { 0x02, 0x07, 0x01, 0x00, 0xba, 0xbe }},
	{"BALANCE1", { 0x02, 0x07, 0x01, 0x00, 0xba, 0x01 }},
	{"BALANCE2", { 0x02, 0x07, 0x01, 0x00, 0xba, 0x02 }},
	{"BALANCE3", { 0x02, 0x07, 0x01, 0x00, 0xba, 0x03 }},
	{"BALANCE4", { 0x02, 0x07, 0x01, 0x00, 0xba, 0x04 }},
	{"BALANCE5", { 0x02, 0x07, 0x01, 0x00, 0xba, 0x05 }},
	{"BALANCE6", { 0x02, 0x07, 0x01, 0x00, 0xba, 0x06 }},
	{"BALANCE7", { 0x02, 0x07, 0x01, 0x00, 0xba, 0x07 }},
	{"BALANCE8", { 0x02, 0x07, 0x01, 0x00, 0xba, 0x08 }},
	{"SW2         ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x15}},
	{"SW3[GSP]    ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x03}},
	{"SW5[MUFFY]  ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x19}},
	{"SW6         ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x1a}},
	{"SW7         ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x37}},
	{"SW8         ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x17}},
	{"SE1         ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x16}},
	{"MFG1        ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x0c}},
	{"MKT0        ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x12}},
	{"MKT1        ",{0x08, 0x00, 0x47, 0x00, 0x00, 0x11}},
	{"IBMPC1      ", {0x02, 0x60, 0x8c, 0x01, 0x20, 0x66 }},
#endif oldaddrs
	{"............", {0,0,0,0,0,0}}
};

struct eentry unknown =
	{"XXXXXXXXXXXX", {0x08, 0x00, 0x47, 0x00, 0x00, 0x00}};

/*
 * spare names to assign trapees
 */

char* enames[5] =
	{".....TRAPEE0", ".....TRAPEE1", ".....TRAPEE2", ".....TRAPEE3",
	".....TRAPEE4"};

char* xtab = "0123456789abcdef";

caddr_t
enameof(ether_host)
	caddr_t ether_host;
{
	struct eentry * ep;
	int i;

	for (ep = &etab[0]; (char) *ep->ename != '.'; ep++) {
		if(jcmp((u_char*)ether_host, (u_char *) ep->eaddr, EADDRLEN)
		    == SAME)
			break;
	}
	if(*ep->ename == '.') {
		for(i=0; i < trapx; i++){
			if(jcmp((u_char*)ether_host, trapees[i].eaddr,
			   EADDRLEN) == SAME)
				break;
		}
		if(i >= trapx) {
		   for(i=0; i < EADDRLEN; i++){
			 unknown.ename[2*i] = xtab[(ether_host[i] >> 4) & 0x0f];
			 unknown.ename[2*i+1] = xtab[ether_host[i] & 0x0f];
		   }
		   ep = &unknown;
		}
		else {
			if(if_debug)printf("name found\n");
			if(if_debug)printf("%s\n", trapees[i-1].ename);
			ep = (struct eentry *) &trapees[i-1];
		}
	}

	return(ep->ename);
}

do_do_addname()
{
	struct eentry * ep;

	if(trapx > TRAPEES) {
		printf("trap table full\n");
		return;
	}
	ep = (struct eentry *) &trapees[trapx].ename;
	fill_ename(ep);
	return;
}

fill_ename(ep)
	struct eentry * ep;
{
	char arg[16];
	char xarg[3];
	int l, i;

	xarg[2] = '\0';

	for(i=0; i < EADDRLEN; i++) 
		ep->eaddr[i] = 0;

	getarg(arg, 13);

	i = 0;
	l = strlen(arg);

	/*
	 * can input up to 16 hex nibbles.  If less that 12, then right
	 * justify and zero fill for the address -
	 * traps all addresses that match the bytes provided
	 * (low order to high order).
	 */

	if(l == 1) { /* allow exactly one to be specified */

		xarg[0] = '\0';
		xarg[1] = *arg;
		ep->eaddr[5] = atox(xarg);
		goto endit;
	}
	if(l >= 12) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		l--; l--;
		ep->eaddr[0] = atox(xarg);
	}
	if(l >= 10) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		ep->eaddr[1] = atox(xarg);
		l--; l--;
	}
	if(l >= 8) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		ep->eaddr[2] = atox(xarg);
		l--; l--;
	}
	if(l >= 6) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		ep->eaddr[3] = atox(xarg);
		l--; l--;
	}
	if(l >= 4) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		ep->eaddr[4] = atox(xarg);
		l--; l--;
	}
	if(l >= 2) {
		xarg[0] = arg[i++];
		xarg[1] = arg[i++];
		ep->eaddr[5] = atox(xarg);
		l--; l--;
	}
endit:
	for(i=0; i < TNAMLEN; i++){
		arg[i] = ' ';
	}
	getarg(arg, 16);
	for(i=0; i < TNAMLEN; i++) {
		ep->ename[i] = arg[i];
	}

	ep->ename[12] = '\0';

	printf("%x %x %x %x %x %x\n",
		ep->eaddr[0], ep->eaddr[1], ep->eaddr[2],
		ep->eaddr[3], ep->eaddr[4], ep->eaddr[5]);
	printf("Now named %s - %s\n", arg, ep->ename);
	return;
}

jcmp(s1, s2, len)	/* reverse compare */
	u_char * s1;
	u_char * s2;
	int len;
{
	s1 += len - 1;
	s2 += len - 1;
	for (; len > 0; len--)
		if(*s1-- != *s2--)
			return(~SAME);
	return(SAME);
}
