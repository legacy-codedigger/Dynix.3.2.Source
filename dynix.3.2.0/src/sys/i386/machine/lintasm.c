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

/*
 * $Header: lintasm.c 2.24 1992/02/13 00:15:59 $
 *
 * lintasm.c
 *	Provide C-declarations for asm-declared procedures/data, for lint.
 */

/* $Log: lintasm.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../netinet/in.h"
#include "../net/route.h"
#include "../netinet/in_pcb.h"
#ifdef	NFS
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#endif	NFS

#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/cfg.h"

#include "../machine/ioconf.h"
#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/trap.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/psl.h"

/*ARGSUSED*/
useracc(base, count, rw) char *base; u_int count; { return(0); }

/*ARGSUSED*/
bcopy(from, to, count) char *from, *to; unsigned count; { /*NOTREACHED*/ }

/*ARGSUSED*/
ovbcopy(from, to, size) char *from, *to; unsigned size; { /*NOTREACHED*/ }

/*ARGSUSED*/
bzero(base, length) char *base; unsigned length; { /*NOTREACHED*/ }

/*ARGSUSED*/
ptefill(pte, val, cnt) struct pte *pte; int val; size_t cnt; { /*NOTREACHED*/ }

save() { return(0); }

/*ARGSUSED*/
resume(curproc, newproc, locked)
	struct proc *curproc, *newproc, *locked; { /*NOTREACHED*/ }

use_private() { /*NOTREACHED*/ }

disable_fpu() { /*NOTREACHED*/ }

restore_fpu() { /*NOTREACHED*/ }

#ifdef	FPU_SIGNAL_BUG
/*ARGSUSED*/
save_fpu(addr) 
	struct fpusave *addr; { /*NOTREACHED*/ }
#else
save_fpu() { /*NOTREACHED*/ }
#endif

save_fpu_fork() { /*NOTREACHED*/ }

char	icode[];
int	szicode;

spl_t	spl0() { return(0); }
spl_t	spl1() { return(0); }
spl_t	spl5() { return(0); }
spl_t	splimp() { return(0); }
spl_t	splhi() { return(0); }

/*ARGSUSED*/
splx(s) spl_t s; { /*NOTREACHED*/ }

swt_undef() { /*NOTREACHED*/ }

resched() { /*NOTREACHED*/ }

undef() { /*NOTREACHED*/ }

/*ARGSUSED*/
addupc(upc, u_prof, ticks) struct uprof *u_prof; { /*NOTREACHED*/ }

/*ARGSUSED*/
bcmp(s1, s2, n) char *s1, *s2; unsigned n; { return(0); }

/*ARGSUSED*/
ffs(mask) int mask; { return(0); }

/*ARGSUSED*/
strlen(string) char *string; { return(0); }

#ifdef	NFS
/*ARGSUSED*/
ip_hdr_cksum(iphdr) struct ip *iphdr; { return(0); }
#endif	NFS

htonl(hostorder) u_long hostorder; { return(hostorder); }

ntohl(hostorder) u_long hostorder; { return(hostorder); }

ntohs(s) u_short s; { return ((int)s); }

htons(s) u_short s; { return ((int)s); }

/*ARGSUSED*/
setjmp(jmp_buf) label_t *jmp_buf; { return(0); }

/*ARGSUSED*/
fsetjmp(jmp_buf) label_t *jmp_buf; { return(0); }

/*ARGSUSED*/
longjmp(jmp_buf) label_t *jmp_buf; { /*NOTREACHED*/ }

/*ARGSUSED*/
pushlongjmp(uarea, jmp_buf)
	struct user *uarea;
	label_t *jmp_buf;
{ /*NOTREACHED*/ }

struct	user	u;
struct	plocal	l;

short	upyet;

/*
 * Make refs to things called from asm or conditionally configured.
 */

refstuff(i)
{
	extern	int	(*softvec[])();
	extern	int	(*swtrapvec[])();
	extern	struct	domain unixdomain;
	extern	struct	domain rawEdomain;
	extern	struct	domain atdomain;
	extern  struct  cntlrs	b8k_cntlrs[];
	extern	int	rthashsize;
	extern	int	arptab_size;
	extern	int	syscall_nhandler;
	struct in_addr	in_arg;

	sysinit("","");

	trap(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	syscall_3_0(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	nosyscall(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	lint_ref_int(syscall_nhandler);
	bogusint(0, 0);
	hardclock(i, 0, 0, 0, 0, (caddr_t)0, 0);
	(void) in_localaddr(in_arg);
	in_losing((struct inpcb*) 0);

	minphys((struct buf *)0);

	slpboot();
	setmonop();
	sleep((caddr_t)0, 0);
	wakeup((caddr_t)0);

	(void) bmpopen((dev_t)0, 0);
	(void) bmpclose((dev_t)0, 0);
	(void) bmpstrat((struct buf *)0);
	(void) bmpminphys((struct buf *)0);

	(void) cmpopen((dev_t)0, 0);
	(void) cmpclose((dev_t)0, 0);
	(void) cmpread((dev_t)0, (struct uio *)0);
	(void) cmpwrite((dev_t)0, (struct uio *)0);
	(void) cmpselect((dev_t)0, 0);
	(void) cmpioctl((dev_t)0, 0, (caddr_t)0, 0);

	(*softvec[0])();
	(*swtrapvec[0])();

	bzero((caddr_t)&unixdomain, sizeof(unixdomain));	/* just a ref */
	bzero((caddr_t)&rawEdomain, sizeof(rawEdomain));	/* just a ref */
	bzero((caddr_t)&atdomain, sizeof(atdomain));		/* just a ref */
	if(rthashsize == 0 || arptab_size == 0)			/* just a ref */
		printf("no rt or arptab??\n");

	b8k_cntlrs[0].conf_b8k((struct cfg_boot *)0);
	setgm(0,0);

	(void) _copyin((caddr_t)0, (caddr_t)0, 0);
	(void) _copyout((caddr_t)0, (caddr_t)0, 0);
	(void) _useracc((caddr_t)0, 0, 0);

	(void) fuibyte((caddr_t)0);

	bufalloc((struct buf *)0);
	buffree((struct buf *)0);

	fpa_trap((long) 0);

	(void) rdSubslave((u_char)0, (u_char)0, (u_char)0);
	wrSubslave((u_char)0, (u_char)0, (u_char)0, (u_char)0);

#ifdef	KERNEL_PROFILING
	(void) kp_nmi(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif	KERNEL_PROFILING
}

/*
 * Procedure to reference an integer, to satisfy various lint gripes
 * (variables decleared for 032 & SGS portability that aren't used in SGS).
 */

lint_ref_int(i) int i; { refstuff(i); }

/*ARGSUSED*/
copyin(from, to, count) char *from, *to; u_int count; { return(0); }

/*ARGSUSED*/
copyout(from, to, count) char *from, *to; u_int count; { return(0); }

/*ARGSUSED*/
copyinstr(fromaddr, toaddr, maxlen, lencopied)
	char	*fromaddr, *toaddr;
	int	maxlen, *lencopied;
{ return(0); }

/*ARGSUSED*/
copyoutstr(fromaddr, toaddr, maxlen, lencopied)
	char	*fromaddr, *toaddr;
	int	maxlen, *lencopied;
{ return(0); }

/*ARGSUSED*/
copystr(fromaddr, toaddr, maxlen, lencopied)
	char	*fromaddr, *toaddr;
	int	maxlen, *lencopied;
{ return(0); }

/*ARGSUSED*/
fubyte(address) char *address; { return(0); }

/*ARGSUSED*/
fuibyte(address) char *address; { return(0); }

/*ARGSUSED*/
fuword(address) char *address; { return(0); }

/*ARGSUSED*/
fuiword(address) char *address; { return(0); }

/*ARGSUSED*/
subyte(address, data) char *address; char data; { return(0); }

/*ARGSUSED*/
suibyte(address, data) char *address; char data; { return(0); }

/*ARGSUSED*/
suword(address, data) char *address; int data; { return(0); }

/*ARGSUSED*/
suiword(address, data) char *address; int data; { return(0); }

void wfpusave(fpusave) struct fpusave *fpusave; { fpusave->fpu_status = 0; }
 
void wfpurestor(fpusave) struct fpusave *fpusave; { fpusave->fpu_status = 0; }

void init_i387() { return; } 
 
int get_sw() { return(0); }
 
/*ARGSUSED*/
void set_cw(val) { return; }
 
void adds(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void adds_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void addd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; }
 
void addd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void floats(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void floatd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void fixs(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 

void fixs_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void fixd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 

void fixd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void cvtds(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void cvtds_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void cvtsd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void cvtsd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void muls(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void muls_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void muld(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void muld_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void subs(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void subs_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void subd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void subd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void divs(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void divs_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void divd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void divd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void mulns(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void mulns_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void mulnd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void mulnd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void cmps(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void cmps_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void cmpd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void cmpd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void tsts(src) long *src; { *src = 0; return; } 
 
void tsts_fast(src) long *src; { *src = 0; return; } 
 
void tstd(src) long *src; { *src = 0; return; } 
 
void tstd_fast(src) long *src; { *src = 0; return; } 
 
void negs(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void negs_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void negd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void negd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void abss(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void abss_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void absd(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void absd_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void amuls(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void amuls_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void amuld(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void amuld_fast(src, dst) long *src, *dst; { *src = 0; *dst = 0; return; } 
 
void macs(src, dst, acc) long *src, *dst, *acc; { *src = 0; *dst = 0; *acc = 0; return; }
 
void macs_fast(src, dst, acc) long *src, *dst, *acc; { *src = 0; *dst = 0; *acc = 0; return; }
 
void macd(src, dst, acc) long *src, *dst, *acc; { *src = 0; *dst = 0; *acc = 0; return; } 
 
void macd_fast(src, dst, acc) long *src, *dst, *acc; { *src = 0; *dst = 0; *acc = 0; return; } 

int segdesc_D() {return(1);}

void loads(src, dst) long *src, *dst; { *src = 0; *dst = 0; }

void loadd(src, dst) long *src, *dst; { *src = 0; *dst = 0; }
