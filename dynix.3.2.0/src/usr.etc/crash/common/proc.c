/* $Header: proc.c 2.6 1991/07/01 16:21:12 $ */

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

#ifndef lint
static char rcsid[] = "$Header: proc.c 2.6 1991/07/01 16:21:12 $";
#endif

/*
 * $Log: proc.c,v $
 *
 *
 */

#include "crash.h"

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/dir.h>
#ifdef BSD
#include "sys/kernel.h"
#else
#include <sys/resource.h>
#include <sys/signal.h>
#endif
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#ifdef _SEQUENT_
#include <sys/session.h>
#endif

struct proc *proc, *procmax, *procNPROC;

proc_init()
{
	readv(search("proc"), &proc, sizeof proc);
	readv(search("procNPROC"), &procNPROC, sizeof procNPROC);
#ifdef BSD
	readv(search("procmax"), &procmax, sizeof procmax);
#else
	procmax = v.ve_proc;
#endif
	if (debug[16]) {
		printf("proc_init()\n");
		printf("  proc = 0x%x, size is %d bytes\n", proc, sizeof proc);
		printf("  procmax = b.ve_proc = 0x%x\n", procmax);
		printf("  procNPROC = 0x%x\n", procNPROC);
	}
}

Proc()
{
	struct proc *p;
	int l=0, r=0, n;
	int i = 0;
	char *arg;

	if (live || debug[16])
		proc_init();

	printf(" SLT ST ENG  PID  PPID  PGRP");
#ifdef BSD
	printf("   UID PRI CPU      EVENT NAME      FLAGS\n");
#else
	printf("   SID   UID PRI CPU      EVENT NAME      FLAGS\n");
#endif
	arg = token();
	if( (arg) && (*arg == '-') ) { 
		if( arg[1] == 'r' ) 
			r = 1;
		else if( arg[1] == 'o' ) 
			r = 2;
		else
			l = 1;
		arg = token();
	}
	if(arg == NULL) {
		if (debug[16])
			printf("Proc(): proc = 0x%x, procmax = 0x%x\n", proc, procmax);
		for(p = proc; p < procmax; p++) {
			if (debug[16])
				printf("Proc(): slot %d, p = 0x%x\n", i++, p);
			prproc(p, l, r, 0);
		}
	} else {
		do {
			if( p = getproc(arg) )
				prproc(p, l, r, 1);
		} while((arg = token()) != NULL);
	}
}

prproc(pp, md, run, all)
register struct proc *pp;
int md, run, all;
{
	register struct proc *p;
	register int	i;
	register char	*cp, ch;

	if( (p = readproc(pp)) == NULL )
		return;
	if(!all && p->p_stat == NULL)
		return;

	switch(run)  {
	case 0:
		break;
	case 1:
		if( (p->p_stat != SRUN) && (p->p_stat != SONPROC) )
			return;
		break;
	case 2:
		if( p->p_stat != SONPROC)
			return;
		break;
	}
	
	switch(p->p_stat) {
		case NULL:	ch = ' '; break;
		case SSLEEP:	ch = 'S'; break;
		case SWAIT:	ch = 'W'; break;
		case SRUN:	ch = 'R'; break;
		case SIDL:	ch = 'I'; break;
		case SZOMB:	ch = 'Z'; break;
		case SSTOP:	ch = 'T'; break;
		case SONPROC:	ch = 'O'; break;
		default:	ch = '?'; break;
	}
	printf("%4d %c%c %2d", pp-proc, ch, p->p_flag & SLOAD ? ' ' : 'W' , p->p_engno);
	printf(" %5u %5u %5u ", p->p_pid, p->p_ppid, p->p_pgrp);

#ifdef _SEQUENT_
	printf("%5u", p->p_sid);
#endif
	printf("%5u %3u %3u",
		p->p_uid, p->p_pri & 0377, p->p_cpu & 0377);
	if (p->p_wchan == 0)
		printf("           ");
	else  {
		cp = addr_str(p->p_wchan);
		if (cp[0] == '_') ++cp;
		if (strncmp("0x", cp, 2) == 0)
			printf(" %#010x", p->p_wchan);
		else
			printf("%11.10s", cp);
	}
	cp = "       ";
	if (p->p_stat == NULL)
		cp = "UNUSED";
	else if ((p->p_flag & SLOAD) || live) {
		char *ub, *getusera();
		struct user *u;
		if ( (ub = getuarea(pp)) == BADREAD ) {
			cp = "read err on uarea";
		} else {
#ifdef UBTOUSER
			u = UBTOUSER(ub);
#else
			u = (struct user *)ub;
#endif
			cp = u->u_comm;
			for(i = 0; i < 8 && cp[i]; i++) {
				if(cp[i] < 040 || cp[i] > 0176) {
					cp = "[unknown]";
					break;
				}
			}
		}
	}
#ifdef BSD
	if (p->p_pid == 0)
		cp = "swapper";
	if (p->p_pid == 1)
		cp = "init";
	if (p->p_pid == 2)
		cp = "pagedaemon";
#endif
	else if(p->p_stat == NULL)
		cp = "";
	else if(p->p_stat == SZOMB)
		cp = "ZOMBIE";
	printf(" %-8.8s", cp);
#ifdef	notdef
	if( p->p_flag & SSYS ) printf("S");
	if( p->p_flag & STRC ) printf("T");
	if( p->p_flag & SWTED ) printf("E");
	if( p->p_flag & SWTOK ) printf("O");
	if( p->p_flag & SOMASK ) printf("A");
	if( p->p_flag & SVFORK ) printf("V");
	if( p->p_flag & SNOVM ) printf("N");
	if( p->p_flag & STIMO ) printf("M");
	if( p->p_flag & SOUSIG ) printf("G");
	if( p->p_flag & SSEL ) printf("L");
	if( p->p_flag & SIGWOKE ) printf("W");
	if( p->p_flag & SWPSYNC ) printf("Y");
	if( p->p_flag & SFSWAP ) printf("F");
#ifdef STRCSTP
	if( p->p_flag & SNOSWAP ) printf("P");
	if( p->p_flag & SNOAGE ) printf("P");
	if( p->p_flag & SMPDBGR ) printf("");
	if( p->p_flag & SMPTRC ) printf("");
	if( p->p_flag & SMPSTOP ) printf("");
	if( p->p_flag & SMPWTED ) printf("");
	if( p->p_flag & STRCSTP ) printf("");
#endif
#endif
	printf(" %6x\n", p->p_flag);
	if(md == 0)
		return;
	if(p->p_stat == SZOMB) {
#ifdef	WANT
		printf("     exit: %1.1o  user time: %ld  sys time: %ld\n",
		xp->xp_xstat, xp->xp_utime, xp->xp_stime);
#endif
		printf("\n");
		return;
	}
	printf("    sz(d+s):%u tm:%u nice:%d alm:%u uarea:%#06x proc:%#06x\n",
		p->p_dsize + p->p_ssize, 
		p->p_time, p->p_nice, p->p_realtimer.it_interval.tv_sec, p->p_uarea,pp);
	printf("    sig:%8.8x sigmask:%8.8x sigcatch:%8.8x sigign:%8.8x cursig:%d\n",
		p->p_sig, p->p_sigmask, p->p_sigcatch, p->p_sigignore, p->p_cursig);
#ifdef	WANT
	printf(" q:%3d" , ((unsigned)p->p_link -
		Proc->n_value) / sizeof(struct proc));
	if( p->p_textp )
		printf(" tx:%3d", ((unsigned)p->p_textp - Text->n_value) /
			sizeof(struct text))
#endif
	printf("\n");
}

struct proc *
getproc(arg)
	char *arg;
{
	int n = atoi(arg);
	if ( err_atoi ) {
		printf("'%s', %s\n", arg, err_atoi);
		return NULL;
	}
	if( n > (procNPROC - proc) ) {
		printf("%d out of range\n", n);
		return NULL;
	}
	return proc+n;
}

struct proc *
readproc(pp)
struct proc *pp;
{
	static struct proc procbuf;

	if(readv(pp, &procbuf, sizeof procbuf) != sizeof procbuf) {
		printf("%3d  read error on proc table\n", pp-proc);
		return NULL;
	}
	return &procbuf;
}
