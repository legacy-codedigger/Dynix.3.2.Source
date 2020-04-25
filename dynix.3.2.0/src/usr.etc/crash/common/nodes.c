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
static char rcsid[] = "$Header: nodes.c 1.11 1991/08/02 15:34:09 $";
#endif

#include	"crash.h"

#ifdef BSD

#include	<sys/time.h>
#include	<sys/vnode.h>
#define KERNEL
#include	<sys/inode.h>
#undef KERNEL

#else /* PTX */

#include	<sys/vnode.h>
#ifdef CROSS
#include	</usr/include/time.h>
#else
#include	<time.h>
#endif
#define INKERNEL
#include 	<sys/ufsinode.h>
#include 	<sys/snode.h>
#include 	<sys/fifo.h>
#include 	<sys/fifonode.h>
#undef INKERNEL
#include 	<sys/sysmacros.h>

#endif

struct inode *readinode(), *inode, *inodeNINODE;
int ninode;
struct	vnodeops *specfs_vnode, *fifo_vnode;
int	stablesize;
struct	vnode *readvnode(), *rootdir;
struct	vnodeops *ufs_vnode, *dev_vnode;
#ifdef SACC
struct	snode *readsnode();
struct  snode **stable;
#endif
#ifdef FN_RBUSY
struct	fifonode *readfnode();
struct	fifonode *fifostuff;
struct	fifoinfo fifoinfo;
#endif

/*
 * vnode_init() must be called before inode_init().
 * vnode_init() need be called only once; it reads static information.
 * inode_init() may be repeatedly called; it reads dynamic information.
 */

inode_init()
{
#ifdef _SEQUENT_
	struct var *vaddr;
	extern struct var v;

	readv(search("inode"), &inode, sizeof inode);
	inodeNINODE = v.ve_inode;
	ninode = v.v_inode;
#else
	readv(search("inode"), &inode, sizeof inode);
	readv(search("inodeNINODE"), &inodeNINODE, sizeof inodeNINODE);
	readv(search("ninode"), &ninode, sizeof ninode);
#endif
#ifdef SACC
	readv(search("stable"), stable, stablesize * sizeof(struct snode *));
#endif
#ifdef FN_RBUSY
	readv(search("fifoinfo"), &fifoinfo, sizeof fifoinfo);
#endif
}

vnode_init()
{

	readv(search("rootdir"), &rootdir, sizeof rootdir);
	ufs_vnode    = (struct vnodeops *)search("ufs_vnodeops");
	specfs_vnode = (struct vnodeops *)search("spec_vnodeops");
	fifo_vnode = (struct vnodeops *)search("fifo_vnodeops");
	dev_vnode = (struct vnodeops *)search("dev_vnode_ops");
#ifdef SACC
	readv(search("stablesize"), &stablesize, sizeof stablesize);
	stable = (struct snode **)calloc(stablesize, sizeof(struct snode *));
#endif
#ifdef FN_RBUSY
	readv(search("fifostuff"), &fifostuff, sizeof fifostuff);
#endif
}

Inode()
{

	struct inode *ii, *i = inode;
	register int j;
	char *arg;
	int count, slt;

	if (live)
		inode_init();

#ifdef BSD
	printf("SLT     ADDR     VNODE  MAJ MIN   INUMB LINK UID  GID MCNT  MODE    FLAGS\n");
#else
	printf("SLT     ADDR     VNODE   DEVVP    INUMB  LINK UID  GID   MODE  FLAGS\n");
#endif
						
	if ((arg = token()) == NULL) {		/* print the works */
		for (j = ninode; j > 0; i++, j--) {
			printf("%-3d", ninode - j);
			prinode(i);
		}
	} else if (memcmp("0x", arg, 2) == 0) {/* inode address supplied */
		printf("%-3d",
		      (((struct inode *)atox(arg+2) - inode) / sizeof (struct inode))); 
		prinode((struct inode *)atox(arg+2));
	} else if (strcmp("-l", arg) == 0) {
		struct vnode *v;

		for (j = ninode; j > 0; i++, j--) {
			if ((v = readvnode(&(i->i_vnode))) == NULL) {
				continue;
			}
			if (v->v_nodemutex.rw_count < 0) {
				printf("%-3d", ninode - j);
				prinode(i);
			}
		}
	} else {				/* selective print */
		slt = atoi(arg);
		i +=  slt;
		count = atoi(token());
		if (!count) count = 1;
		count = ((count + slt) > ninode ) ? (ninode - slt) : count;
		for (j = count; j > 0; i++, j--, slt++) {
			printf("%-3d", slt);
			prinode(i);
		}
	}	

}

Vnode()
{
	
	struct inode *ii, *i = inode;
	struct vnode *vp;
	register int j;
	char *arg;
	int count, slt;

#ifdef BSD
	printf("ISLT    ADDR      INODE   CNT LCNT ECNT TYPE FS   DEV     FLAGS\n");
#else

	printf("ISLT    ADDR       NODE   CNT TYP  FS       DEVVP     SNODE    DEV     FLAGS\n");
#endif

	if ((arg = token()) == NULL) {		/* print the works */
		for (j = ninode; j > 0; i++, j--) {
			printf("%-3d", ninode - j);
			prvnode(&i->i_vnode);
		}
	} else if (memcmp("0x", arg, 2) == 0) {/* vnode address supplied */
		vp = (struct vnode *)(atoi(arg));
		for (j = ninode; j > 0; i++, j--) {
			if (&i->i_vnode == vp)
				break;
		}
		if ( j != 0 )
			printf("%-3d", ninode - j);
		else
			printf("   ");
		prvnode(vp);
	} else {
		slt = atoi(arg);
		i +=  slt;
		count = atoi(token());
		if (!count) count = 1;
		for (j = count; j > 0; i++, j--, slt++) {
			printf("%-3d", slt);
			prvnode(&i->i_vnode);
		}
	}	
}	

#ifdef SACC
Snode()
{
	struct inode *ii, *i = inode;
	register int j;
	char *arg;
	int count, slt;
	char *title = "SNODE      VNODE      REALVP    BDEVVP   TYP DEV      SPTR        NEXT\n";

	if (live)
		inode_init();

	if ((arg = token()) == NULL) {		/* print the works */
		printf("ISLT  %s", title);
		for (j = ninode; j > 0; i++, j--) {
			if ((ii = readinode(i)) == NULL)
				continue;
			if (ii->i_vnode.v_snode == 0)
				continue;
			printf("%3d", ninode - j);
			prsnode(ii->i_vnode.v_snode);
		}
	} else if (memcmp("-b", arg, 2) == 0) {/* print "common" VBLK snodes */
		struct snode *p;

		printf("HSLT  %s", title);
		for (j = 0; j < stablesize; j++) {
			for (p = stable[j]; p != NULL; p = p->s_next) {
				printf("%2d ", j);
				prsnode(p);
				p = readsnode(p);
			}
		}
	} else if (memcmp("0x", arg, 2) == 0) {/* vnode address supplied */
		printf("      %s", title);
		printf("   ");
		prsnode(atoi(arg));
	} else {
		printf("ISLT  %s", title);
		slt = atoi(arg);
		i +=  slt;
		count = atoi(token());
		if (!count) count = 1;
		count = ((count + slt) > ninode ) ? (ninode - slt) : count;
		for (j = count; j > 0; i++, j--, slt++) {
			if ((ii = readinode(i)) == NULL)
				continue;
			if (ii->i_vnode.v_snode == 0)
				continue;
			printf("%3d", slt);
			prsnode(ii->i_vnode.v_snode);
		}
	}	
	
}	
#endif

#ifdef FN_RBUSY
Fnode()
{
	struct fifonode *f;
	char *arg;
	int count, slt;
	int	nfifo;
	char	buf[10];

	char *title = "  FNODE    VNODE    WCNT RCNT  WPTR   RPTR  SIZE  FLAGS\n";

	if (live)
		inode_init();

	nfifo = fifoinfo.nfifo;
	if ((arg = token()) == NULL) {		/* print the works */
		printf("%d fifos configured\n", nfifo);
		printf("FSLT  %s", title);
		for (f = fifostuff; f<&fifostuff[nfifo]; f++) {
			sprintf(buf, "%3d", f - fifostuff);
			prfnode(f, buf);
		}
	} else if (memcmp("0x", arg, 2) == 0) {/* fnode address supplied */
		printf("      %s", title);
		printf("   ");
		prfnode(atoi(arg), 0);
	} else {
		printf("FSLT  %s", title);
		slt = atoi(arg);
		count = atoi(token());
		if (!count) 
			count = 1;
		for (f = &fifostuff[slt]; f<&fifostuff[nfifo]; f++) {
			printf("%3d", slt);
			prfnode(f, 0);
			if (--count <= 0)
				break;
		}
	}	
	
}	
#endif

static
prinode(ii)
	register struct inode *ii;
{

	register struct inode	*i;
	register char	ch;

	if ((i = readinode(ii)) == NULL)
		return;

#ifdef BSD
	printf(" %#10x %#10x %3.3o %3.3o %-7u %-4d %4-d %-4d %3d",
		ii, &ii->i_vnode, major(i->i_dev), minor(i->i_dev), 
	        i->i_number, i->i_nlink, i->i_uid, i->i_gid, i->i_mutex.rw_count);
#else
	printf(" %#10x %#10x %#6x %-7u %-4d %4-d %-4d",
		ii, &ii->i_vnode, i->i_devvp,
	        i->i_number, i->i_nlink, i->i_uid, i->i_gid);
#endif

	printf(" %s%s%s%3o",
	i->i_mode & ISUID ? "u" : "-",
	i->i_mode & ISGID ? "g" : "-",
	i->i_mode & ISVTX ? "v" : "-",
	i->i_mode & 0777);

	switch(i->i_mode & IFMT) {
	case IFDIR: 
		ch = 'd'; 
		break;
	case IFCHR: 
		ch = 'c'; 
		break;
	case IFBLK: 
		ch = 'b'; 
		break;
	case IFREG: 
		ch = 'f'; 
		break;
	case IFIFO: 
		ch = 'p'; 
		break;
	case IFSOCK: 
		ch = 'S'; 
		break;
	case IFLNK: 
		ch = 'l'; 
		break;
	default:    
		ch = '-'; 
		break;
	}
	printf(" %c", ch);

	printf("%s%s%s%s\n",
	i->i_flag & IUPD ? " upd" : "",
	i->i_flag & IACC ? " acc" : "",
	i->i_flag & ICHG ? " chg" : "",
	i->i_flag & IFREE ? " free": "");

	return;

}

static
prvnode(vv)
	register struct vnode *vv;
{

	register struct vnode *v;
	register char	ch;

	if ((v = readvnode(vv)) == NULL)
		return;

#ifdef BSD
	printf(" %#10x %#10x %-3u  %-2u   %-2u   ",
		vv, v->v_data, v->v_count, v->v_shlockc, v->v_exlockc);
#else
	printf(" %#10x %#10x %-3u ",
		vv, v->v_data, v->v_count);
#endif

	switch (v->v_type) {
	case VNON:
		ch = 'n';
		break;
	case VREG:
		ch = 'r';
		break;
	case VDIR:
		ch = 'd';
		break;
	case VBLK:
		ch = 'b';
		break;
	case VCHR:
		ch = 'c';
		break;
	case VLNK:
		ch = 'l';
		break;
	case VSOCK:
		ch = 's';
		break;
	case VBAD:
		ch = 'b';
		break;
	default:
		ch = '-';
		break;
	}

	printf("%c", ch);

	if (v->v_op == ufs_vnode)
		printf("  ufs  ");
	else if (v->v_op == fifo_vnode)
		printf("  fifo ");
	else if (v->v_op == specfs_vnode)
		printf(" specfs");
	else
		printf("  vfs  ");

#ifdef _SEQUENT_
	printf(" 0x%08x 0x%08x", v->v_devvp, v->v_snode);

#endif
	printf(" %3d,%-5d", major(v->v_rdev), minor(v->v_rdev));

	printf(" %s%s%s%s%s",
		v->v_flag & VROOT ? " vrt" : "",
		v->v_flag & VMOUNTING ? " mnt": "",
		v->v_flag & VTEXT ? " txt": "",
		v->v_flag & VMAPPED ? " map": "",
		v->v_flag & VMAPSYNC ? " syn": ""
	);
#ifdef BSD
	printf(" %s%s%s",
		v->v_exlockc ? " exl" : "",
		v->v_shlockc ? " shl" : "",
		(v->v_exsema.s_count || v->v_shsema.s_count) ? " lwt" : ""
	);
#endif
#ifdef VMNDLCK
	if (v->v_flag & VMNDLCK)
		printf(" mndl");
#endif
#ifdef VBTAPE
	if (v->v_flag & VBTAPE)
		printf(" tap");
#endif
#ifdef VISSWAP
	if (v->v_flag & VISSWAP)
		printf(" isswap");
#endif
#ifdef VNBACCT
	if (v->v_flag & VNBACCT)
		printf(" dev");
#endif
#ifdef VNOLINKS
	if (v->v_flag & VNOLINKS)
		printf(" nol");
#endif
	printf("\n");
	
}	


#ifdef SACC
static
prsnode(ss)
	register struct snode *ss;
{

	register struct snode *s;
	register struct vnode *v;
	register char	ch;

	if ((s = readsnode(ss)) == NULL)
		return;
	
	printf(" 0x%08x 0x%08x 0x%08x 0x%08x ",
		ss, &ss->s_vnode, s->s_realvp, s->s_bdevvp);

	v = &s->s_vnode;

	switch (v->v_type) {
	case VNON:
		ch = 'n';
		break;
	case VREG:
		ch = 'r';
		break;
	case VDIR:
		ch = 'd';
		break;
	case VBLK:
		ch = 'b';
		break;
	case VCHR:
		ch = 'c';
		break;
	case VFIFO:
		ch = 'p';
		break;
	case VLNK:
		ch = 'l';
		break;
	case VSOCK:
		ch = 's';
		break;
	case VBAD:
		ch = 'b';
		break;
	default:
		ch = '-';
		break;
	}

	printf("%c ", ch);

	printf("%2d,%-5d", major(s->s_dev), minor(s->s_dev));
	printf(" 0x%08x 0x%08x", s->s_sptr, s->s_next);

	if (v->v_op == ufs_vnode) {
		printf("\nINCORRECT VOPS: ufs");
	} else if (v->v_op == fifo_vnode) {
		if (v->v_type != VFIFO)
			printf("\nINCORRECT VOPS: fifo");
	} else if (v->v_op == specfs_vnode) {
		if ((v->v_type != VCHR) && (v->v_type != VBLK))
			printf("\nINCORRECT VOPS: specfs");
	} else {
		printf("\nINCORRECT VOPS: unknown");
	}

	printf("\n");
	
}	
#endif

#ifdef FN_RBUSY
static
prfnode(ff, all)
	register struct fifonode *ff;
	char *all;
{

	register struct fifonode *f;
	register struct vnode *v;
	register char	ch;

	if ((f = readfnode(ff)) == NULL)
		return;
	
	if (all) {
		if (f->fn_wcnt == 0 && f->fn_rcnt == 0)
			return;
		printf("%s", all);
	}
	printf(" 0x%08x 0x%08x ", ff, &ff->fn_vnode);

	printf(" %3d %3d ", f->fn_wcnt, f->fn_rcnt);
	printf(" %5d %5d %5d", f->fn_wptr, f->fn_rptr, f->fn_size);

	if (f->fn_flag & FN_RBUSY)
		printf("R ");
	if (f->fn_flag & FN_WBUSY)
		printf("W ");
	printf("\n");
	
}	
#endif

static struct inode *
readinode(ii)
	struct inode *ii;
{

	static struct inode inodebuf;

	if (readv(ii, &inodebuf, sizeof inodebuf) != sizeof inodebuf) {
		printf("read error on inode table\n");
		return NULL;
	}
	return &inodebuf;

}


#ifdef SACC
static struct snode *
readsnode(ss)
	struct snode *ss;
{

	static struct snode snodebuf;

	if (readv(ss, &snodebuf, sizeof snodebuf) != sizeof snodebuf) {
		printf("read error on snode\n");
		return NULL;
	}
	return &snodebuf;

}
#endif
#ifdef FN_RBUSY
static struct fifonode *
readfnode(ff)
	struct fifonode *ff;
{

	static struct fifonode fifonodebuf;

	if (readv(ff, &fifonodebuf, sizeof fifonodebuf) != sizeof fifonodebuf) {
		printf("read error on fifonode\n");
		return NULL;
	}
	return &fifonodebuf;

}
#endif

struct vnode *
readvnode(vv)
	struct vnode *vv;
{

	static struct vnode vnodebuf;

	if (readv(vv, &vnodebuf, sizeof vnodebuf) != sizeof vnodebuf) {
		printf("read error on vnode\n");
		return NULL;
	}
	return &vnodebuf;

}

atox(p)
	register char *p;
{
	register int n = 0;

	for(; *p != '\0'; p++) {
		n <<= 4;
		if (*p >= 'a' && *p <= 'f') {
			n += (*p - 'a' + 0xa);
		} else if (*p >= 'A' && *p <= 'F') {
			n += (*p - 'A' + 0xa);
		} else {
			n += (*p - '0');
		}
	}
	return(n);
}

