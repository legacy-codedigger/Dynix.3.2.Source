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

#ifndef lint
static char rcsid[] = "$Header: dmesg.c 2.2 87/04/09 $";
#endif

/*
 *	Suck up system messages
 *	dmesg [ /dev/kmem [ /dynix ] ]
 *		print current buffer
 *	dmesg - [ /dev/kmem [ /dynix ] ]
 *		print and update incremental history
 *
 * $Log:	dmesg.c,v $
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/vm.h>
#include <sys/ioctl.h>
#include <sec/sec.h>

#define	TRUE	1
#define	FALSE	0

char		*unix_file;		/* name of kernel to get names from */
char		*kmem_file;		/* name of kernel data area to use */
char		*scsi_file;		/* name of scsi device to use */
char		*curbuffer;		/* buffer read out of scsi */
struct sec_mem	cur;			/* describes scsi buffer */
char		*savebuffer;		/* buffer read from save file */
struct sec_mem	save;			/* describes save buffer */
int		sflg;			/* invoked with '-'? */
int		save_fd	= -1;		/* save buffer file descriptor */

char		*malloc();
char		*calloc();

main(argc, argv)
	int argc;
	char **argv;
{
	int wrapped;
	int save_index;
	int cur_index;
	register int i;
	register char c;

	if (argc>1 && argv[1][0] == '-') {
		argc--;
		argv++;
		sflg = TRUE;
	}

	/*
	 * use specific files from command line?
	 */

	unix_file = (argc>1 ? argv[1] : "/dynix" );
	kmem_file = (argc>2 ? argv[2] : "/dev/kmem" );
	scsi_file = (argc>3 ? argv[3] : "/dev/smemco");

	/*
	 * get message buffers 'curbuffer' and 'savebuffer' and associated 
	 * sec_mem structures 'cur' and 'save'.
	 */

	get_cur_msg_buffer( scsi_file );
	get_saved_msg_buffer( "/usr/adm/msgbuf" );

	/*
	 * compare the buffers to see if pointer wrapped around since
	 * the last time we looked.  this means looking from the
	 * current pointer to the saved pointer.  this area will not
	 * have changed since the last time if the pointer didn't
	 * wrap.
	 */

	save_index = save.mm_nextchar - save.mm_buffer;
	cur_index = cur.mm_nextchar - cur.mm_buffer;

	wrapped = FALSE;
	i = cur_index;
	do {
		if (curbuffer[i] != savebuffer[i]) {
			wrapped = TRUE;
			break;
		}
		i = (i+1) % cur.mm_size;
	} while ( i != save_index );

	/*
	 * if it hasn't wrapped and the pointers are the same,
	 * then the buffer hasn't changed since we looked last.
	 */

	if (!wrapped && save_index == cur_index && sflg)
		exit(0);

	/*
	 * dump out the differences.  dump out the whole thing
	 * if no '-' flag or we wrapped around.
	 */

	pdate();
	if (sflg && !wrapped) {
		i = save_index;
	} else {				/* wrapped or no sflag */
		i = cur_index;
		printf("...\n");
	}

	do {
		c = curbuffer[i];
		if (c && ((c & 0200) == 0) && c != '\r' )
			putchar(c);
		i = (i+1) % cur.mm_size;
	} while ( i != cur_index );

	done((char *)NULL);
}

/*
 * fills in cur and curbuffer with data from scsi
 */
get_cur_msg_buffer( devname )
	char *devname;
{
	int fd;

	fd = open(devname, O_RDONLY, 0);
	if( fd == -1 ) {
		fprintf(stderr, "can't open %s\n", devname);
		exit(1);
	}
	ioctl(fd, SMIOGETLOG, &cur);
	curbuffer = malloc(cur.mm_size);
	lseek(fd, cur.mm_buffer, 0);
	read(fd, curbuffer, cur.mm_size);
	close(fd);

#ifdef DEBUG
	printf("cur buffer: loc==0x%x ptr=%d  size==%d nchar==%d\n",
		cur.mm_buffer, cur.mm_nextchar, cur.mm_size, cur.mm_nchar);
#endif DEBUG
}

/*
 * fills in save and savebuffer with data from save file
 */
get_saved_msg_buffer( filename )
	char *filename;
{
	savebuffer = calloc(1, cur.mm_size);
	if (!sflg) 
		return;

	save_fd = open(filename, O_RDWR|O_CREAT, 0644);
	if( save_fd == -1 ) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}
	read(save_fd, &save, sizeof(save));
	if (save.mm_size == 0)
		save.mm_size = cur.mm_size;
	read(save_fd, savebuffer, save.mm_size);

#ifdef DEBUG
	printf("save buffer: loc==0x%x ptr=%d  size==%d nchar==%d\n",
		save.mm_buffer, save.mm_nextchar, save.mm_size, save.mm_nchar);
#endif DEBUG
}

done(s)
	char *s;
{
	if (s) {
		pdate();
		printf(s);
	}
	if (sflg) {
		lseek(save_fd, 0L, 0);
		write(save_fd, &cur, sizeof(cur));
		write(save_fd, curbuffer, cur.mm_size);
		close(save_fd);
	}
	exit(s!=NULL);
}

pdate()
{
	extern char *ctime();
	static firstime;
	time_t tbuf;

	if (firstime==0) {
		firstime++;
		time(&tbuf);
		printf("\n%.12s\n", ctime(&tbuf)+4);
	}
}
