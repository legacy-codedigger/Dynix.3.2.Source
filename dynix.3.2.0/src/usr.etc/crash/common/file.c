/* $Header: file.c 1.6 1991/05/31 18:49:07 $ */

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
 * $Log: file.c,v $
 *
 */

#ifndef lint
static char rcsid[] = "$Header: file.c 1.6 1991/05/31 18:49:07 $";
#endif

#include	"crash.h"

#ifdef BSD
#define KERNEL 1
#include	<sys/file.h>
#else
#define INKERNEL
#include	<sys/file.h>
#undef INKERNEL
#endif

struct file *file;
struct file *fileNFILE;
struct file *readfile();
int nfile;

file_init()
{
	readv(search("file"), &file, sizeof file);
#ifdef BSD
	readv(search("fileNFILE"), &fileNFILE, sizeof fileNFILE);
	readv(search("nfile"), &nfile, sizeof nfile);
#else
	nfile = v.v_file;
#endif
}

File()
{

	struct file *ff, *f = file;
	register int j;
	char *arg;
	int count, slt;

	if (live)
		file_init();

	printf("SLT        ADDR REF     VNODE    FLAGS\n");
						
	if ((arg = token()) == NULL) {		/* print the works */
		for (j = nfile; j > 0; f++, j--) {
			if ((ff = readfile(f)) == NULL)
				continue;
			if (ff->f_count == 0)
				continue;
			printf("%-3d 0x%8.8x", nfile - j, f);
			prfile(ff);
		}
	 } else {				/* selective print */
		slt = atoi(arg);
		f += slt;
		count = atoi(token());
		if (!count) count = 1;
		count = ((count + slt) > nfile) ? (nfile - slt) : count;
		for (j = count; j > 0; f++, j--, slt++) {
			if ((ff = readfile(f)) == NULL)
				continue;
			if (ff->f_count == NULL)
				continue;
			printf("%-3d 0x%8.8x", slt, f);
			prfile(ff);
		}
	}	

}

prfile(f)
	register struct file *f;
{

	register char	ch;

#ifdef BSD
	printf(" %2d 0x%8.8x  ", f->f_count, f->f_data);
#else
	printf(" %2d 0x%8.8x  ", f->f_count, f->f_vnode);
#endif

	printf("%s%s%s%s%s%s%s%s%s",
	f->f_flag & FOPEN ? " opn" : "",
	f->f_flag & FREAD ? " rd" : "",
	f->f_flag & FWRITE ? " wrt" : "",
#ifdef O_NDELAY
	f->f_flag & O_NDELAY ? " dly" : "",
#else
	f->f_flag & FNDELAY ? " dly" : "",
#endif
#ifdef O_APPEND
	f->f_flag & O_APPEND ? " app": "",
#else
	f->f_flag & FAPPEND ? " app": "",
#endif
#ifdef O_SYNC
	f->f_flag & O_SYNC ? " syn": "",
#else
	f->f_flag & FSYNC ? " syn": "",
#endif
#ifdef O_CREAT
	f->f_flag & O_CREAT ? " crt": "",
#else
	f->f_flag & FCREAT ? " crt": "",
#endif
#ifdef O_TRUNC
	f->f_flag & O_TRUNC ? " trn": "",
#else
	f->f_flag & FTRUNC ? " trn": "",
#endif
#ifdef O_EXCL
	f->f_flag & O_EXCL ? " exc": "");
#else
	f->f_flag & FEXCL ? " exc": "");
#endif


#ifdef O_ASYNC
	if (f->f_flag & O_ASYNC)
		printf(" asy");
#else
#ifdef FASYNC
	if (f->f_flag & FASYNC)
		printf(" asy");
#endif
#endif
#ifdef O_MARK
	if (f->f_flag & O_MARK)
		printf(" mrk");
#else
#ifdef FMARK
	if (f->f_flag & FMARK)
		printf(" mrk");
#endif
#endif
#ifdef O_DEFER
	if (f->f_flag & O_DEFER)
		printf(" def");
#else
#ifdef FDEFER
	if (f->f_flag & FDEFER)
		printf(" def");
#endif
#endif
#ifdef O_SHLOCK
	if (f->f_flag & O_SHLOCK)
		printf(" shl");
#else
#ifdef FSHLOCK
	if (f->f_flag & FSHLOCK)
		printf(" shl");
#endif
#endif
#ifdef O_EXLOCK
	if (f->f_flag & O_EXLOCK)
		printf(" exl");
#else
#ifdef FEXLOCK
	if (f->f_flag & FEXLOCK)
		printf(" exl");
#endif
#endif
	printf("\n");
	return;

}

struct file *
readfile(f)
	struct file *f;
{

	static struct file filebuf;

	if (readv(f, &filebuf, sizeof filebuf) != sizeof filebuf) {
		printf("read error on file\n");
		return NULL;
	}
	return &filebuf;

}

