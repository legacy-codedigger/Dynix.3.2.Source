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
static	char	rcsid[] = "$Header: kern_ofile.c 1.3 91/02/28 $";
#endif

/*
 * kern_ofile.c
 *	Implementation of open-file table objects.
 *
 * NOTES:
 *
 *	Implementation assumes all open-file tables are ofile_tab objects;
 *	there is no longer any use of U-area local ofile array.  Yields much
 *	cleaner implementation, more structured, etc, and accomodates shared
 *	open file table more cleanly.  All direct manipulation of "ofile array"
 *	is done here or thru abstracted interfaces like GETF().
 *
 *	Implements fully shared fd table.  Ie, given ofile_tab object can be
 *	referenced by more than one process.
 *
 *	When ofile_tab isn't shared, works nearly identical as previous
 *	system.  GETF/GETFR/getf/etc test bounds on table and return relevant
 *	fp.
 *
 *	When ofile_tab is shared, need to be careful about concurrent close
 *	of open-file-table entry.  Ie, can't just use the 'fp' from the
 *	table, since another process may concurrently close that fp.  Thus,
 *	bump fp's reference count, and store the fp in u.u_fpref (see
 *	ofile_getf()).  u.u_fpref is tested at syscall exit; if != NULL, must
 *	de-ref this.  Normally NULL, so don't need to zap at syscall entry.
 *	One such should be enough for all relevant system calls.  Note that
 *	in C++ could have auto-instance of class variable that does the
 *	de-ref on exit (via destructor), thus handles *all* scope/procedure
 *	exits without worrying about this (sigh ;-).
 *
 *	Open/other_fd_creators: exclusive lock ofile_tab.  Find one, keep fp
 *	part NULL but set UF_ALLOC so GETF/etc will fail and other slot
 *	allocation will not find this one.  See ofile_alloc().  Creators
 *	must be careful to install fp then clear flag *after* all is set up
 *	(see ofile_install()).
 *
 *	Fork: if sharing ofile_tab, just copy ofile_tab pointer(s) and bump
 *	ref-count (ofile_addref()).  Otherwise, clone it.  ofile_clone()
 *	dups fp's as well as create new ofile_tab object.
 *
 *	Exit:  decrement ref-count on ofile_tab (ofile_deref()).  When
 *	ref-count on ofile_tab object hits zero, implicitly close all fp's
 *	and release the object.
 *
 *	Exec:  Back out if table is shared (clone ofile table and install
 *	when exec completes).
 *
 *	dup2() can race with open/etc in a shared ofile table.  It will work,
 *	but results can be non-deterministic.  See ofile_install(), ofile_dup().
 *	Similarly, shrinking ofile_capacity() can race with concurrent
 *	open/dup/etc; this is ok, the open will be implicitly closed.
 *
 *	Implementation doesn't swap out extended ofile tables.  This is to
 *	keep the implementation simple and accommodate most needs.  If need
 *	to support swap out, need separate sema_t to guard refcnt and
 *	"core-count" (since swapout/swapin/deref can all occur
 *	asynchronously).  This is ugly and more overhead.  This also moves
 *	more easily to use of heap memory allocator.  Note that can eliminate
 *	most need for extended table via Nofile configuration, and can
 *	constrain it to (eg) one page each via max_nofile (thus minimize
 *	risk).
 *
 *	Need to worry about TLB coherency in shared table when/if change size
 *	of table in ofile_capacity(), since this uses wmemall() which doesn't
 *	guarantee TLB coherency.  Implementation sends interrupts to
 *	processors, waiting for all to flush their TLB.  See balance/tlb.c
 *
 *	Requires change to u_mmap[].mm_fdidx interpretation.  Older semantics
 *	of mmap() storing fdidx and extending u_mmap[] don't work, since
 *	another process can close/re-open the same fd...  Good solution looks
 *	like mm_fdidx is index to file table (eg, &file[mm_fdidx]), *and*
 *	FDBUMP this entry to avoid having it disappear.  vmmapfree() needs to
 *	closef() when detach and count reaches zero; appears to be no
 *	deadlocks/etc here.  fork() needs FDBUMP &file[u_mmap[].mm_fdidx] as
 *	appropriate.  Also drop close ==> unmap semantics in shared ofile
 *	table instances (unclear semantics if shared ofile_tab, and we later
 *	decided this was incorrect choice anyhow).  Note that shmdt() in SVAE
 *	uses close() semantics to flush the mapping -- should be easy to fix.
 *
 *	Note that oft_ofile pointer is "volatile" in most cases, since if
 *	process blocks (eg, when closing a fp) it can swap.  Thus, between
 *	references to oft_ofile it *can* change.  Need to keep this in mind
 *	for ATS C (eg, in ofile_close_all()).
 *
 * TODO:
 *
 *	Verify no need for fsetjmp() in syscall u_fpref handling, and in
 *	various fp close cases here.  Think all drivers handle signals
 *	themselves (note that exec "exclose" handling has never worried about
 *	signals).
 *
 *	Resolve record-locking semantics vs deref_file() and shared open-file
 *	table.  Current prototype may not work with record locks, since
 *	stores pid and another pid might actually close the reference.  Ie,
 *	may need to tie "who asserted lock" to the shared file-table, *not*
 *	any given process.  Also, what is semantics of cleanlocks() if
 *	program dup'd the file-descriptor?  Think 1st close will drop locks,
 *	even if close is on alternate fd (eg, not the one the lock operation
 *	was done on) !!  Also, should have some state in vnode (or file-table
 *	entry) which ==> there is something (or maybe there is) something to
 *	look at (to make closef()/cleanlocks() faster).
 *
 *		Note: Dynix/psx roots record-lock list in vnode, so closef()
 *		need not unconditionally call cleanlocks().  Interface is pid
 *		based, though, and fcntl(F_GETFLK) returns structure with
 *		"locking pid" in it.  May need to store a pid in an ofile_tab
 *		object, use this pid (inherit from creating process) for all
 *		record locking fuss in shared ofile-table processes.  This
 *		might need to extend interfaces (eg, closef(fp, pid)), and
 *		still need ofile_deref().  OR, make record locking illegal in
 *		shared ofile table's (plus check in ofile_addref() when go
 *		from 1 to 2 that no locks are asserted).  Leave this as
 *		unresolved in the prototype.
 *
 *	Consider in-line expanding deref_file() in relevant places
 *	(particularly syscall()).
 *
 *	Consider unify ofile_install() and ofile_dup(), or decide they really
 *	are distinct functions.
 *
 *	Consider in selscan() hold ofile-table locked during the entire
 *	scan, so know things are stable.  Need new interfaces to lock, unlock
 *	and access element.  This violates the abstraction, though, so should
 *	measure additional overhead in selscan() of current approach.
 *
 *	Is there any need to clean up syioctl(TIOCNOTTY)?  Maybe use similar
 *	technique as selscan().
 *
 *	Consider allocate ofile_tab objects from the heap.  Ie, can allocate
 *	sufficient size from heap, and always use oft_lofile[] array.  Drops
 *	all dymanic memory fussing, at a cost in more memory consumed.  Also
 *	faster, since don't indirect thru oft_ofile pointer.
 *
 *	If shared file-table overheads are too high (even if not ;-) should
 *	think about using atomic inc/dec on f_count.  Must be careful about
 *	vhangup(), maybe others.  But this could make FDBUMP() *very* fast, and
 *	appropriate inlining of deref_file() also real fast.
 *
 *	Consider optimize ofile_deref() to avoid locking ofile_tab if it's
 *	never been shared.  Not sufficient to just test ref-count at entry,
 *	since concurrent ofile_deref() calls could race; thus would need
 *	field (bool_t) true if it was ever shared, turn FALSE when safe
 *	(somehow).  Other cases (eg, GETF()) are ok to test refcnt outside
 *	lock since racing deref won't reference contents.  Probably a nit.
 *
 *	When/if done "for real", drop ux_* space holder fields.  Kept in
 *	prototype only to keep struct user same size for ps and friends.
 *	Also, move FM_xxx to header file for public consumption.  Probably
 *	mod FM_VFORK to 0x80000000 to keep it "out of the way", or do this
 *	more cleanly.  Also, have main() call newproc(FM_SHARE_OFILE), since
 *	sched() and pageout() can share one (they'll never use it anyhow).
 *
 *	Resolve mm_lastfd: is this really needed?  Think it can be removed.
 *	Pass PROT_LASTFD to map_unmap() in vmmapfree() if mm_fdidx < 0 or
 *	f_count == 1, and if f_count==1, pass 'detach' instead of mm_lastfd
 *	to map_derefpg().  This should simplify things (removes the silly
 *	loops setting mm_lastfd), and is more like what Dynix/psx does.
 *
 *	Note that fork() is shfork(0); when this is done "for real" could
 *	implement it as such -- ie, only one kernel interface, but 2 library
 *	interfaces.  Similar for vfork().  Current prototype shfork() library
 *	interface is a kludge and needs to be fixed.  No current shvfork()
 *	library interface, since don't want to bother at this time, and not
 *	clear how useful this is.
 *
 * TODONT:
 *
 *	Consider conditional lock/unlock interface: lock ofile_tab if it
 *	appears shared, unlock if caller holds it locked.  Should eliminate
 *	the "is_shared" variables and clean things up some.  This should
 *	be valid, since if caller didn't think it shared on entry, it cannot
 *	be when exiting.  WRONG: race with ofile_deref().
 */

/* $Log:	kern_ofile.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/file.h"
#include "../h/vm.h"

#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../machine/mftpr.h"

extern	int	max_nofile;		/* system-defined MAX */

/*
 * Module local definitions.
 */

int		Nofile = NOFILE;	/* default # open files per process */
extern	int	nofile_tab;		/* # allocated entries */

static	struct	ofile_tab *ofile_free_list;	/* free list */
static	lock_t	ofile_list_mutex;		/* mutex on free list */

#ifdef	DEBUG
static	int	ofile_num_free;		/* count # free entries */
#endif	DEBUG

struct	ofile_tab *ofile_new();

/*
 * SZ_OFILE() is size in bytes of an ofile_tab structure for nfds fd's.
 */

#define	SZ_OFILE(nfds)		((nfds) * sizeof(struct ofile))

/*
 * Macros to lock/unlock free-list of ofile_tab objects.
 */

#define	LOCK_OFILE_LIST(s)	(s) = p_lock(&ofile_list_mutex, SPLFS)
#define	UNLOCK_OFILE_LIST(s)	v_lock(&ofile_list_mutex, (s))

/*
 * Macros to lock/unlock individual ofile_tab object.
 * LOCK_SHARED_OFILE() and UNLOCK_SHARED_OFILE() are for coding convenience.
 */

#define	LOCK_OFILE(oft, s)	(s) = p_lock(&(oft)->oft_mutex, SPLFD)
#define	UNLOCK_OFILE(oft, s)	v_lock(&(oft)->oft_mutex, (s))

#define	LOCK_SHARED_OFILE(oft, s, shared) { \
	if (OFILE_SHARED(oft)) { \
		++(shared); \
		LOCK_OFILE(oft, s); \
	} \
}

#define	UNLOCK_SHARED_OFILE(oft, s, shared) \
	if (shared) { UNLOCK_OFILE(oft, s); }

/*
 * ofile_boot()
 *	Allocate set of open-file-table objects of appropriate size
 *	to hold an array of Nofile ofile's.
 *
 * Called early during boot, while calloc() is still legal.
 */

ofile_boot()
{
	register struct ofile_tab *oft;
	register int i;
	int	szoft;

	/*
	 * Insure Nofile is legit.
	 */

	if (Nofile < NOFILE)
		Nofile = NOFILE;
	if (Nofile > max_nofile)
		Nofile = max_nofile;

	szoft = sizeof(struct ofile_tab) + (Nofile - 1) * sizeof(struct ofile);

	/*
	 * Allocate and initialize list of open-file table objects.
	 */

	init_lock(&ofile_list_mutex, G_FILELIST);

	for (i = 0; i < nofile_tab; i++) {
		oft = (struct ofile_tab *) calloc(szoft);
		oft->oft_next = ofile_free_list;
		ofile_free_list = oft;
		oft->oft_lastfile = -1;
		oft->oft_nofile = Nofile;
		oft->oft_ofile = NULL;
		init_lock(&oft->oft_mutex, G_FILMIN + i%(G_FILMAX-G_FILMIN+1));
#ifdef	DEBUG
		++ofile_num_free;
#endif	DEBUG
	}

	InitFlushTLB();		/* this should be called from somewhere else */
}

/*
 * ofile_init()
 *	Initialize 1st process's ofile object.
 *
 * Called during system initialization.
 */

ofile_init()
{
	u.u_ofile_tab = ofile_new(Nofile);
	(void) ofile_alloc_mem(u.u_ofile_tab);
	u.u_fpref = NULL;
}

/*
 * ofile_alloc()
 *	Allocate a user file descriptor (new version of ufalloc()).
 *
 * If shared table, must insure exclusive access to table.  Find
 * "free" entry, and mark it as tentatively allocated, so concurrent
 * open/etc can't find same slot.  Caller must call ofile_install() to
 * either install valid fp or install NULL (error unwind).
 *
 * Returns file descriptor index (>= 0) if succeed, else -1.
 * Does *not* set u.u_error; inappropriate to set this here.
 */

ofile_alloc(oft, fd)
	register struct ofile_tab *oft;
	register int	fd;		/* smallest desired file-descriptor */
{
	register struct ofile *ofp;
	bool_t	is_shared = 0;
	spl_t	s;

	ASSERT_DEBUG(fd >= 0, "ofile_alloc: bad fd");

	LOCK_SHARED_OFILE(oft, s, is_shared);

	for (ofp = &oft->oft_ofile[fd]; fd < oft->oft_nofile; fd++, ofp++) {
		if (ofp->of_file == NULL && ofp->of_flags == 0) {
			ofp->of_flags = UF_ALLOC;
			UNLOCK_SHARED_OFILE(oft, s, is_shared);
			return fd;
		}
	}

	UNLOCK_SHARED_OFILE(oft, s, is_shared);

	return -1;
}

/*
 * ofile_install()
 *	Install entry in open file table.
 *
 * If shared, lock table so installation can be atomic.  Use exclusive lock
 * since may adjust u_lastfile (and expect latency to be very small).
 *
 * Can have race between (eg) dup2() and open; can get here and something is
 * already installed here.  In this case, install new one and drop old one;
 * application is racing, gets non-deterministic results.  Similarly, error
 * unwind (installing NULL) could replace a "dup2" result.  Could tighten this
 * up a bit, but there are still races.
 *
 * Can also race between shrinking ofile_capacity() and an open/dup/etc.
 * In this case, the install is ignored (eg, fd is beyond nofile), and the
 * fp is dereferenced.  This seems consistent semantics between racing open and
 * shrinking ofile_capacity(): the open will loose in either case.
 *
 * Installing NULL frees an entry previously allocated (error unwind).
 */

ofile_install(oft, fd, fp)
	register struct	ofile_tab *oft;
	int	fd;
	struct	file	*fp;
{
	struct	ofile	*ofp;
	struct	file	*oldfp;
	bool_t	is_shared = 0;
	spl_t	s;

	LOCK_SHARED_OFILE(oft, s, is_shared);

	/*
	 * Ignore out of range index (implicit "close").
	 */

	if ((u_int) fd >= oft->oft_nofile) {
		UNLOCK_SHARED_OFILE(oft, s, is_shared);
		if (fp)
			closef(fp);
		return;
	}

	/*
	 * Install entry.  Remember old fp, in case installing over something.
	 */

	ofp = &oft->oft_ofile[fd];
	oldfp = ofp->of_file;			/* in case raced (eg) dup2 */
	ofp->of_file = fp;
	ofp->of_flags = 0;

	/*
	 * If installing an entry (!NULL) and this is beyond lastfile, update.
	 */

	if (fp != NULL && fd > oft->oft_lastfile)
		oft->oft_lastfile = fd;

	UNLOCK_SHARED_OFILE(oft, s, is_shared);

	if (oldfp)
		closef(oldfp);
}

/*
 * ofile_getf()
 *	Convert a user supplied file descriptor into a pointer
 *	to a file structure, when caller has a shared file table.
 *
 * Assumes ofile table is shared.  Doesn't hurt if it's not (ie, raced with a
 * reference to ofile table going away), just a bit more overhead.
 *
 * All callers test for shared ofile_tab before calling here.
 *
 * Check range of the descriptor, and if legit file-table entry, bump its
 * reference count and remember (u_fpref) this was done.
 *
 * Returns pointer to file-table entry (with bumped reference count) or NULL.
 */

struct file *
ofile_getf(oft, fd)
	register struct	ofile_tab *oft;
	int	fd;
{
	register struct file *fp = NULL;
	spl_t	s;

	ASSERT_DEBUG(u.u_fpref == NULL, "ofile_getf: u_fpref");

	LOCK_OFILE(oft, s);

	if ((unsigned)fd < oft->oft_nofile) {
		fp = oft->oft_ofile[fd].of_file;
		if (fp != NULL) {
			FDBUMP(fp);
			u.u_fpref = fp;
		}
	}

	UNLOCK_OFILE(oft, s);

	return (fp);
}

/*
 * ofile_dup()
 *	Duplicate one open file table entry into another.
 *
 * `fd' argument may reference a valid, unallocated, or incompletely filled out
 * index.
 *
 * Installs flags == 0 in new entry.
 *
 * Note that nothing prevents dup2()'ing on top of ofile entry that's in the
 * process of being opened.  No problem, but results are non-deterministic
 * (the last one in leaves their fp).  See ofile_install().
 *
 * NOTE: very similar to ofile_install(), except assume must bump fp's
 * reference count, null fp isn't legit, and does bounds check.
 *
 * Only error is index out of range; typically result of concurrent
 * shrinking ofile_capacity() (eg, between ofile_alloc() and ofile_dup()).
 * In this case, there is no ofile slot left and need not call
 * ofile_install(NULL) to unwind.
 *
 * Returns 0 for error, error code.
 */

ofile_dup(oft, fd, fp)
	register struct	ofile_tab *oft;
	int	fd;
	register struct file *fp;
{
	register struct ofile *ofp;
	struct	file	*oldfp;
	bool_t	is_shared = 0;
	spl_t	s;

	ASSERT_DEBUG(fp != NULL, "ofile_dup: NULL fp");

	LOCK_SHARED_OFILE(oft, s, is_shared);

	if ((u_int) fd >= oft->oft_nofile) {
		UNLOCK_SHARED_OFILE(oft, s, is_shared);
		return EBADF;
	}

	/*
	 * Install new one, bumping reference count.  Remember old `fp'.
	 *
	 * Note: no check for same fp (should be rare, and this is correct
	 * anyhow).
	 */

	FDBUMP(fp);
	ofp = &oft->oft_ofile[fd];
	oldfp = ofp->of_file;
	ofp->of_file = fp;
	ofp->of_flags = 0;
	if (fd > oft->oft_lastfile)
		oft->oft_lastfile = fd;

	UNLOCK_SHARED_OFILE(oft, s, is_shared);

	/*
	 * If just dup'd over something, release old one.
	 */

	if (oldfp)
		closef(oldfp);

	return 0;
}

/*
 * ofile_get_flags()
 *	Return open file flags.  Maintains nice structured interface.
 *
 * Returns 0 for success, else error code.
 */

ofile_get_flags(oft, fd, flagsp)
	register struct	ofile_tab *oft;
	int	fd;
	int	*flagsp;
{
	bool_t	is_shared = 0;
	spl_t	s;

	LOCK_SHARED_OFILE(oft, s, is_shared);

	if ((u_int)fd >= oft->oft_nofile || oft->oft_ofile[fd].of_file == NULL){
		UNLOCK_SHARED_OFILE(oft, s, is_shared);
		return EBADF;
	}

	*flagsp = oft->oft_ofile[fd].of_flags & UF_EXCLOSE;

	UNLOCK_SHARED_OFILE(oft, s, is_shared);

	return 0;
}

/*
 * ofile_set_flags()
 *	Set open-file flags.  Maintains nice structured interface.
 *
 * Returns 0 for success, else error code.
 */

ofile_set_flags(oft, fd, flags)
	register struct	ofile_tab *oft;
	int	fd;
	int	flags;
{
	bool_t	is_shared = 0;
	spl_t	s;

	LOCK_SHARED_OFILE(oft, s, is_shared);

	if ((u_int)fd>=oft->oft_nofile || oft->oft_ofile[fd].of_file == NULL) {
		UNLOCK_SHARED_OFILE(oft, s, is_shared);
		return EBADF;
	}

	oft->oft_ofile[fd].of_flags &= ~UF_EXCLOSE;	/* only settable bit */
	oft->oft_ofile[fd].of_flags |= (flags & UF_EXCLOSE);

	UNLOCK_SHARED_OFILE(oft, s, is_shared);

	return 0;
}

/*
 * ofile_close()
 *	Atomicly return file-table entry and clear entry in open file table.
 *	This is "close" as far as the ofile_tab is concerned: the slot is free.
 *
 * Clear of_flags and update oft_lastfile if appropriate.
 * Returns `fp' without additional reference.  Caller will deal with this.
 *
 * Returns file-table entry, or NULL if bad index or non-existent entry.
 */

struct file *
ofile_close(oft, fd)
	register struct ofile_tab *oft;
	int	fd;
{
	register struct	ofile	*ofp;
	struct	file	*fp;
	bool_t	is_shared = 0;
	spl_t	s;

	LOCK_SHARED_OFILE(oft, s, is_shared);

	if ((unsigned) fd < oft->oft_nofile) {
		ofp = &oft->oft_ofile[fd];
		fp = ofp->of_file;
		if (fp != NULL) {
			ofp->of_file = NULL;
			ofp->of_flags = 0;
			/*
			 * If closing high-water mark, find new high-water.
			 */
			if (fd == oft->oft_lastfile) {
				for (--ofp; ofp >= oft->oft_ofile; ofp--)
					if (ofp->of_file != NULL)
						break;
				oft->oft_lastfile = (ofp - oft->oft_ofile);
			}
		}
	} else
		fp = NULL;

	UNLOCK_SHARED_OFILE(oft, s, is_shared);

	return fp;
}

/*
 * ofile_close_all()
 *	Close all ofile table entries matching a flag mask.
 *
 * Currently, all callers hold the only reference (ie, the ofile_tab isn't
 * shared).
 *
 * Avoid use of ofile pointers since process can swap in closef().
 *
 * Also, adjust oft_lastfile.
 */

static
ofile_close_all(oft, mask)
	register struct ofile_tab *oft;
	int	mask;
{
	register struct file *fp;
	register int i;

	ASSERT_DEBUG(!OFILE_SHARED(oft), "ofile_close_all: shared");

	for (i = oft->oft_lastfile; i >= 0; --i) {
		fp = oft->oft_ofile[i].of_file;
		if (fp != NULL && (mask == 0 || (oft->oft_ofile[i].of_flags & mask))) {
			oft->oft_ofile[i].of_file = NULL;
			oft->oft_ofile[i].of_flags = 0;
			closef(fp);
		}
	}

	if (mask == 0) {
#ifdef	DEBUG
		for (i = 0; i < oft->oft_nofile; i++) {
			ASSERT(oft->oft_ofile[i].of_file == NULL,
						"ofile_close_all: skew");
		}
#endif	DEBUG
		oft->oft_lastfile = -1;
	} else {
		for (i = oft->oft_lastfile;
				i>=0 && oft->oft_ofile[i].of_file == NULL; --i)
			continue;
		oft->oft_lastfile = i;
	}
}

/*
 * ofile_capacity()
 *	Adjust capacity of ofile_tab object -- change # ofile's it can hold.
 *
 * Supports growing and shrinking, and shared ofile_tab's.
 *
 * Imposes absolute max number fd's to avoid problems in select() (must be able
 * to fit 6 fd_set's in an 8k buffer).  For MAXBSIZE=8192 and NFDBITS=32, this
 * works out to 10912 (should be enough for really bizzare applications ;-)
 * This is a strange reason for a limit.
 *
 * Returns 0 for success, else error number.
 */

ofile_capacity(oft, nfds)
	register struct ofile_tab *oft;
	register int	nfds;
{
	register struct ofile_tab *noft;
	struct	ofile	*tof;
	bool_t	is_shared = 0;
	u_int	szuse;
	spl_t	s;

	/*
	 * Insure nfds no larger than system-defined max, but at least Nofile.
	 */

	nfds = MIN(nfds, ((MAXBSIZE * NBBY) / 6) & ~(NFDBITS-1));
	nfds = MIN(nfds, max_nofile);
	nfds = MAX(nfds, Nofile);

	/*
	 * Allocate a new ofile_tab, fill out for correct size.
	 */

	noft = ofile_new(nfds);
	if (noft == NULL)
		return ENFILE;

	if (!ofile_alloc_mem(noft)) {
		ofile_free(noft);
		return ENOMEM;
	}

	/*
	 * If new ofile table memory is "extended" and current table is
	 * shared, flush other processors TLB's for the new address range,
	 * since they may access the table before context switching (or
	 * otherwise flushing TLB).
	 *
	 * If not shared, it takes a fork to get that way and new process(es)
	 * will context switch (and flush TLB) before accessing table.
	 * Ok to test shared outside lock: only race is with ofile_deref().
	 */

	if (OFILE_SHARED(oft) && noft->oft_ofile != noft->oft_lofile)
		FlushTLB((caddr_t) noft->oft_ofile, SZ_OFILE(noft->oft_nofile));

	/*
	 * Move relevant fd's into new table, zap old table.
	 * Then swap memory between new and old open-file table objects.
	 */

	LOCK_SHARED_OFILE(oft, s, is_shared);

	szuse = SZ_OFILE(MIN(nfds, oft->oft_nofile));

	if (nfds == oft->oft_nofile) {
		/*
		 * Rare: request that doesn't change size.  NOP.
		 */
	} else if (oft->oft_nofile == Nofile) {
		/*
		 * New ofile_tab larger than current which is "standard" size.
		 * New lofile[] array is zero since it was just allocated.
		 */
		bcopy((caddr_t)oft->oft_ofile, (caddr_t)noft->oft_ofile, szuse);
		bzero((caddr_t)oft->oft_ofile, szuse);
		oft->oft_ofile = noft->oft_ofile;
		noft->oft_ofile = noft->oft_lofile;
	} else if (nfds == Nofile) {
		/*
		 * New ofile_tab is "standard" size which is smaller than
		 * current size.  Copy head of current extended array into
		 * current lofile[], zap 1st Nofile entries.
		 */
		bcopy((caddr_t)oft->oft_ofile, (caddr_t)oft->oft_lofile, szuse);
		bzero((caddr_t)oft->oft_ofile, szuse);
		noft->oft_ofile = oft->oft_ofile;
		oft->oft_ofile = oft->oft_lofile;
	} else {
		/*
		 * Both old and new ofile_tab's are non-standard size.
		 * Copy/zap relevant part from old to new, then swap memory.
		 * Copy up to nofile to get outstanding UF_ALLOC flags
		 * (concurrent opens).
		 */
		bcopy((caddr_t)oft->oft_ofile, (caddr_t)noft->oft_ofile, szuse);
		bzero((caddr_t)oft->oft_ofile, szuse);
		tof = oft->oft_ofile;
		oft->oft_ofile = noft->oft_ofile;
		noft->oft_ofile = tof;
	}

	/*
	 * Set nofile, lastfile fields.
	 */

	noft->oft_nofile = oft->oft_nofile;
	noft->oft_lastfile = (nfds < oft->oft_nofile) ? oft->oft_lastfile : -1;
	oft->oft_nofile = nfds;

	oft->oft_lastfile = nfds-1;
	while (oft->oft_lastfile >= 0
	&&     oft->oft_ofile[oft->oft_lastfile].of_file == NULL)
		--oft->oft_lastfile;

	/*
	 * If it's shared, others need to see the mapping change.
	 */

	UNLOCK_SHARED_OFILE(oft, s, is_shared);

	/*
	 * "New" ofile_tab may now be released.
	 */

	ofile_deref(noft);

	return 0;
}

/*
 * ofile_addref()
 *	Add a new reference to an ofile_tab object.
 *
 * Called during fork() when processes want to share their open-file table.
 * Returns "self" to make calling sequences cleaner.
 */

struct ofile_tab *
ofile_addref(oft)
	register struct ofile_tab *oft;
{
	spl_t	s;

	LOCK_OFILE(oft, s);
	ASSERT_DEBUG(oft->oft_refcnt > 0, "ofile_addref: oft_refcnt");
	++oft->oft_refcnt;
	UNLOCK_OFILE(oft, s);
	return oft;
}

/*
 * ofile_deref()
 *	Drop a reference to an ofile_tab object.
 *
 * If referece count will reach zero, close all non-null file-table
 * entries and release object.
 *
 * Used in exit(), execve() and other places when process no longer needs an
 * open file table object.
 */

ofile_deref(oft)
	register struct ofile_tab *oft;
{
	spl_t	s;

	ASSERT_DEBUG(oft->oft_refcnt > 0, "ofile_deref: refcnt");

	LOCK_OFILE(oft, s);

	if (--oft->oft_refcnt == 0) {
		UNLOCK_OFILE(oft, s);
		ofile_close_all(oft, 0);
		ofile_free_mem(oft);
		ofile_free(oft);
	} else {
		UNLOCK_OFILE(oft, s);
	}
}

/*
 * ofile_exclose()
 *	Close open file table entries marked "close on exec".
 *
 * Caller insures this is an unshared open-file table object.
 */

ofile_exclose(oft)
	struct ofile_tab *oft;
{
	ofile_close_all(oft, UF_EXCLOSE);
}

/*
 * ofile_clone()
 *	Duplicate an open-file table object.
 *
 * Copies ofile entries and bump ref-counts on file-table entries.
 * Used in fork() and execve().
 *
 * All fd's in new table will appear as "dup'd" with anyone else sharing
 * the table; eg, will see mods to f_offset (as if just forked).
 *
 * Returns new ofile_tab or NULL for failure (also returns error code).
 */

struct ofile_tab *
ofile_clone(oft, errorp)
	register struct ofile_tab *oft;
	char	*errorp;
{
	register struct ofile_tab *nft;
	register struct file *fp;
	register int	i;
	bool_t	is_shared = 0;
	int	nofile;
	spl_t	s;

	/*
	 * Allocate space for new file descriptors.
	 * Since ofile_alloc_mem() may block, we cannot lock oft
	 * until later.  Therefore, during the alloc, nofile may change.
	 * We check for this case and retry as needed.
	 * Seems like a low probability case: I'm forking/execing
	 * and someone sharing oft is changing its size?
	 */

	for (;;) {
		nofile = oft->oft_nofile;

		nft = ofile_new(nofile);
		if (nft == NULL) {
			*errorp = ENFILE;
			return NULL;
		}
		if (!ofile_alloc_mem(nft)) {
			ofile_free(nft);
			*errorp = ENOMEM;
			return NULL;
		}

		LOCK_SHARED_OFILE(oft, s, is_shared);

		if (nofile == oft->oft_nofile)
			break;

		UNLOCK_SHARED_OFILE(oft, s, is_shared);
		ofile_free(nft);
	}

	/*
	 * Dup relevant fd's into new ofile table.
	 *
	 * Awkward code courtesy PCC compiler; simpler is:
	 *	nft->oft_ofile[i] = oft->oft_ofile[i];
	 *	fp = oft->oft_ofile[i].of_file;
	 *	FDBUMP(fp);
	 */

	for (i = 0; i <= oft->oft_lastfile; i++) {
		fp = oft->oft_ofile[i].of_file;
		nft->oft_ofile[i].of_file = fp;
		nft->oft_ofile[i].of_flags = oft->oft_ofile[i].of_flags;
		if (fp)
			FDBUMP(fp);
	}
	nft->oft_lastfile = oft->oft_lastfile;

	UNLOCK_SHARED_OFILE(oft, s, is_shared);

	return nft;
}

/*
 * ofile_new()
 *	Allocate an open-file table object.
 *
 * Does not guarantee there will be memory to implement the object.
 *
 * Note: could allocate from the "heap"; these algorithms only use the
 * linked-list to keep track of a boot-time allocated set of structures;
 * there is no other need to have an array of ofile_tab's.
 *
 * Returns ofile_tab pointer or NULL if fail.
 */

static struct ofile_tab *
ofile_new(nfds)
	int	nfds;
{
	register struct ofile_tab *oft;
	spl_t	s;

	LOCK_OFILE_LIST(s);
	if ((oft = ofile_free_list) == NULL) {
		UNLOCK_OFILE_LIST(s);
		tablefull("open-file");
		return NULL;
	}
	ofile_free_list = oft->oft_next;
#ifdef	DEBUG
	--ofile_num_free;
#endif	DEBUG
	UNLOCK_OFILE_LIST(s);

	oft->oft_nofile = nfds;
	oft->oft_lastfile = -1;
	oft->oft_refcnt = 1;
	oft->oft_ofile = NULL;
	return oft;
}

/*
 * ofile_free()
 *	Put an ofile_tab back on free-list.
 *
 * See comments in ofile_new() on possible use of heap.
 */

static
ofile_free(oft)
	register struct ofile_tab *oft;
{
	spl_t	s;

	LOCK_OFILE_LIST(s);
	oft->oft_next = ofile_free_list;
	ofile_free_list = oft;
#ifdef	DEBUG
	++ofile_num_free;
#endif	DEBUG
	UNLOCK_OFILE_LIST(s);
}

/*
 * ofile_alloc_mem()
 *	Try to allocate memory for an open-file table object.
 *
 * Zeroes the memory of the array.
 *
 * Returns true for success, else false.
 */

static
ofile_alloc_mem(oft)
	register struct ofile_tab *oft;
{
	int	szofile = SZ_OFILE(oft->oft_nofile);

	/*
	 * If small enough, no extra memory needed.
	 * In this case, array is already zeroed (inductive; close and
	 * close_all insure anything allocated is zeroed).
	 *
	 * Otherwise, allocate memory.
	 */

	if (oft->oft_nofile <= Nofile)
		oft->oft_ofile = oft->oft_lofile;
	else if (oft->oft_ofile = (struct ofile *) wmemall(szofile, 1))
		bzero((caddr_t) oft->oft_ofile, (u_int) szofile);

	return (oft->oft_ofile != NULL);
}

/*
 * ofile_free_mem()
 *	Free memory associated with an open-file table object.
 */

static
ofile_free_mem(oft)
	register struct ofile_tab *oft;
{
	if (oft->oft_ofile != oft->oft_lofile)
		wmemfree((caddr_t) oft->oft_ofile, SZ_OFILE(oft->oft_nofile));
	oft->oft_ofile = NULL;
}
