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


/*
 * reg.c: version 1.8 of 8/29/83
 * 
 */
# ifndef lint
static char rcsid[] = "$Header: reg.c 2.1 86/04/17 $";
# endif

/*
 * host defines
 */
#include <setjmp.h>

/*
 * target defines 
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dir.h> 
#include <sys/user.h>
#include <sys/vm.h>
#include <machine/reg.h>
#include <machine/vmparam.h>
#include <machine/pte.h>
#include <machine/plocal.h>
#include "main.h"
#include "parse.h"
#include "display.h"
#include "error.h"
#include "machine.h"
#include "fpu.h"			/* from GENIX /usr/include */

#define	USIZE	ctob(UPAGES)		/* size of u area */
#define PGSIZE	NBPG			/* size of a page */
#define TOPUSER	MAXADDR			/* highest addr in user space */
#define RFP	FP			/* fp offset from u_ar0 */
#define RSP	SP			/* sp offset from u_ar0 */
#define RPC	PC			/* pc offset from u_ar0 */
#define RPSRMOD	MODPSR			/* mod and psr offset from u_ar0 */

extern struct user u;
extern char 	uspace[10+USIZE - sizeof(struct user)];

extern int pid;
extern int errno;

/* these functions will be for accessing the user's
   registers. An invalid register will cause longjmp back
   to main.
*/

regabort()
{
	longjmp(resetbuf, REG_ERROR);
}

/*
 * set register contents
 */
setreg(regn,val)
	int regn, val;
{
	int regvalue;

	if (regn >= 0) {
	    printf("\r\nillegal reg %s\r\n",regstr(regn));
	    regabort();
	}
	if (imageonly) {
		printf("\r\nNo registers accessable with imageonly\r\n");
		regabort();
	} else {
		int *ar0;		/* address of r0 on kernel stack */
					/* the rest of the regs are near it */
		ar0 = (int *) (
			machine(RREG, pid, &(((struct user *)0)->u_ar0), 0)
			- VA_UAREA
			);
		switch (regn) {
		case SR0:
		    regvalue = (int)&ar0[R0];
		    break;
		case SR1:
		    regvalue = (int)&ar0[R1];
		    break;
		case SR2:
		    regvalue = (int)&ar0[R2];
		    break;
		case SR3:
		    regvalue = (int)&ar0[R3];
		    break;
		case SR4:
		    regvalue = (int)&ar0[R4];
		    break;
		case SR5:
		    regvalue = (int)&ar0[R5];
		    break;
		case SR6:
		    regvalue = (int)&ar0[R6];
		    break;
		case SR7:
		    regvalue = (int)&ar0[R7];
		    break;
		case SFP:
		    regvalue = (int)&ar0[RFP];
		    break;
		case SSP:
		    regvalue = (int)&ar0[RSP];
		    break;
		case SPC:
		    regvalue = (int)&ar0[RPC];
		    break;
		case SMOD:
		    regvalue = (int)&ar0[RPSRMOD];
		    break;
		case SPSR:
		    regvalue = (int)&ar0[RPSRMOD];
		    break;
		default:
		    if ((regn <= F0) && (regn >= FSR)) {
			regvalue = (int)&(((struct user *)0)->u_fpusave) + (F0-regn)*sizeof(int);
			/* regvalue = (int)&(((struct user *)0)->u_fpusave[F0-regn]); */
		    } else {
			printf("\tillegal reg %s\r\n",regstr(regn));
			regabort();
		    }
		    break;
		}
		if (pid == -1) {
		    printf("\r\nno process running\r\n");
		    regabort();
		}
		if (regn == SPSR) {
		    val = (val << 16) | getreg(SMOD);
		} else if (regn == SMOD) {
		    val = (getreg(SPSR) << 16) | (val & 0x0000ffff);
		}
		machine(WREG,pid,regvalue,val);
		if (errno) {
		    printf("\r\nwrite of register failed\r\n");
		    regabort();
		}
	}
}

/*
 * get register contents
 */
getreg(regn)
	int regn;
{
	int regvalue;

	if (regn >= 0) {
	    printf("\r\nillegal reg %s\r\n",regstr(regn));
	    regabort();
	}
	if (corefd > 0) {
			switch (regn) {
			case SR0:
			    regvalue = u.u_ar0[R0];
			    break;
			case SR1:
			    regvalue = u.u_ar0[R1];
			    break;
			case SR2:
			    regvalue = u.u_ar0[R2];
			    break;
			case SR3:
			    regvalue = u.u_ar0[R3];
			    break;
			case SR4:
			    regvalue = u.u_ar0[R4];
			    break;
			case SR5:
			    regvalue = u.u_ar0[R5];
			    break;
			case SR6:
			    regvalue = u.u_ar0[R6];
			    break;
			case SR7:
			    regvalue = u.u_ar0[R7];
			    break;
			case SFP:
			    regvalue = u.u_ar0[RFP];
			    break;
			case SSP:
			    regvalue = u.u_ar0[RSP];
			    break;
			case SPC:
			    regvalue = u.u_ar0[RPC];
			    break;
			case SMOD:
			    regvalue = (u.u_ar0[RPSRMOD]) & 0x0000ffff;
			    break;
			case SPSR:
			    regvalue = (u.u_ar0[RPSRMOD] >> 16) & 0x0000ffff;
			    break;
			default:
			    if ((regn <= F0) && (regn >= FSR)) {
				regvalue = *(((int*)&u.u_fpusave) + (F0 - regn));
			    } else {
				printf("\r\nthis reg is not in core file\r\n");
				regabort();
			    }
			    break;
			}
	} else if (imageonly) {
		printf("\r\nNo registers accessable with imageonly\r\n");
		regabort();
	} else {
		int *ar0;		/* address of r0 on kernel stack */
					/* the rest of the regs are near it */
		ar0 = (int *) (
			machine(RREG, pid, &(((struct user *)0)->u_ar0), 0)
			- VA_UAREA
			);

		switch (regn) {
		case SR0:
		    regvalue = (int)&ar0[R0];
		    break;
		case SR1:
		    regvalue = (int)&ar0[R1];
		    break;
		case SR2:
		    regvalue = (int)&ar0[R2];
		    break;
		case SR3:
		    regvalue = (int)&ar0[R3];
		    break;
		case SR4:
		    regvalue = (int)&ar0[R4];
		    break;
		case SR5:
		    regvalue = (int)&ar0[R5];
		    break;
		case SR6:
		    regvalue = (int)&ar0[R6];
		    break;
		case SR7:
		    regvalue = (int)&ar0[R7];
		    break;
		case SFP:
		    regvalue = (int)&ar0[RFP];
		    break;
		case SSP:
		    regvalue = (int)&ar0[RSP];
		    break;
		case SPC:
		    regvalue = (int)&ar0[RPC];
		    break;
		case SMOD:
		case SPSR:
		    regvalue = (int)&ar0[RPSRMOD];
		    break;
		default:
		    if ((regn <= F0) && (regn >= FSR)) {
			regvalue = (int)&(((struct user *)0)->u_fpusave) + (F0-regn)*sizeof(int);
			/* regvalue = (int)&(((struct user *)0)->u_fpusave[F0-regn]); */
		    } else {
			printf("\r\nillegal reg %s\r\n",regstr(regn));
			regabort();
		    }
		    break;
		}
		if (pid == -1) {
		    printf("\r\nno process running\r\n");
		    regabort();
		}
		regvalue = machine(RREG,pid,regvalue,0);
		if (errno) {
		    printf("\r\nread of register failed\r\n");
		    regabort();
		}
		if (regn == SPSR) {
		    regvalue = (regvalue >> 16) & 0x0000ffff;
		} else if (regn == SMOD) {
		    regvalue = (regvalue) & 0x0000ffff;
		} 
	}
	return(regvalue);
}
