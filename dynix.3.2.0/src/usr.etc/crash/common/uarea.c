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
static char rcsid[] = "$Header: uarea.c 2.12 1991/07/17 02:54:11 $";
#endif

/*
 * $Log: uarea.c,v $
 *
 *
 *
 *
 *
 */

#include "crash.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dir.h> 

#ifdef BSD
#define KERNEL 1
#include	<sys/file.h>
#else
#define INKERNEL
#include	<sys/file.h>
#undef INKERNEL
#endif

#ifdef _SEQUENT_
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/timer.h>
#endif
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#ifdef BSD
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <machine/plocal.h>
#else
#include <sys/pte.h>
#include <sys/plocal.h>
#include <sys/sysmacros.h>
#endif

static char *U;	/* last u address */

Uarea()
{
	char *arg;
	struct proc *p;

	arg = token();

	if ( arg == NULL ) {
		for(p = proc; p < procmax; p++)
			pruarea(p);
	} else do {
		if( p = getproc(arg) )
			pruarea(p);
	} while((arg = token()) != NULL);
}

pruarea(pp)
	struct proc *pp;	/* virtual address of proc entry */
{
	register  int  i;
	char	*u;
	register struct user *USR;
	struct proc *p;
#ifdef OFILE_NOFILE
	struct ofile_tab      ofile_tab;
#endif
	struct ofile         *ofp;
	int		     nofile;
	struct	file	     file;

	if ((u = getuarea(pp)) == BADREAD) {
		printf("bad read of uarea\n");
		return(-1);
	}
	if (u == SWAPPED) {
		printf("process is swapped\n");
		return(-1);
	}
	if (u == NULL)
		return(-1);

#ifdef UBTOUSER
	USR = UBTOUSER(u);
#else
	USR = (struct user *)u;
#endif
	printf("PER PROCESS USER AREA: 0x%x\n", U);
	printf("USER ID's:\t");
	printf("uid: %u, gid: %u, real uid: %u, real gid: %u\n",
		USR->u_uid, USR->u_gid, USR->u_ruid, USR->u_rgid);
	printf("GROUPS:\t");
#ifdef BSD
	for (i = 0; USR->u_groups[i] >= 0; i++)
		printf(" %d", USR->u_groups[i]);
#else
	for (i = 0; USR->u_groups[i] != INVAL_GID; i++)
		printf(" %d", USR->u_groups[i]);
#endif
	printf("\n");
	printf("CRED ref count = %d\n", USR->u_cred->cr_ref);
#ifdef WANT
	printf("PROCESS TIMES:\t");
	printf("user: %ld, sys: %ld, child user: %ld, child sys: %ld\n",
		USR->u_utime, USR->u_stime, USR->u_cutime, USR->u_cstime);
#endif
	printf("PROCESS MISC:\t");
	printf("proc slot: %d", USR->u_procp - proc );
#ifdef BSD
	if(USR->u_ttyd != 0)
		printf(", cntrl tty: maj(%.2d) min(%2.2d)",
			major(USR->u_ttyd), minor(USR->u_ttyd));
	else
		printf(", cntrl tty: maj(%s) min(%s)", "??","??");
#endif
	printf(", u_sp: %#06x\n", USR->u_sp);
#ifdef	WANT
	printf("IPC:\t\t");
#endif

#ifdef	WANT
	printf("locks:%s%s%s%s",
		((USR->u_lock & UNLOCK) == UNLOCK) ? " unlocked" : "",
		((USR->u_lock & PROCLOCK) == PROCLOCK) ? " proc" : "",
		((USR->u_lock & TXTLOCK) == TXTLOCK) ? " text" : "",
		((USR->u_lock & DATLOCK) == DATLOCK) ? " data" : "");
#endif
	printf("FILE I/O:\tfile offset: %ld, bytes: %u\n", USR->u_offset, USR->u_count);
#ifdef WANT
	printf("\t\tsegment: %s,", USR->u_segflg == 0 ? "data" :
		(USR->u_segflg == 1 ? "sys" : "text"));
	printf(" umask: %#03o, ulimit: %ld\n", USR->u_cmask, USR->u_limit);
	printf("ACCOUNTING:\t");
	printf("command: %.14s, memory: %ld, type: %s%s%s%s\n\t\tstart: %s",
		procslot ? USR->u_comm : "swapper",
		USR->u_mem, USR->u_acflag & AFORK ? "fork" : "exec",
		USR->u_acflag & ASU ? " su-user" : "",
		USR->u_exdata.ux_mag == VAXWRMAGIC ? " lpd" : "",
		USR->u_exdata.ux_mag == VAXROMAGIC ? " rd/only-txt" : "",

		ctime(&USR->u_start.tv_sec));
#endif
#ifdef OFILE_NOFILE
	if( readv(USR->u_ofile_tab, &ofile_tab, sizeof (struct ofile_tab)) != sizeof (struct ofile_tab) ) 
		return;
	if ( (nofile = ofile_tab.oft_nofile) <= 0 ) {
		printf("\n");
		return;
	}
	i = sizeof(struct ofile) * nofile;
	ofp = (struct ofile *)malloc( i );
	if ( ofp == NULL)
		return;
	if( readv(ofile_tab.oft_ofile, ofp, i) != i) {
		free( ofp );
		printf("\n");
		return;
	}
	if (ofile_tab.oft_lastfile < 0) {
		free( ofp );
		printf("\n");
		return;
	}
	printf("OPEN FILES: %d(%d) @ 0x%x", nofile, 
					    ofile_tab.oft_lastfile,
					    ofile_tab.oft_ofile);
	if (ofile_tab.oft_refcnt > 1)
		printf(" shared by %d procs", ofile_tab.oft_refcnt);
	printf("\n");
#else

	if ( (nofile = USR->u_nofile) == 0 || USR->u_lastfile < 0) {
		printf("\n");
		return;
	}
	i = sizeof(struct ofile) * nofile;
	ofp = (struct ofile *)malloc( i );
	if ( ofp == NULL)
		return;
	if( readv(USR->u_ofile, ofp, i) != i) {
		free( ofp );
		return;
	}
	printf("OPEN FILES: %d(%d) @ 0x%x", nofile, 
					    USR->u_lastfile,
					    USR->u_ofile);
	printf("\n");
#endif
#ifdef BSD
	printf(" fd\tfile  of_flags of_inuse     f_ops (s/v)node\n");
#else
	printf(" fd\tfile  of_flags of_inuse     (s/v)node\n");
#endif
	for(i = 0; i < nofile; i++) {
		if( ofp[i].of_file ) {
			printf("%3d 0x%8.8x\t%1x\t%3d\t", i, 
				(unsigned)ofp[i].of_file,
				ofp[i].of_flags, ofp[i].of_inuse); 
			if (readv(ofp[i].of_file, &file, sizeof file)) {
#ifdef BSD
				printf("%9.9s ", addr_str(file.f_ops));
				printf("0x%8.8x\n",file.f_data);
#else
				printf("0x%8.8x\n",file.f_vnode);
#endif
			} else {
				printf("\n");
			}
		}
	}
	free(ofp);
	printf("\n");
	printf("\n");
}

struct proc Last_proc;		/* last proc read as getuarea */

/*
 * return a pointer to the user urea that corresponds to the proc
 * pointed to by pp. As a side affect fill in "Proc" with that proc.
 */
char *
getuarea(pp)
	struct proc *pp;	/* virtual address of proc slot */
{
	register struct user *up;
	static char pagealign[(UPAGES*NBPG)+(CLBYTES-1)];
	char *u = (char *)(((int)pagealign + (CLBYTES-1)) &~ (CLBYTES-1));
	static struct ucred cred;

	U = 0;
	if (readv(pp, &Last_proc, sizeof (struct proc)) != sizeof (struct proc)) 
		return BADREAD;
	switch (Last_proc.p_stat) {
	case SWAIT:
	case SIDL:
	case SZOMB:
	case 0:
		return(NULL);
	case SRUN:
	case SSLEEP:
	case SSTOP:
	case SONPROC:
		break;
	}
	U = (char *)Last_proc.p_uarea;	/* keep for external references */
	if (Last_proc.p_flag & SLOAD) {
#ifdef USERTOUB
		if (readv(USERTOUB(Last_proc.p_uarea), u, UPAGES*NBPG) != 
								UPAGES*NBPG)
#else
		if (readv(U, u, UPAGES*NBPG) != UPAGES*NBPG)
#endif
			return BADREAD;
	} else if (live && swapfd) {
		lseek(swapfd, (long)dtob(Last_proc.p_swaddr), 0);
		if (read(swapfd, (char *)u, UPAGES*NBPG) != UPAGES*NBPG)
			return BADREAD;
	} else
		return SWAPPED;
#ifdef UBTOUSER
	up = UBTOUSER(u);
#else
	up = (struct user *)u;
#endif

	/*
	 * Got the uarea, see about getting the cred structure.
	 */
	if (readv(up->u_cred, &cred, sizeof(cred)) != sizeof(cred))
		return BADREAD;
	up->u_cred = &cred;
	return u;
}

