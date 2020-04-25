/*
 *$Copyright: $
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

#ident "$Header: stack.c 1.14 1991/07/19 20:16:30 $"

/*
 * $Log: stack.c,v $
 *
 *
 */

#include "crash.h"

#include <string.h>
#ifdef BSD
#include <sdb.h>
#else
#include <a.out.h>
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dir.h> 
#ifdef _SEQUENT_
#include <sys/resource.h>
#include <signal.h>
#endif
#include <sys/user.h>
#ifdef _SEQUENT_
#include <sys/timer.h>
#endif
#include <sys/proc.h>
#include <sys/vm.h>
#ifdef BSD
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <machine/engine.h>
#include <machine/plocal.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/intctl.h>
#else
#include <sys/pte.h>
#include <sys/engine.h>
#include <sys/plocal.h>
#include <sys/reg.h>
#include <sys/trap.h>
#include <sys/intctl.h>
#endif

#ifndef VA_UBLOCK
#define VA_UBLOCK VA_UAREA
#endif

static	int	fp_length = 0;
static	int	*fp_tail = 0;
static	int	fp_offset = 0;
static	int	virt_sp = 0;
static	int	virt_sp_mask = (UPAGES*NBPG)-1;

static	int	*kfp = 0;
static	int	usekfp = 0;
static	int	disreg = 0;
static	int	on_line = 0;
extern	int	Last_eng;

static int *exception = 0;	/* next frame is an exception */
static int *interrupt = 0;	/* next frame is an interrupt */
extern int xdebug;

stack()
{
	register char *arg;
	register struct proc *p;
	int eng, n, didstack;

	eng = didstack = disreg = usekfp = on_line = 0;
	if ((arg=token()) == NULL) { /* "s" */
stackproc:
		if (live) {
			proc_init();
		}
		for (p = proc; p < procmax; p++) {
			fp_tail = 0;
			fp_length = 0;
			proctrace(p);
		}
		return;
	}
	while (arg != NULL) {
		/* display only online processes if "-o" */
		if (strcmp(arg, "-o") == 0) {
			on_line = 1;
			arg = token();
			continue;
		}
		/* display registers and fp if "-r" */
		if (strcmp(arg, "-r") == 0) {
			disreg = 1;
			arg = token();
			continue;
		}
		/* user selected kfp if "-f" */
		if (strcmp(arg, "-f") == 0) {
			usekfp = 1;
			arg = token();
			continue;
		}
		if (strcmp(arg, "-e") == 0) {
			eng = !eng;
			arg = token();
			if (arg == NULL)
				for (eng=0; eng < Nengine; eng++) {
					fp_tail = 0;
					fp_length = 0;
					didstack = 1;
					engtrace(eng);
				}
			continue;
		} else if (eng) {
			n = atoi(arg);
			if ( err_atoi ) {
				printf("'%s', %s\n", arg, err_atoi);
			} else {
				fp_tail = 0;
				fp_length = 0;
				didstack = 1;
				engtrace(n);
			}
			Last_eng = eng;
		} else {
			fp_tail = 0;
			fp_length = 0;
			didstack = 1;
			proctrace(getproc(arg));
		}
		arg = token();
	}
	if (!didstack && (disreg || usekfp || on_line))
		goto stackproc;
}

static
Dmpstk()
{
	register char *arg;

	if ((arg = token()) == NULL) {
		printf("dump what stack?\n");
		return;
	}

	dmpstk(getproc(arg));
}

static
engtrace(n)
{
	struct engine *e, *v;
	struct ppriv_pages ppriv_pages;	/* physical address of local stuff */
	register struct priv_map *pm;
	register i,j;

	if (n < 0  || n >= Nengine) {
		printf("engine %d? (0..%d are valid)\n", n, Nengine-1);
		return;
	}
	e = &l_engine[n];
	v = &v_engine[n];
	if (live) 
		readv(v, e, sizeof (struct engine));
	j = readp(e->e_local, &ppriv_pages, sizeof ppriv_pages);
	if (j != sizeof ppriv_pages) {
		printf("engtrace: readp error (%d != %d)\n", j, sizeof ppriv_pages);
		return;
	}
	pm = (struct priv_map *)ppriv_pages.pp_pmap;
	for (j=0; j < UPAGES; j++) {
		i = readp(PTETOPHYS(pm->pm_uarea[j]), (char *) ppriv_pages.pp_uarea[j], NBPG);
		if (i != NBPG) {
			printf("engtrace: readp error (%d != %d)\n", i, NBPG);
			return;
		}
	}
#if CRASH_DEBUG
	if (xdebug > 1) {
		for (j=0; j < (UPAGES*NBPG)/sizeof (int); j++) {
			if ((j & (8-1)) == 0 || j==0) printf("%s%08x", j ? "\n" : NULL, VA_UBLOCK + j * sizeof (int));
			printf(" %08x", ((int *)ppriv_pages.pp_uarea)[j]);
		}
		printf("\n");
	}
#endif
	printf("\n*** Engine %d ***\n", n);
	virt_sp = (int) PTETOPHYS(pm->pm_uarea[0]);
	utrace(ppriv_pages.pp_uarea, 0, 1);
}

static
proctrace(pp)
	struct proc *pp;	/* virtual address of proc entry */
{
	extern struct proc Last_proc;
	register char *ret;
	int	use_u_sp;

	if (pp == NULL)
		return;
	ret = getuarea(pp);
	if (on_line && (Last_proc.p_stat != SONPROC))
		return;
	switch ((int)ret) {
	case 0:
		return;
	case (int)BADREAD:
		printf("bad read of Uarea\n"); 
		return;
	case (int)SWAPPED:
		printf("process is swapped\n"); 
		return; 
	}
#if CRASH_DEBUG
	if (xdebug > 1) {
		register int j;

		for (j=0; j < (UPAGES*NBPG)/sizeof (int); j++) {
			if ((j & (8-1)) == 0 || j==0) printf("%s%08x", j ? "\n" : NULL, VA_UBLOCK + j * sizeof (int));
			printf(" %08x", ((int *)ret)[j]);
		}
		printf("\n");
	}
#endif
	/*
	 * If process anything but running on a processor, 
	 * find frame pointer from u.u_sp.
	 */

	printf("\n*** Proc slot %d ***\n", pp - proc);

	if (Last_proc.p_stat && Last_proc.p_stat != SONPROC)
		use_u_sp = 1;
	else
		use_u_sp = 0;
	virt_sp = (int)USERTOUB(Last_proc.p_uarea);
	utrace(ret, use_u_sp, 0);
}

static
dmpstk(pp)
	struct proc *pp;
{
	register char *ret;
	register int j;

	if (pp == NULL)
		return;

	ret = getuarea(pp);
	if (ret == (char *) -1) {
		printf("bad read of Uarea\n");
		return -1;
	}

	if (ret == (char *) SWAPPED) {
		printf("process is swapped\n");
		return -1;
	}

	printf("*** Proc slot %d stack dump ***\n", pp - proc);

	for (j=0; j < (UPAGES*NBPG)/sizeof (int); j++) {
		if ((j & (8-1)) == 0 || j==0) {
			printf("%s%08x", j ? "\n" : NULL, 
						VA_UAREA + j * sizeof (int));
		}
		printf(" %08x", ((int *)ret)[j]);
	}
	printf("\n");
}

static
utrace(ub, use_u_sp, idle)
	char *ub;			/* pointer to U-block */
	int	use_u_sp;		/* derive fp from u_sp ?? */
	int	idle;			/* running on idle stack */
{
	register  int *sp;
	register struct user *u;
	int	*guessfp;
	int	*top;
	int	*bot;
	int	*fp;

	u = UBTOUSER(ub);
	fp_offset = (VA_UBLOCK - (int)ub);
	sp = (int *)(u->u_sp - fp_offset);

	if (VA_UAREA == VA_UBLOCK) {
		top = (int *)(VA_UBLOCK + (UPAGES*NBPG) - fp_offset);
		bot = (int *)(VA_UAREA + sizeof(struct user) + 32 - fp_offset);
	} else {
		top = (int *)(VA_UAREA - fp_offset);
		bot = (int *)(VA_UBLOCK - fp_offset);
	}

	if (xdebug) {
		printf("(bottom %#x, top %#x)\n",
			(int)bot+fp_offset, (int)top+fp_offset-4);
	}

	if (usekfp && (kfp != (int *)0)) {
		guessfp = (int *)((int)kfp - (int)fp_offset);
	 } else if (usekfp && (kfp == 0)) {
		printf("bad kfp\n");
		return;
	} else if (use_u_sp) {
		/*
		 * Stack frame when process is sleeping or stopped is
		 * u_sp -> EBP, ret PC.  386/implementation specific.
		 */
		guessfp = sp;
	} else 
		guessfp = (int *) (scanfp(top, bot, idle) - fp_offset);

#if CRASH_DEBUG
	if (xdebug > 1) 
		printf(">> utrace: fp_offset %#x, sp %#x, fp %#x, guessfp %#x, top %#x, bot %#x\n", 
			fp_offset, sp, fp, guessfp, top, bot);
#endif

	/* bad u_sp and scanfp failed */
	if ((int)guessfp+fp_offset == 0 ||
	    (unsigned)guessfp > (unsigned)top ||
	    (unsigned)guessfp < (unsigned)bot) {
		printf("no reasonable trace possible\n");
		return;
	}
	exception = 0;
	printstacktrace(guessfp, "unknown", top, bot);
}

static
scanfp(top, bot, idle)
	int	*bot;
	int	*top;
	int	idle;
{
	/*
	 * ok lets use some heurestics for a good start.
	 * there 3 main types of calling mechanism
	 * interrupts faults and exceptions
	 * system calls from user mode will always be from user text priv 3
	 * and hence the first byte on the stack is 23 in this case
	 * when called from the idle stack if in debug mode then information
	 * from the previous stack may be available.
	 */
	if (idle) {
		if ((top[-2] == 0) && (top[-0x12] == 0xfffffff8))
			top -= 1;
		else
			top -= 0x11;	/* idle stack starts 0x40 byte down */
	}
	if (*(top-1) == USER_CS) {
		if (xdebug) 
			printf("*** onproc %s in a system call or trap\n",
							live?"is":"was");
		fpsearch(top-10, bot, 0);
		return (int) fp_tail;
	}
	fpsearch(top, bot, 0);
	return (int) fp_tail;
}

static
fpsearch(top, bot, depth)
	int *top, *bot;
{
	register int *i, *me;

	if (depth > (UPAGES*NBPG)/sizeof (int *)) {
		printf("fpsearch: recursion detected\n");
		return;
	}
	if (depth == 0) { /* top level */
		for (i=top-1; i >= top-48; i--) {
			me = (int *)(*i - fp_offset);
			if (me >= bot && me <= top)
				fpsearch(me, bot, depth+1);
		}
	} else {	/* looking for links */
		for (i=top-1; i >= bot; i--) {
			me = (int *)(*i - fp_offset);
			if (me == top) { /* found a link */
				if (depth > fp_length) {
					fp_length++;
					fp_tail = (int *)((int)i + fp_offset);
				}
				/* the divine recurse */
				fpsearch(i, bot, depth+1);
			}
		}
	}
}

/*
 * getregs()
 *	Look at at call frame for a given procedure, and determine
 *	how many (if any) register variables, and return their values.
 *
 * Assumes register order is %edi, %esi, %ebx.
 *
 * Returns # register variables, and values in regs[].
 */

/*
 * proc_entry[]:	Binary for "pushl %ebp; movl %esp, %ebp"
 * subl_byte_esp[]:	Binary for "subl $n, %esp" where n fits in a byte.
 * subl_long_esp[]:	Binary for "subl $n, %esp" where n fits in a long.
 * push_regs[]:		Binary for "pushl %edi; pushl %esi; pushl %ebx"
 */

static	char	*regname[] = { "edi", "esi", "ebx" };

static	unsigned char	proc_entry[] = { 0x55, 0x8b, 0xec };
static	unsigned char	subl_byte_esp[] = { 0x83, 0xec };
static	unsigned char	subl_long_esp[] = { 0x81, 0xec };
static	unsigned char	push_regs[] = { 0x57, 0x56, 0x53 };

static
getregs(sym, fp, regs)
	char	*sym;		/* entry symbol */
	int	*fp;		/* adjusted frame pointer for "sym" call */
	int	*regs;		/* array of registers to fill out */
{
	register unsigned char *ip;
	register int	i;
	unsigned int	val;
	unsigned char	instr[12];

	if (!lookbysym(sym, &val, N_GSYM) ||
		readv(val, instr, sizeof(instr)) != sizeof(instr))
		return(0);

	/*
	 * If not standard procedure entry, assume no register variables.
	 */

	if (memcmp(instr, proc_entry, sizeof(proc_entry)) != 0)
		return(0);

	/*
	 * Skip a "subl $n, %esp" instruction (allocates locals before
	 * push registers), if one exists.
	 */

	ip = &instr[sizeof(proc_entry)];
	if (memcmp(ip, subl_byte_esp, sizeof(subl_byte_esp)) == 0) {
		ip += sizeof(subl_byte_esp);	/* skip to immediate */
		fp -= (*ip / 4);		/* adjust fp from immediate */
		ip += 1;			/* skip single byte immediate */
	} else if (memcmp(ip, subl_long_esp, sizeof(subl_long_esp)) == 0) {
		ip += sizeof(subl_long_esp);	/* skip to immediate */
		fp -= (*(long *)ip / 4);	/* adjust fp from immediate */
		ip += 4;			/* skip 4-byte immediate */
	}

	/*
	 * The next bytes of code push the registers (if any).
	 */

	for (i = 0; i < sizeof(push_regs); i++) {
		if (*ip++ != push_regs[i])
			break;
		regs[i] = *--fp;
	}

	return(i);
}

/*
 * printstacktrace()
 *	Print a stack trace, optionally with register variables.
 *
 * This could be augmented with more smarts about syscalls and locore
 * entries.
 */

static
printstacktrace(fp, whence, top, bot)
	register int *fp;		/* frame pointer of current frame */
	char	*whence;		/* from where we was called */
	int	*top, *bot;		/* range of stack values */
{
	int *newfp; 			/* frame pointer of next frame */
	int *oldpc; 			/* pc this frame returns to */
	int *oldap; 			/* point to first argument */
	int  nargs;			/* number of arguments passed */
	int prev_instr;
	char buft[100];
	unsigned char offset[sizeof(int)];
	unsigned int i;
	int	col;
	int locore;
	int *eax;
	char *p;
	int argindex;

	/*
	 * Check if the newframepointer is valid if so then
	 * we will only access the old pc and display accordingly.
	 */
	if ((unsigned)fp <= (unsigned)top && (unsigned)fp >= (unsigned)bot) {
		newfp = (int *)(fp[0]);	      /* frame pointer of next frame */
		oldpc = (int *)(fp[1]);	      /* pc this frame returns to */
		oldap = (int *)(&fp[2]);      /* point to first argument */
	} else {
		newfp = 0;
		oldpc = 0;
	}
again:
	locore = 0;
	nargs = 0;
	argindex = 0;
	/*
	 * Get the instruction following call (at ret pc).
	 */

	readv(oldpc, &prev_instr, sizeof prev_instr);

	/*
	 * determine number of arguments from:
	 *	stack pointer subtract instruction 
	 *	or pop into register
	 */

#define POP_ECX           0x59  /* opcode for 386 'popl %ecx' instruction */
#define ADDLIMM8        0xc483  /* opcode for 386 'addl $imm8' instruction */
#define ADDLIMM32       0xc481  /* opcode for 386 'addl $imm32' instruction */

	if ((int)oldpc < Etext) {
		if ((prev_instr & 0xff) == POP_ECX) {
			nargs = 1;
        	} else if ((prev_instr & 0xffff) == ADDLIMM8) {
			nargs = ((prev_instr >> 16) & 0xff) / 4;
		} else if ((prev_instr & 0xffff) == ADDLIMM32) {
			readv(oldpc+2, offset, sizeof(offset));
			nargs = (offset[0] >> 24) | (offset[1] >>  8) | 
			        (offset[2] <<  8) | (offset[3] << 24);
		}
	}

	if (nargs > 6)		  /* sanity */
		nargs = 6;

	/*
	 * Figure procedure we're in from data below on the stack (eg, whence
	 * argument), and set up `whence' for next iteration (from "oldpc").
	 * Print arguments.
	 */

	strcpy(buft, whence);
	if (strrchr(buft, '+'))
		*(char *)strrchr(buft, '+') = '\0';
	/*
	 * adjust for BSD pcc.
	 */
	p = buft;
	if (*p == '_')
		p++;
	/*
	 * Fix disable watchpoint code entry points for ptx.
	 */
	if (*p == 'w') {
		if (strncmp(p, "wprc_", 5) == 0)
			p += 5;
		if (strncmp(p, "wpd_", 4) == 0)
			p += 4;
		if (strncmp(p, "wpdc_", 5) == 0)
			p += 5;
		if (strncmp(p, "wpsti_", 6) == 0)
			p += 6;
		if (strncmp(p, "wpr_", 4) == 0)
			p += 4;
		if (strncmp(p, "wpdk_", 5) == 0)
			p += 5;
	}
	if (xdebug) {
		printf("[fp=%8.8x(%8.8x)] ", (int)fp+fp_offset,
		      (((int)fp + fp_offset & virt_sp_mask)+ virt_sp ));
	}
	printf("%s(", buft);

	/*
	 * Now fix the arguments for "called from assembler" routines.
	 * Also set locore to disable the "from ..." being printed.
	 */
#ifdef BSD
	if (strcmp(p, "trap") == 0) {
		nargs = 14;
	} else if (strcmp(p, "syscall") == 0) {
		nargs = 13;
	} else if (strcmp(p, "dev_common") == 0) {
		nargs = 0;
		locore = 1;
	} else if (strcmp(p, "bin0int") == 0) {
		nargs = 0;
		locore = 1;
	} else if (strcmp(p, "trap_common") == 0) {
		nargs=0;
		locore = 1;
	} else if (strcmp(p, "pagein") == 0) {
		nargs = 1;
	} else if (strncmp(p, "t_pgflt", 7) == 0) {
		nargs=0;
		locore = 1;
	}
#else
	if (strcmp(p, "trap") == 0) {
		nargs = 14;
	} else if (strcmp(p, "syscall") == 0) {
		nargs = 13;
	} else if (strcmp(p, "dev_common") == 0) {
		nargs = 0;
		locore = 1;
	} else if (strcmp(p, "bin0int") == 0) {
		nargs = 0;
		locore = 1;
	} else if (strcmp(p, "trap_common") == 0) {
		nargs=0;
		locore = 1;
	} else if (strcmp(p, "pagein") == 0) {
		nargs = 1 ;
	} else if (strncmp(p, "t_pgflt", 7) == 0) {
		nargs=0;
		locore = 1;
	}
#endif
	argindex = search_arg(p);
	col = strlen(buft) +1;
	/*
	 * Now print the arguments from the stack.
	 */
	while (nargs) {
		if (argindex) {
			argindex = printarg(argindex, &col);
		}
		if (col > 80-12) {
			printf("\n  ");
			col=2;
		}
		i = *oldap++;
		if (i < 9) {
			printf("%x", i);
			col += 1;
		} else if (i < 0x10) {
			printf("0x%x", i);
			col += 3;
		} else if (i < 0x100) {
			printf("0x%2.2x", i);
			col += 4;
		} else if (i < 0x10000) {
			printf("0x%4.4x", i);
			col += 6;
		} else {
			printf("0x%8.8x", i);
			col += 10;
		}
		
		if( --nargs )
			printf(", ");
		col += 2;
	}
	printf(") ");
	col += 2;
	if ((long)oldpc > Etext) {
		if (col > 80-24) {
			printf("\n  ");
		}
		if (fp != top) {
			printf("from unknown (0x%8.8x)", oldpc);
			whence = "unknown";
		} else {
			printf("\n");
			return;
		}
	} else {
		whence = addr_str(oldpc);
		if (col + strlen(whence) > 80-6) {
			printf("\n  ");
		}
		if ((oldpc != 0) && (!locore)) {
			printf("from %s", whence);
		}
	}

#ifdef BSD
	/*
	 * If in a "panic" frame, dump registers at time of panic.
	 * Else if looking at registers, dump them from current frame.
	 */

	if (strcmp(p, "panic") == 0) {
		/*
		 * If panic() entry changes, so must this.
		 */
		eax = fp - 8;
		printf("\n\t[eax=%8.8x ebx=%8.8x ecx=%8.8x edx=%8.8x \n\t esi=%8.8x edi=%8.8x] fp=%8.8x",
				eax[EAX], eax[EBX], eax[ECX],
				eax[EDX], eax[ESI], eax[EDI],
				(int)fp + fp_offset);
	}
#else
	/*
	 * If in a "cmn_err" frame, dump registers at time of panic.
	 * Else if looking at registers, dump them from current frame.
	 */

	if (strcmp(p, "cmn_err") == 0) {
		/*
		 * If cmn_err() entry changes, so must this.
		 */
	
		int	e;
		e = entry_args();
		eax = fp - e;
		printf("\n\t[eax=%8.8x ebx=%8.8x ecx=%8.8x edx=%8.8x\n\t esi=%8.8x edi=%8.8x] fp=%8.8x\n",
				eax[EAX], eax[EBX], eax[ECX],
				eax[EDX], eax[ESI], eax[EDI],
				(int)fp + fp_offset);
	}
#endif
	  else if (disreg) {
		int	regs[3];
		int	nregs;
		if (nregs = getregs(buft, fp, regs)) {
			printf("\t[");
			for (i = 0; i < nregs; i++) {
				if (i != 0) printf(" ");
				printf("%s=%#x", regname[i], regs[i]);
			}
			printf("]");
		}
		printf(" fp=%#x",(unsigned int)fp+fp_offset);
	}

	/*
	 * set "buft" (and "p") to be the "called from" address.
	 */
	strcpy(buft, whence);
	p = buft;
	if (*p == '_')
		p++;
	/*
	 * Fix disable watchpoint code entry points.
	 */
	if (*p == 'w') {
		if (strncmp(p, "wprc_", 5) == 0)
			p += 5;
		if (strncmp(p, "wpd_", 4) == 0)
			p += 4;
		if (strncmp(p, "wpdc_", 5) == 0)
			p += 5;
		if (strncmp(p, "wpsti_", 6) == 0)
			p += 6;
		if (strncmp(p, "wpr_", 4) == 0)
			p += 4;
		if (strncmp(p, "wpdk_", 5) == 0)
			p += 5;
	}

	/*
	 * Were we called from an exception handler?
	 */

	if (exception) {	
		printf("\n <<exception (");
		/*
		 * test for pagein code which does not have an exception
		 * frame or an interrupt frame if pagein() was called.
		 */
		if (interrupt) {
			exception = 0;
			if ((interrupt[INTR_SP_CS] & RPL_MASK) == USER_RPL) {
				printf("T_USER+T_PGFLT) %%esp = 0x%x>> ", 
								interrupt[7]);
				pr_userpc(top, interrupt[4]);
				return;
			} else {
				whence = addr_str(interrupt[4]);
				printf("T_PGFLT) %%esp = 0x%x (0x%x)", 
					(int)interrupt + fp_offset,
					(((int)interrupt + fp_offset & 
						virt_sp_mask) + virt_sp));
				printf(">> from %s", whence);
			}
		} else {
			/*
			 * Standard call to trap().
			 */
			trap_type(*exception++);
			if ((exception[SP_CS] & RPL_MASK) == USER_RPL) {
				printf(")>> ");
				pr_userpc(top, exception[SP_EIP]);
				return;
			} else {
				whence = addr_str(exception[SP_EIP]);
				printf(") %%esp = 0x%x (0x%x) ", 
					(int)exception + fp_offset,
					(((int)exception + fp_offset & 
					   virt_sp_mask) + virt_sp));
				printf(">> from %s", whence);
			}
		}
		printf("\n");
		exception = 0;
		interrupt = 0;
		goto again;
	} else if (interrupt) {
		/*
		 * true device interrupts. 
		 */
		printf("\n <<interrupt #%d ", interrupt[0]);
		interrupt++;
		printf("spl%d->spl%d ",
				spl_decode(interrupt[0] & 0xff),
				spl_decode(interrupt[0]>>8 & 0xff));
		if ((interrupt[INTR_SP_CS] & RPL_MASK) == USER_RPL) {
			printf("%%esp = 0x%x>> \n", interrupt[7]);
			pr_userpc(top, interrupt[4]);
			return;
		} else {
			whence = addr_str(interrupt[4]);
			printf("%%esp = 0x%x (0x%x)", 
				(int)interrupt + fp_offset,
				(((int)interrupt + fp_offset & virt_sp_mask) +
				 virt_sp));
			printf(">> from %s", whence);
		}
		printf("\n");
		interrupt = 0;
		goto again;
	} else {
		/*
		 * Get the new frame pointer.
		 */
		newfp = (int *)((int)newfp - fp_offset);
	}
	printf("\n");

	/*
	 * now check if we were called from an exception or interrupt
	 * handler. If so set up "interrupt" or "exception" for next 
	 * call to printstack.
	 */
#ifdef BSD
	if (strncmp(p, "dev_common", 10) == 0) {
		interrupt = fp + 2;
	} else if (strncmp(p, "bin0int", 7) == 0) {
		interrupt = fp + 2 ;
	} else if (strncmp(p, "trap_common", 11) == 0) {
		exception = fp + 2;
	} else if (strncmp(p, "t_pgflt", 7) == 0) {
		exception = fp + 2;
		interrupt = exception;
	}
#else
	if (strncmp(p, "dev_common", 10) == 0) {
		interrupt = fp + 2;
	} else if (strncmp(p, "bin0int", 7) == 0) {
		interrupt = fp + 2 ;
	} else if (strncmp(p, "trap_common", 11) == 0) {
		exception = fp + 2;
	} else if (strncmp(p, "t_pgflt", 7) == 0) {
		exception = fp + 2;
		interrupt = exception;
	}
#endif
	/*
	 * Was this a standard user to kernel exception?.
	 * If so we are done.
	 */
	if ((int)fp == ((int)top - 15*4)) {
		pr_userpc(top, fp[10]);
		return;
	}

	/*
	 * Check this was not the end of the chain of frame pointers.
	 */
	if ((unsigned)fp >= (unsigned)top || (unsigned)fp <= (unsigned)bot)
		return;

	if (((int)top - (int)fp) < 8*4) {
		printf("unable to continue  [fp=%#x]\n", (int)fp+fp_offset);
		return;
	}

	/*
	 * Print the next stack frame.
	 */
	printstacktrace(newfp, buft, top, bot);
}

/*
 * Print the command and user pc.
 */
static
pr_userpc(top, pc)
	int *top;
	int  pc;
{
	if (VA_UAREA == VA_UBLOCK) {
		printf("from \"%s\", user pc %#x\n",
		      user_cmd((struct user *)((int)top-(UPAGES*NBPG))), pc);
	} else {
		printf("from \"%s\", user pc %#x\n",
			user_cmd((struct user *)top), pc);
	}
}


Kfp()
{
	register char *arg;

	if ((arg = token()) == NULL)
		printf("kfp = %08#x\n", kfp);
	else 
		kfp = (int *)atoi(arg);
}

get_locore()
{
	extern int end_locore, start_locore;

#ifdef BSD
	start_locore = search("_bin1int");
#else
	start_locore = search("bin1int");
#endif
	end_locore = search("sys_copy_err");
	if (end_locore == 0 || start_locore == 0)
		printf("Warning: locore symbols out of date\n");
}


/*
 * Perform vitual to physical emulating the hardware.
 */
_vtop(vaddr)
{
	struct engine *e, *v;
	struct pte	kl1pte;
	struct pte	kl2pte;
	int	kl1_ptr;
	int	kl2_ptr;
	struct ppriv_pages ppriv_pages;	/* physical address of local stuff */
	int	segment;
	int	dir;
	int	page;
	int	offset;
	int	j;

	if (Nengine == 0)
		return(vaddr);

	segment = KSOFF;
	offset = (vaddr & (NBPG-1));
	page   = L2IDX(vaddr);
	dir    = L1IDX(vaddr);
	if (xdebug) {
		printf("\nvaddr: 0x%x engine %d\n", vaddr, Last_eng);
		printf(" segment:0x%x(0x%x) dir:0x%x page:0x%x offset:0x%x\n",
			segment, L1IDX(segment), dir, page, offset);
	}
	e = &l_engine[Last_eng];
	v = &v_engine[Last_eng];
	if (live) 
		readv(v, e, sizeof (struct engine));
	j = readp(e->e_local, &ppriv_pages, sizeof ppriv_pages);
	if (j != sizeof ppriv_pages) {
		printf("_vtop: readp error (%d != %d)\n", j, 
					sizeof ppriv_pages);
		return(0);
	}
	dir = (dir+L1IDX(segment)) & 0x3ff;
	kl1pte = ((struct pte *)ppriv_pages.pp_kl1pt)[dir];
	if (xdebug)
		printf(" k1lpte = pp_k1lpt[0x%x] =  0x%x\n", dir, kl1pte);
	if (kl1pte.pg_v == 0) {
		printf("_vtop: kernel level 1 page not valid\n");
		return(0);
	}

	kl1_ptr = PTETOPHYS(kl1pte)+(page * sizeof(struct pte));
	if (xdebug)
		printf(" k1l_ptr = 0x%x\n", kl1_ptr);
	j = readp(kl1_ptr , &kl2pte, sizeof kl2pte);
	if (j != sizeof kl2pte) {
		printf("_vtop: readp error (%d != %d)\n", j, 
					sizeof kl2pte);
		return(0);
	}

	if (xdebug)
		printf(" k12pte = 0x%x\n", kl2pte);
	if (kl2pte.pg_v == 0) {
		printf("_vtop: kernel level 2 page not valid\n");
		return(0);
	}
	kl2_ptr = PTETOPHYS(kl2pte);
	if (xdebug)
		printf(" k12_ptr = 0x%x\n", kl2_ptr);

	if (xdebug)
		printf("paddr: 0x%x\n", kl2_ptr+offset);
	return (kl2_ptr + offset);
}


trap_type(t)
{
	if (t & T_USER)
		printf("T_USER+");
 	t &= ~T_USER;

	switch (t) {
	case T_DBG:	printf("T_DBG"); break;
	case T_NMI:	printf("T_NMI"); break;
	case T_INT3:	printf("T_INT3"); break;
	case T_INTO:	printf("T_INTO"); break;
	case T_CHECK:	printf("T_CHECK"); break;
	case T_UND:	printf("T_UND"); break;
	case T_DNA:	printf("T_DNA"); break;
	case T_SYSERR:	printf("T_SYSERR"); break;
	case T_RES:	printf("T_RES"); break;
	case T_BADTSS:	printf("T_BADTSS"); break;
	case T_NOTPRES:	printf("T_NOTPRES"); break;
	case T_STKFLT:	printf("T_STKFLT"); break;
	case T_GPFLT:	printf("T_GPFLT"); break;
	case T_PGFLT:	printf("T_PGFLT"); break;
	case T_COPERR:	printf("T_COPERR"); break;
	case T_SWTCH:	printf("T_SWTCH"); break;
	default:	printf("0x%x");break;
	}
}

spl_decode(s)
{
	switch (s) {
	case SPL0: return  0;
	case SPL1: return  1;
	case SPL2: return  2;
	case SPL3: return  3;
	case SPL4: return  4;
	case SPL5: return  5;
	case SPL6: return  6;
	case SPL7: return  7;
	}
	return(-1);
}
#ifdef _SEQUENT_
/*
 * entry_args -- calculate entry args for cmn_err.
 *
 * This routine computes the value that must be subtracted from the frame
 * pointer to find out where the "pushal" instruction placed all the registers.
 * It returns the number of WORDS so frame pointer must be adjusted by 4 times
 * the return value.
 *
 * Expected instruction sequence must not be changed, but the exact value
 * used for local argument storage may change.  It is because this value
 * for local storage changes that the value cannot be hard coded.
 */

static
entry_args()
{
	char *m;
	unsigned addr;
	int length;
	int i, n;

	if (!lookbysym("cmn_err", &addr, N_GSYM)) {
		printf("\nUnable to locate routine cmn_err\n");
err:
		printf("\nWARNING: entry_args failed to calculate register location\n");
		printf("Using constant value to locate stored registers.\n");
		printf("Register values may be incorrect.\n");
		return(14);
	}

	/* expect to see "pushl  %ebp" as first instruction. */

	m = (char *) dis386(addr, &length);
	if (length == 0)
		goto err;
	addr += length;
	if (strncmp(m, "pushl", 5) != 0) {
		printf("\nDid not find \"pushl\" as first instruction of cmn_err.\n");
		goto err;
	}

	/* expect to see "movl  %esp, %ebp" */

	m = (char *) dis386(addr, &length);
	if (length == 0)
		goto err;
	addr += length;
	if (strncmp(m, "movl", 4) != 0) {
		printf("\nDid not find \"movl\" as second instruction of cmn_err.\n");
		goto err;
	}

	/*
	 * Now expect to see "subl  $0x28,%esp" but the value may not
	 * be 0x28.  We want to pick up this value.  It is some number
	 * of bytes, which we expect to be divisible by 4.  We want to
	 * divide by 4 to calculate the number of words.
	 */

	m = (char *) dis386(addr, &length);
	if (length == 0)
		goto err;
	addr += length;
	if (strncmp(m, "subl", 4) != 0) {
		printf("\nDid not find \"movl\" as third instruction of cmn_err.\n");
		goto err;
	}

	if ( (sscanf(m, "%*[^$]$0x%x,%%esp", &i) != 1) &&	
	     (sscanf(m, "%*[^$]$x%x,%%esp", &i) != 1)    ) {
		printf("\nCould not decode stack adjust value in cmn_err.\n");
		goto err;
	}
	
	n = i / 4;
	if ((n * 4) != i) {
		printf("Stack adjust value in cmn_err not multiple of 4.\n");
		goto err;
	}

	/*
	 * Now increment return value for each register pushed on stack
	 * with "pushl" instruction.
	 */

	for (;;) {
		m = (char *) dis386(addr, &length);
		if (strncmp(m, "pushal", 6) == 0) {
			return(n + 1);
		} else if (strncmp(m, "pushl", 5) == 0) {
			if (length == 0)
				goto err;
			n++;
			addr += length;
		} else {
			printf("Unable to locate \"pushal\" instruction in cmn_err.\n");
			goto err;
		}
	}
}
#endif
