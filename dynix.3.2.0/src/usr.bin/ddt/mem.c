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
 * mem.c: version 1.8 of 7/14/83
 * 
 */
# ifndef lint
static char rcsid[] = "$Header: mem.c 2.1 87/06/05 $";
# endif

/*
 * host defines
 */
#include <setjmp.h>
#include <stdio.h>

/*
 * target defines
 */
#include <a.out.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/vm.h>
#include <machine/vmparam.h>
#include <machine/reg.h>
#include <machine/pte.h>
#include <machine/plocal.h>

#define	USIZE	ctob(UPAGES)		/* size of u area */
#define PGSIZE	NBPG			/* size of a page */
#define TOPUSER	USRSTACK		/* highest addr in user space */

/*
 * ddt defines
 */
#include "sym.h"
#include "main.h"
#include "parse.h"
#include "display.h"
#include "error.h"
#include "machine.h"

extern int pid;
extern int errno;

struct user u;
char 	uspace[10+USIZE - sizeof(struct user)];

/* these functions will be for accessing the user's
   address space. An invalid address will cause longjmp back
   to main.
*/

memabort()
{
	longjmp(resetbuf, MEM_ERROR);
}

getbyte(ataddr)
{
int data = 0;
		getmemb(ataddr,1,&data);
		return(data);
}

getword(ataddr)
{
int data = 0;
		getmemb(ataddr,2,&data);
		return(data);
}

getdouble(ataddr)
{
int data = 0;
		getmemb(ataddr,4,&data);
		return(data);
}

getmem(ataddr)
{
int data = 0;
	switch (acontext) {
		case 1: getmemb(ataddr,1,&data);
			break;
		case 2: getmemb(ataddr,2,&data);
			break;
		case 4: getmemb(ataddr,4,&data);
			break;
		default: printf("\tcontext bug %d\r\n",acontext);
			 memabort();
	}
	return(data);
}

getinimage(mstart,msize,mbuf)
int mstart, msize;
char *mbuf;
{
	if (imagefd < 0) {
		printf("\r\nno object file\r\n");
		memabort();
	}
	if (mstart >= N_ADDRADJ(ddtheader)) {
		if (lseek(imagefd, N_TXTOFF(ddtheader) - N_ADDRADJ(ddtheader) + mstart, 0) < 0) {
			printf("\r\nillegal text address %x\r\n", mstart);
			memabort();
		} else if (read(imagefd, mbuf, msize) != msize) {
			printf("\r\ntext read for %x fails\r\n", mstart);
			memabort();
		} else
			return;
	}
	for (; msize > 0; msize--) {
		*mbuf = (char)0;
		mbuf++;
	}
	write(1,"\7",1);				
}

getmemb(mstart,msize,mbuf)
	int mstart, msize;
	char *mbuf;
{
	int *pint;
	register struct exec *hpr = &ddtheader;

	if (mstart < 0) {
		for (; msize > 0; msize--) {
			*mbuf = (char)0;
			mbuf++;
		}
		write(1,"\7",1);				
		return;
	}
	if (skipptrace)  {
		if (mstart < hpr->a_text) {
			getinimage(mstart,msize,mbuf);
			return;
		}
	}
	if (usefile == TRUE) {
		if (memfd > 0) {
			if (lseek(memfd, mstart, 0) < 0) {
				printf("\tinvalid address %x\n", mstart);
				memabort();
			}
			if (read(memfd, mbuf, msize) != msize) {
				printf("\tbad read from %x\r\n", mstart);
				memabort();
			}
			return;
		}
	}
	if (corefd > 0) {
		if (mstart < u.u_tsize)	{			/* in text */
			getinimage(mstart, msize, mbuf);
			return;
		} else if (mstart < (u.u_tsize + u.u_dsize)) {	/* in data */
			if (lseek(corefd, (USIZE + mstart - u.u_tsize), 0) < 0){
				printf("\tinvalid address %x\n",mstart);
				memabort();
			}
		} else {					/* in stack */
		    if (mstart >= (TOPUSER - u.u_ssize) && mstart < TOPUSER) {
			mstart -= (TOPUSER - u.u_ssize);
			if (lseek(corefd, (USIZE + u.u_dsize + mstart), 0) < 0){
				printf("\tstack address not in core file %x\r\n",mstart);
				memabort();
			}

		    } else {
			printf("\tstack address not in core file %x\r\n", mstart);
			memabort();
		    }
		}
		if (read(corefd, mbuf, msize) != msize) {
		    printf("\t core read from %x fails\r\n", mstart);
			memabort();
		}
	} else if ((imageonly) || (pid == -1)) {
		getinimage(mstart,msize,mbuf);
	} else {
		pint = (int *) mbuf;
		*pint = machine(RMEM, pid, mstart, 0);
		if (errno) {
		    printf("\tcan not read process data\r\n");
		    memabort();
		}
		switch (msize) {
		case 1:
			*pint &= 0xff;
			break;
		case 2:
			*pint &= 0xffff;
			break;
		case 4:
		default:
			break;
		}
	}
}

setbyte(ataddr,val)
	int ataddr, val;
{
	int tempval = 0;

	if (pid == -1) {
		printf("\tno process to write data in\r\n");
		memabort();
	}
	tempval = machine(RMEM,pid,ataddr,0);
	if (errno) {
		printf("\tcan not read data %x\r\n",ataddr);
		memabort();
	}
	val = (tempval & 0xffffff00) | (0xff & val);
	machine(WMEM,pid,ataddr,val);
	if (errno) {
		printf("\tcan not insert data here %x\r\n",ataddr);
		memabort();
	}
}

setword(ataddr,val)
	int ataddr, val;
{
	int tempval = 0;

	if (pid == -1) {
		printf("\tno process to write data in\r\n");
		memabort();
	}
	tempval = machine(RMEM,pid,ataddr,0);
	if (errno) {
		printf("\tcan not read data %x\r\n",ataddr);
		memabort();
	}
	val = (tempval & 0xffff0000) | (0xffff & val);
	machine(WMEM,pid,ataddr,val);
	if (errno) {
		printf("\tcan not insert data here %x\r\n",ataddr);
		memabort();
	}
}

setdouble(ataddr,val)
	int ataddr, val;
{
	if (pid == -1) {
		printf("\tno process to write data in\r\n");
		memabort();
	}
	machine(WMEM,pid,ataddr,val);
	if (errno) {
		printf("\tcan not insert data here %x\r\n",ataddr);
		memabort();
	}
}

setmem(ataddr,val)
	int ataddr, val;
{
	if (imageonly == TRUE) {
		if (ataddr < N_ADDRADJ(ddtheader)) {
			printf("\taddress %x not in file\r\n", ataddr);
			memabort();
		}
		if (lseek(imagefd, N_TXTOFF(ddtheader) - N_ADDRADJ(ddtheader) + ataddr, 0) < 0) {
			printf("\taddress %x not in file\r\n", ataddr);
			memabort();
		}
		if (write(imagefd, ((char *) &val), tempmodes.context) != 
		    tempmodes.context) {
			printf("\tbad write to %x\r\n",ataddr);
			memabort();
		}
		return;
	}
	if (usefile == TRUE) {
		if (memfd > 0) {
			if (lseek(memfd, ataddr, 0) < 0) {
				printf("\taddress %x not in file\r\n", ataddr);
				memabort();
			}
			if (write(memfd, ((char *) &val), tempmodes.context) != tempmodes.context) {
				printf("\twrite to file fails %x\r\n", ataddr);
				memabort();
			}
			return;
		}
	}
	switch (tempmodes.context) {
		case 1: setbyte(ataddr, val);
			break;
		case 2: setword(ataddr, val);
			break;
		case 4: setdouble(ataddr, val);
			break;
		default: printf("\tcontext bug %d\r\n", acontext);
			 memabort();
	}
}

getudot()
{
	if (lseek(corefd,0,0)<0)
		printf("\tio core failure\n\r");
	if (read(corefd, &u, USIZE)<0)
		printf("\tio core failure\n\r");

	u.u_ar0 = (int *) ((int)u.u_ar0 - VA_UAREA + (int)&u);

	if (u.u_tsize == 0) {
		printf("Warning u.u_tsize is zero\r\n");
		if (imagefd > 0) {
			printf("Changing to object text size\r\n");
			u.u_tsize = ddtheader.a_text;
		}
	} else {
		u.u_tsize = ctob(u.u_tsize);
	}

	if (u.u_dsize == 0) {
		printf("Warning u.u_dsize is zero\r\n");
		if (imagefd > 0) {
			printf("Changing to object data size\r\n");
			u.u_dsize = ddtheader.a_data+ddtheader.a_bss;
		}
	} else {
		u.u_dsize = ctob(u.u_dsize) - u.u_tsize;
	}

	if (u.u_ssize == 0)
		printf("Warning u.u_ssize is zero\r\n");
	else 
		u.u_ssize = ctob(u.u_ssize);
}

char *signame[NSIG] = {
	"", "HUP", "INT", "QUIT", "ILL", "TRAP", "IOT", "EMT", "FPE", "KILL", 
	"BUS", "SEGV", "SYS", "PIPE", "ALRM", "TERM", "URG", "STOP", "TSTP", 
	"CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU", "XFSZ", "VTALRM", 
	"PROF", "WINCH" 
};

char *sigill_code[] = {
	"privileged instruction fault",
	"reserved operand fault"
};

char *sigfpe_code[] = {
	"integer divide by zero",
	"floating underflow",
	"floating overflow",
	"floating divide by zero",
	"floating illegal instruction",
	"floating invalid operation",
	"floating inexact result",
	"floating reserved for future"
};

whycore()
{
	int signo;
	extern char *sys_siglist[];

	printf("core for `%s' caused by ", u.u_comm); 
	signo = u.u_arg[0]&017;
	if (signo == 0 || signo > NSIG)
		printf("signal %d", signo);
	else
		printf("SIG%s", signame[signo]);
	if (signo == SIGILL && u.u_code < 2)
		printf(" (%s)", sigill_code[u.u_code]); 
	else if (signo == SIGFPE && u.u_code < 8)
		printf(" (%s)", sigfpe_code[u.u_code]); 
	printf("\r\n");
}

fpcheck(fpat)
int fpat;
{
	if (fpat == 0)
		return(FALSE);
	else
		return(TRUE);
}
