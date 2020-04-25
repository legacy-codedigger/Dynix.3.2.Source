/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred. */

#ifndef lint
static char rcsid[] = "$Header: tdmp.c 1.1 91/04/03 $";
#endif

/*
 * tdmp.c:
 * The first half of trpt.c -- This program sits in a tight loop and dumps
 * the tcp debugging buffer in binary format to the specified output file,
 * by default, called tdmp.dmp.
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#define PRUREQUESTS
#include <sys/protosw.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#define	TCPTIMERS
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#define	TANAMES
#include <netinet/tcp_debug.h>

#include <stdio.h>
#include <errno.h>
#include <nlist.h>

struct	tcp_debug *tcp_debug_start;
struct	tcp_debug *tcp_debugp;
int	*tcp_debxp;
int	km;
int	debug = 0;
int	oneshot = 0;
int	pagesize;
char	*unix_file = "/dynix";
char	*kmem_file = "/dev/kmem";
FILE	*outfile;
int	ofno;
#define	OUTFILE	"tdmp.dmp"
char	*outfilename = OUTFILE;
int	records = 0;
int	bytes = 0;

main(argc, argv)
	int argc;
	char **argv;
{
	char	ch;
	extern	char *optarg;
	extern	int optind;
	int	prstats();

	while((ch = getopt(argc, argv, "df:o")) != EOF) {
		switch(ch) {
		case 'd':
			debug = 1;
			break;
		case 'o':
			oneshot = 1;
			break;
		case 'f':
			outfilename = optarg;
			break;
		default:
			fprintf(stderr, "usage: %s [ -d ] [ -o ] [ -f file ] kern core\n",
				argv[0]);
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 0) {
		unix_file = argv[0];
		if (argc > 1)
			kmem_file = argv[1];
	}
	init_outfile();
	init_mmap();

	signal(SIGINT, prstats);

	do_dump();
	prstats();

	
}

prstats()
{
	fprintf(stderr, "dumped %d records, %d bytes\n", records, bytes);
	exit(0);
}

init_outfile()
{
	if (*outfilename == '-')
		ofno = fileno(stdout);
	else
		if ((ofno = open(outfilename, O_WRONLY|O_CREAT|O_TRUNC,
		    0644)) < 0) {
			fprintf(stderr, "can't open output file %s:",
			    outfilename);
			perror("open");
			exit(1);
		}
}

extern	char *sbrk();
extern	char *valloc();

char*
do_mmap(size, kernaddr)
	int size;
	u_long kernaddr;
{
        off_t	pos;
        int	sz, off;
        char	*va;
	static	times = 0;
	int 	i;

	va = (caddr_t) (((int)sbrk(0) + (pagesize-1)) & ~(pagesize-1));
#ifdef notdef
	va = valloc(sz*2);
#endif
        pos = (unsigned)kernaddr & ~(pagesize-1);
        off = (unsigned)kernaddr - pos;
        sz = size + off;
        sz = (sz+pagesize-1) & ~(pagesize-1);
	if (mmap(va, sz, PROT_READ, MAP_SHARED, km, pos) != 0) {
		perror("mmap");
		exit(1);
	}
        return((va + off));
}

struct	nlist nl[] = {
	{ "_tcp_debug" },
	{ "_tcp_debx" },
	0
};


init_mmap()
{
	static int initted = 0;

	if (initted)
		return;
	initted++;

	if ((km = open(kmem_file, O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open kmem file %s\n", kmem_file);
		perror("open");
		exit(1);
	}

	nlist(unix_file, nl);
	if (nl[0].n_value == 0) {
		fprintf(stderr, "Can't find tcp_debug in %s\n", unix_file);
		perror("nlist");
		exit(1);
	}
	if (nl[1].n_value == 0) {
		fprintf(stderr, "Can't find tcp_debx in %s\n", unix_file);
		perror("nlist");
		exit(1);
	}
	pagesize = getpagesize();
	tcp_debxp = (int *)do_mmap(sizeof(int), nl[1].n_value);
	tcp_debugp = (struct tcp_debug *)do_mmap(
	    sizeof(struct tcp_debug) * TCP_NDEBUG, nl[0].n_value);


	if (debug) {
		fprintf(stderr, "tcp_debug= %x, tcp_debx= %x\n", 
		    tcp_debugp, tcp_debxp);
		fprintf(stderr, "tcp_debug= %x, tcp_debx= %x\n",
		    *tcp_debugp, *tcp_debxp);
	}
}

do_dump()
{
	int loop = !oneshot;	
	int last = 0;
	int n;
	
	do {
		while (last != *tcp_debxp) {
			n = write(ofno, &tcp_debugp[last],
				sizeof(struct tcp_debug));
			if (n < 0)
				perror("write error??");
			else {
				bytes += sizeof(struct tcp_debug);
				records++;
			}

			if (last++ == TCP_NDEBUG)
				last = 0;
		}
	} while (loop);
}
