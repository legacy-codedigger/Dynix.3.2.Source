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
static	char	rcsid[] = "$Header: shmsys.c 1.7 90/04/02 $";
#endif

/*
 * SVshm.c
 *	System V shared-memory emulation using _mmmap() mapped files.
 *
 * Basic idea:
 *
 *	Ascii encoding of shmid is file name of file containing
 *	shmid_ds structure (1st page) and mapping memory (rest).
 *
 *	Ascii encoding of `key' is name of link to shmem file.
 *
 *	Thus private file has no key link.
 *
 *	Note: key != shmid since need to support private shmem's and
 *	can shmat() with shmid only.
 *
 * Differences from System V:
 *
 *	each attached shared-mem holds an open file-descriptor.
 *	Thus, some operations may fail with EMFILE.
 *
 *	shmctl(IPC_STAT) can dump core rather than return EFAULT.
 *
 *	no attempt made to maintain shm_nattch, shm_cnattch; hard or
 *	impossible unless implementation is moved inside kernel.
 *
 *	Allows multiple shmat's to specify same or overlapping addresses
 *	(could be fixed).
 *
 *	SHM_LOCK, SHM_UNLOCK not really supported.
 *
 * TODO:
 *	Deal with multiple shmem's at same vaddr.
 *
 *	Check out error return values for Sys V consistency.
 *
 *	Insure access checking is correct.
 *
 *	Think about various cases where calling process dies (what
 *	junk can it leave around?)
 */

/* $Log:	shmsys.c,v $
 */

/*
 * Sys V Includes.
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/times.h>

#define	u_long	unsigned long
#define	u_short unsigned short
#define	u_char	unsigned char

#define NULL	0

#include "mman.h"	/* DYNIX include file */
#include "file.h"	/* DYNIX include file */

#define SHMMIN	(1)
#define SHMMAX	(200*1024*1024)
#define SHMSLOP	(2*1024*1024)

/* Mask off high bit of a long */
#define HIGHBITMASK (0x7FFFFFFF)

/* Defines for DYNIX support */
#define	flock	_flock
#define	getpagesize _getpagesize
#define	mmap	__mmmap
#define lseek	_lseek
#define fchmod	_fchmod
#define BLOCK	oldmask=_sigblock(0xffffffff)
#define UNBLOCK	_sigsetmask(oldmask)
extern int	_pgoff;			/* getpagesize() - 1 */
extern char *_shmat_start;
#define	PGRND(x)	(((int)(x) + _pgoff) & ~_pgoff)
#define	PG_ALIGN(x)	(char *) (((int)(x)) & ~_pgoff)

int oldmask;

#ifdef	DEBUG
#define	static
#endif

#define	MAXNAME	128

static	char	*shmkeyname = "/usr/tmp/SysVshmem/key_";	/* key names */
static	char	*shmidname = "/usr/tmp/SysVshmem/id_";		/* ID names */

extern	int	errno;

extern	char	*sbrk(), *strcpy(), *strcat();

char		*itoxn();



/*
 * shmid file structure -- this lives in 1st page of file.
 * File stores copy of key and shmid, so can find either from
 * the other.
 */

struct	shmdata {
	int		sd_id;			/* copy of shmid */
	struct	shmid_ds sd_shm;		/* Sys V stuff */
};

/*
 * Per process list of attached shm's.
 * Should set NSHM >= NUMMAP.
 */

#define	NSHM	10

static
struct	pp_shm	{
	int		ps_fd;			/* fd of mapping file */
	char		*ps_vaddr;		/* mapped addr */
	char		*ps_taddr;		/* mapped addr */
	int		ps_flags;
}	pp_shm[NSHM];

/* ps_flags */
#define	PS_ALLOC	001			/* entry in use */

/*
 * shmget()
 *	Turn a `key' into a shmid, creating a shared-memory if necessary.
 */

shmget(key, size, shmflg)
	register key_t	key;
	int		size;
	register int	shmflg;
{
	register int	shmfd;
	register int	flags;
	int		val, oumask;
	struct	stat	sb;
	char		shmname[MAXNAME];
	char		keyname[MAXNAME];
	static long	shmid;
	extern time_t	time();
	static int	mypid = 0;
#ifdef PERFMON
	int		tries = -1;
#endif

	/*
	 * Start our search for our shmid at the current time. On
	 * collision search downwards by our pid value.
	 */
	if (!shmid) {
		(void)time((time_t *)&shmid);
		if (shmid < 0)		/* Ensure positive */
			shmid &= HIGHBITMASK;
	}
	if (!mypid)
		mypid = getpid();

	/*
	 * If key == IPC_PRIVATE, create a new id with no associated key.
	 *
	 * Probably should look at error, loop only if EEXIST.
	 * Also, race with other process attaching; could re-test
	 * after flock() and re-enter loop...
	 */

	if (key == IPC_PRIVATE) {
		BLOCK;
		do {
#ifdef PERFMON
			tries += 1;
#endif
			if ((shmid -= mypid) < 0)
				shmid &= HIGHBITMASK;
			(void) itoxn(shmid, shmidname, shmname);
		} while((shmfd=open(shmname, O_RDWR|O_CREAT|O_EXCL, 0666)) < 0);
#ifdef PERFMON
		if (tries > 0)
			printf("shmget: %d tries\n", tries);
#endif
		(void) flock(shmfd, LOCK_EX);
		val = shminit(shmfd, key, shmid, size, shmflg);
		close(shmfd);
		UNBLOCK;
		/* ERROR UNWIND -- unlink file(s) */
		if ((val == -1) || (size < SHMMIN) || (size > SHMMAX)) {
			errno = EINVAL;
			unlink(shmname);
			return(-1);
		}
		return(val);
	}

	/*
	 * Not private, see about attaching to old one.
	 * Complain if doesn't exist and can't create, or does exist and
	 * insist on creating.
	 */

	(void) itoxn(key, shmkeyname, keyname);

	flags = O_RDWR;
	if (shmflg & IPC_CREAT)
		flags |= O_CREAT;
	if (shmflg & IPC_EXCL)
		flags |= O_EXCL;

	BLOCK;
	oumask = umask(0);
	shmfd = open(keyname, flags, 0666);
	(void) umask(oumask);
	if (shmfd < 0) {
		UNBLOCK;
		return(-1);
	}

	/*
	 * Get exclusive access to the file.
	 * If new create, size will be zero.
	 */

	(void) flock(shmfd, LOCK_EX);

	fstat(shmfd, &sb);				/* CHECK ERROR! */

	if (sb.st_size == 0) {
		/*
		 * New segment -- find shmid for it, and initialize.
		 */
		do {
			if ((shmid -= mypid) < 0)
				shmid = -shmid;
			(void) itoxn(shmid, shmidname, shmname);
		} while(link(keyname, shmname) < 0);
		val = shminit(shmfd, key, shmid, size, shmflg);
		close(shmfd);
		UNBLOCK;
		if ((val == -1) || (size < SHMMIN) || (size > SHMMAX)) {
			errno = EINVAL;
			unlink(keyname);
			unlink(shmname);
			return(-1);
		}
		return(val);
	}

	/*
	 * Is previously existing segment.  Insure requested size
	 * is valid.
	 */

	val = shmckold(shmfd, key, size, shmflg);
	close(shmfd);
	UNBLOCK;
	return(val);
}

/*
 * shmat()
 *	Attach to a given shared-mem segment.
 *
 * Keeps track of shmaddr's mapped so it can shmdt() later.
 */

extern char *_curbrk;

char *
shmat(shmid, shmaddr, shmflg)
	int		shmid;
	register char	*shmaddr;
	int		shmflg;
{
	register struct pp_shm *ps, *ips;
	register int	shmfd;
	register int	prot;
	char *		taddr;
	int		size;
	struct	shmdata	shmd;
	int res;

	if (_pgoff == 0)
		_pgoff = getpagesize() - 1;
	/*
	 * Fix up requested address.
	 */
	if (shmflg & SHM_RND)
		shmaddr = (char*)(((long)shmaddr) & ~(SHMLBA-1));
	else
		shmaddr = PG_ALIGN(shmaddr);

	/* 
	 * Get file-desc for shmid (exclusive access) & insure it's ok.
	 *	Also, return with signals blocked (unless error occurs).
	 */
	shmfd = shmlookup(shmid, shmflg, &shmd);
	if (shmfd < 0)
		return((char*)(-1));
	/*
	 * check if address runs into data/bss/heap
	 *	Do after shmlookup, so that signals are blocked.
	 */
	if ((shmaddr != 0) && (shmaddr <= _curbrk)) {
		close(shmfd);
		UNBLOCK;
		errno = EINVAL;
		return((char*)(-1));
	}

	size = PGRND(shmd.sd_shm.shm_segsz);

	/*
	 * Find unused pp_shm slot.
	 * check for overlap with other attached segments.
	 * Find highest attached address (for shmat addr == 0)
	 */
	ps = NULL;
	taddr = 0;
	for(ips = pp_shm; ips < &pp_shm[NSHM]; ips++) {
		/*
		 * keep track of first unused slot
		 */
		if ((ips->ps_flags & PS_ALLOC) == 0) {
			if (ps == NULL)
				ps = ips;
			continue;
		}
		/*
		 * if new attached overlaps an existing one,
		 *	break out of loop.  ps set to NULL
		 *	indicates failure
		 */
		if ((shmaddr != 0) && check_overlap(shmaddr, size, ips)) {
			ps = NULL;	
			break;
		}
		/*
		 * keep track of highest attached address, for
		 *	possible use if shmaddr == 0
		 */
		if (taddr < ips->ps_taddr)
			taddr = ips->ps_taddr + 1;
	}

	/*
	 * failure!
	 */
	if (ps == NULL) {
		if (ips == &pp_shm[NSHM])
			/* ran out of shared memory slots */
			errno = EMFILE;
		else
			/* bad attach address */
			errno = EINVAL;
		close(shmfd);
		UNBLOCK;
		return((char*)(-1));
	}

	/*
	 * If shmaddr is zero, calculate what default attach
	 * address should be.
	 *	If there are attached regions, use the top
	 *	attached address.
	 *
	 *	Otherwise, use some SHMSLOP space above curbrk.
	 */
	if (shmaddr == 0)
		shmaddr = taddr;
	if (shmaddr == 0) {
		shmaddr = _curbrk + SHMSLOP;
		shmaddr = (char*)PGRND(shmaddr);
	} 

	/*
	 * Try to map it.
	 * Note that SysV code appears to allow truncated page-size,
	 * and a zero size is a NOP.  Some test code ==> expect
	 * size to be rounded up.
	 */

	if (shmflg & SHM_RDONLY)
		prot = PROT_READ;
	else
		prot = PROT_READ|PROT_WRITE;

	if (size && mmap(shmaddr, size, prot, MAP_SHARED, shmfd, _pgoff+1) < 0) {
		close(shmfd);
		UNBLOCK;
		return((char*)(-1));
	}

	/*
	 * Success.  Update Sys V info, set up table and return vaddr.
	 */

	ps->ps_fd = shmfd;
	ps->ps_vaddr = shmaddr;
	ps->ps_taddr = shmaddr + size - 1;
	ps->ps_flags |= PS_ALLOC;

	shmd.sd_shm.shm_lpid = getpid();
	shmd.sd_shm.shm_atime = time(0);
	/*
	 * set close on exec flag.  So, if a process with an attached
	 * shared memory region exec's, the file descriptor gets closed out
	 */
	res = fcntl(shmfd, F_SETFD, 1);
	if (res == -1) {
		perror("fcntl in shmat");
		abort();
	}
	if ((_shmat_start == 0) || (_shmat_start > shmaddr))
		_shmat_start = shmaddr;
	shmupdate(shmfd, &shmd);			/* also unlocks */
							/* and unblocks sigs */
	return(shmaddr);
}

static
check_overlap(shmaddr, size, ips)
char *shmaddr;
int size;
struct pp_shm *ips;
{
	return((ips->ps_vaddr < shmaddr + size) &&
		(ips->ps_taddr + 1 > shmaddr));
}


/*
 * shmdt()
 *	Detach from a previously attached shared-memory.
 *
 * shmaddr must correspond exactly to something returned from shmat().
 *
 * Could get fancy and brk() down if this is the last vaddr's in the
 * process.
 */

shmdt(shmaddr)
	register char	*shmaddr;
{
	register struct pp_shm *ps;
	struct	shmdata	shmd;
	char *new_low = (char *)NULL;
	int success = 0;

	BLOCK;
	for(ps = pp_shm; ps < &pp_shm[NSHM]; ps++) {
		if ((ps->ps_flags & PS_ALLOC) == 0)
			continue;
		if (ps->ps_vaddr == shmaddr) {
			/*
			 * Update dtime.
			 */
			if (flock(ps->ps_fd, LOCK_EX) < 0
			||  lseek(ps->ps_fd, 0L, 0) < 0
			||  read(ps->ps_fd, &shmd, sizeof(shmd)) != sizeof(shmd)) {
				(void) flock(ps->ps_fd, LOCK_UN);
				UNBLOCK;
				abort();
			}
			shmd.sd_shm.shm_lpid = getpid();
			shmd.sd_shm.shm_dtime = time(0);
			if (lseek(ps->ps_fd, 0L, 0) < 0
			||  write(ps->ps_fd, &shmd, sizeof(shmd)) != sizeof(shmd)) {
				(void) flock(ps->ps_fd, LOCK_UN);
				UNBLOCK;
				abort();
			}
			/*
			 * Close fd and munmap shmem
			 */
			(void) flock(ps->ps_fd, LOCK_UN);
			close(ps->ps_fd);
			_munmap(ps->ps_vaddr, ps->ps_taddr - ps->ps_vaddr + 1);
			_shmat_start = new_low;
			ps->ps_vaddr = 0;
			ps->ps_flags &= ~PS_ALLOC;
			success = 1;
			break;
		} else {
			if ((new_low == NULL) || (new_low > ps->ps_vaddr))
				new_low = ps->ps_vaddr;
		}
	}
	UNBLOCK;
	if (success) {
		return(0);
	}
	errno = EINVAL;
	return(-1);
}

/*
 * shmctl()
 *	Various shared-memory control functions.
 */

shmctl(shmid, cmd, buf)
	register int	shmid;
	int		cmd;
	struct	shmid_ds *buf;
{
	register int	shmfd;
	register int	euid = geteuid();
	char		fname[MAXNAME];
	struct	shmdata	shmd;

	/* locks and blocks signals */
	shmfd = shmlookup(shmid, (cmd == IPC_STAT ? SHM_RDONLY : -1), &shmd);
	if (shmfd < 0)
		return(-1);

	errno = 0;

	switch(cmd) {

	case IPC_STAT:
		*buf = shmd.sd_shm;
		break;

	case IPC_SET:
		if (euid != shmd.sd_shm.shm_perm.uid
		&&  euid != shmd.sd_shm.shm_perm.cuid
		&&  euid != 0)
			errno = EPERM;
		else {
			shmd.sd_shm.shm_perm.uid = buf->shm_perm.uid;
			shmd.sd_shm.shm_perm.gid = buf->shm_perm.gid;
			shmd.sd_shm.shm_perm.mode = (buf->shm_perm.mode & 0777);
			shmd.sd_shm.shm_ctime = time(0);
			shmupdate(shmfd, &shmd);	/* also unlocks */
							/* & enable signals */
		}
		break;

	case IPC_RMID:
		/*
		 * To remove a shmem ID, just unlink it's key and id files.
		 * System automatically deletes files/resources when no
		 * longer in use.
		 */
		if (euid != shmd.sd_shm.shm_perm.uid
		&&  euid != shmd.sd_shm.shm_perm.cuid
		&&  euid != 0)
			errno = EPERM;
		else {
			(void) unlink(itoxn(shmd.sd_shm.shm_perm.key, shmkeyname, fname));
			(void) unlink(itoxn(shmd.sd_id, shmidname, fname));
			errno = 0;
		}
		break;

	case SHM_LOCK:
	case SHM_UNLOCK:
		/*
		 * Not really supported, but return reasonable error codes.
		 * Could do vm_ctl() to make self non-swappable...
		 */
		if (geteuid() != 0)
			errno = EPERM;
		break;

	default:
		errno = EINVAL;
		break;
	}

	close(shmfd);
	UNBLOCK;
	return(errno ? -1 : 0);
}

/*
 * shminit()
 *	Init a shmem with given values.
 *
 * Fills out shmdata structure according to the rules, and write
 * into shmfd.
 *
 * Returns shmid, or -1 for error (and filled out errno).
 */

static
shminit(shmfd, key, shmid, size, shmflg)
	int	shmfd;
	int	key;
	int	shmid;
	int	size;
	int	shmflg;
{
	struct shmdata shmd;

	bzero((caddr_t)&shmd, sizeof(shmd));

	shmd.sd_shm.shm_perm.key = key;
	shmd.sd_id = shmid;

	shmd.sd_shm.shm_perm.cuid = shmd.sd_shm.shm_perm.uid = geteuid();
	shmd.sd_shm.shm_perm.cgid = shmd.sd_shm.shm_perm.gid = getegid();
	shmd.sd_shm.shm_perm.mode = shmflg & 0777;

	shmd.sd_shm.shm_lpid = 0;
	shmd.sd_shm.shm_cpid = getpid();
	shmd.sd_shm.shm_segsz = size;
	shmd.sd_shm.shm_ctime = time(0);

	fchmod(shmfd, 0666);				/* avoid UMASK hose */
	(void) lseek(shmfd, 0L, 0);			/* paranoia */
	if (write(shmfd, &shmd, sizeof(shmd)) != sizeof(shmd))
		return(-1);
	return(shmid);
}

/*
 * shmckold()
 *	shmfd attached to old shmem -- verify size and flags against
 *	permissions.
 *
 *	Called only by shmget().
 *
 * Returns shmid, or -1 for error (and filled out errno).
 */

static
shmckold(shmfd, key, size, shmflg)
	int	shmfd;
	int	key;
	int	size;
	int	shmflg;
{
	struct	shmdata	shmd;

	if (read(shmfd, &shmd, sizeof(shmd)) != sizeof(shmd)) {
		errno = EINVAL;
		return(-1);
	}

	if (size == 0)
		size = shmd.sd_shm.shm_segsz;

	if (size > shmd.sd_shm.shm_segsz || key != shmd.sd_shm.shm_perm.key) {
		errno = EINVAL;
		return(-1);
	}

/*
	This model of access permissions isn't followed 
	by shmget.

	if (!shmaccess(shmflg, &shmd.sd_shm.shm_perm)) {
		errno = EACCES;
		return(-1);
	}
*/

	if((shmflg & (IPC_CREAT | IPC_EXCL)) ==
		(IPC_CREAT | IPC_EXCL)) {
		errno = EEXIST;
		return(-1);
	}
	
	if((shmflg & 0777) & ~shmd.sd_shm.shm_perm.mode) {
		errno = EACCES;
		return(-1);
	}

	return(shmd.sd_id);
}

/* 
 * shmlookup()
 *	Get file-desc for shmid (exclusive access) & insure it's ok.
 *
 * shmflg == 0 ==> don't check permissions.
 * Else check based on SHM_R and SHM_W.
 *
 * Reads shmdata part of file.
 *
 * Returns fd or -1.
 */

static
shmlookup(shmid, shmflg, shmdp)
	int		shmid;
	int		shmflg;
	register struct	shmdata	*shmdp;
{
	int	shmfd;
	char	fname[MAXNAME];

	BLOCK;
	shmfd = open(itoxn(shmid, shmidname, fname), O_RDWR);
	if (shmfd < 0) {
		UNBLOCK;
		errno = EINVAL;
		return(-1);
	}

	if (read(shmfd, shmdp, sizeof(*shmdp)) != sizeof(*shmdp)
	||  shmid != shmdp->sd_id) {
		close(shmfd);
		UNBLOCK;
		errno = EINVAL;
		return(-1);
	}

	if ((shmflg != -1) && !shmaccess(shmflg, &shmdp->sd_shm.shm_perm)) {
		close(shmfd);
		UNBLOCK;
		errno = EACCES;
		return(-1);
	}

	(void) flock(shmfd, LOCK_EX);
	return(shmfd);
}

/*
 * shmupdate()
 *	Update shmdata structure and unlock shmfd.
 */

static
shmupdate(shmfd, shmdp)
	int	shmfd;
	struct	shmdata	*shmdp;
{
	(void) lseek(shmfd, 0L, 0);
	(void) write(shmfd, shmdp, sizeof(*shmdp));		/* ERROR? */
	(void) flock(shmfd, LOCK_UN);
	UNBLOCK;
}

/*
 * shmaccess()
 *	Check access (R/W) against permissions.
 *
 * Return non-zero for success, else 0.
 */

static
shmaccess(wanted, perm)
	int		wanted;			/* SHM_RDONLY */
	struct	ipc_perm *perm;
{
	int	uid = geteuid();
	int	gid = getegid();
	int	rw;

	if (wanted & SHM_RDONLY)
		rw = SHM_R;
	else
		rw = SHM_R|SHM_W;

	if (uid != perm->cuid && uid != perm->uid) {
		rw >>= 3;
		if (gid != perm->cgid && gid != perm->gid)
			rw >>= 3;
	}

	return((perm->mode & rw) || uid == 0);
}

/*
 * itoxn()
 *	Generate file name encoding hex value of key/shmid.
 *
 * Returns fname argument.
 */

static char *
itoxn(Val, basename, fname)
	int	Val;			/* value to encode in name */
	char	*basename;		/* base of name */
	char	*fname;			/* buffer to hold name */
{
	register unsigned val = Val;
	register char *t;
	char	hex[8+1];
	static	char	hexd[] = "0123456789abcdef";

	t = &hex[sizeof(hex)];
	*--t = '\0';
	while(val) {
		*--t = hexd[val & 0xf];
		val >>= 4;
	}
	return(strcat(strcpy(fname, basename), t));
}

static
bzero(base, length)
	register char *base;
	register int length;
{
	while ((length--) > 0)
		*base++ = 0;
}

