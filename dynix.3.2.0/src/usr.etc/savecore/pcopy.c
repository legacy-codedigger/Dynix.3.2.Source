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

#ident "$Header: pcopy.c 1.4 1991/08/29 00:23:15 $"

/*
 * pcopy.c--implement parallel copy/compress of a saved core image into
 * the filesystem.
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/tmp_ctl.h>
#include <setjmp.h>
#include <errno.h>
#ifdef _SEQUENT_
#include <sys/cfg.h>
#else
#include <machine/cfg.h>
#endif
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stand/dump.h>
#include "savecore.h"
#ifdef _SEQUENT_
#include <fcntl.h>
#include <unistd.h>
#endif


#define min(a,b)        ((a>b) ? b : a)
#define PGRND(x)        (((int)(x) + _pgoff) & ~_pgoff)



struct bufhdr *hdrs[NUMBUFS];
int _pgoff;
char *shared_start;
int data_start;
static unsigned ent_no;
extern int *comp_map;
jmp_buf no_compress;
int num_segs;
char hole = 0;
char *tmp_buf;

extern init_compress(),write_header(),copystat(),compress();
extern struct  config_desc *cfg_ptr;
extern struct dumplist flist;
extern unsigned short num_devs;
extern unsigned int num_disksegs;
extern short num_compressors;
extern char *pathname;
extern seg_size;
static char *infile;
static char *ofname;
static int ofd;
static unsigned position;
extern int fflag;
extern int dumplo;


usr1_handler();
usr2_handler();
child_died();
term_handler();
char *valloc();

#ifdef _SEQUENT_
#define rindex strchr
extern char     *strchr();
#define signal sigset
#endif



pcopy(dumpfile,fd,dump_size)
	char *dumpfile;
	int fd;
	int dump_size;
{
	int pid,i;
	int n_procs;

	if ((n_procs = tmp_ctl(TMP_NENG,0)) == -1) {
		perror("pcopy:tmp_ctl");
		exit(1);
	}

	/* if num_compressors not specified, set it to num. of processors */
	if (num_compressors)
		num_compressors = min(num_compressors,n_procs);
	else
		num_compressors = n_procs;
	if ((int)(signal(SIGUSR1,usr1_handler)) == -1) {
		perror("pcopy:signal SIGUSR1");
		exit(1);
	}
	if ((int)(signal(SIGUSR2,usr2_handler)) == -1) {
		perror("pcopy:signal SIGUSR2");
		exit(1);
	}
	signal(SIGCHLD,child_died);

	ofname = pathname;
	infile = dumpfile;
	ofd = fd;

    	num_segs = init_compress(dump_size,seg_size,&data_start);
		
	/* set up shared memory between compressors */
	setup_shared();	
	init_shared();

	if ((tmp_buf = valloc(seg_size)) == 0) {
		printf("pcopy: valloc failed\n");
		exit(1);
	}
	/* put all children in this pgrp so that the writer can
	 * broadcast buffer availibility to them.
	 */
	if (setpgrp(0,getpid()) == -1) {
		perror("pcopy:setpgrp");
		exit(1);
	}
	fflush(stdout);
	fflush(stderr);

	/* fork off the compressor processes.
	 * The parent acts as the writer process, writing out compressed
	 * data, in sequence to the output file.
	 */
	for (i =0; i < num_compressors; i++) {
		 if ((pid = fork()) < 0){
			 perror("pcopy:fork failed\n");
			 fflush(stdout);
			 exit(1);
		  }
		  if (pid == 0)
			break;
	}

	if (pid) {
		/* parent - writer process */

		/* leave space in the beginning for the comp_map */
		lseek(ofd, data_start, 0);
		if (do_write() == -1) {
			kill(0,9);	
			exit(1);
		} else {
#ifdef DEBUG
			printf("data written; writing header\n");
			fflush(stdout);
#endif /* DEBUG */
			write_hdr(ofd);
			signal(SIGCHLD,SIG_DFL);
			putchar('\n');
			return(0);
		}
	} else {
		/* children - compressors */
		close(ofd);

		/* each child reopens the file, so that flock works */
		if ((ofd = open(ofname,O_RDWR)) < 0)  {
		    perror(ofname);
		    printf("pcopy:open of output file failed\n");
		    exit(1);
		}

		signal(SIGTERM,term_handler);
		/* compress the input, exit when all done */
		do_compress();
	}
}


#ifdef _SEQUENT_
child_died() /*sigchld handler*/
{
        int status;

        waitpid(-1,&status,WNOHANG);
        if (WIFSIGNALED(status) || WEXITSTATUS(status) == 1) {

		/* An error was encountered, or a fatal signal
		 * was recieved. Abort the compression.
		 */
                printf("error/signal killing all kids\n");
                fflush(stdout);
                kill(0,9);
                exit(1);
        }
}
#else
child_died() /*sigchld handler*/
{
        union wait status;
	int pid;

	pid = wait3(&status, WNOHANG, (struct rusage *)0);
	if (pid == 0)
		return;
	if (WIFSIGNALED(status) || (WIFEXITED(status) && status.w_retcode == 1)){
		/* An error was encountered, or a fatal signal
		 * was recieved. Abort the compression.
		 */
		printf("error/signal killing all kids\n");
		fflush(stdout);
		kill(0,9);
		exit(1);
	}
}
#endif

/* mmap shared memory between compressor processes.
 * The shared space includes:
 * 1. next_buf_p
 * 2. next_mem_p
 * 3. next_pos_p
 * 4. NUNBUFS Buffer headers (struct bufhdr).
 * 5. NUMBUFS BUFFERS (each seg_size big).
 */

setup_shared()
{
	int pgsz,fd;
	int size;

	pgsz = getpagesize();
	_pgoff = pgsz - 1;
	size = 3*sizeof(int) + NUMBUFS*sizeof(struct bufhdr) + NUMBUFS*seg_size;
	size = PGRND(size);
	shared_start = (char *) ( ((int)sbrk(0) + (pgsz-1)) & ~(pgsz-1) );

#ifdef _SEQUENT_
	if (brk(shared_start+size)) {
		perror("setup_shared:brk on mmap");
		exit(1);
	}
#endif /* _SEQUENT_ */
	fd = open ("shmem", O_CREAT | O_RDWR, 0666);
	unlink("shmem");
	if (mmap (shared_start, size, PROT_RDWR, MAP_SHARED, fd, 0) < 0) {
		perror("setup_shared:mmap failed\n");
		exit(1);
	}
}
	
/* initialize the buffer headers */
init_shared()
{
	unsigned int *i; 
	struct bufhdr *j;
	unsigned char *k;
	int num; 

	i = (unsigned int *)shared_start;
	next_buf_p = i++;
	next_mem_p = i++;
	next_pos_p = i++;
	*next_buf_p = *next_mem_p = *next_pos_p = 0;
	j = (struct bufhdr *)i;
	for (num =0; num < NUMBUFS ; num++) {
		hdrs[num] = j++;
		hdrs[num]->status = FREE;
	}
	k = (unsigned char *)j;
	for (num =0; num < NUMBUFS ; num++) {
		hdrs[num]->buf_p = k;
		k += seg_size;
	}
}

/* executed by each compressor process */
do_compress()
{
	int oldmask;
	int size;
	int ifd;
	unsigned char *outp;
	unsigned buf,pos;
	int ppid;
	int n;

	for ( ; ; ) {
#ifdef _SEQUENT_
                sighold(SIGUSR2);
#else
		oldmask = sigblock(sigmask(SIGUSR2));
#endif

                /* Look for seg to compress, and buffer to write output
                 * into. Go to sleep if next buffer not FREE . Writer process
                 * will signal the compressors when it frees a buffer.
                 */
		while (find_work(&buf,&pos) < 0) {
			/* next buffer not free. Sleep. */
#ifdef DEBUG
			printf("####proc %d sleeping###\n",getpid());
			fflush(stdout);
#endif /* DEBUG */
#ifdef _SEQUENT_
                        sigpause(SIGUSR2);
                        sighold(SIGUSR2);
#else
                        sigpause(0);
#endif

		}
#ifdef _SEQUENT_
                sigrelse(SIGUSR2);
#else
                sigsetmask(oldmask);
#endif

		/* compressing input at location "pos" into "buf" */

		/* If the current memory segment is a hole, no compression
		 * to be done.
		 */ 		
		if (hole){
			ppid = getppid();
			hdrs[buf]->length = -1;
			hdrs[buf]->status = DONE;
			kill(ppid,SIGUSR1);
			continue;
		}
		outp = hdrs[buf]->buf_p;

		/* if savecore using -f, we have to map our logical 
	 	 * segment into a device and offset.
		 */
		if (fflag == 1)
			map_seg(pos);
		else
			position = dumplo + pos * seg_size;

                /* Call compress to do the compression.
                 * compress longjmp's out as soon as compressed output
                 * becomes larger then input.
                 */
		if (setjmp(no_compress) == 0) {
			if ((size = compress(infile,position,outp)) == -1) {
				printf("do_compress:error in compress\n");
				exit(1);
			} 
		} else {

                        /* compress lonjmp'd out.
                         * since this segment actually becomes bigger
                         * after expansion, just leave it uncompressed
                         * in the output file.
                         */
#ifdef DEBUG
			printf("expansion happened at seg %d\n",pos);
			fflush(stdout);
#endif /* DEBUG */
			if ((ifd = open(infile,O_RDONLY)) == NULL) {
			    printf("do_compress:");
			    perror(infile);
			    exit(1);
			}

			if (lseek(ifd,position,0) == -1) {
				perror("do_compress:lseek failed");
				exit(1);
			}
				
			if ((n = read(ifd,tmp_buf,seg_size)) == -1){
				perror("do_compress:read failed");
				exit(1);
			}
			if (n != seg_size) {
				printf("do_compress:short read n = %d\n",n);
				exit(1);
			}
			bcopy(tmp_buf,outp,seg_size);
			size = seg_size;
			close(ifd);
		}

		ppid = getppid();
		hdrs[buf]->length = size;
		hdrs[buf]->status = DONE;

                /* Inform the writer that there is work to be done */
		kill(ppid,SIGUSR1);
	}
}

/* Writer process code. The writer makes sure to write out buffers
 * in the correct order. If the next buffer in sequence is not DONE
 * it sleeps.
 */

do_write()
{
	int oldmask;
	unsigned char *datap;
	int size;
	int num_written = 0;
	int num_holes = 0;

	for ( ; ; ) {
#ifdef _SEQUENT_
                sighold(SIGUSR1);
#else
                oldmask = sigblock(sigmask(SIGUSR1));
#endif

		while (hdrs[next_write]->status != DONE) {
#ifdef DEBUG
			printf("writer sleeping \n");
			fflush(stdout);
#endif /* DEBUG */
#ifdef _SEQUENT_
                        sigpause(SIGUSR1);
                        sighold(SIGUSR1);
#else
                        sigpause(0);
#endif
		}
#ifdef _SEQUENT_
                sigrelse(SIGUSR1);
#else
                sigsetmask(oldmask);
#endif

		datap = hdrs[next_write]->buf_p;
		size = hdrs[next_write]->length;
#ifdef DEBUG
		printf("***writing out seg %d***\n",ent_no);
		fflush(stdout);
#endif /* DEBUG */
		if (size > 0) {

			/* This Buffer contains compressed data. 
                	 * Update comp_map with the location of this
                 	 * segment in the output file.
                 	 */

			comp_map[ent_no++] = lseek(ofd,0,1);
			if (write(ofd,datap,size) < 0) {
				perror("do_write:write of data failed\n");
				return(-1);
			} 
			num_written++;
			if (!((num_written * seg_size) % MC_CLICK)) {
				printf(".");
				fflush(stdout);
			}
		} else {
			/* This buffer corresponds to a memory hole */
			num_holes++;
			if (!((num_holes * seg_size) % MC_CLICK)) {
				printf("x");
				fflush(stdout);
			}
			comp_map[ent_no++] = SEG_MISSING;
		}

		/* mark the buffer as FREE. */
		hdrs[next_write]->status = FREE;
		next_write++;
		next_write %= NUMBUFS;

		/* Inform the compressors of the FREE buffer */
		if (kill(0,SIGUSR2) == -1){
			perror("do_write:kill");
			exit(1);
		}

		if (ent_no == num_segs || num_written == num_disksegs){

			/* All segments written out */
			comp_map[ent_no] = EOF_MAP;
			return(0);
		}
	}

}

/* Return next buffer and segment to compressor, if available. */
find_work(bufp,posp)
	int *bufp;
	int *posp;
{
	unsigned buf,mem_seg,disk_pos,click;
	hole = 0;

	/* exclude the rest */
	plock();

	/* The next buf,seg,pos in sequence */
	buf = *next_buf_p;
	mem_seg = *next_mem_p;
	disk_pos = *next_pos_p;

	/* Work pool exhausted. This compressor quiting.
	 *  Be paranoid and signal writer before leaving.
	 */
	if (mem_seg >= num_segs) {
		kill(getppid(),SIGUSR1);
		exit(0);
	}
	if (hdrs[buf]->status == FREE){
		*bufp = buf;
		*posp = disk_pos;
		hdrs[buf]->status = INUSE;
	} else {
		return(-1);
	}
	click = (mem_seg * seg_size) / MC_CLICK;

	/* if current memory segment is a hole, then do not advance 
	 * the position pointer.
	 */
	if (MC_MMAP(click,cfg_ptr)) 
		(*next_pos_p)++;
	else {
		hole = 1;
	}
	(*next_mem_p)++;
	(*next_buf_p)++;
	(*next_buf_p) %= NUMBUFS;
	/* release lock */
	vlock();
	return(0);
}

/* plock and vlock implemented using file locking */
#ifdef _SEQUENT_
plock()
{
        lseek(ofd, 0L, 0);
        while (lockf(ofd, F_LOCK, 0L) == -1) {
                if (errno != EWOULDBLOCK) {
                        perror("plock:lockf");
                        exit(1);
                }
        }
}

vlock()
{
        lseek(ofd, 0L, 0);
        if (lockf(ofd, F_ULOCK, 0L) == -1) {
                perror("vlock:can't F_ULOCK");
                exit(1);
        }

}
#else /*dynix3*/

plock()
{
	 while (flock(ofd,LOCK_EX|LOCK_NB) == -1) {
		if (errno != EWOULDBLOCK) {
			perror("plock:flock");
			exit(1);
		}
	 }
}

vlock()
{
	if (flock(ofd,LOCK_UN) == -1) {
		perror("vlock:can't LOCK_UN");
		exit(1);
	}

}
#endif

/* maps from a linear view of the memory dump, into the multiple 
 * partition view, as specified in /etc/DUMPLIST 
 */

map_seg(seg)
	int seg;
{
	int i;

	/* No more work for this compressor. Exit */
	if (seg >= num_disksegs) {
		kill(getppid(),SIGUSR1);
		exit(0);
	}

	/* Go thru the flist structure, created by parsing 
	 * /etc/DUMPLIST. This flist structure contains a list of
	 * device names and the number of segments of size "seg_size"
	 * contained in each of those devices.
	 */
	for (i= 0; i < num_devs; i++) {
		if (seg < flist.n_segs[i]) {
			infile = flist.ifname[i];
			position = flist.offset[i] + seg * seg_size;
			return;
		}
		seg -= flist.n_segs[i];
	}
}

usr1_handler()
{
}

usr2_handler()
{
}
term_handler()
{
exit(1);
}
