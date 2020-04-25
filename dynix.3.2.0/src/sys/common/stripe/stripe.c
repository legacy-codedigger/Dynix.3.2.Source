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

/*	$Header: stripe.c 1.4 1991/07/09 00:06:37 $	*/

#ifndef lint
static  char    rcsid[] = "$Header: stripe.c 1.4 1991/07/09 00:06:37 $";
#endif

/*
 * stripe.c
 * 	disk striping psuedo driver.
 * 	
 * Things (hopefully) worth noting about this implementation
 * that future implementations or maintenance should consider:
 *	
 *	a) The driver is unable to detect if a stripe is
 *	   configured the way it was when initially written 
 *	   upon.  The administrator must assure this.  An
 *	   implementation using volume labels could prevent
 *	   this (not currently available).
 *
 *	b) Stripes cannot be used for the root filesystem
 *	   and swap space, since the stripe configurations
 *	   are not available to the standalone boot utilities
 *	   nor the kernel until after single user boot.
 *	
 *	c) Writing directly to physical partitions from which 
 *	   a stripe is composed can corrupt the stripe's data. 
 *	   The administrator must prevent this from occurring.
 *	   Again, volume labeling could prevent this.
 *	
 *	d) The stripe block size must be either the maximum 
 *	   filesystem block size or a multiple thereof.  We chose
 *	   the max since it would be difficult to assess the size
 *	   being used prior to building the filesystem on the raw
 *	   stripe. Currently this value is 8k.  Note also that
 *	   the current implementation only supports 4k and 8k.
 *
 *	   This restriction is due to the manner that existing
 *	   kernel code optimizes block I/O and dma setups - they 
 *	   assume the PTE addressed by the buf structure describes
 *	   the data buffer to read from or write into, and that
 *	   these buffers are page and/or cluster aligned (offset
 *	   offset zero).  Since this type of I/O only requests
 *	   data from within a filesystem block, this restriction
 *	   prevents the driver from trying to doctor up the buf
 *	   while fragmenting it - currently not able to do.  Raw
 *	   I/O has no such restriction.
 *	
 *	e) Note that the device numbers in the stripe configuration
 *	   are assumed to be block devices and that the driver
 *	   uses their bdevswitch entries rather blindly.  Again,
 *	   administrators must use care!
 *
 *	f) A stripes total size must be less than two gigabytes,
 *	   due to 32 bit signed offsets used for things such as
 *	   the seek call.  This limitation must be generally be
 *	   addressed by Dynix in the future.
 *	   the future.
 *	
 * Possible performance enhancements that need further investigation:
 *
 *	a) Start fragments as soon as they are constructed, instead
 *	   of after all fragments are constructed.  Starting head
 *	   movement sooner might speed things up.  It would also
 *	   eliminate the need for the linked lists which might lower
 *	   our overhead as long as we don't add lock trips.  Since
 *	   block I/O is not fragmented, this benefits raw I/O only.
 *
 *	b) Find a way to short circuit block I/O requests so they
 *	   don't have to contend with the local buf pool.  Since
 *	   they always result in one fragment, some way to save
 *	   and restore info with the original buf structure would
 *	   speed things up and save synchronization bottlenecks.
 *	
 *	c) There is one lock per stripe unit for synchonizing 
 *	   packet termination handling for all requests on a given
 *	   device.  If there was one lock per original request
 *	   then multiple requests to the same stripe wouldn't 
 *	   have to synchronize with each other.  Problem is there
 * 	   is not a lock in the original buf structure, except 
 *	   for the one located in the buf structure's wait semaphore,
 *	   which is not currently in use.  This would be an UGLY
 *	   solution and I'm not sure its worth it.
 */

/* $Log: stripe.c,v $
 *
 *
 *
 */

#define	STRIPEMACS
#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/conf.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../stripe/stripe.h"

/*
 * Local variables for managing
 * the local buf pool.
 */
static int freecount;		/* Free buf list counter 		    */
static sema_t freewait;		/* Free buf list wait mechanism when empty  */
static lock_t freelock;		/* Mutex for accessing the free buf list    */
static struct buf freebufs;	/* Free buf list head for allocating bufs   */
static struct buf *bufpool;	/* Array of bufs allocated for local pool   */

static int stripe_devices;	/* The number of configured stripe devices.
				 * Configured into the kernel build file,
				 * passed to and saved by stripeboot(). 
				 * Must be less than or equal to 
				 * MAX_STRIPE_DEVICES defined in stripe.h
				 */
static stripeinfo_t *stripe_info; /* Info table for managing stripes */
static void stripe_breakup();	/* Forward reference		     */
static stripe_done();		/* Forward reference		     */
static void stripe_queue();	/* Forward reference		     */
static struct buf *stripe_getbuf();/* Forward refernce 	             */
static void stripe_relbuf();	/* Forward refernce 		     */

extern int nblkdev;		/* Size of the block device switch table */
extern caddr_t calloc();

/*
 * stripeboot()
 *	Invoked from conf_pseudo() to perform
 *	necessary boot-time initialization for
 *	the striping driver.  Its argument is
 *	the number of stripes the kernel is
 *	configured for.
 */
stripeboot(n)
	u_long n;
{
	register struct buf *fp, *bp;
	register int i;
	register stripeinfo_t *infop;

	/*
	 * Paranoia: Make sure configurable parameters 
	 * and reasonable allocated/initialized before
	 * making "stripe_devices" non-zero. 
	 */
	if (n == 0) {
		printf("stripeboot: no stripe devices configured.\n"); 
		/*
		 *+ There were no stripe devices configured in the kernel.
		 */
		return;
	} else if (nstripebufs <= 0) {
		printf("stripeboot: 'nstripebufs' must be greater than ");
		printf("zero in conf_stripe.c - no stripes configured.\n");
		/*
		 *+ The "nstripebuf" variable in conf_stripe.c must be
		 *+ configured to be greater than zero.
		 */
		return;
	} else if (stripe_maxbufs <= 0 || stripe_maxbufs > nstripebufs) {
		printf("stripeboot: stripe_maxbufs not 0 < x <= nstripebufs ");
		printf("in conf_stripe.c - no stripes configured.\n");
		/*
		 *+ The maximum number of stripe buffers is not within
		 *+ the range of 0 and nstripebufs.
		 */
		return;
	} else if (n > MAX_STRIPE_DEVICES) {
		printf("stripeboot: %d configured exceeds limit; %d assumed.\n",
			n, MAX_STRIPE_DEVICES);
		/*
		 *+ The number of stripe devices configured exceeds the
		 *+ maximum allowable (MAX_STRIPE_DEVICES).
		 */
		n = MAX_STRIPE_DEVICES;
	}

	/* 
	 * Attempt to allocate resources for 
	 * managing the stripe, its configuration,
	 * and stats.
 	 */
	stripe_info = (stripeinfo_t *)calloc((int)n * sizeof(stripeinfo_t));

	/* 
	 * Complete initialization for the
	 * stripe info table.
 	 */
	for (i = 0, infop = stripe_info; i < n; i++, infop++) {
		init_sema(&infop->user_sync, 1, 0, stripegate);
		init_lock(&infop->buf_lock, stripegate);
	}

	/* 
	 * Set up the local buf pool.
	 * Allocate bufs and chain 
	 * together on free list, the
	 * head of which is freebufs.
	 */
	bufpool = (struct buf *)calloc(nstripebufs * sizeof(struct buf));

	freecount = nstripebufs;
	init_lock(&freelock, stripegate);
	init_sema(&freewait, 0, 0, stripegate);
	fp = &freebufs;			/* Pointer to the free list */
	bhashself(fp);			/* Init empty - point to self */
	fp->b_flags = 0;		/* Initialize flags */

	for (i=0, bp = bufpool; i < nstripebufs; i++, bp++) { 
		bufinit(bp, stripegate);
			/* inits b_iowait(0 - not done) and 
		 	 * b_alloc(1 - available) semas in bp.
			 * should use bufalloc/buffree instead 
			 * sleep/wakeup */

		binshash(bp,fp); 	/* Add local bufs to free list */
	}

	stripe_devices = n;		/* Initialization succeeded */
}

/*
 * stripeopen()
 * 	Attempt to open the stripe device.
 *	Validate the minor device number and
 *	verify that it is not being opened for
 *	exclusive access by other than the superuser
 *	on its first open.
 */
stripeopen(dev,flag)
	dev_t dev;
	int flag;
{	
	register i;
	stripeinfo_t *infop;
	register dev_t *dp;
	int retval=0, limit;

	if (minor(dev) < 0 || minor(dev) >= stripe_devices) {
#ifdef STRIPE_DEBUG
		printf("stripeopen: failure - minor number %d out of range.\n",
			minor(dev));
		/*
		 *+ debug only.
		 */
#endif
		return ENODEV;
	}

	infop = &stripe_info[minor(dev)];
	p_sema(&infop->user_sync, PRIBIO);  /* sync. open/close/ioctl */

	/*
	 * Verify that the stripe is accessible.
	 */
	if (infop->exclusive_open) {
#ifdef STRIPE_DEBUG
		printf("stripeopen: failure - device busy w/exclusive open.\n");
		/*
		 *+ debug only.
		 */
#endif
		retval = EBUSY;
		goto exit_open;
	} else if (flag & FEXCL) {
		if (infop->n_opens != 0) {
#ifdef STRIPE_DEBUG
			printf("stripeopen: failure - device busy while ");
			printf("attempting exclusive open.\n");
			/*
			 *+ debug only.
			 */
#endif
			retval = EBUSY;
			goto exit_open;
		} else if (!suser()) {
#ifdef STRIPE_DEBUG
			printf("stripeopen: failure - not superuser while ");
			printf("trying exclusive open.\n");
			/*
			 *+ debug only.
			 */
#endif
			retval = EPERM;
			goto exit_open;
		}
	}

	
	/*
	 * If this stripe is already open, then don't 
	 * open the physical devices again since they 
	 * should already be open to the stripe driver.  
	 * Just count the new reference and do the 
	 * converse when closing.
	 *
	 * Otherwise, attempt to open the underlying 
	 * devices that make up the stripe.  If one 
	 * fails close those that were opened and 
	 * return an error.
	 *
	 * If the stripe configuration has not been
	 * loaded yet, the loop will be skipped.
	 */ 
    	if (!infop->n_opens) {
		for (i=0, limit = sndev(&infop->config, 0), 
		     dp = &infop->config.st_dev[0]; i < limit; i++, dp++) {
			retval = (*bdevsw[major(*dp)].d_open)(*dp, flag);
			if (retval != 0) {
				/*
				 * Warn the caller of configuration
				 * failure.  If the device is not already
				 * open then clear this configuration
				 * so the caller may reload a valid one
				 * after the next open.  Otherwise, leave
				 * it be so that active I/O on other opens
				 * shut themselves down with device errors,
				 * this config can be cleared afterwards.
				 */
				printf("stripeopen: ds%d failure %d on open of ",
					minor(dev), retval);
				printf("dev %d (major %d, minor %d).\n",
						i, major(*dp), minor(*dp));
				/*
				 *+ A failure was encountered when
				 *+ trying to open this particular stripe
				 *+ device.
				 */
				if (!infop->n_opens) {
					/* Clear the stripe config table */
					sndev(&infop->config, 0) = 0;	
				}
				while(i--) {
					dp--;
					(*bdevsw[major(*dp)].d_close)(*dp, flag);
				}
				goto exit_open;
			}
    		}
    	}

	infop->n_opens++;		/* Successful open - note reference */
	if (flag & FEXCL) 
		infop->exclusive_open = 1;
exit_open:
	v_sema(&infop->user_sync);
	return (retval);
}

/*
 * stripeclose()
 *	Close the stripe device. 
 * 	Decrement its reference count
 * 	and close the underlying devices 
 *	if this is the last close.	
 */
stripeclose(dev,flag)
	dev_t dev;
	int flag;
{	
	stripeinfo_t *infop = &stripe_info[minor(dev)];
	register int i = 0;
	register dev_t *dp = &infop->config.st_dev[0];
	int limit = sndev(&infop->config, 0);

	p_sema(&infop->user_sync, PRIBIO);  /* sync. open/close/ioctl */

	if (--infop->n_opens == 0) {
		/* 
       	         * Last close, close the
		 * underlying devices. 
		 * Also clear exclusive access
		 * flag that may be set.
		 */
		for (; i < limit; i++, dp++) 
			(*bdevsw[major(*dp)].d_close)(*dp, flag);
		infop->exclusive_open = 0;
	}

	v_sema(&infop->user_sync);
}
	
/*
 * stripestrat()
 *	Strategy routine for stripe driver.
 *	Validates that the device is configured
 *	and then breaks the request into multiple
 *	requests to the underlying physical devices.
 */
stripestrat(bp)
	register struct buf *bp;
{	
	stripeinfo_t *infop = &stripe_info[minor(bp->b_dev)];
	struct buf *p, *q;
	spl_t s;

#define ADDRALIGN       16
	
	/*
	 * Fail the request if this stripe's
	 * configuration has not yet been
	 * loaded into the kernel.
	 */
	if (sndev(&infop->config, 0) == 0) {
#ifdef STRIPE_DEBUG
		printf("stripestrat: stripe configuration not loaded ");
		printf("for /dev/ds%d\n", minor(bp->b_dev));
		/*
		 *+ debug only.
		 */
#endif
		bp->b_flags |= B_ERROR;
                bp->b_error = EINVAL;
                bp->b_resid = bp->b_bcount;
                biodone(bp);
                return;
	}

	/*
	 * Fail the request if it contains a bogus byte count,
	 * size is not a device block multiple, the buffer is
	 * not properly aligned, or the transfer is not entirely
	 * within the striped partition.
	 */
	if (bp->b_bcount <= 0
	||  bp->b_bcount & DEV_BSIZE - 1 
	||  bp->b_iotype == B_RAWIO && ((int)bp->b_un.b_addr & ADDRALIGN - 1) 
	||  bp->b_blkno < 0
	||  bp->b_blkno + howmany(bp->b_bcount,DEV_BSIZE) > 
	    infop->config.st_total_size) {
		bp->b_flags |= B_ERROR;
                bp->b_error = EINVAL;
                bp->b_resid = bp->b_bcount;
                biodone(bp);
                return;
	}

	s = p_lock(&infop->buf_lock, SPLBUF);
	infop->qlen++;			/* # requests currently on the queue */
	infop->bcount += howmany(bp->b_bcount,DEV_BSIZE);
	v_lock(&infop->buf_lock, s);

	/*
	 * Reset the fragment list by making the 
	 * original bp's b_actl point back to this
	 * bp so that the end of the frag list starts 
	 * out as the front of it.  Fragments are
	 * added to the end 
	 */
	bp->b_actf = NULL;
	bp->b_actl = bp;	
	bp->b_resid = 0;		/* # fragments we broke request into */
	bp->b_error = 0;
	stripe_breakup(bp, infop);	/* Breaks request into fragments */

	/*
	 * bp is now the head of a linked list of 
	 * request fragments to start.  Follow
	 * the links (in b_actf) and start the
	 * real requests.  To avoid a race with
	 * the completion function, locate the
	 * next fragment before starting the 
	 * current one.
	 * The fragment bufs also have a link back
	 * to the real buf (b_forw), which is the 
	 * list head.  It will be used by the 
	 * completion function.
	 */
	ASSERT(bp->b_actf, "stripestrat: empty queue, nothing to start");
	/*
	 *+ The stripe strategy routine thought that were was work
	 *+ to do, but the list was empty.
	 */

#ifdef notyet
	/*
	 * Just a test to verify that file and page 
	 * table I/O will always fit within one file 
	 * system block, of which the stripe block is
	 * a multiple.  If so, there should never be 
	 * more than one fragment.  In my testing, this
	 * assertion never failed, leaving open the future
	 * posibility to optimize the breakup process 
	 * to use the original buf instead allocating 
	 * one from the local buf pool, which still needs
	 * more thought.
	 */
	ASSERT(bp->b_iotype == B_RAWIO || bp->b_resid == 1,
	    "stripestrat: more than one fragment for a non-raw I/O");
	/*
	 *+ debug only.
	 */
#endif 
	for (q = bp->b_actf; q; ) {
		p = q;
		q = p->b_actf;
		p->b_back = p->b_actf = p->b_actl = NULL;
		(*bdevsw[major(p->b_dev)].d_strategy)(p); 
	}
	return;
}

/*
 * striperead()
 *	Standard raw read procedure.
 */
striperead(dev, uio)
	dev_t dev;
	struct uio *uio;
{	
	int err, diff;
	stripeinfo_t *infop = &stripe_info[minor(dev)];
	off_t lim = infop->config.st_total_size << DEV_BSHIFT;

	if ((err = physck(lim, uio, B_READ, &diff)) != 0) 
		/* When -1 its not an error, but a request of 0 bytes */
                return ((err == -1) ? 0 : err);  

	err = physio(stripestrat, (struct buf *)0, dev, 
			B_READ, (int (*)())minphys, uio);
	uio->uio_resid += diff;
	return (err);
}

/*
 * stripewrite()
 *	Standard raw write procedure.
 */
stripewrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{	
	int err, diff;
	stripeinfo_t *infop = &stripe_info[minor(dev)];
	off_t lim = infop->config.st_total_size << DEV_BSHIFT;

	if ((err = physck(lim, uio, B_WRITE, &diff)) != 0) 
		/* When -1 its not an error, but a request of 0 bytes */
                return ((err == -1) ? 0 : err);  

	err = physio(stripestrat, (struct buf *)0, dev, 
			B_WRITE, (int (*)())minphys, uio);
	uio->uio_resid += diff;
	return (err);
}
	
/*
 * stripesize()
 *	Used for swap-space partition calculations.
 */
stripesize(dev)
	dev_t dev;
{	
	stripeinfo_t *infop = &stripe_info[minor(dev)];
	return (infop->config.st_total_size);
}
	
/*
 * stripeioctl()
 *	I/O control operations for stripe driver.
 *	Note that access to the stripe configuration
 *	must be synchronized with open and close to
 *	insure data stability.  
 */
stripeioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	char **data;
	int flag;
{	
	stripeinfo_t *infop = &stripe_info[minor(dev)];
	stripe_t stripetmp;
	register stripe_t *t = &infop->config;
	register dev_t *dp;
	int i, prevdevs, ndevs, retval=0;
	long nblocks, nextstart, size;

	p_sema(&infop->user_sync, PRIBIO);

	/* 
	 * The copyin and copyout below are 
	 * a kludge way of fixing a limitation 
	 * in the ioctl code, that transfers must 
	 * be less than 128 bytes. 
	 */
	switch (cmd) {
	case STPUTTABLE:
		/*
		 * Reload this stripe's configuration.
		 * This operation is limited to the
		 * superuser, who must be the only one
		 * with the device open.
		 */
		if (!suser() || infop->n_opens > 1) {
			retval = EPERM;
			goto exit_ioctl;
		}

		/* 
		 * Copy in the new configuration from
		 * user land.  If this fails, leave
		 * the configuration as before.
		 */
		if (retval = copyin(*data, (caddr_t)&stripetmp, sizeof(stripe_t))) 
			goto exit_ioctl;	/* Copyin failed */

		if (stripetmp.st_total_size > MAX_DEV_BLKS) {
#ifdef STRIPE_DEBUG
			printf("stripeioctl: ds%d total size of %d blocks ",
				minor(dev), stripetmp.st_total_size);
			printf(" exceeds current maximum of %d.\n",
				MAX_DEV_BLKS);
			/*
			 *+ debug only.
			 */
#endif
			retval = EINVAL;
			goto exit_ioctl;

		}

		/*
		 * Verify that the major device numbers are
		 * at least in the block device table.
		 */
		for (i = 0, ndevs = sndev(&stripetmp, 0); 
		     i < sndev(&stripetmp, 0); i++) 
			if (major(stripetmp.st_dev[i]) >= nblkdev) {
#ifdef STRIPE_DEBUG
				printf("stripeioctl: ds%d device %d, major#%d",
				    minor(dev), i, major(stripetmp.st_dev[i]));
				printf(" not in block device table.\n");
				/*
				 *+ debug only.
				 */
#endif
				retval = EINVAL;
				goto exit_ioctl;

			}

		/* 
		 * Some more sanity checking to verify that
		 * the configured sizes match the total
		 * size of the stripe.  Verify that each
		 * section description contains fewer
		 * devices than the previous one, that
		 * they all add up correctly to the stripe's
		 * total block size, and that the start 
		 * address of each section correctly follows
		 * the previous one.  Also, the stripe block
		 * and section sizes must be a multiple of 
		 * MIN_SBLK, so other than RAWIO does not 
		 * straddle stripe blocks (see stripequeue).
		 */
		prevdevs = MAX_STRIPE_PARTITIONS + 1;
		nextstart = nblocks = i = 0;
		for ( ; i < MAX_STRIPE_PARTITIONS; i++, prevdevs = ndevs) {
			ndevs = sndev(&stripetmp, i);
			size = ssize(&stripetmp, i);
#ifndef STRIPE_DEBUG
			if (ndevs < 0 
			||  ndevs > 0 && (ndevs >= prevdevs 
			   		 || sstart(&stripetmp, i) != nextstart)
			||  ndevs == 0 && size != 0
			||  sblock(&stripetmp, i) > size
			||  size % MIN_SBLK
			||  sblock(&stripetmp, i) % MIN_SBLK) {
				retval = EINVAL;
				goto exit_ioctl;
			}
#else
			if (ndevs < 0) {
				printf("stripeioctl: ds%d section %d, ndevs ",
					minor(dev), i);
				printf("is less than zero.\n");
				/*
				 *+ A sanity check has failed while checking
				 *+ for problems with stripe devices.
				 */
				retval = EINVAL;
				goto exit_ioctl;
			} else if (ndevs > 0) {
				if (ndevs >= prevdevs) {
					printf("stripeioctl: ds%d section %d," ,
						minor(dev), i);
					printf("ndevs >= previous section.\n");
					/*
					 *+ A sanity check has failed while checking
					 *+ for problems with stripe devices.
					 */
					retval = EINVAL;
					goto exit_ioctl;
				} else if (sstart(&stripetmp, i) != nextstart) {
					printf("stripeioctl: ds%d section %d," ,
						minor(dev), i);
					printf("unexpected start block.\n");
					/*
					 *+ A sanity check has failed while checking
					 *+ for problems with stripe devices.
					 */
					retval = EINVAL;
					goto exit_ioctl;
				}
			} else if (ndevs == 0 && size != 0) {
				printf("stripeioctl: ds%d section %d size ",
					minor(dev), i);
				printf("(%d) must be zero.\n", size);
				/*
				 *+ A sanity check has failed while checking
				 *+ for problems with stripe devices.
				 */
				retval = EINVAL;
				goto exit_ioctl;
			} 

			if (sblock(&stripetmp, i) > size) {
				printf("stripeioctl: ds%d stripe block size ",
					minor(dev));
				printf("(%d) larger than section size (%d).\n",
					sblock(&stripetmp, i), size);
				/*
				 *+ A sanity check has failed while checking
				 *+ for problems with stripe devices.
				 */
				retval = EINVAL;
				goto exit_ioctl;
			} else if (size % MIN_SBLK) {
				printf("stripeioctl: ds%d section size (%d) ",
					minor(dev), size);
				printf("not multiple of the minimum stripe ");
				printf("block size(%d).\n", MIN_SBLK);
				/*
				 *+ A sanity check has failed while checking
				 *+ for problems with stripe devices.
				 */
				retval = EINVAL;
				goto exit_ioctl;
			} else if (sblock(&stripetmp, i) % MIN_SBLK) {
				printf("stripeioctl: ds%d interleave factor ",
					minor(dev));
				printf("(%d) not multiple of the minimum ",
					sblock(&stripetmp, i)); 
				printf("stripe block size(%d).\n", MIN_SBLK);
				/*
				 *+ A sanity check has failed while checking
				 *+ for problems with stripe devices.
				 */
				retval = EINVAL;
				goto exit_ioctl;
			}
#endif
			size *= ndevs;
			nblocks += size;
			nextstart += size;
		}

		if (nblocks != stripetmp.st_total_size) {
			retval = EINVAL;
			goto exit_ioctl;
		}

		/*
		 * Close the underlying devices and
		 * then replace the old configuration
		 * with the new one.
 		 */ 
		for (i = 0, dp = &t->st_dev[0]; i < sndev(t, 0); i++, dp++)
		      (*bdevsw[major(*dp)].d_close)(*dp, flag);
		bcopy((caddr_t)&stripetmp, (caddr_t)t, sizeof(stripe_t));

		/*
		 * Now open the devices from the new
		 * configuration. If this fails, close
		 * those that have been opened and mark
		 * the configuration as not loaded.
		 */
		for (i = 0, dp = &t->st_dev[0]; i < sndev(t, 0); i++, dp++)
		    if ((retval = 
			(*bdevsw[major(*dp)].d_open)(*dp, flag)) != 0) {
				printf("stripeioctl: failure %d on open ",
					retval);
				printf("dev %d (major %d, minor %d).\n",
					i, major(*dp), minor(*dp));
				/*
				 *+ A failure occured while trying to open
				 *+ a stripe device.
				 */
			for (dp--; i-- > 0; dp--) 
				(*bdevsw[major(*dp)].d_close)(*dp,flag);
			sndev(t, 0) = 0;	/* In effect, clear the table */
			goto exit_ioctl;
		    }
		break; 		/* Successful operation */
	case STGETTABLE:
		/*
		 * Copy the configuration table
		 * of the specified device to the
		 * user's buffer.
		 */
		retval = copyout((caddr_t)t, *data, sizeof(stripe_t));
		break;
	default:
		retval = EINVAL;
		break;
	}

exit_ioctl:
	v_sema(&infop->user_sync);
	return (retval);
}

/* 
 * stripe_breakup()
 * 	Distribute the original request accross
 *	the devices that make up the stripe.
 *	Fragment the request into multiple
 *	smaller requests if needed.
 *
 *	The Algorithm is to locate the starting 
 *	stripe section linearly through the
 *	stripe configuration table of this unit.
 *	While part of the request fits in that
 *	stripe section, cycle through the stripe
 *	blocks in that section, issuing I/O
 *	requests to the devices to which they
 *	belong, determining the real device, its
 *	start block and length. When a stripe
 *	section has been exhausted in this manner,
 *	locate the next section and repeat until
 *	the requested amount of data is mapped.
 */
static void
stripe_breakup(bp, infop)
	struct buf *bp;	
	register stripeinfo_t *infop;
{	
	register stripe_t *t = &infop->config;
	register daddr_t block; 
	register int nblocks, nbytes;	
	register i;
	dev_t	dev;
	long devnbytes;
	daddr_t offset, devblk, dstart, devaddr;
	long substripe, group;
	int devnblk;

	block = bp->b_blkno;	/* The beginning block # of the request */
	nblocks = howmany(bp->b_bcount, DEV_BSIZE);  /* Total blocks to xfer */
	nbytes = bp->b_bcount;	/* Total number of bytes to xfer */


	/* 
	 * Locate the stripe section to
	 * start fragmenting the transfer
	 * against.
	 */
	devaddr = 0;
	dstart = 0;
	for (i=0; sndev(t,i) > 0 && ssize(t,i) > 0; i++) {
		if (block >= sstart(t,i) &&
			block < sstart(t,i) + ssize(t,i) * sndev(t,i))
			break;	/* It starts in stripe section 'i' */

		dstart += ssize(t,i);	/* Try the next stripe section */
	}

	/*
	 * Start stripe section has been
	 * located.  While part of the
	 * original request remains to
	 * be mapped into a fragment ...
	 */
	while (nblocks > 0) {
		/* Make certain this is a valid stripe section */
		if (sndev(t,i) == 0 || ssize(t,i) == 0)
			return;

		/* 
		 * While part of request is still 
		 * in current stripe section, set up
		 * an I/O request for a fragment for
		 * the underlying device.  This must
		 * account for interleaving.
		 */
		while (nblocks > 0 &&
		   (block - sstart(t,i)) < (ssize(t,i) * sndev(t,i)) )	{

			/* 
			 * Within this stripe section, locate the
			 * stripe block in that section, and physical
			 * block offset into that stripe block where
			 * I/O should occur.
			 */
			group = (block - sstart(t,i)) / sblock(t,i);
			offset = (block - sstart(t,i)) % sblock(t,i);

			/* 
			 * Determine which underlying device the
			 * stripe block belongs to and compute
			 * which stripe block on that device it
			 * is and the real device block address
			 * for this transfer fragment.
			 */
			dev = t->st_dev[group % sndev(t,i)];
			substripe = group / sndev(t,i);
			devblk = dstart + substripe * sblock(t,i) + offset;

			if (bdevsw[major(dev)].d_flags & B_TAPE) /* for tape */
				devblk /= sblock(t,i);

			/*
			 * Now determine how many physical blocks
			 * and bytes of the original request fit 
			 * into this stripe block on the real device
			 * and queue up a request for that fragment.
			 */
			devnblk = MIN((sblock(t,i)-offset),nblocks);
			devnbytes = MIN(devnblk * DEV_BSIZE, nbytes);
			stripe_queue(bp, devblk, devnbytes, devaddr, dev);

			/* 
			 * Adjust counters for next fragment,
			 * in the next stripe block, on the
			 * next device in this stripe section
			 * and repeat.
			 */
			block += devnblk;
			nblocks -= devnblk;
			nbytes -= devnbytes;
			devaddr += DEV_BSIZE * devnblk;
		}
		/*
		 * Adjust counters to the next stripe
		 * section and continue fragmenting the 
		 * request into stripe blocks in that 
		 * section on its underlying devices.
		 */
		dstart += ssize(t,i);
		i++;
	}
	ASSERT (nbytes == 0, "stripe_breakup: nbytes non zero on exit");
	/*
	 *+ After completing work with fragmenting a request, the
	 *+ striping driver discovered extraneous data.
	 */
}

/*
 * stripe_done()
 *	Called when a fragment of a transfer completes.
 *	Assume that bp->b_forw addresses the real bp for
 *	the entire stripe request, of which this is part.
 *	Note errors, adjust resid, and terminate real bp
 *	when all fragments have completed.
 */
static
stripe_done(bp)
	struct buf *bp;
{	
	
	register struct buf *realbp = bp->b_forw;
	stripeinfo_t *infop = &stripe_info[minor(realbp->b_dev)];
	spl_t s = p_lock(&infop->buf_lock, SPLBUF);

	/*
	 * Since this is a striped device, any
	 * fragment which errors or comes up short
	 * is fatal - mark the entire request as bad. 
	 */
	if (bp->b_flags & B_ERROR || bp->b_resid) 
		realbp->b_flags |= B_ERROR;

	if (--realbp->b_resid == 0) {	/* if last request */
		/*
		 * Last fragment on this part of
		 * the request. Awaken anyone waiting.
		 */
		infop->rcount++;
		infop->qlen--;
		v_lock(&infop->buf_lock, s);
		biodone(realbp);	  /* biodone without lock!! */
	} else	v_lock(&infop->buf_lock, s);

	stripe_relbuf(bp);	/* release the old buffer header */
}

/*
 * stripe_queue()
 *	Given a description of an I/O request 
 *	which is a fragment of the original 
 *	bp, build up a buf structure for it
 *	and add it to the list of fragments
 *	to be executed associated with the 
 *	actual request.  
 *	
 *	If the fragment threshold has been
 *	reached, then start the ones on the
 *	list and await their completion
 *	prior to completing this operation.
 */
static void
stripe_queue(bp, devblk, devnbytes, devaddr, dev)
	register struct buf *bp;
	dev_t dev;
	long devnbytes;
	daddr_t devblk, devaddr;
{	
	register struct buf *hd;	
	struct buf *p, *q;
	int (*call)() = NULL;

	if (bp->b_resid > stripe_maxbufs) {
		/* 
		 * The threshold for fragments on a
		 * single request has been reached.
		 * Start those already queued and
		 * await their completion prior to
		 * building this one.
		 */

		if (bp->b_flags & B_CALL) {	
			/* Dont call the real termination function yet... */
			call = bp->b_iodone;
			bp->b_flags &= ~B_CALL;
		}

		ASSERT(bp->b_actf, "stripe_queue: empty queue - deadlocked");
		/*
		 *+ A deadlock was encountered within the stripe driver.
		 */

		/*
		 * bp is the head of a linked list of 
		 * request fragments to start.  Follow
		 * the links (in b_actf) and start the
		 * real requests. To avoid a race with
		 * the completion function, locate the
		 * next fragment before starting the 
		 * current one.
		 * The fragment bufs also have a link back
		 * to the real buf (b_forw), which is the 
		 * list head.  It will be used by the 
		 * completion function.
		 */
		for (q = bp->b_actf; q; ) {
			p = q;
			q = p->b_actf;
			p->b_back = p->b_actf = p->b_actl = NULL;
			(*bdevsw[major(p->b_dev)].d_strategy)(p); 
		}

		biowait(bp);		/* Wait for packets to terminate */

		ASSERT(bp->b_resid == 0, "stripe_queue: queue not empty");
		/*
		 *+ The stripe driver discovered information on a queue
		 *+ that should have been empty.
		 */
		if (call)  { 
			/* Restore previously saved call information */
			bp->b_iodone = call; 
			bp->b_flags |= B_CALL; 
		}
		/*
		 * Reset the fragment list by making the 
		 * original bp's b_actl point back to this
		 * bp so that the end of the frag list starts 
		 * out as the front of it.  Fragments are
		 * added to the end 
		 */
		bp->b_actf = NULL;
		bp->b_actl = bp;	
	}

	/*
	 * Build a buf to perform a portion
	 * of this request with one of the 
	 * devices the stripe is made up of.
	 * Then link the local packet to the
	 * original buf, it will be started
	 * by the code above, or by strategy.
	 */
	hd = stripe_getbuf();		/* Allocate a buf for fragment */

	/* Fill in the buf for the fragment */
	hd->b_flags = (bp->b_flags & B_READ) | B_CALL;
	hd->b_error = 0;
	hd->b_dev = dev;		/* Real device for xfer */
	hd->b_blkno = devblk;		/* Real device block addr for xfer */
	hd->b_bufsize = hd->b_bcount = devnbytes;    /* Bytes to xfer */
	hd->b_resid = 0;
	hd->av_forw = hd->av_back = 0;	/* Just for cleanliness - in use */
	hd->b_proc = bp->b_proc;
	hd->b_forw = bp;		/* Link back to the original request */
	hd->b_iodone = stripe_done;	/* The routine called when done */

	/* 
	 * Special code for fragmenting block I/O.
	 * Stripe blocks/sections must be multiples of
	 * the file system maximum block size, MIN_SBLK.  
	 * This should prevent any block I/O from being
	 * fragmented across multiple stripe blocks,
	 * hence devices, and save us from making
	 * massive kernel changes for non-page aligned 
	 * dma buffers for the subsequent fragment.
	 * Future implementations should try to correct
	 * this so this restriction is not needed.
	 */
	switch (hd->b_iotype = bp->b_iotype) {
	case B_RAWIO:
		/*
		 * No page alignment guaranteed.  
		 * Assume the address is translated
		 * through the user's page table.
		 */
		hd->b_un.b_addr = bp->b_un.b_addr + devaddr; 
		break;
	case B_FILIO:
		/*
		 * A cluster aligned xfer that is
		 * a multiple of the device block
		 * size.
		 */
		if (devaddr % CLBYTES) {
		    printf("stripequeue: FILIO devaddr 0x%x not aligned.\n", 
			devaddr);
		    /*
		     *+ A cluster aligned transfer that is a multiple of
		     *+ the device block size was not aligned.
		     */
		    panic("stripe: Invalid assertion");
		    /*
		     *+ A problem occured with fragmenting block i/o in
		     *+ the striping driver.
		     */
		} 
		hd->b_un.b_addr = bp->b_un.b_addr + devaddr; 
		break;
	case B_PTEIO:
		/*
		 * A cluster aligned xfer that is
		 * a multiple of the cluster size.
		 * Use the buf pte.  Much like PTBIO.
		 */
	case B_PTBIO:
		/* 
		 * A page, but not necessarily cluster, 
		 * aligned xfer that is a multiple of
		 * the page size.  Use the buf pte.
		 */
		if (devaddr % CLBYTES) {
		    printf("stripequeue: PTE/B IO devaddr 0x%x not aligned.\n", 
			devaddr);
		    /*
		     *+ A page, but not necessarily cluster, aligned
		     *+ transfer that is a multiple of the device block
		     *+ size was not aligned.
		     */
		    panic("Invalid assertion");
		    /*
		     *+ A problem occured with fragmenting block i/o in
		     *+ the striping driver.
		     */
		} 
		hd->b_un.b_pte = bp->b_un.b_pte + devaddr / NBPG; 
		break;
	default:
		panic("stripequeue: unsupported buf i/o type encountered");
		/*
		 *+ An unsupported buffer i/o operation was encountered when
		 *+ fragmenting block i/o.
		 */
		break;
	}

	/*
	 * Insert the new fragment at the tail
	 * of the list for this request.  It is
	 * added to the tail so that requests
	 * are started in a more natural seek 
	 * order from the head of the list.
	 */
	bp->b_actl->b_actf = hd;
	bp->b_actl = hd;
	bp->b_resid++;			/* Number of fragments on list */
}

/*
 * stripe_getbuf()
 *	Allocate a buffer from the local
 *	buf pool.  If not currently
 *	available, wait until one is.
 */
static struct buf *
stripe_getbuf()
{	
	register struct buf *bp;
	spl_t s;

	s = p_lock(&freelock, SPLBUF);
	while (!freecount) {
		/* 
		 * Wait until a buffer is available.
		 * Once awakened, grab lock and try 
		 * again in case another process not
		 * previously waiting beats us to it.
		 */
		p_sema_v_lock(&freewait, PRIBIO, &freelock, s);
		s = p_lock(&freelock, SPLBUF);
	}

	freecount--;
	bp = freebufs.b_forw;
	bremhash(bp);			/* Remove the buffer from free list */
	v_lock(&freelock, s);
	bp->b_flags = 0;		/* Clear all misc. flags */
	return (bp);
}

/*
 * stripe_relbuf()
 *	Return a buffer to the local
 *	buf pool and notify waiting
 *	processes of its availability.
 */
static void
stripe_relbuf(bp)
	register struct buf *bp;
{
	spl_t s;

	s = p_lock(&freelock, SPLBUF);
	binshash(bp,&freebufs);
	freecount++;			/* Note that buf is now on free list */
	(void)cv_sema(&freewait);	/* Awaken first process waiting on it */
	v_lock(&freelock, s);
}
