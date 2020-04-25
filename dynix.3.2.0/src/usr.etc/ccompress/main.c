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

#ident "$Header: main.c 1.4 1991/08/29 00:11:31 $"

/*
 * main.c
 *  Main functions for ccompress utility
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/tmp_ctl.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stand/dump.h>

#define NUMBUFS 32

/* Definition of Buffer header */
struct bufhdr {
	int status;		/* FREE,DONE,INUSE */
	unsigned length;	/* Size of compressed data in the buffer */
	unsigned char *buf_p;   /* Pointer to data */
};

#define FREE 1
#define DONE 2
#define INUSE 3

/*
 * Pointers to shared data
 */

unsigned int *next_buf_p;	/* Next buffer to be used */
unsigned int *next_mem_p;	/* Next segment to be compressed */
unsigned int *next_pos_p;	/* Unused */
unsigned next_write;		/* Next buffer to be written out */


#ifdef _SEQUENT_
#include <fcntl.h>
#include <unistd.h>
#endif

#define min(a,b)        ((a>b) ? b : a)
#define PGRND(x)        (char *) (((int)(x) + _pgoff) & ~_pgoff)
#define ARGVAL() (*++(*argv) || (--argc && *++argv))

extern init_compress(),write_header(),copystat(),compress();
extern int *comp_map;

int _pgoff;
char *shared_start;
struct bufhdr *hdrs[NUMBUFS];	/*hdrs for buffers containing compressed data*/

char *infile;
char ofname [100];
char *outdir;
static ofd;

unsigned seg_size = UNIT_SZ;	/* unit of compression */
short num_compressors;		/* num. of concurrent compress processes */
int start;
static unsigned ent_no;
jmp_buf no_compress;
int num_segs;	/*number of segments of size "seg_size" in the input dump */

/* interrupt handlers */
usr1_handler();
usr2_handler();
child_died();

#ifdef _SEQUENT_
#define rindex strchr
#define signal sigset
#endif
extern char	*rindex();



main(argc,argv)
int argc;
char **argv;
{
	char *file;
	struct stat statbuf;
	int fsize;
	int pid,i;
	int n_procs;

	if (argc < 2) {
		printf("Need a file name\n");
		exit(1);
	}
	for (argc--,argv++; argc > 0; argc--,argv++) {
		if (**argv == '-') {    /* A flag argument */
			while (*++(*argv)) {
				switch (**argv) {
				case 's':
					if (!ARGVAL()) {
						fprintf(stderr, 
						"Missing seg_size\n");
						exit(1);
					}
					seg_size = atoi(*argv);
					seg_size *= 1024;
					goto nextarg;
				case 'P':
					if (!ARGVAL()) {
						fprintf(stderr, 
						"Missing num_compressors \n");
						exit(1);
					}
					num_compressors = atoi(*argv);
					goto nextarg;
				case 'o':
					if (!ARGVAL()) {
						fprintf(stderr, 
						"Missing outdir\n");
						exit(1);
					}
					outdir = *argv;
					goto nextarg;
				default:
					fprintf(stderr, 
					       "Unknown flag: '%c'; ", **argv);
					exit(1);
				}
			}
		} else {
			infile = *argv;
		}
nextarg: 
		continue;
	}
	if ((n_procs = tmp_ctl(TMP_NENG,0)) == -1) {
		perror("ccompress:tmp_ctl");
		exit(1);
	}
	/* if num_compressors not specified, set it to num. of processors */

	if (num_compressors)
		num_compressors = min(num_compressors,n_procs);
	else
		num_compressors = n_procs;


	if (infile == "") {
		printf("non-null input file needs to be specified\n");
		exit(1);
	}
	signal(SIGUSR1, usr1_handler);
	signal(SIGUSR2, usr2_handler);
	signal(SIGCHLD, child_died);

	stat(infile, &statbuf);
	fsize = (long) statbuf.st_size;

	/* Generate output filename */
	file = rindex(infile, '/');
	if (file) file++; 
	else file = infile;
	if (outdir)
		(void) sprintf(ofname, "%s/%s", outdir, file);
	else
		strcpy(ofname, infile);
	strcat(ofname, ".ZZ");

	if ((ofd = open(ofname,O_RDWR|O_CREAT, 0666)) < 0)  {
		perror(ofname);
		printf("ccompress:open of output file failed\n");
		exit(1);
	}
	num_segs = init_compress(fsize,seg_size,&start);

	/*set up shared memory between compressors */

	setup_shared();
	init_shared();

	/* put all children in this pgrp so that the writer can
	 * broadcast buffer availibility to them. 
	 */
	if (setpgrp(0,getpid()) == -1) {
		perror("ccompress:setpgrp");
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
			perror("ccompress:fork failed\n");
			fflush(stdout);
			exit(1);
		}
		if (pid == 0)
			break;
	}

	if (pid) {
		/*parent - writer process*/

		/* leave space in the beginning for the comp_map */
		lseek(ofd, start, 0);
		if (do_write() == -1) {
			kill(0,9);
			exit(1);
		} else {
#ifdef DEBUG
			printf("data written; writing header\n");
			fflush(stdout);
#endif /* DEBUG */
			write_hdr(ofd);
			copystat(infile, ofname,ofd);	/* Copy stats */
			exit(0);
		}
	} else {
		/*children - compressors*/

		close(ofd);
		/* each child reopens the file, so that flock works */

		if ((ofd = open(ofname,O_RDWR)) < 0)  {
			perror(ofname);
			printf("ccompress:open of output file failed\n");
			exit(1);
		}

		/* compress the input, exit when all done */
		do_compress();
	}
}

#ifdef _SEQUENT_
child_died() /*sigchld handler*/
{
	int status;

	waitpid(-1,&status,WNOHANG|WUNTRACED);
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
	if (WIFSIGNALED(status) || (WIFEXITED(status) && status.w_retcode == 1))
	{
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
	size = 3*sizeof(int) + NUMBUFS*sizeof(struct bufhdr) +
		NUMBUFS*seg_size;
	size = (int)PGRND(size);
	shared_start = (char *) ( ((int)sbrk(0) + (pgsz-1)) & ~(pgsz-1) );
#ifdef _SEQUENT_
	if (brk(shared_start+size)) {
		perror("setup_shared:brk on mmap");
		exit(1);
	}
#endif /* _SEQUENT_ */
	if ((fd = open ("shmem", O_CREAT | O_RDWR, 0666)) < 0) {
		perror("setup_shared:shmem");
		exit(1);
	}
	unlink("shmem");
	if (mmap (shared_start, size, PROT_RDWR, MAP_SHARED, fd, 0) < 0) {
		perror("setup_shared:mmap");
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
	for (num = 0; num < NUMBUFS ; num++) {
		hdrs[num] = j++;
		hdrs[num]->status = FREE;
	}
	k = (unsigned char *)j;
	for (num = 0; num < NUMBUFS ; num++) {
		hdrs[num]->buf_p = k;
		k += seg_size;
	}
}
/* for getting a good profile */
do_exit()
{
	exit(1);
}

/* executed by each compressor process */
do_compress()
{
	int oldmask;
	int size;
	int ifd;
	unsigned char *outp;
	unsigned buf,seg;
	int ppid;
	int n;

	signal(SIGTERM, do_exit);
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

		while (find_work(&buf,&seg) < 0) {
			/* next buffer not free. Sleep.*/
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
		/* compressing "seg" into "buf" */
		outp = hdrs[buf]->buf_p;

		/* Call compress to do the compression. 
		 * compress longjmp's out as soon as compressed output 
		 * becomes larger then input. 
		 */

		if (setjmp(no_compress) == 0) {
			if ((size = compress(infile,seg*seg_size,outp)) == -1) {
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
			printf("expansion happened\n");
			fflush(stdout);
#endif /* DEBUG */
			if ((ifd = open(infile,2)) == NULL) {
				printf("do_compress");
				perror(infile);
				exit(1);
			}
			if (lseek(ifd,seg*seg_size,0) == -1) {
				perror("do_compress:lseek failed");
				exit(1);
			}
			if ((n = read(ifd,outp,seg_size)) == -1){
				perror("do_compress:read failed");
				exit(1);
			}
			if (n != seg_size) {
				printf("do_compress:short read n = %d\n",n);
				exit(1);
			}
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

	for (;;) {
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
		/* Found a buffer to write out */

		datap = hdrs[next_write]->buf_p;
		size = hdrs[next_write]->length;
#ifdef DEBUG
		printf("***writing out seg %d***\n",ent_no);
		fflush(stdout);
#endif /* DEBUG */
		/* Update comp_map with the location of this
		 * segment in the output file.
		 */
		comp_map[ent_no++] = lseek(ofd,0,1);
		if (write(ofd,datap,size) < 0) {
			perror("do_write:write of data failed\n");
			return(-1);
		} else {
			/* mark the buffer as FREE. */

			hdrs[next_write]->status = FREE;
			next_write++;
			next_write %= NUMBUFS;

			/* Inform the compressors of the FREE buffer */

			if (kill(0,SIGUSR2) == -1){
				perror("do_write:Could not send SIGUSR2\n");
				if (kill(0,9) == -1)
					perror(
					"do_write:Could not send SIGKILL\n");
				exit(1);
			}
			/* All segments written out */
			if (ent_no == num_segs){
				comp_map[ent_no] = EOF_MAP;
				return(0);
			}
		}
	}
}

/* Return next buffer and segment to compressor, if available. */
find_work(bufp,segp)
	int *bufp;
	int *segp;
{
	unsigned buf,seg;

	/* exclude the rest */
	plock();

	/* The next buf,seg in sequence */
	buf = *next_buf_p;
	seg = *next_mem_p;

	/* Work pool exhausted. This compressor quiting. 
	 * Be paranoid and signal writer before leaving.
	 */
	if (seg >= num_segs) {
		kill(getppid(),SIGUSR1);
		exit(0);
	}
	if (hdrs[buf]->status == FREE){
		*bufp = buf;
		*segp = seg;
		hdrs[buf]->status = INUSE;
	} else {
		/* Buffer not FREE. No work for the compressor right now */
		return(-1);
	}

	/* slide the window */
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
#else
plock()
{
	while (flock(ofd,LOCK_EX|LOCK_NB) == -1) {
		if (errno != EWOULDBLOCK) {
			perror("plock:flock\n");
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

usr1_handler()
{
}
usr2_handler()
{
}
