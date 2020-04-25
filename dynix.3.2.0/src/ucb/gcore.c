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

#ifndef lint
static char rcsid[] = "$Header: gcore.c 2.9 91/03/28 $";
#endif

/*
 * gcore - get core images of running processes
 *
 * Author: Eric Cooper
 * Written: Fall 1981.
 *
 * Inspired by a version 6 program by Len Levin, 1978.
 * Several pieces of code lifted from Bill Joy's 4BSD ps.
 *
 * Permission to copy or modify this program in whole or in part is hereby
 * granted, provided that the above credits are preserved.
 *
 * This code performs a simple simulation of the virtual memory system in user
 * code.  If the virtual memory system changes, this program must be modified
 * accordingly.  It must also be recompiled whenever system data structures
 * change.
 */

#include <sys/types.h>
#include <stdio.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/vm.h>
#define	KERNEL
#include <sys/user.h>
#include <balance/pmap.h>
#undef	KERNEL
#include <sys/proc.h>
#include <machine/pte.h>
#include <setjmp.h>
#define	KERNEL
#undef FILE
#include <sys/vnode.h>
#undef	KERNEL
#include <sys/stat.h>
#include <sys/file.h>

extern off_t lseek();
extern char *malloc();
extern char *valloc();
extern char *strcpy(), *strcat();



#define	FPTE(x,y)	((struct fpte *)(x))->y

/* Various macros for efficiency. */

#define	min(a, b)	(a < b ? a : b)

#define	Seek(f, pos) {\
	if (lseek(f, (off_t) (pos), 0) != (off_t) (pos)) \
		panic("seek error"); \
}

#define	Read(f, addr, n) {\
	if (read(f, (char *) (addr), (int) (n)) != (int) (n)) \
		panic("read error"); \
}

#define Write(f, addr, n) {\
	if (write(f, (char *) (addr), (int) (n)) != (int) (n)) \
		panic("write error"); \
}

#define	Get(f, pos, addr, n) {\
	Seek(f, pos); \
	Read(f, addr, n); \
}

struct	nlist nl[] = {
	{ "_proc" },
#define	X_PROC		0
	{ "_nswap" },
#define	X_NSWAP		1
	{ "_nproc" },
#define	X_NPROC		2
	{ "_dmmin" },
#define	X_DMMIN		3
	{ "_dmmax" },
#define	X_DMMAX		4
	{ "_pmap_unit" },
#define	X_PMAP		5
	{ 0 },
};

#define FEW	64		/* for fewer system calls */
struct	proc proc[FEW];
struct	user *u1;
struct dmapext *dmep;
struct dmapext *smep;

int sizeof_dmapext;
long sizeof_dmapext_disk;

#define NLIST	"/dynix"
#define KMEM	"/dev/kmem"
#define MEM	"/dev/mem"
#define SWAP	"/dev/drum"	/* "/dev/swap" on some systems */

int	nproc;
int	nswap;
int	dmmin, dmmax;
off_t	pmap;
char	coref[20];
int	kmem, mem, swap, cor;
jmp_buf	cont_frame;

#ifdef	DEBUG
int	debug_flag = 0;
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	register int i, j;
	register struct proc *p;
	off_t procbase, procp;
	int pid, uid;

#ifdef DEBUG
	while(argv[1][0] == '-') {
		switch( argv[1][1] ) {
		case 'd':
			debug_flag = 1;
			break;
		default:
			fprintf(stderr, "unknown flag: %s\n", argv[1]);
			exit(1);
		}
		argv++;
		argc--;
	}
#endif
	if (argc < 2) {
		fprintf(stderr, "Usage: %s pid ...\n", argv[0]);
		exit(1);
	}
	u1 = (struct user *)valloc(ctob(UPAGES));
	if (u1 == NULL) {
		fprintf(stderr, "Cannot valloc u.\n");
		exit(1);
	}
	bzero(u1, ctob(UPAGES));
	openfiles();
	getkvars();
	procbase = Getw((off_t)nl[X_PROC].n_value);
	nproc = Getw((off_t)nl[X_NPROC].n_value);
	nswap = Getw((off_t)nl[X_NSWAP].n_value);
	dmmin = Getw((off_t)nl[X_DMMIN].n_value);
	dmmax = Getw((off_t)nl[X_DMMAX].n_value);
	pmap = Getw((off_t)nl[X_PMAP].n_value);
	init_dmap();
	while (--argc > 0) {
		if ((pid = atoi(*++argv)) <= 0 || setjmp(cont_frame))
			continue;
		printf("%d: ", pid);
		procp = procbase;
		for (i = 0; i < nproc; i += FEW) {
			Seek(kmem, procp);
			j = nproc - i;
			if (j > FEW)
				j = FEW;
			j *= sizeof(struct proc);
			Read(kmem, (char *) proc, j);
			procp += j;
			for (j = j / sizeof(struct proc) - 1; j >= 0; j--) {
				p = &proc[j];
				if (p->p_pid == pid)
					goto found;
			}
		}
		printf("Process not found.\n");
		continue;
	found:
		if (p->p_uid != (uid = getuid()) && uid != 0) {
			printf("Not owner.\n");
			continue;
		}
		if (p->p_stat == SZOMB) {
			printf("Zombie.\n");
			continue;
		}
		if (p->p_flag & SSYS) {
			printf("System process.\n");
			/* i.e. swapper or pagedaemon */
			continue;
		}
		(void) sprintf(coref, "core.%d", pid);
		if ((cor = creat(coref, 0666)) < 0) {
			perror(coref);
			exit(1);
		}
		core(p);
		(void) close(cor);
		printf("%s dumped\n", coref);
	}
}

/*
 * init_dmap()
 *	Initialize dmap related values.
 */

init_dmap()
{
	register int blk;
	register u_int vaddr;
	register int dm_nemap = 0;

	/* 
	 * Determine number of entries in dmapext array (dm_nemap),
	 * and size of dmapext object (sizeof_dmapext).
	 */

	for (blk = dmmin, vaddr = 0; vaddr < MAXADDR; ) {
		++dm_nemap;
		vaddr += ctob(dtoc(blk));
		if (blk < dmmax)
			blk *= 2;
	}

#ifdef	i386
	/*
	 * Kludge to get sizeof_dmapext to not overflow into next page by
	 * only a few bytes -- assuming dmmin == 16k, dmmax == 256k, need
	 * to compensate for 6 longs (2 from struct dmapext and 4 from
	 * the short blocks).  In practice, this drops about 1.5Meg from
	 * the max size segment, and since stack consumes at least 4Meg
	 * of virtual space (due to virtual hole, data can't grow into
	 * top 4Meg of 256Meg), this is a don't care.
	 *
	 * Not an issue on ns32000, since 16Meg address space and 256k max
	 * chunk doesn't fill a HW page.
	 */
	dm_nemap -= 6;
#endif	i386

	sizeof_dmapext = sizeof(struct dmapext) + sizeof(swblk_t)*(dm_nemap-1);
	sizeof_dmapext_disk = ctod(clrnd(btoc(sizeof_dmapext)));
	dmep = (struct dmapext *)valloc(sizeof_dmapext_disk);
	smep = (struct dmapext *)valloc(sizeof_dmapext_disk);
}


Getw(loc)
	off_t loc;
{
	int word;

	Get(kmem, loc, &word, sizeof(int));
	return (word);
}

openfiles()
{
	kmem = open(KMEM, O_RDONLY);
	if (kmem < 0) {
		perror(KMEM);
		exit(1);
	}
	mem = open(MEM, O_RDONLY);
	if (mem < 0) {
		perror(MEM);
		exit(1);
	}
	swap = open(SWAP, O_RDONLY);
	if (swap < 0) {
		perror(SWAP);
		exit(1);
	}
}

getkvars()
{
	nlist(NLIST, nl);
	if (nl[0].n_type == 0) {
		fprintf(stderr, "%s: No namelist\n", NLIST);
		exit(1);
	}
}

/*
 * returns a file descriptor for the device the process p has its
 * text on.
 */
int blkno;
int
findtext(p,ptep,offset)
	struct proc *p;
	struct pte  *ptep;
{
	struct vnode v, vv;
	register DIR *df;
	struct user u2;
	struct mmap um;
	struct mfile mf;
	struct direct *dbuf;
	static struct stat s;
	static char devname[MAXNAMLEN+sizeof("/dev/")+1];
	static int fd = -1;
	int found = 0;
	u_long mfoff;

	/*
	 * find device process text is on
	 */
	/* get the uarea */
	Get(kmem, p->p_uarea, &u2, sizeof(u2));
	/* now get the mmap structure */
	um = u2.u_mmap[PTETOMAPX(*ptep)];
	if (um.mm_paged) {
		struct pte *newpteaddr;
		struct pte newpte;

		/* paged handle is an mfile, need to extract vnode */
		Get(kmem, um.mm_handle, &mf, sizeof(mf));
		mfoff = offset + um.mm_off - um.mm_1stpg;
		newpteaddr = MFOFFTOPTE(&mf,mfoff);
		Get(kmem, newpteaddr , &newpte, sizeof(newpte));
		blkno = PTE_TO_BLKNO(newpte);
		/* mf contains a pointer to vnode, get it! */
		Get(kmem, mf.mf_vp, &v, sizeof(v));
		/* now get the device's vnode */
		Get(kmem, v.v_devvp, &vv, sizeof(vv));
		if ((vv.v_flag & VNBACCT) == 0) {
			fprintf(stderr, "gcore: warning, can't get fill-on-demand pages on remote files\n");
			return (-1);
		}
	} else {
		/* not paged */
		struct pmap_unit pm;

		vv.v_rdev = um.mm_handle;
		/* for the block number */
		Get(kmem, pmap+(sizeof(struct pmap_unit)*(minor(vv.v_rdev))),
			&pm, sizeof(struct pmap_unit));
		mfoff = offset + um.mm_off - um.mm_1stpg;
		blkno = btodb( pm.pm_paddr + mfoff * NBPG);
	}

	/*
	 * Check to see if we have already have the device open
	 * if so, just return the fd
	 */
	if (fd >= 0) {
		if (s.st_rdev == vv.v_rdev)
			return (fd);
		(void) close(fd);
	}


	/*
	 * stat the devices in /dev looking for the device the process
	 * text is on
	 */
	if ((df = opendir("/dev")) == NULL) {
		fprintf(stderr, "gcore: can't read /dev\n");
		exit(1);
	}
	while ((dbuf = readdir(df)) != NULL) {
		if (dbuf->d_name[1] == 't' 
		   && (dbuf->d_name[0] == 't' || dbuf->d_name[0] == 'p'))
			continue; 	/* don't stat ttys */
		(void) strcpy(devname, "/dev/");
		(void) strcat(devname, dbuf->d_name);
		if (stat(devname, &s) >= 0 && s.st_rdev == vv.v_rdev) {
			found = 1;
			break;
		}
	}
	closedir(df);

	if (!found) {
		fprintf(stderr, "gcore: can't find text device\n");
		return (-1);
	}

	fd = open(devname, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, 
	"gcore: can't open %s to read fill-on-demand pages\n", devname);
		return (-1);
	}
#ifdef DEBUG
	if (debug_flag)
		printf("%s\n", devname);
#endif DEBUG
	return(fd);
}

/*
 * Build the core file.
 */
core(p)
	register struct proc *p;
{

	/*
	 * Get u-area, data, and stack areas
	 */
	getu(p);
	getseg(p, p->p_dsize - u1->u_tsize, dptopte(p, 0), &u1->u_dmap, 0);
	getseg(p, p->p_ssize, sptopte(p, p->p_ssize-1), &u1->u_smap, 1);
}

/*
 * Get the u area.
 * Keeps around the u structure for later use
 * (the data and stack disk map structures).
 * p->p_uarea is pointer to process u-area in kernel virtual memory.
 */
getu(p)
	register struct proc *p;
{
	if ((p->p_flag & SLOAD) == 0) {
		Get(swap, dtob(p->p_swaddr), u1, ctob(UPAGES));
		if (u1->u_dmap.dm_ext) {
			Get(swap, dtob(u1->u_dmap.dm_daddr), dmep,
			    dtob(sizeof_dmapext_disk));
			u1->u_dmap.dm_ext = dmep;
		}
		if (u1->u_smap.dm_ext) {
			Get(swap, dtob(u1->u_smap.dm_daddr), smep,
			    dtob(sizeof_dmapext_disk));
			u1->u_smap.dm_ext = smep;
		}
	} else {
		Get(kmem, p->p_uarea, u1, ctob(UPAGES));
	}
	Write(cor, u1, ctob(UPAGES));
}

/*
 * Copy a segment to the core file.
 * The segment is described by its size in clicks,
 * its page table, its disk map, and whether or not
 * it grows backwards.
 * Note that the page table address is allowed to be meaningless
 * if the process is swapped out.
 */
getseg(p, segsize, pages, map, stack)
	register struct proc *p;
	int segsize;
	struct pte *pages;
	struct dmap *map;
{
	register int i;
	register struct pte *l2_pte;	/* level 2 page table for segment */
	unsigned int ptsize;		/* size in bytes of page table */
	struct dblock db;
	int size, incore, fd, offset;
	daddr_t bn;
	char pagealign[ctob(CLSIZE)+(CLBYTES-1)];
	char *buf = (char *)(((int)pagealign + (CLBYTES-1)) &~ (CLBYTES-1));
	int mmap_warning = 0;

#ifdef	DEBUG
	if (debug_flag)
		printf("** getseg(p=%#x,segsize=%d,pages=%#x,map=%#x,stack=%d)\n", p, segsize, pages, map, stack);
#endif
	incore = (p->p_flag & SLOAD);
	ptsize = sizeof(struct pte) * segsize;
	l2_pte = (struct pte *)valloc((ptsize+DEV_BSIZE-1) &~ (DEV_BSIZE-1));
	if (l2_pte == NULL)
		panic("valloc error");
	if (incore) {
		Get(kmem, pages, l2_pte, ptsize);
	} else {
#ifdef	DEBUG
		if (debug_flag) {
			printf("swapped: u_tsize=%d p_dsize=%d p_ssize=%d p_swaddr=%d\n",
				u1->u_tsize, p->p_dsize, p->p_ssize, p->p_swaddr);
		}
#endif
		/* 
		 * Page tables for a swapped process live right
		 * after the uarea (text+data+stack).  Have to
		 * be careful to get offsets and alignment right.
		 */
		if (stack)
			offset = dtob(p->p_swaddr) + 
				 ctob(clrnd(UPAGES + SZSWPT(p) - (u1->u_tsize/NPTEPG))) -
				 (p->p_ssize*sizeof(struct pte));
		else
			offset = dtob(p->p_swaddr) +
				 ctob(UPAGES) +
				 (u1->u_tsize % NPTEPG) * sizeof(struct pte);
		Get(swap, 
		  (offset + DEV_BSIZE-1) &~ (DEV_BSIZE-1), 
		  l2_pte, 
		  (ptsize + DEV_BSIZE-1) &~ (DEV_BSIZE-1));
#ifdef	DEBUG
		if (debug_flag)
			for (i=0; i < segsize; i++) {
				printf("pte=%08x\n", *(int*)(&l2_pte[i]));
			}
#endif
	}

	for (i = 0; i < segsize; i += CLSIZE) {
		size = min(CLSIZE, segsize - i);
		if (!incore) {
		from_swap:
			if (stack) 
				vstodb(ctod(segsize - i - size), ctod(size), map, &db, stack);
			else
				vstodb(ctod(i), ctod(size), map, &db, stack);
			Get(swap, ctob(db.db_base), buf, ctob(size));
			Write(cor, buf, ctob(size));
		} else if (*(int *)(&l2_pte[i]) == PG_INVAL) {
			(void) lseek(cor, (off_t) size, 1);
		} else if (l2_pte[i].pg_fod) {
			/*
			 * Fill on demand can only be a zero fill
			 * or an filesystem demand load page.  Actually
			 * read a demand load page from the filesystem.
			 */
#ifdef	notdef
			if (!FPTE(&l2_pte[i], pg_fzero) && (fd = findtext(p)) >= 0) {
				bn = FPTE(&l2_pte[i], pg_blkno);
				Get(fd, dbtob(bn), buf, ctob(size));
			} else
#endif
				bzero(buf,  (unsigned) ctob(size));
			Write(cor, buf, ctob(size));
		} else if ( PTEMAPPED(l2_pte[i]) ) {
			/* if in valid try to get it from memory */
			if (l2_pte[i].pg_v) {
				Get(mem, PTETOPHYS(l2_pte[i]), buf, ctob(size));
			} else {
				bzero(buf,  (unsigned) ctob(size));
				fd = findtext(p,&l2_pte[i],i);
				if (fd >= 0) {
#ifdef	DEBUG
					printf("getting %d blocks form %x\n",
						ctob(size),dbtob(blkno));
#endif
					Get(fd, dbtob(blkno), buf, ctob(size));
				} else {
					bzero(buf,  (unsigned) ctob(size));
					if (mmap_warning == 0) {
						fprintf(stderr,
		"gcore: warning: some mmap page(s) dumped as zero's\n");
						++mmap_warning;
					}
				}
			}
			Write(cor, buf, ctob(size));
		} else if (PTEPF(l2_pte[i]) == 0) {
			goto from_swap;
		} else { 	/* get the page from memory */
			Get(mem, PTETOPHYS(l2_pte[i]), buf, ctob(size));
			Write(cor, buf, ctob(size));
		}
#ifdef DEBUG
		if (debug_flag) {
			char *s;
			int at = 0;
			if (!incore) {
				s = "swapped";
				at = db.db_base;
			} else if (*(int *)(&l2_pte[i]) == PG_INVAL)
				s = "invalid";
			else if (l2_pte[i].pg_fod) {
#ifdef	notdef
			     if (FPTE(&l2_pte[i], pg_fzero))
				s = "zero fill-on-demand";
			    else
#endif
				s = "fill-on-demand";
			} 
			else if (PTEMAPPED(l2_pte[i]))
				s = "mmapped";
			else if (PTEPF(l2_pte[i]) == 0)
				s = "paged out";
			else {
				at = PTETOPHYS(l2_pte[i]);
				if (l2_pte[i].pg_v)
					s = "valid incore";
				else
					s = "reclaimable incore";
			}
			printf("virtual=(%6x) physical=(%6x) pte=%08x (%s page)\n",
				ctob(pages - p->p_ul2pt) + ctob(i), at,
				*(int *)(&l2_pte[i]), s);
		}
#endif DEBUG
	}
}

/*
 * vstodb()
 *	Given a base/size pair in virtual swap area,
 *	return a physical base/size pair which is the
 *	(largest) initial, physically contiguous block.
 */

vstodb(vsbase, vssize, dmp, dbp, rev)
	register int	vsbase;
	register int	vssize;
	struct dmap	*dmp;
	register struct dblock *dbp;
	int		rev;
{
	register swblk_t *ip;
	register int blk;

	if (dmp->dm_ext)
		ip = dmp->dm_ext->dme_map;
	else
		ip = dmp->dm_map;

	blk = dmmin;
	while (vsbase >= blk) {
		ip++;
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		else {				/* reached constant size blks */
			ip += (vsbase / blk);
			vsbase %= blk;
			break;
		}
	}
	dbp->db_size = min(vssize, blk - vsbase);
	dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
}

/*VARARGS1*/
panic(cp, a, b, c, d)
	char *cp;
{
	printf(cp, a, b, c, d);
	printf("\n");
	longjmp(cont_frame, 1);
}

imin(a, b)
{

	return (a < b ? a : b);
}
