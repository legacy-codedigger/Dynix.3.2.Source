/* $Copyright:	$
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

#ifndef	lint
static	char	rcsid[] = "$Header: kern_acct.c 2.9 91/03/26 $";
#endif

/*
 * kern_acct.c
 *	Accounting Support
 */

/* $Log:	kern_acct.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/kernel.h"
#include "../h/acct.h"
#include "../h/uio.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../balance/clock.h"

struct	vnode	*acctp;
struct	vnode	*savacctp;

/*
 * Semaphore to coordinate access to accounting mechanism.
 */

sema_t	acct_sema;

/*
 * sysacct()
 *	Perform process accounting functions (system-call).
 */

sysacct()
{
 	struct vnode *vp;
	register struct a {
		char	*fname;
	} *uap = (struct a *)u.u_ap;

	if (suser()) {					/* must be root */
		p_sema(&acct_sema, PZERO);		/* one at a time */
		/*
		 * If currently suspended, make believe it's on.
		 */
		if (savacctp) {
			acctp = savacctp;
			savacctp = NULL;
		}
		/*
		 * If argument is NULL, turn accounting OFF.
		 */
		if (uap->fname==NULL) {
			if (acctp) {
				VN_RELE(acctp);
				acctp = NULL;
			}
			v_sema(&acct_sema);
			return;
		}
		/*
		 * Want to turn on accounting...  get a vnode for
		 * the accounting file. If SysV acct, return error
		 * if already on.
		 */
		if (u.u_tuniverse == U_ATT && acctp) {
			u.u_error = EBUSY;
			v_sema(&acct_sema);
			return;
		}
		u.u_error = lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
						(struct vnode **)0, &vp);
		if (u.u_error) {
			v_sema(&acct_sema);
			return;
		}
		if (vp->v_type != VREG) {
			u.u_error = EACCES;
			VN_PUT(vp);
			v_sema(&acct_sema);
			return;
		}
		/*
		 * Release old accounting file (if any),
		 * and return with accounting enabled.
		 */
		if (acctp)
			VN_RELE(acctp);
		acctp = vp;
		VN_UNLOCKNODE(vp);
		v_sema(&acct_sema);
	}
}

/*
 * acct()
 *	On exit, write a record on the accounting file.
 */

int	acctsuspend = 2;	/* stop accounting when < 2% free space left */
int	acctresume = 4;		/* resume when free space risen to > 4% */

struct	acct acctbuf;

acct(rv)
	int	rv;
{
	register int i;
	register struct acct *ap = &acctbuf;
	register struct rusage *ru;
	struct vnode *vp;
	struct timeval t;
	struct statfs sb;
	extern int sys5acct;
	GATESPL(s_ipl);

	/*
	 * First make quick check if accounting is enabled.
	 */

	if (acctp == NULL && savacctp == NULL)
		return;

	/*
	 * If suspended, resume if there is enough space now.
	 */

	p_sema(&acct_sema, PZERO);
	if (savacctp) {
		(void) VFS_STATFS(savacctp->v_vfsp, &sb);
		if (sb.f_bavail > (acctresume * sb.f_blocks / 100)) {
			acctp = savacctp;
			savacctp = NULL;
			printf("Accounting resumed\n");
			/*
			 *+ Enought space now exists on the disk containing
			 *+ the system accounting file. Accounting records
			 *+ will now be added.
			 */
		}
	}

	/*
	 * If not accounting now, nothing to do.
	 */

	if ((vp = acctp) == NULL) {
		v_sema(&acct_sema);
		return;
	}

	/*
	 * If not enough room, suspend accounting.
	 */

	(void) VFS_STATFS(acctp->v_vfsp, &sb);
	if (sb.f_bavail <= (acctsuspend * sb.f_blocks / 100)) {
		savacctp = acctp;
		acctp = NULL;
		printf("Accounting suspended\n");
		/*
		 *+ Not enought space exists on the disk containing
		 *+ the system accounting file. Accounting records
		 *+ will not be added untill space is freed.
		 */
		v_sema(&acct_sema);
		return;
	}

	/*
	 * Go for it -- generate and write accounting record.
	 *
	 * Note: 4.2bsd fields u.u_ru.ru_i[dsx]rss not used.
	 */

	for (i = 0; i < sizeof (ap->ac_comm); i++)
		ap->ac_comm[i] = u.u_comm[i];
	ru = &u.u_ru;
	/*
	 * Store times in HZ units (ticks)
	 */
#define	toticks(s,us)	((s) * HZ + (us) / (1000000/HZ))

	ap->ac_utime = compress(toticks(ru->ru_utime.tv_sec,
						ru->ru_utime.tv_usec));
	ap->ac_stime = compress(toticks(ru->ru_stime.tv_sec,
						ru->ru_stime.tv_usec));
	P_GATE(G_TIME, s_ipl); t = time; V_GATE(G_TIME, s_ipl);
	timevalsub(&t, &u.u_start);
	ap->ac_etime = compress(toticks(t.tv_sec, t.tv_usec));
#undef	toticks
	ap->ac_btime = u.u_start.tv_sec;
	ap->ac_uid = u.u_ruid;
	ap->ac_gid = u.u_rgid;
	ap->ac_mem = 0;
	ap->ac_rw = compress(ru->ru_inblock + ru->ru_oublock);
	ap->ac_io = sys5acct ? compress(u.u_ioch) : ap->ac_rw;
	if (u.u_ttyp)
		ap->ac_tty = u.u_ttyd;
	else
		ap->ac_tty = NODEV;
	ap->ac_flag = u.u_acflag;
	ap->ac_stat = (char) rv;

	VN_LOCKNODE(vp);
	u.u_error = 0;				/* in case died from signal */
	u.u_error = vn_rdwr(UIO_WRITE, vp, (caddr_t)ap, sizeof(acctbuf), 0,
					UIOSEG_KERNEL, IO_APPEND, (int *)0);

	/*
	 * Ignore write error.  In the *rare* case that this happens, worst
	 * case is mangled acct file.
	 */

	VN_UNLOCKNODE(vp);
	v_sema(&acct_sema);
}

/*
 * compress()
 *	Produce a pseudo-floating point representation
 *	with 3 bits base-8 exponent, 13 bits fraction.
 */

compress(t)
	register long t;
{
	register exp = 0, round = 0;

	while (t >= 8192) {
		exp++;
		round = t&04;
		t >>= 3;
	}
	if (round) {
		t++;
		if (t >= 8192) {
			t >>= 3;
			exp++;
		}
	}
	return ((exp<<13) + t);
}
