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
 * process.c: version 1.12 of 8/29/83
 * 
 */
# ifndef lint
static char rcsid[] = "$Header: process.c 2.0 86/01/28 $";
# endif


/* process control and execution functions */

/*
 * host defines
 */

#include <setjmp.h>
#include <stdio.h>
#include <signal.h>

/*
 * target defines
 */

#include <sys/wait.h>
#include <machine/reg.h>
#include "main.h"
#include "sym.h"
#include "display.h"
#include "parse.h"
#include "error.h"
#include "scan.h"
#include "bpt.h"
#include "machine.h"

extern char **environ;
extern int errno;

int	pid = -1;
int	lastsig = SIGTRAP;	/* lastsig is the signal that stopped the */
				/* child the last time ddt collected it. */
				/* a value of SIGTRAP means "no signal". */


processabort()
{
	longjmp(resetbuf, PROC_ERROR);
}

run()
{
	int wrst, wstatus;

	lastsig = SIGTRAP;
	if (pid != -1) {
		machine(KILL, pid, 0, 0);
		pid = -1;
	}
	startup();
	if (pid != -1) {
	    for (wrst = 0; (wrst != pid); ) {
		wrst = waitfor(&wstatus);
		if (wrst == -1) {
			pid = -1;
			stty(0, &ddtstty);
			printf("\tprocess died\r\n");
			processabort();
		} 
	    }
	    stty(0, &ddtstty);
	    seesig(wstatus);
	    if (pid == -1)
		processabort();
	} else {
		printf("\tfork failed\n\r");
		processabort();
	}
}

runit()
{
    getexecargs();
    run();
    dogo();
}

dostep(inbpts) 
{
	if (pid == -1)
		run();
	proceed(STEP, inbpts);
	showspot();
	dostop(-1);
}

dostepover(inbpts) 
{
	int pcis,instr,instr1,instr2,instlen;
	extern char *dis32000();

	pcis = getreg(SPC);
	instr = getword(pcis);
	instr1 = instr & 0xff;
	instr2 = instr & 0x07fc;
	if ((instr1 == 0x22) || (instr1 == 0x02) ||
	     (instr2 == 0x67c) || (instr2 == 0x7c)) {
	     if (dis32000(pcis, &instlen) != (char *)0) {
		 maketemp(pcis + instlen);
		 putinbpts(TEMP);
		 proceed(GOGO,inbpts);
		 outbpts(TEMP);
		 if (lastsig == SIGBPT)
		     lastsig = SIGTRAP;
	     } else
		proceed(STEP,inbpts);
	     showspot();
	     dostop(-1);
	} else {
	    proceed(STEP,inbpts);
	    showspot();
	    dostop(-1);
	}
}

uplevel(perm)
{
	int framepc;

	framepc = getdouble(getreg(SFP) + 4);
	if (perm == TRUE)
	    makebpt(framepc);
	else {
	    maketemp(framepc);
	    putinbpts(TEMP);
	    proceed(GOGO, FALSE);
	    outbpts(TEMP);
	    if (lastsig == SIGBPT)
		lastsig = SIGTRAP;
	    showspot();
	    dostop(-1);
	}
}

showspot() 
{
	int pcis;

	if (pid == -1) {
		printf("\tprocess gone\r\n");
		return;
	}
	if (lastsig == SIGTRAP) {
		pcis = getreg(SPC);
		dot = pcis;
		address = dot;
		printf("\r\n");
		typeout(address,"*/\t");
		lastdata = display(address);
		printf("\t");
		fflush(stdout);
	} else if (lastsig == SIGBPT) {
		pcis = getreg(SPC);
		dot = pcis;
		address = dot;
		showbpt(pcis);
		printf("\t");
		fflush(stdout);
	} else {
		pcis = getreg(SPC);
		dot = pcis;
		address = dot;
		printf("\r\n");
		typeout(address,"*/\t");
		lastdata = display(address);
		printf("\t");
		fflush(stdout);
	}
}

dogo() 
{
	proceed(GOGO,TRUE);
	showspot();
	dostop(-1);
}

doproceed() 
{
	if (lastsig != SIGTRAP) {
		printf("\r\nproceeding with ");
		printsig(lastsig);
	}

	proceed(STEP, FALSE);

	if (lastsig == SIGTRAP)
		proceed(GOGO, TRUE);

	showspot();
}

proceed(prop,dobpts)
{
	int wrst, wstatus;

	if (pid != -1) {
	    if (dobpts) {
		putinbpts(0);
	    }
	    if (prop == GOGO) {
		stty(0, &userstty);
		printf("\n");
	    }
	    if (lastsig == SIGTRAP)
		lastsig = 0;
	    machine(prop, pid, 1, lastsig);
	    if (errno) {
		    printf("\r\nproceed failed\r\n");
		    if (prop == GOGO) {
			stty(0, &ddtstty);
		    }
		    processabort();
	    }
	    for (wrst=0; (wrst != pid);) {
		wrst = waitfor(&wstatus);
		if (wrst == -1) {
			machine(KILL, pid, 0, 0);
			pid = -1;
			if (prop == GOGO) {
			    stty(0, &ddtstty);
			}
			printf("\r\nproceed failed\r\n");
			processabort();
		}
	    }
	    if (prop == GOGO) {
		stty(0, &ddtstty);
	    }
	    seesig(wstatus);
	    if (dobpts && (pid != -1))
		outbpts(0);
	    if (pid == -1)
		processabort();
	} else {
		printf("\n\rno process to proceed\n\r");
		processabort();
	}
}

seesig(thesig)
	union wait thesig;
{
	if (thesig.w_stopval == WSTOPPED) {
		lastsig = thesig.w_stopsig;
		printsig(lastsig);
	} else {
		pid = -1;
		lastsig = SIGTRAP;
		if (thesig.w_termsig == 0) {
			printf("\r\nprocess exits status %x\r\n",
				thesig.w_retcode);
		} else {
			printsig(thesig.w_termsig);
			if (thesig.w_coredump) {
				printf("\r\ncore dump\r\n");
				if (corefd)
					close(corefd);
				corefd = open("core", 0);
				if (corefd > 0)
					getudot();
			}
		}
	}
}

printsig(thesig)
{
	switch (thesig) {
	case 0:
	case SIGBPT:
	case SIGTRAP:
		return;
	case SIGHUP:
		printf("\r\nSIGHUP \r\n");
		break;
	case SIGINT:
		printf("\r\nSIGINT \r\n");
		break;
	case SIGQUIT:
		printf("\r\nSIGQUIT\r\n");
		break;
	case SIGILL:
		printf("\r\nSIGILL \r\n");
		break;
	case SIGIOT:
		printf("\r\nSIGIOT \r\n");
		break;
	case SIGEMT:
		printf("\r\nSIGEMT \r\n");
		break;
	case SIGFPE:
		printf("\r\nSIGFPE \r\n");
		break;
	case SIGKILL:
		printf("\r\nSIGKILL\r\n");
		break;
	case SIGBUS:
		printf("\r\nSIGBUS \r\n");
		break;
	case SIGSEGV:
		printf("\r\nSIGSEGV\r\n");
		break;
	case SIGSYS:
		printf("\r\nSIGSYS \r\n");
		break;
	case SIGPIPE:
		printf("\r\nSIGPIPE\r\n");
		break;
	case SIGALRM:
		printf("\r\nSIGALRM\r\n");
		break;
	case SIGTERM:
		printf("\r\nSIGTERM\r\n");
		break;
	case SIGURG:
		printf("\r\nSIGURG\r\n");
		break;
	case SIGSTOP:
		printf("\r\nSIGSTOP\r\n");
		break;
	case SIGTSTP:
		printf("\r\nSIGTSTP\r\n");
		break;
	case SIGCONT:
		printf("\r\nSIGCONT\r\n");
		break;
	case SIGCHLD:
		printf("\r\nSIGCHLD\r\n");
		break;
	case SIGTTIN:
		printf("\r\nSIGTTIN\r\n");
		break;
	case SIGTTOU:
		printf("\r\nSIGTTOU\r\n");
		break;
	case SIGIO:
		printf("\r\nSIGIO\r\n");
		break;
	case SIGXCPU:
		printf("\r\nSIGXCPU\r\n");
		break;
	case SIGXFSZ:
		printf("\r\nSIGXFSZ\r\n");
		break;
	case SIGVTALRM:
		printf("\r\nSIGVTALRM \r\n");
		break;
	case SIGPROF:
		printf("\r\nSIGPROF \r\n");
		break;
	default: printf("\r\nSIG #%x\r\n",thesig);
		break;
	}
	fflush(stdout);
}
