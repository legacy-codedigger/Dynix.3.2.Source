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

#ifdef RCS
static char rcsid[] = "$Header: sys.c 2.7 90/09/11 $";
#endif

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include "saio.h"

ino_t	dlook();
caddr_t calloc();

struct dirstuff {
	int loc;
	struct iob *io;
};

static
openi(n, io)
	register struct iob *io;
{
	register struct dinode *dp;
	int cc;

	io->i_offset = 0;
	io->i_bn = fsbtodb(io->i_fs, itod(io->i_fs, n)) + io->i_boff;
	io->i_cc = io->i_fs->fs_bsize;
	io->i_ma = io->i_buf;
	cc = devread(io);
	dp = (struct dinode *)io->i_buf;
	io->i_ino.i_ic = dp[itoo(io->i_fs, n)].di_ic;
	return (cc);
}

static
find(path, file)
	register char *path;
	struct iob *file;
{
	register char *q;
	char c;
	int n = 0;

#if !defined(BOOTXX)
	if (path==NULL || *path=='\0') {
		printf("null path\n");
		return (0);
	}
	if (strcmp(path, "/") == 0) path = "/.";
#endif

	if (openi((ino_t) ROOTINO, file) < 0) {
		printf("can't read root inode\n");
		return (0);
	}
	while (*path) {
		while (*path == '/')
			path++;
		q = path;
		while(*q != '/' && *q != '\0')
			q++;
		c = *q;
		*q = '\0';

		if ((n = dlook(path, file)) != 0) {
			if (c == '\0')
				break;
			if (openi(n, file) < 0)
				return (0);
			*q = c;
			path = q;
			continue;
		} else {
			printf("%s not found\n", path);
			return (0);
		}
	}
	return (n);
}

static daddr_t
sbmap(io, bn)
	register struct iob *io;
	daddr_t bn;
{
	register struct inode *ip;
	register int j, sh;
	int i;
	daddr_t nb, *bap;

	ip = &io->i_ino;
#if !defined(BOOTXX)
	if (bn < 0) {
		printf("bn negative\n");
		return ((daddr_t)0);
	}
#endif

	/*
	 * blocks 0..NDADDR are direct blocks
	 */
	if(bn < NDADDR) {
		nb = ip->i_db[bn];
		return (nb);
	}

	/*
	 * addresses NIADDR have single and double indirect blocks.
	 * the first step is to determine how many levels of indirection.
	 */
	sh = 1;
	bn -= NDADDR;
	for (j = 0; j < NIADDR; j++) {
		sh *= NINDIR(io->i_fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
#if !defined(BOOTXX)
	if (j == NIADDR) {
		printf("bn ovf %D\n", bn);
		return ((daddr_t)0);
	}
#endif

	/*
	 * fetch the first indirect block address from the inode
	 */
	nb = ip->i_ib[j];
#if !defined(BOOTXX)
	if (nb == 0) {
		printf("bn void %D\n",bn);
		return ((daddr_t)0);
	}
#endif

	/*
	 * fetch through the indirect blocks
	 */
	for (; j >= 0; j--) {
		if (blknos[j] != nb) {
			io->i_bn = fsbtodb(io->i_fs, nb) + io->i_boff;
			io->i_ma = b[j];
			io->i_cc = io->i_fs->fs_bsize;
			if (devread(io) != io->i_fs->fs_bsize) {
				if (io->i_error)
					errno = io->i_error;
				printf("bn %D: read error\n", io->i_bn);
				return ((daddr_t)0);
			}
			blknos[j] = nb;
		}
		bap = (daddr_t *)b[j];
		sh /= NINDIR(io->i_fs);
		i = (bn / sh) % NINDIR(io->i_fs);
		nb = bap[i];
		if(nb == 0) {
			printf("bn void %D\n",bn);
			return ((daddr_t)0);
		}
	}
#if !defined(BOOTXX)
	/*
	 * Invalidate indirect blocks since next lookup might be for a
	 * different device (and we would use the wrong block).
	 */
	for (j=0; j < NIADDR; j++)
		blknos[j] = 0;
#endif
	return (nb);
}

static ino_t
dlook(s, io)
	char *s;
	register struct iob *io;
{
	register struct direct *dp;
	register struct inode *ip;
	struct dirstuff dirp;
	int len;

#if !defined(BOOTXX)
	if (s == NULL || *s == '\0')
		return (0);
#endif
	ip = &io->i_ino;
	if ((ip->i_mode&IFMT) != IFDIR) {
		printf("not a directory\n");
		return (0);
	}
#if !defined(BOOTXX)
	if (ip->i_size == 0) {
		printf("zero length directory\n");
		return (0);
	}
#endif
	len = strlen(s);
	dirp.loc = 0;
	dirp.io = io;
	for (dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		if(dp->d_ino == 0)
			continue;
		if (dp->d_namlen == len && !strcmp(s, dp->d_name))
			return (dp->d_ino);
	}
	return (0);
}

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
	register struct dirstuff *dirp;
{
	register struct direct *dp;
	register struct iob *io;
	daddr_t lbn, d;
	int off;

	io = dirp->io;
	for(;;) {
		if (dirp->loc >= io->i_ino.i_size)
			return (NULL);
		off = blkoff(io->i_fs, dirp->loc);
		if (off == 0) {
			lbn = lblkno(io->i_fs, dirp->loc);
			d = sbmap(io, lbn);
			if(d == 0)
				return NULL;
			io->i_bn = fsbtodb(io->i_fs, d) + io->i_boff;
			io->i_ma = io->i_buf;
			io->i_cc = blksize(io->i_fs, &io->i_ino, lbn);
			if (devread(io) < 0) {
				errno = io->i_error;
				printf("bn %D: read error\n", io->i_bn);
				return (NULL);
			}
		}
		dp = (struct direct *)(io->i_buf + off);
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

#if !defined(BOOTXX)
lseek(fdesc, addr, ptr)
	int fdesc, ptr;
	off_t addr;
{
	register struct iob *io;

	if (ptr != 0) {
		printf("Seek not from beginning of file\n");
		errno = EOFFSET;
		return (-1);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((io = &iob[fdesc])->i_flgs & F_ALLOC) == 0) {
		errno = EBADF;
		return (-1);
	}
	/*
	 * if packet, call the device seeker
	 */
	if (io->i_flgs&F_PACKET) 
		return(devlseek(io, addr, ptr));
	io->i_offset = addr;
	io->i_bn = addr / DEV_BSIZE;
	io->i_cc = 0;
	return (0);
}
#endif

#if !defined(BOOTXX)
getc(fdesc)
	register fdesc;
{
	register struct iob *io;
	char c;


	if (fdesc >= 0 && fdesc <= 2)
		return (getchar());
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((io = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		return (-1);
	}
	if (iomove(io, &c, 1) != 1)
		return(-1);
	return((unsigned)c);
}
#endif

#ifdef	notdef
getw(fdesc)
	int fdesc;
{
	register w,i;
	register char *cp;
	int val;

	for (i = 0, val = 0, cp = (char *) &val; i < sizeof(val); i++) {
		w = getc(fdesc);
		if (w < 0) {
			if (i == 0)
				return (-1);
			else
				return (val);
		}
		*cp++ = w;
	}
	return (val);
}
#endif

int	errno;

read(fdesc, buf, count)
	register int fdesc;
	char *buf;
	register int count;
{
	register struct iob *file;
	register int i;

#if !defined(BOOTXX)
	errno = 0;
	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		do {
			*buf = getchar();
		} while (--i && *buf++ != '\n');
		return (count - i);
	}
#endif
	fdesc -= 3;
#if !defined(BOOTXX)
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		return (-1);
	}
	if ((file->i_flgs&F_READ) == 0) {
		errno = EBADF;
		return (-1);
	}
#else
	file = &iob[fdesc];
#endif

	if ((file->i_flgs & F_FILE) == 0) {
		file->i_cc = count;
		file->i_ma = buf;
		file->i_bn = file->i_boff + (file->i_offset / DEV_BSIZE);
		i = devread(file);
		file->i_offset += count;
		if (i < 0)
			errno = file->i_error;
		return (i);
	} else {
		if (file->i_offset+count > file->i_ino.i_size)
			count = file->i_ino.i_size - file->i_offset;
		if (count <= 0)
			return (0);
		return (iomove(file, buf, count));
	}
}

iomove(io, to, nbytes)
	register struct iob *io;
	char *to;
	int nbytes;
{
	char *from;
	register int count, n;
	register struct fs *fs;
	register int off;
	int lbn, size, diff;

	n = nbytes;
	while (n) {
		from = io->i_ma;
		if (io->i_cc <= 0) {
			if ((io->i_flgs & F_FILE) != 0) {
				diff = io->i_ino.i_size - io->i_offset;
				if (diff <= 0)
					return (-1);
				fs = io->i_fs;
				lbn = lblkno(fs, io->i_offset);
				io->i_bn = fsbtodb(fs, sbmap(io, lbn)) 
						+ io->i_boff;
				off = blkoff(fs, io->i_offset);
				size = blksize(fs, &io->i_ino, lbn);
			} else {
				io->i_bn = io->i_offset / DEV_BSIZE;
				off = 0;
				size = DEV_BSIZE;
			}
			io->i_ma = io->i_buf;
			io->i_cc = size;
			if (devread(io) < 0) {
				errno = io->i_error;
				return (-1);
			}
			if ((io->i_flgs & F_FILE) != 0) {
				if (io->i_offset - off + size >= 
							io->i_ino.i_size)
					io->i_cc = diff + off;
				io->i_cc -= off;
			}
			from = &io->i_buf[off];
		}
		count = MIN(io->i_cc, n);
		bcopy(from, to, count);
		n -= count;
		to += count;
		io->i_cc -= count;
		io->i_offset += count;
		io->i_ma += count;
	}
	return (nbytes);
}

#if !defined(BOOTXX)
write(fdesc, buf, count)
	int fdesc, count;
	char *buf;
{
	register i;
	register struct iob *file;

	errno = 0;
	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		while (i--)
			putchar(*buf++);
		return (count);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		return (-1);
	}
	if ((file->i_flgs&F_WRITE) == 0) {
		errno = EBADF;
		return (-1);
	}
	file->i_cc = count;
	file->i_ma = buf;
	file->i_bn = file->i_boff + (file->i_offset / DEV_BSIZE);
	i = devwrite(file);
	file->i_offset += count;
	if (i < 0)
		errno = file->i_error;
	return (i);
}
#endif

#if !defined(BOOTXX)
int	openfirst = 1;
#endif

open(str, how)
	char *str;
	int how;
{
	register char *cp;
	register struct iob *file;
	register struct devsw *dp = devsw;
	register int fdesc;
	int i;
	extern int n_devsw;

#if !defined(BOOTXX)
	if (openfirst) {
		callocrnd(DEV_BSIZE);
		for (i = 0; i < NFILES; i++) {
			iob[i].i_flgs = 0;
			iob[i].i_buf = calloc(MAXBSIZE);
			iob[i].i_fs = (struct fs *)calloc(SBSIZE);
		}
		for (i = 0; i < NIADDR; i++)
			b[i] = calloc(MAXBSIZE);
		openfirst = 0;
	}

	for (fdesc = 0; fdesc < NFILES; fdesc++)
		if (iob[fdesc].i_flgs == 0)
			goto gotfile;
	_stop("No more file slots");
#else
	fdesc = 0;
	callocrnd(DEV_BSIZE);
	iob[0].i_buf = calloc(MAXBSIZE);
	iob[0].i_fs = (struct fs *)calloc(SBSIZE);
	for (i = 0; i < NIADDR; i++)
		b[i] = calloc(MAXBSIZE);
#endif

gotfile:
	(file = &iob[fdesc])->i_flgs |= F_ALLOC;

	for (cp = str; *cp && *cp != '('; cp++)
		continue;
#if !defined(BOOTXX)
	if (*cp != '(') {
		printf("Bad device\n");
		file->i_flgs = 0;
		errno = EDEV;
		return (-1);
	}
	*cp++ = '\0';
	for (i = n_devsw; i > 0; i--) {
		if (!strcmp(str, dp->dv_name))
			goto gotdev;
		++dp;
	}
	printf("Unknown device\n");
	file->i_flgs = 0;
	errno = ENXIO;
	return (-1);
gotdev:
	*(cp-1) = '(';
	file->i_ino.i_dev = dp-devsw;
	file->i_flgs |= (dp->dv_flags & D_PACKET ? F_PACKET : 0);
	file->i_flgs |= (dp->dv_flags & D_TAPE ? F_TAPE : 0);
#else
	cp++;
	file->i_ino.i_dev = 0;
#endif
	file->i_unit = *cp++ - '0';
	while (*cp >= '0' && *cp <= '9')
		file->i_unit = file->i_unit * 10 + *cp++ - '0';
#if !defined(BOOTXX)
	if (file->i_unit < 0 || file->i_unit > 32768) {
		printf("Bad unit specifier\n");
		file->i_flgs = 0;
		errno = EUNIT;
		return (-1);
	}
#endif
	if (*cp++ != ',') {
badoff:
		printf("Missing offset specification\n");
		file->i_flgs = 0;
		errno = EOFFSET;
		return (-1);
	}
	file->i_boff = *cp++ - '0';
	while (*cp >= '0' && *cp <= '9')
		file->i_boff = file->i_boff * 10 + *cp++ - '0';
	for (;;) {
		if (*cp == ')')
			break;
		if (*cp++)
			continue;
		goto badoff;
	}
	file->i_howto = how;
	strcpy(file->i_fname, str);
	file->i_error = 0;
	devopen(file);
	if (file->i_error) {
		file->i_flgs = 0;
		errno = file->i_error;
		return(-1);
	}
	/*
	 * If raw device or packet device, stop here.
	 */
	if (*++cp == '\0' || (file->i_flgs&(F_PACKET|F_TAPE))) {
		file->i_fname[cp-str] = '\0';
		file->i_flgs |= how+1;
		file->i_cc = 0;
		file->i_offset = 0;
		return (fdesc+3);
	}
	file->i_ma = (char *)(file->i_fs);
	file->i_cc = SBSIZE;
	file->i_bn = SBLOCK + file->i_boff;
	file->i_offset = 0;
	if (devread(file) < 0) {
		printf("super block read error\n");
		file->i_flgs = 0;
		errno = file->i_error;
		return (-1);
	}
	if ((i = find(cp, file)) == 0) {
		file->i_flgs = 0;
		errno = ESRCH;
		return (-1);
	}
#if !defined(BOOTXX)
	if (how != 0) {
		printf("Can't write files yet.. Sorry\n");
		file->i_flgs = 0;
		errno = EIO;
		return (-1);
	}
#endif
	if (openi(i, file) < 0) {
		file->i_flgs = 0;
		errno = file->i_error;
		return (-1);
	}
	file->i_offset = 0;
	file->i_cc = 0;
	file->i_flgs |= F_FILE | (how+1);
	return (fdesc+3);
}

#if !defined(BOOTXX)
close(fdesc)
	int fdesc;
{
	struct iob *file;

	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		return (-1);
	}
	if ((file->i_flgs&F_FILE) == 0)
		devclose(file);
	file->i_flgs = 0;
	return (0);
}
#endif

#if !defined(BOOTXX)
ioctl(fdesc, cmd, arg)
	int fdesc, cmd;
	char *arg;
{
	register struct iob *file;
	int error = 0;

	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		return (-1);
	}
	switch (cmd) {

	case SAIOHDR:
		file->i_flgs |= F_HDR;
		break;

	case SAIOCHECK:
		file->i_flgs |= F_CHECK;
		break;

	case SAIOHCHECK:
		file->i_flgs |= F_HCHECK;
		break;

	case SAIONOBAD:
		file->i_flgs |= F_NBSF;
		break;

	case SAIODOBAD:
		file->i_flgs &= ~F_NBSF;
		break;

	case SAIOECCLIM:
		file->i_flgs |= F_ECCLM;
		break;

	case SAIOECCUNL:
		file->i_flgs &= ~F_ECCLM;
		break;

	case SAIOSEVRE:
		file->i_flgs |= F_SEVRE;
		break;

	case SAIONSEVRE:
		file->i_flgs &= ~F_SEVRE;
		break;

	default:
		error = devioctl(file, cmd, arg);
		break;
	}
	if (error < 0)
		errno = file->i_error;
	return (error);
}
#endif
