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
static char rcsid[] = "$Header: tprint.c 1.1 91/04/03 $";
#endif

/*
 * tprint.c:
 * The second half of trpt.c -- This program reads from a file the binary
 * tcp debugging data which tdmp produced, formatting the output as
 * trpt would have.  All options to this program are identical to trpt,
 * with the addition of a file name argument.
 */


#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
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

n_time	ntime;
int	sflag;
int	tflag;
int	jflag;
int	aflag;
int	numeric();
struct	tcp_debug *tcp_info;
int	tcp_debug_size;
caddr_t	*tcp_pcbs;
int	tcp_debx;
char	*ntoa();
char	*infilename = "tdmp.dmp";
char	*outfilename = "tprint.print";
FILE	*infile;
FILE	*outfile;
int	ifno;
int	records = 0;

main(argc, argv)
	int argc;
	char **argv;
{
	int	i, npcbs = 0;
	extern	char *optarg;
	extern	int optind;
	char	ch;

	while((ch = getopt(argc, argv, "astjp:o:i:")) != EOF) {
		switch(ch) {
		case 'a':
			aflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'j':
			jflag = 1;
			break;
		case 'p':
			if (npcbs >= TCP_NDEBUG) {
				fprintf(stderr,
				    "-p: too many pcb's specified\n");
				exit(1);
			}
			sscanf(optarg, "%x", &tcp_pcbs[npcbs++]);
			break;
		case 'i':
			infilename = optarg;
			break;
		case 'o':
			outfilename = optarg;
			break;
		default:
usage:
			fprintf(stderr, 
"usage: %s [ -a ] [ -s ] [ -t ] [ -j ] [ -p pcb ] [ -i ifile ] [ -o ofile ]\n",
				argv[0]);
			exit(1);
		}
	}

	if (argc - optind != 0)
		goto usage;
	
	if (*infilename == '-')
		ifno = fileno(stdin);
	else
		if ((ifno = open(infilename, O_RDONLY, 0)) <0) {
			fprintf(stderr, "can't open input file %s:",
			    infilename);
			perror("open");
			exit(1);
		}

	if (*outfilename == '-') {
		outfile = stdout;
		outfilename = "stdout";
	} else
		if ((outfile = fopen(outfilename, "w+")) == NULL) {
			fprintf(stderr, "can't open output file %s:",
			    outfilename);
			perror("open");
			exit(1);
		}

	if (*infilename == '-') {
		tcp_info = (struct tcp_debug *)
				malloc(sizeof(struct tcp_debug) * TCP_NDEBUG);
		tcp_pcbs = (caddr_t *) malloc(sizeof(caddr_t) * TCP_NDEBUG);
		tcp_debug_size = TCP_NDEBUG;
	} else {
		struct stat s;

		if (fstat(ifno, &s) < 0) {
			perror("fstat");
			exit(1);
		}
		tcp_info = (struct tcp_debug *) malloc(s.st_size);
		tcp_debug_size = s.st_size / sizeof(struct tcp_debug);
		if (tcp_debug_size * sizeof(struct tcp_debug) != s.st_size)
			tcp_debug_size--;
		tcp_pcbs = (caddr_t *) malloc(sizeof(caddr_t) * tcp_debug_size);
	}

	for (i = 0; i < tcp_debug_size; i++) {
		if (read(ifno, tcp_info+i, sizeof(struct tcp_debug)) < 0) {
			break;	/* EOF?? */
		}
	}
	tcp_debug_size = i;

	/*
	 * If no control blocks have been specified, figure
	 * out how many distinct one we have and summarize
	 * them in tcp_pcbs for sorting the trace records
	 * below.
	 */
	if (npcbs == 0) {
		for (i = 0; i < tcp_debug_size; i++) {
			register int j;
			register struct tcp_debug *td = tcp_info+i;

			if (td->td_tcb == 0)
				continue;
			for (j = 0; j < npcbs; j++)
				if (tcp_pcbs[j] == td->td_tcb)
					break;
			if (j >= npcbs)
				tcp_pcbs[npcbs++] = td->td_tcb;
		}
	}
	qsort(tcp_pcbs, npcbs, sizeof (caddr_t), numeric);
	if (jflag) {
		char *cp = "";

		for (i = 0; i < npcbs; i++) {
			fprintf(outfile, "%s%x", cp, tcp_pcbs[i]);
			cp = ", ";
		}
		if (*cp)
			fprintf(outfile,"\n");
		exit(0);
	}
	for (i = 0; i < npcbs; i++) {
		fprintf(outfile, "\n%x:\n", tcp_pcbs[i]);
		dotrace(tcp_pcbs[i]);
	}
	printf("formatted %d records to file %s\n", records, outfilename);

	exit(0);
}

dotrace(tcpcb)
	register caddr_t tcpcb;
{
	register int i;
	register struct tcp_debug *td;

	for (i = 0; i < tcp_debug_size; i++) {
		td = tcp_info+i;
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, td->td_tcb, &td->td_cb,
		    &td->td_ti, td->td_req);
		records++;
	}
}

/*
 * Tcp debug routines
 */
tcp_trace(act, ostate, atp, tp, ti, req)
	short act, ostate;
	struct tcpcb *atp, *tp;
	struct tcpiphdr *ti;
	int req;
{
	tcp_seq seq, ack;
	int len, flags, win, timer;
	char *cp;

	ptime(ntime);
	fprintf(outfile, "%s:%s ", tcpstates[ostate], tanames[act]);
	switch (act) {

	case TA_INPUT:
	case TA_OUTPUT:
	case TA_RESPOND:
	case TA_DROP:
		if (aflag) {
			fprintf(outfile, "(src=%s,%d, ", ntoa(ti->ti_src),
				ntohs(ti->ti_sport));
			fprintf(outfile, "dst=%s,%d)", ntoa(ti->ti_dst),
				ntohs(ti->ti_dport));
		}
		seq = ti->ti_seq;
		ack = ti->ti_ack;
		len = ti->ti_len;
		win = ti->ti_win;
		if (act == TA_OUTPUT) {
			seq = ntohl(seq);
			ack = ntohl(ack);
			len = ntohs(len);
			win = ntohs(win);
		}
		if (act == TA_OUTPUT)
			len -= sizeof (struct tcphdr);
		if (len)
			fprintf(outfile, "[%x..%x)", seq, seq+len);
		else
			fprintf(outfile, "%x", seq);
		fprintf(outfile, "@%x", ack);
		if (win)
			fprintf(outfile, "(win=%x)", win);
		flags = ti->ti_flags;
		if (flags) {
			char *cp = "<";
#define pf(f) { if (ti->ti_flags&TH_/**/f) { fprintf(outfile, "%s%s", cp, "f"); cp = ","; } }
			pf(SYN); pf(ACK); pf(FIN); pf(RST); pf(PUSH); pf(URG);
			fprintf(outfile, ">");
		}
		break;

	case TA_USER:
		timer = req >> 8;
		req &= 0xff;
		fprintf(outfile, "%s", prurequests[req]);
		if (req == PRU_SLOWTIMO || req == PRU_FASTTIMO)
			fprintf(outfile, "<%s>", tcptimers[timer]);
		break;
	}
	fprintf(outfile, " -> %s", tcpstates[tp->t_state]);
	/* print out internal state of tp !?! */
	fprintf(outfile, "\n");
	if (sflag) {
		fprintf(outfile, "\trcv_nxt %x rcv_wnd %x snd_una %x snd_nxt %x snd_max %x\n",
		    tp->rcv_nxt, tp->rcv_wnd, tp->snd_una, tp->snd_nxt,
		    tp->snd_max);
		fprintf(outfile, "\tsnd_wl1 %x snd_wl2 %x snd_wnd %x iss %x irs %x\n", tp->snd_wl1,
		    tp->snd_wl2, tp->snd_wnd, tp->iss, tp->irs);
	}
	/* print out timers? */
	if (tflag) {
		char *cp = "\t";
		register int i;

		for (i = 0; i < TCPT_NTIMERS; i++) {
			if (tp->t_timer[i] == 0)
				continue;
			fprintf(outfile, "%s%s=%d", cp, tcptimers[i], tp->t_timer[i]);
			if (i == TCPT_REXMT)
				fprintf(outfile, " (t_rxtshft=%d)", tp->t_rxtshift);
			cp = ", ";
		}
		if (*cp != '\t')
			fprintf(outfile, "\n");
	}
}

ptime(ms)
	int ms;
{

	fprintf(outfile, "%05d ", (ms/10) % 100000);
}

numeric(c1, c2)
	caddr_t *c1, *c2;
{
	
	return (*c1 - *c2);
}

/*
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */
char *
ntoa(in)
	struct in_addr in;
{
	static char b[18];
	register char *p;

	in.s_addr = ntohl(in.s_addr);
	p = (char *)&in;
#define	UC(b)	(((int)b)&0xff)
	sprintf(b, "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
	return (b);
}
