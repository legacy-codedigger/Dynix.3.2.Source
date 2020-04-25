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
static	char	rcsid[] = "$Header: heap_kmem.c 1.9 91/04/03 $";
#endif

/*
 * kernel heap management routines
 */

/* $Log:	heap_kmem.c,v $
 */

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

 /*
  * Conditions on use:
  * kmem_alloc and kmem_free must not be called from interrupt level,
  * except from software interrupt level.  This is because they are
  * not reentrant, and only block out software interrupts.  They take
  * too long to block any real devices.  There is a routine
  * kmem_free_intr that can be used to free blocks at interrupt level,
  * but only up to splimp, not higher.  This is because kmem_free_intr
  * only spl's to splimp.
  *
  * Also, these routines are not that fast, so they should not be used
  * in very frequent operations (e.g. operations that happen more often
  * than, say, once every few seconds).
  */
/*
 * description:
 *	Yet another memory allocator, this one based on a method
 *	described in C.J. Stephenson, "Fast Fits", IBM Sys. Journal
 *
 *	The basic data structure is a "Cartesian" binary tree, in which
 *	nodes are ordered by ascending addresses (thus minimizing free
 *	list insertion time) and block sizes decrease with depth in the
 *	tree (thus minimizing search time for a block of a given size).
 *
 *	In other words, for any node s, letting D(s) denote
 *	the set of descendents of s, we have:
 *
 *	a. addr(D(left(s))) <  addr(s) <  addr(D(right(s)))
 *	b. len(D(left(s)))  <= len(s)  >= len(D(right(s)))
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/map.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/kernel.h"
#include "../h/vm.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/pte.h"

/*
 * The node header structure.
 * 
 * To reduce storage consumption, a header block is associated with
 * free blocks only, not allocated blocks.
 * When a free block is allocated, its header block is put on 
 * a free header block list.
 *
 * This creates a header space and a free block space.
 * The left pointer of a header blocks is used to chain free header
 * blocks together.
 */

typedef struct	freehdr	*Freehdr;
typedef struct	dblk	*Dblk;

/*
 * Description of a header for a free block
 * Only free blocks have such headers.
 */
struct 	freehdr	{
	Freehdr	left;			/* Left tree pointer */
	Freehdr	right;			/* Right tree pointer */
	Dblk	block;			/* Ptr to the data block */
	u_int	size;			/* Size of the data block */
};

#define NIL		((Freehdr)0)
#define WORDSIZE	sizeof(int)
#define	SMALLEST_BLK	1	 	/* Size of smallest block */

/*
 * Description of a data block.  
 */
struct	dblk	{
	char	data[1];		/* Addr returned to the caller */
};

/*
 * weight(x) is the size of a block, in bytes; or 0 if and only if x
 * is a null pointer. It is the responsibility of kmem_alloc() and
 * kmem_free() to keep zero-length blocks out of the arena.
 */

#define	weight(x)	((x) == NIL? 0: ((x)->size))
#define	nextblk(p, size) ((Dblk) ((char *)(p) + (size)))
#define	max(a, b)	((a) < (b)? (b): (a))

static	Freehdr	getfreehdr();
static	caddr_t	morecore();
static	caddr_t	getpages();
caddr_t	kmem_alloc();

/*
 * Structure containing various info about allocated memory.
 */
#define	NEED_TO_FREE_SIZE	10
struct kmem_info {
	Freehdr	free_root;
	Freehdr	free_hdr_list;
	struct map *map;
	struct pte *pte;
	caddr_t	vaddr;
	lock_t	kmem_lock;
	lock_t	need_listlock;
	struct need_to_free {
		caddr_t addr;
		u_int	nbytes;
	} need_to_free_list,need_to_free[NEED_TO_FREE_SIZE];
} kmem_info;

#ifdef	PERFSTAT
struct kmem_stat {
	u_long	ks_heap_pages;		/* # pages allocated to heap */
	u_long	ks_alloc_cnt;		/* # calls to kmem_alloc() */
	u_long	ks_free_cnt;		/* # calls to kmem_free() */
	u_long	ks_insert_cnt;		/* # calls to insert() */
	u_long	ks_delete_cnt;		/* # calls to delete() */
	u_long	ks_demote_cnt;		/* # calls to demote() */
	u_long	ks_nosplit;		/* # times chunk not split */
	u_long	ks_split;		/* # times aligned chunk split */
	u_long	ks_algnsplit;		/* # times chunk split to align */
	u_long	ks_split_free;		/* # kmem_free calls during split */
	u_long	ks_hdr_cnt;		/* # times got hdr from list */
	u_long	ks_hdralloc_cnt;	/* # times alloc'ed memory for hdrs */
	u_long	ks_free_thread;		/* # times kmem_free_intr threads */
	u_long	ks_free_list;		/* # times kmem_free_intr adds to list*/
	u_long	ks_lbolt_core;		/* # times lbolt wait for morecore */
	u_long	ks_lbolt_hdr;		/* # times lbolt wait for hdrs */
} kmem_stat;
#endif	PERFSTAT


/*
 * Initialize kernel memory allocator
 * Called by main().
 */
kmem_init()
{
	register int i;
	register struct need_to_free *ntf;

	kmem_info.free_root = NIL;
	kmem_info.free_hdr_list = NULL;
	kmem_info.map = uptmap;
	kmem_info.pte = Usrptmap;
	kmem_info.vaddr = (caddr_t)usrpt;
	init_lock(&kmem_info.kmem_lock, G_UPT);

	init_lock(&kmem_info.need_listlock, G_UPT);
	kmem_info.need_to_free_list.addr = 0;
	ntf = kmem_info.need_to_free;
	for (i = 0; i < NEED_TO_FREE_SIZE; i++) {
		ntf[i].addr = 0;
	}
}

/*
 * Insert a new node in a cartesian tree or subtree, placing it
 * in the correct position with respect to the existing nodes.
 *
 * algorithm:
 *	Starting from the root, a binary search is made for the new
 *	node. If this search were allowed to continue, it would
 *	eventually fail (since there cannot already be a node at the
 *	given address); but in fact it stops when it reaches a node in
 *	the tree which has a length less than that of the new node (or
 *	when it reaches a null tree pointer).  The new node is then
 *	inserted at the root of the subtree for which the shorter node
 *	forms the old root (or in place of the null pointer).
 *
 * Called with kmem_info.kmem_lock held at SPLIMP.
 */
static
insert(p, len, tree, newhdr)
	register Dblk p;		/* Ptr to the block to insert */
	register u_int len;		/* Length of new node */
	register Freehdr *tree;		/* Address of ptr to root */
	Freehdr newhdr;			/* hdr to use when inserting */
{
	register Freehdr x;
	register Freehdr *left_hook;	/* Temp for insertion */
	register Freehdr *right_hook;	/* Temp for insertion */

#ifdef	PERFSTAT
	kmem_stat.ks_insert_cnt++;
#endif	PERFSTAT

	x = *tree;
	/*
	 * Search for the first node which has a weight less
	 *	than that of the new node; this will be the
	 *	point at which we insert the new node.
	 */

	while (weight(x) >= len) {	
		if (p < x->block)
			tree = &x->left;
		else
			tree = &x->right;
		x = *tree;
	}

	/*
	 * Perform root insertion. The variable x traces a path through
	 * the tree, and with the help of left_hook and right_hook,
	 * rewrites all links that cross the territory occupied
	 * by p. Note that this improves performance under paging.
	 */ 

	*tree = newhdr;
	left_hook = &newhdr->left;
	right_hook = &newhdr->right;

	newhdr->left = NIL;
	newhdr->right = NIL;
	newhdr->block = p;
	newhdr->size = len;

	while (x != NIL) {
		/*
		 * Remark:
		 *	The name 'left_hook' is somewhat confusing, since
		 *	it is always set to the address of a .right link
		 *	field.  However, its value is always an address
		 *	below (i.e., to the left of) p. Similarly
		 *	for right_hook. The values of left_hook and
		 *	right_hook converge toward the value of p,
		 *	as in a classical binary search.
		 */
		if (x->block < p) {
			/*
			 * rewrite link crossing from the left
			 */
			*left_hook = x;
			left_hook = &x->right;
			x = x->right;
		} else {
			/*
			 * rewrite link crossing from the right
			 */
			*right_hook = x;
			right_hook = &x->left;
			x = x->left;
		} /*else*/
	} /*while*/

	*left_hook = *right_hook = NIL;		/* clear remaining hooks */

} /*insert*/

/*
 * Delete a node from a cartesian tree. p is the address of
 * a pointer to the node which is to be deleted.
 *
 * algorithm:
 *	The left and right sons of the node to be deleted define two
 *	subtrees which are to be merged and attached in place of the
 *	deleted node.  Each node on the inside edges of these two
 *	subtrees is examined and longer nodes are placed above the
 *	shorter ones.
 *
 * On entry:
 *	*p is assumed to be non-null.
 *
 * Called with kmem_info.kmem_lock held at SPLIMP.
 */
static
delete(p)
	register Freehdr *p;
{
	register Freehdr x;
	register Freehdr left_branch;	/* left subtree of deleted node */
	register Freehdr right_branch;	/* right subtree of deleted node */

#ifdef	PERFSTAT
	kmem_stat.ks_delete_cnt++;
#endif	PERFSTAT

	x = *p;
	left_branch = x->left;
	right_branch = x->right;

	while (left_branch != right_branch) {	
		/*
		 * iterate until left branch and right branch are
		 * both NIL.
		 */
		if (weight(left_branch) >= weight(right_branch)) {
			/*
			 * promote the left branch
			 */
			*p = left_branch;
			p = &left_branch->right;
			left_branch = left_branch->right;
		} else {
			/*
			 * promote the right branch
			 */
			*p = right_branch;
			p = &right_branch->left;
			right_branch = right_branch->left;
		}/*else*/
	}/*while*/
	*p = NIL;
	freehdr(x);
} /*delete*/


/*
 * Demote a node in a cartesian tree, if necessary, to establish
 * the required vertical ordering.
 *
 * algorithm:
 *	The left and right subtrees of the node to be demoted are to
 *	be partially merged and attached in place of the demoted node.
 *	The nodes on the inside edges of these two subtrees are
 *	examined and the longer nodes are placed above the shorter
 *	ones, until a node is reached which has a length no greater
 *	than that of the node being demoted (or until a null pointer
 *	is reached).  The node is then attached at this point, and
 *	the remaining subtrees (if any) become its descendants.
 *
 * on entry:
 *   a. All the nodes in the tree, including the one to be demoted,
 *	must be correctly ordered horizontally;
 *   b. All the nodes except the one to be demoted must also be
 *	correctly positioned vertically.  The node to be demoted
 *	may be already correctly positioned vertically, or it may
 *	have a length which is less than that of one or both of
 *	its progeny.
 *   c. *p is non-null
 *
 * Called with kmem_info.kmem_lock held at SPLIMP.
 */
static
demote(p)
	register Freehdr *p;
{
	register Freehdr x;		/* addr of node to be demoted */
	register Freehdr left_branch;
	register Freehdr right_branch;
	register u_int    wx;

#ifdef	PERFSTAT
	kmem_stat.ks_demote_cnt++;
#endif	PERFSTAT

	x = *p;
	left_branch = x->left;
	right_branch = x->right;
	wx = weight(x);

	while (weight(left_branch) > wx || weight(right_branch) > wx) {
		/*
		 * select a descendant branch for promotion
		 */
		if (weight(left_branch) >= weight(right_branch)) {
			/*
			 * promote the left branch
			 */
			*p = left_branch;
			p = &left_branch->right;
			left_branch = *p;
		} else {
			/*
			 * promote the right branch
			 */
			*p = right_branch;
			p = &right_branch->left;
			right_branch = *p;
		} /*else*/
	} /*while*/

	*p = x;				/* attach demoted node here */
	x->left = left_branch;
	x->right = right_branch;
} /*demote*/

/*
 * Allocate a block of storage
 *
 * algorithm:
 *	The freelist is searched by descending the tree from the root
 *	so that at each decision point the "better fitting" child node
 *	is chosen (i.e., the shorter one, if it is long enough, or
 *	the longer one, otherwise).  The descent stops when both
 *	child nodes are too short.
 *
 * function result:
 *	kmem_alloc returns a pointer to the allocated block; a null
 *	pointer indicates storage could not be allocated.
 */
/*
 * We need to return blocks that are on word boundaries so that callers
 * that are putting int's into the area will work.  Since we allow
 * arbitrary free'ing, we need a weight function that considers
 * free blocks starting on an odd boundary special.  Allocation is
 * aligned to 4 byte boundaries (ALIGN).
 */
#define	ALIGN		sizeof(int)
#define	ALIGNMASK	(ALIGN-1)
#define	ALIGNMORE(addr)	(ALIGN - ((int)(addr) & ALIGNMASK))

#define	mweight(x) ((x) == NIL ? 0 : \
    ((((int)(x)->block) & ALIGNMASK) && ((x)->size > ALIGNMORE((x)->block)))\
    ? (x)->size - ALIGNMORE((x)->block) : (x)->size)

caddr_t
kmem_alloc(nbytes)
	register u_int	nbytes;
{
	register Freehdr a;		/* ptr to node to be allocated */
	register Freehdr *p;		/* address of ptr to node */
	register u_int	 left_weight;
	register u_int	 right_weight;
	register Freehdr left_son;
	register Freehdr right_son;
	register char	 *retblock;	/* Address returned to the user */
	spl_t s;

#ifdef	PERFSTAT
	kmem_stat.ks_alloc_cnt++;
#endif	PERFSTAT

	if (nbytes == 0) {
		return(NULL);
	}

	if (nbytes < SMALLEST_BLK) {
		printf("illegal kmem_alloc call for %d bytes\n", nbytes);
		panic("kmem_alloc");
		/*
                 *+ The kernel memory allocator was requested to allocate
                 *+ a memory block smaller than the smallest allowable size.
                 *+ This indicates a kernel software problem.
                 */
	}
	check_need_to_free();

	s = p_lock(&kmem_info.kmem_lock, SPLIMP);

	/*
	 * ensure that at least one block is big enough to satisfy the request.
	 */
	while (mweight(kmem_info.free_root) < nbytes) {
		/*
		 * the largest block is not enough. 
		 */
		v_lock(&kmem_info.kmem_lock, s);
		retblock = morecore(nbytes);
		if (retblock != (caddr_t)0) {
			return(retblock);
		}
#ifdef	HEAP_DEBUG
		printf("kmem_alloc failed, nbytes %d\n", nbytes);
#endif	HEAP_DEBUG
		p_sema(&lbolt, PRSWAIT);
		check_need_to_free();
		s = p_lock(&kmem_info.kmem_lock, SPLIMP);
#ifdef	PERFSTAT
		kmem_stat.ks_lbolt_core++;
#endif	PERFSTAT
	}

	/*
	 * search down through the tree until a suitable block is
	 * found. At each decision point, select the better fitting node.
	 */

	p = (Freehdr *) &kmem_info.free_root;
	a = *p;
	left_son = a->left;
	right_son = a->right;
	left_weight =  mweight(left_son);
	right_weight = mweight(right_son);

	while (left_weight >= nbytes || right_weight >= nbytes) {
		if (left_weight <= right_weight) {
			if (left_weight >= nbytes) {
				p = &a->left;
				a = left_son;
			} else {
				p = &a->right;
				a = right_son;
			}
		} else {
			if (right_weight >= nbytes) {
				p = &a->right;
				a = right_son;
			} else {
				p = &a->left;
				a = left_son;
			}
		}
		left_son =  a->left;
		right_son = a->right;
		left_weight =  mweight(left_son);
		right_weight = mweight(right_son);
	} /*while*/	

	/*
	 * allocate storage from the selected node.
	 */
	
	if ((a->size - nbytes) < SMALLEST_BLK) {
		/*
		 * not big enough to split; must leave at least
		 * a dblk's worth of space.
		 */
#ifdef	PERFSTAT
		kmem_stat.ks_nosplit++;
#endif	PERFSTAT
		retblock = a->block->data;
		delete(p);
	} else {

		/*
		 * split the node, allocating nbytes from the top.
		 * Remember we've already accounted for the
		 * allocated node's header space.
		 */
		if ((int) a->block->data & ALIGNMASK &&
		    a->size > ALIGNMORE(a->block->data)) {

			int	alm;		/* ALIGNMORE factor */
			u_int	alsize;		/* aligned size */

#ifdef	PERFSTAT
			kmem_stat.ks_algnsplit++;
#endif	PERFSTAT
			alm = ALIGNMORE(a->block->data);
			retblock = a->block->data + alm;

			/*
			 * Re-use this header.
			 */
			alsize = a->size - alm;
			a->size = alm;
			/*
			 * the node pointed to by *p has become smaller;
			 * move it down to its appropriate place in the tree.
			 */
			demote(p);

			if (alsize > nbytes) {
#ifdef	PERFSTAT
				kmem_stat.ks_split_free++;
#endif	PERFSTAT
				/*
				 * place trailing bytes back into the heap.
				 */
				v_lock(&kmem_info.kmem_lock, s);
				kmem_free(retblock + nbytes,
					    (u_int)alsize - nbytes);
				return (retblock);
			}
		} else {
#ifdef	PERFSTAT
			kmem_stat.ks_split++;
#endif	PERFSTAT
			retblock = a->block->data;
			a->block = nextblk(a->block, nbytes);
			a->size -= nbytes;
			/*
			 * the node pointed to by *p has become smaller;
			 * move it down to its appropriate place in the tree.
			 */
			demote(p);
		}
	}
#ifdef	HEAP_DEBUG
	printf("kmem_alloc returning %x\n", retblock);
	prtree(kmem_info.free_root, "kmem_alloc");
#endif	HEAP_DEBUG
	v_lock(&kmem_info.kmem_lock, s);
	return (retblock);

} /*malloc*/

/*
 * Return a block to the free space tree.
 * 
 * algorithm:
 *	Starting at the root, search for and coalesce free blocks
 *	adjacent to one given.  When the appropriate place in the
 *	tree is found, insert the given block.
 *
 * Do some sanity checks to avoid total confusion in the tree.
 * If the block has already been freed, panic.
 * If the ptr is not from the arena, panic.
 */
kmem_free(ptr, nbytes)
	caddr_t ptr;
	register u_int 	 nbytes;	/* Size of node to be released */
{
	register Freehdr *np;		/* For deletion from free list */
	register Freehdr neighbor;	/* Node to be coalesced */
	register char	 *neigh_block;	/* Ptr to potential neighbor */
	register u_int	 neigh_size;	/* Size of potential neighbor */
	register Freehdr newhdr;
	spl_t s;

#ifdef	HEAP_DEBUG
	printf("kmem_free. ptr %x nbytes %d\n", ptr, nbytes);
	s = p_lock(&kmem_info.kmem_lock, SPLIMP);
	prtree(kmem_info.free_root, "kmem_free");
	v_lock(&kmem_info.kmem_lock, s);
#endif	HEAP_DEBUG

#ifdef	PERFSTAT
	kmem_stat.ks_free_cnt++;
#endif	PERFSTAT

	if (nbytes == 0) {
		return;
	}

	/*
	 * check bounds of pointer.
	 */
	if (ptr < (caddr_t)usrpt ||
	    ptr > (caddr_t) usrpt + (Usrptsize * NBPG)) {
		printf("kmem_free: illegal pointer %x\n",ptr);
		panic("kmem_free");
                /*
                 *+ The kernel memory allocator was requested to free
                 *+ a memory block whose starting address was outside
                 *+ the range of allowable addresses.
                 *+ This indicates a kernel software problem.
                 */
	}

	newhdr = NIL;
	s = p_lock(&kmem_info.kmem_lock, SPLIMP);

search:
	/*
	 * Search the tree for the correct insertion point for this
	 * node, coalescing adjacent free blocks along the way.
	 */
	np = &kmem_info.free_root;
	neighbor = *np;
	while (neighbor != NIL) {
		neigh_block = (char *)neighbor->block;
		neigh_size = neighbor->size;
		if (ptr < neigh_block) {
			if ((ptr + nbytes) == neigh_block) {
				/*
				 * Absorb and delete right neighbor
				 */
				nbytes += neigh_size;
				delete(np);
			} else if ((ptr + nbytes) > neigh_block) {
				/*
				 * The block being freed overlaps
				 * another block in the tree.  This
				 * is bad news.
				 */
				 printf("kmem_free: free block overlap %x+%d over %x\n", ptr, nbytes, neigh_block);
				 panic("kmem_free: free block overlap");
                                /*
                                 *+ The kernel memory allocator was requested
                                 *+ to free a memory block that overlaps
                                 *+ another different memory block managed by
                                 *+ the allocator.
                                 *+ This indicates a kernel software problem.
                                 */
			} else {
				/*
				 * Search to the left
			 	*/
				np = &neighbor->left;
			}
		} else if (ptr > neigh_block) {
			if ((neigh_block + neigh_size) == ptr) {
				/*
				 * Absorb and delete left neighbor
				 */
				ptr = neigh_block;
				nbytes += neigh_size;
				delete(np);
			} else if ((neigh_block + neigh_size) > ptr) {
				/*
				 * This block has already been freed
				 */
				panic("kmem_free block already free");
                                /*
                                 *+ The kernel memory allocator was requested
                                 *+ to free a memory block that is not
                                 *+ currently allocated.
                                 *+ This indicates a kernel software problem.
                                 */
			} else {
				/*
				 * search to the right
				 */
				np = &neighbor->right;
			}
		} else {
			/*
			 * This block has already been freed
			 * as "ptr == neigh_block"
			 */
			panic("kmem_free: block already free as neighbor");
                        /*
                         *+ The kernel memory allocator was requested to free
                         *+ a memory block that is not currently allocated.
                         *+ This indicates a kernel software problem.
                         */
		} /*else*/
		neighbor = *np;
	} /*while*/

	if (newhdr == NIL) {
		if (kmem_info.free_hdr_list != NIL) {
#ifdef	PERFSTAT
			kmem_stat.ks_hdr_cnt++;
#endif	PERFSTAT
			newhdr = getfreehdr(&s);
		} else {
#ifdef	PERFSTAT
			kmem_stat.ks_hdralloc_cnt++;
#endif	PERFSTAT
			/*
			 * If the free_hdr_list is empty then we may 
			 * block acquiring the header. If so, the tree may
			 * look quite a bit different upon return. Therefore,
			 * search the tree again. But only once more.
			 */
			newhdr = getfreehdr(&s);
			goto search;
		}
	}

	/*
	 * Insert the new node into the free space tree
	 */
	insert((Dblk) ptr, nbytes, &kmem_info.free_root, newhdr);

#ifdef	HEAP_DEBUG
	printf("exiting kmem_free\n");
	prtree(kmem_info.free_root, "kmem_free");
#endif	HEAP_DEBUG

	v_lock(&kmem_info.kmem_lock, s);
} /*free*/

/*
 * We maintain a list of blocks that need to be freed.
 * This is because we don't want to spl the relatively long
 * routines malloc and free, but we need to be able to free
 * space at interrupt level.
 */
kmem_free_intr(ptr, nbytes)
	caddr_t ptr;
	register u_int 	 nbytes;	/* Size of node to be released */
{
	register int i;
	register struct need_to_free *ntf;
	spl_t s;

 	if (nbytes >= sizeof(struct need_to_free)) {
 		if ((int)ptr & ALIGNMASK) {
 			i = ALIGNMORE(ptr);
 			kmem_free_intr(ptr, (u_int)i);
 			kmem_free_intr(ptr + i, nbytes - i);
 			return;
 		}
		s = p_lock(&kmem_info.need_listlock, SPLIMP);
#ifdef	PERFSTAT
		kmem_stat.ks_free_thread++;
#endif	PERFSTAT
 		ntf = &kmem_info.need_to_free_list;
 		*(struct need_to_free *)ptr = *ntf;
 		ntf->addr = ptr;
 		ntf->nbytes = nbytes;
 		v_lock(&kmem_info.need_listlock, s);
 		return;
 	}

	/*
	 * Chunk to small to thread.
	 */
	s = p_lock(&kmem_info.need_listlock, SPLIMP);
#ifdef	PERFSTAT
	kmem_stat.ks_free_list++;
#endif	PERFSTAT
	ntf = kmem_info.need_to_free;
	for (i = 0; i < NEED_TO_FREE_SIZE; i++) {
		if (ntf[i].addr == 0) {
			ntf[i].addr = ptr;
			ntf[i].nbytes = nbytes;
			v_lock(&kmem_info.need_listlock, s);
			return;
		}
	}
	/*
	 * No space left in list. This should never happen as current
	 * implementation only frees MCLT_KHEAP mbufs at interrupt level
	 * and these are typically large.
	 */
	panic("kmem_free_intr");
        /*
         *+ During interrupt processing,
         *+ the kernel memory allocator was requested to free
         *+ a memory block and an
         *+ internal table overflowed.
         *+ This indicates a kernel software problem.
         */
}

static
check_need_to_free()
{
	register int i;
	register struct need_to_free *ntf;
	caddr_t addr;
	u_int nbytes;
	spl_t s;

again:
	s = p_lock(&kmem_info.need_listlock, SPLIMP);
	ntf = &kmem_info.need_to_free_list;
	if (ntf->addr) {
		addr = ntf->addr;
		nbytes = ntf->nbytes;
		*ntf = *(struct need_to_free *)ntf->addr;
		v_lock(&kmem_info.need_listlock, s);
		kmem_free(addr, nbytes);
		goto again;
	}
	ntf = kmem_info.need_to_free;
	for (i = 0; i < NEED_TO_FREE_SIZE; i++) {
		if (ntf[i].addr) {
			addr = ntf[i].addr;
			nbytes = ntf[i].nbytes;
			ntf[i].addr = 0;
			v_lock(&kmem_info.need_listlock, s);
			kmem_free(addr, nbytes);
			goto again;
		}
	}
	v_lock(&kmem_info.need_listlock, s);
}

/*
 * morecore
 *	Add a block of at least nbytes to the free space tree.
 *
 * return value:
 *	non-zero - address of newly allocated memory
 *	zero - No memory available
 *
 * remark:
 *	free space is extended by an amount determined by
 *	rounding nbytes up to a multiple of the system page size.
 */

static caddr_t
morecore(nbytes)
	u_int nbytes;
{
	u_int	totbytes;
	caddr_t p;

	totbytes = (nbytes + CLOFSET) & ~CLOFSET;
	p = getpages(totbytes >> PGSHIFT);
#ifdef	PERFSTAT
	if (p)
		kmem_stat.ks_heap_pages += (totbytes >> PGSHIFT);
#endif	PERFSTAT

	if (p && (totbytes != nbytes)) {
		/*
		 * If any memory allocated and more was allocated
		 * than required put any leftover memory into the heap.
		 */
		kmem_free(p + nbytes, totbytes - nbytes);
	}
	return (p);
}

/*
 * get npages pages from the system
 */
static caddr_t
getpages(npages)
	u_int npages;
{
	register caddr_t va;
	register long x;

	x = uptalloc((long)npages, 1);
	if (memall(&(kmem_info.pte)[x], (int)npages, &proc[0], CSYS) == 0) {
		/* No real memory available in the system */
		uptfree((long)npages, x);
		va = ((caddr_t)0);
	} else { 
		/* We got our memory */
		va = kmem_info.vaddr + (x << PGSHIFT);
		vmaccess(&(kmem_info.pte)[x], va, (int)npages);
	}

#ifdef	HEAP_DEBUG
	printf("getpages returning 0x%x, %d pages, x was %d\n", va, npages, x);
#endif	HEAP_DEBUG
	return (va);
}

/*
 * Get a free block header
 * There is a list of available free block headers.
 * When the list is empty, allocate another pagefull.
 *
 * Called from kmem_free() with kmem_info.kmem_lock held at SPLIMP.
 */
static Freehdr
getfreehdr(s)
	spl_t	*s;	/* Saved IPL */
{
	Freehdr r;
	int	n;

retry:
	if (kmem_info.free_hdr_list != NIL) {
		r = kmem_info.free_hdr_list;
		kmem_info.free_hdr_list = kmem_info.free_hdr_list->left;
	} else {
		v_lock(&kmem_info.kmem_lock, *s);
		r = (Freehdr) getpages(CLSIZE);
		if (r == 0) {
			/*
			 * avoid panic. Try again later.
			 */
			p_sema(&lbolt, PRSWAIT);
			*s = p_lock(&kmem_info.kmem_lock, SPLIMP);
#ifdef	PERFSTAT
			kmem_stat.ks_lbolt_hdr++;
#endif	PERFSTAT
			goto retry;
		}
		*s = p_lock(&kmem_info.kmem_lock, SPLIMP);
		for (n = 1; n < CLBYTES / sizeof(*r); n++) {
			freehdr(&r[n]);
		}
	}
	return (r);
}

/*
 * Free a free block header
 * Add it to the list of available headers.
 *
 * Called with kmem_info.kmem_lock held at SPLIMP.
 */
static
freehdr(p)
	Freehdr	p;
{
	p->left = kmem_info.free_hdr_list;
	p->right = NIL;
	p->block = NULL;
	kmem_info.free_hdr_list = p;
}

#ifdef	HEAP_DEBUG
/*
 * Diagnostic routines
 */
static depth = 0;

static
prtree(p, cp)
	Freehdr p;
	char *cp;
{
	int n;
	if (depth == 0) {
		printf("prtree. p %x cp %s\n", p, cp);
/*
	} else {
		printf("prtree. p %x depth %d\n", p, depth);
*/
	}
	if (p != NIL){
		depth++;
		prtree(p->left, (char *)NULL);
		depth--;

		for (n = 0; n < depth; n++) {
			printf("   ");
		}
		printf(
		     "(%x): (left = %x, right = %x, block = %x, size = %d)\n",
			p, p->left, p->right, p->block, p->size);

		depth++;
		prtree(p->right, (char *)NULL);
		depth--;
	}
}
#endif	HEAP_DEBUG
