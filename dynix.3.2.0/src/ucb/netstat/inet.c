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
static char rcsid[] = "$Header: inet.c 2.10 1991/04/16 01:29:00 $";
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <netdb.h>

struct	inpcb inpcb;
struct	tcpcb tcpcb;
struct	socket s;
struct	protosw proto;
extern	int kmem;
extern	int Aflag;
extern	int aflag;
extern	int nflag;

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	32
#endif

static	int first = 1;
char	*inetname();


#define MAXCACHE   256
struct ncache {
	int address;
	char name[32];
};
/*
 * cache of network names for IP mappings
 */
int netcache_free = 0;
struct ncache netcache[MAXCACHE];
/*
 * cache of host name mappings
*/
int hostcache_free = 0;
struct ncache hostcache[MAXCACHE];


/*
 * Print a summary of connections related to an Internet
 * protocol.  For TCP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
protopr(off, name)
	off_t off;
	char *name;
{
	struct inpcb cb;
	register struct inpcb *prev, *next;
	int istcp;
	int i;

	if (off == 0)
		return;
	istcp = strcmp(name, "tcp") == 0;
	for (i = 0; i < INP_HASHSZ; i++,
	    off = (off_t)(((struct inpcb *)off) + 1)) {
		klseek(kmem, off, 0);
		read(kmem, &cb, sizeof (struct inpcb));
		inpcb = cb;
		prev = (struct inpcb *)off;
		if (inpcb.inp_next == (struct inpcb *)off)
			continue;
		while (inpcb.inp_next != (struct inpcb *)off) {
			char *cp;

			next = inpcb.inp_next;
			klseek(kmem, (off_t)next, 0);
			read(kmem, &inpcb, sizeof (inpcb));
			if (inpcb.inp_prev != prev) {
				printf("???\n");
				break;
			}
			if (!aflag &&
			  inet_lnaof(inpcb.inp_laddr.s_addr) == INADDR_ANY) {
				prev = next;
				continue;
			}
			klseek(kmem, (off_t)inpcb.inp_socket, 0);
			read(kmem, &s, sizeof (s));
			if (istcp) {
				klseek(kmem, (off_t)inpcb.inp_ppcb, 0);
				read(kmem, &tcpcb, sizeof (tcpcb));
			}
			if (first) {
				printf("Active Internet connections");
				if (aflag)
					printf(" (including servers)");
				putchar('\n');
				if (Aflag)
					printf("%-8.8s ", "PCB");
				printf(Aflag ?
					"%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
					"%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
					"Proto", "Recv-Q", "Send-Q",
					"Local Address", "Foreign Address", "(state)");
				first = 0;
			}
			if (Aflag)
				if (istcp)
					printf("%8x ", inpcb.inp_ppcb);
				else
					printf("%8x ", next);
			printf("%-5.5s %6d %6d ", name, s.so_rcv.sb_cc,
				s.so_snd.sb_cc);
			inetprint(&inpcb.inp_laddr, inpcb.inp_lport, name);
			inetprint(&inpcb.inp_faddr, inpcb.inp_fport, name);
			if (istcp) {
				if (tcpcb.t_state < 0 || tcpcb.t_state >= TCP_NSTATES)
					printf(" %d", tcpcb.t_state);
				else
					printf(" %s", tcpstates[tcpcb.t_state]);
			}
			putchar('\n');
			prev = next;
		}
	}
}

/*
 * Dump TCP statistics structure.
 */
tcp_stats(off, name)
	off_t off;
	char *name;
{
	struct tcpstat tcpstat;

	if (off == 0)
		return;

	klseek(kmem, off, 0);
	read(kmem, (char *)&tcpstat, sizeof (tcpstat));

#define	p(f, m)		printf(m, tcpstat.f, plural(tcpstat.f))
#define	p2(f1, f2, m)	printf(m, tcpstat.f1, plural(tcpstat.f1), tcpstat.f2, plural(tcpstat.f2))
  
	printf("%s:\n", name);
	p(tcps_sndtotal, "\t%d packet%s sent\n");
	p2(tcps_sndpack,tcps_sndbyte,
		"\t\t%d data packet%s (%d byte%s)\n");
	p2(tcps_sndrexmitpack, tcps_sndrexmitbyte,
		"\t\t%d data packet%s (%d byte%s) retransmitted\n");
	p2(tcps_sndacks, tcps_delack,
		"\t\t%d ack-only packet%s (%d delayed)\n");
	p(tcps_sndurg, "\t\t%d URG only packet%s\n");
	p(tcps_sndprobe, "\t\t%d window probe packet%s\n");
	p(tcps_sndwinup, "\t\t%d window update packet%s\n");
	p(tcps_sndctrl, "\t\t%d control packet%s\n");
	p(tcps_rcvtotal, "\t%d packet%s received\n");
	p2(tcps_rcvackpack, tcps_rcvackbyte, "\t\t%d ack%s (for %d byte%s)\n");
	p(tcps_rcvdupack, "\t\t%d duplicate ack%s\n");
	p(tcps_rcvacktoomuch, "\t\t%d ack%s for unsent data\n");
	p2(tcps_rcvpack, tcps_rcvbyte,
		"\t\t%d packet%s (%d byte%s) received in-sequence\n");
	p2(tcps_rcvduppack, tcps_rcvdupbyte,
		"\t\t%d completely duplicate packet%s (%d byte%s)\n");
	p2(tcps_rcvpartduppack, tcps_rcvpartdupbyte,
		"\t\t%d packet%s with some dup. data (%d byte%s duped)\n");
	p2(tcps_rcvoopack, tcps_rcvoobyte,
		"\t\t%d out-of-order packet%s (%d byte%s)\n");
	p2(tcps_rcvpackafterwin, tcps_rcvbyteafterwin,
		"\t\t%d packet%s (%d byte%s) of data after window\n");
	p(tcps_rcvwinprobe, "\t\t%d window probe%s\n");
	p(tcps_rcvwinupd, "\t\t%d window update packet%s\n");
	p(tcps_rcvafterclose, "\t\t%d packet%s received after close\n");
	p(tcps_rcvbadsum, "\t\t%d discarded for bad checksum%s\n");
	p(tcps_rcvbadoff, "\t\t%d discarded for bad header offset field%s\n");
	p(tcps_rcvshort, "\t\t%d discarded because packet too short\n");
	p(tcps_connattempt, "\t%d connection request%s\n");
	p(tcps_accepts, "\t%d connection accept%s\n");
	p(tcps_connects, "\t%d connection%s established (including accepts)\n");
	p2(tcps_closed, tcps_drops,
		"\t%d connection%s closed (including %d drop%s)\n");
	p(tcps_conndrops, "\t%d embryonic connection%s dropped\n");
	p2(tcps_rttupdated, tcps_segstimed,
		"\t%d segment%s updated rtt (of %d attempt%s)\n");
	p(tcps_rexmttimeo, "\t%d retransmit timeout%s\n");
	p(tcps_timeoutdrop, "\t\t%d connection%s dropped by rexmit timeout\n");
	p(tcps_persisttimeo, "\t%d persist timeout%s\n");
	p(tcps_keeptimeo, "\t%d keepalive timeout%s\n");
	p(tcps_keepprobe, "\t\t%d keepalive probe%s sent\n");
	p(tcps_keepdrops, "\t\t%d connection%s dropped by keepalive\n");
#undef p
#undef p2
}

/*
 * Dump UDP statistics structure.
 */
udp_stats(off, name)
	off_t off;
	char *name;
{
	struct udpstat udpstat;

	if (off == 0) 
		return;

	klseek(kmem, off, 0);
	read(kmem, (char *)&udpstat, sizeof (udpstat));
	printf("%s:\n\t%u incomplete header%s\n", name,
		udpstat.udps_hdrops, plural(udpstat.udps_hdrops));
	printf("\t%u bad data length field%s\n",
		udpstat.udps_badlen, plural(udpstat.udps_badlen));
	printf("\t%u bad checksum%s\n",
		udpstat.udps_badsum, plural(udpstat.udps_badsum));
	printf("\t%d socket overflow%s\n",
		udpstat.udps_fullsock, plural(udpstat.udps_fullsock));
}

/*
 * Dump IP statistics structure.
 */
ip_stats(off, name)
	off_t off;
	char *name;
{
	struct ipstat ipstat;

	if (off == 0)
		return;

	klseek(kmem, off, 0);
	read(kmem, (char *)&ipstat, sizeof (ipstat));
	printf("%s:\n\t%u total packets received\n", name,
		ipstat.ips_total);
	printf("\t%u bad header checksum%s\n",
		ipstat.ips_badsum, plural(ipstat.ips_badsum));
	printf("\t%u with size smaller than minimum\n", ipstat.ips_tooshort);
	printf("\t%u with data size < data length\n", ipstat.ips_toosmall);
	printf("\t%u with header length < data size\n", ipstat.ips_badhlen);
	printf("\t%u with data length < header length\n", ipstat.ips_badlen);
	printf("\t%u fragment%s received\n",
		ipstat.ips_fragments, plural(ipstat.ips_fragments));
	printf("\t%u fragment%s dropped (dup or out of space)\n",
		ipstat.ips_fragdropped, plural(ipstat.ips_fragdropped));
	printf("\t%u fragment%s dropped after timeout\n",
		ipstat.ips_fragtimeout, plural(ipstat.ips_fragtimeout));
	printf("\t%u packet%s forwarded\n",
		ipstat.ips_forward, plural(ipstat.ips_forward));
	printf("\t%u packet%s not forwardable\n",
		ipstat.ips_cantforward, plural(ipstat.ips_cantforward));
	printf("\t%u redirect%s sent\n",
		ipstat.ips_redirectsent, plural(ipstat.ips_redirectsent));
}

static	char *icmpnames[] = {
	"echo reply",
	"#1",
	"#2",
	"destination unreachable",
	"source quench",
	"routing redirect",
	"#6",
	"#7",
	"echo",
	"#9",
	"#10",
	"time exceeded",
	"parameter problem",
	"time stamp",
	"time stamp reply",
	"information request",
	"information request reply",
	"address mask request",
	"address mask reply",
};

/*
 * Dump ICMP statistics.
 */
icmp_stats(off, name)
	off_t off;
	char *name;
{
	struct icmpstat icmpstat;
	register int i, first;

	if (off == 0)
		return;
	klseek(kmem, off, 0);
	read(kmem, (char *)&icmpstat, sizeof (icmpstat));
	printf("%s:\n\t%u call%s to icmp_error\n", name,
		icmpstat.icps_error, plural(icmpstat.icps_error));
	printf("\t%u error%s not generated 'cuz old message was icmp\n",
		icmpstat.icps_oldicmp, plural(icmpstat.icps_oldicmp));
	for (first = 1, i = 0; i < ICMP_IREQREPLY + 1; i++)
		if (icmpstat.icps_outhist[i] != 0) {
			if (first) {
				printf("\tOutput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %u\n", icmpnames[i],
				icmpstat.icps_outhist[i]);
		}
	printf("\t%u message%s with bad code fields\n",
		icmpstat.icps_badcode, plural(icmpstat.icps_badcode));
	printf("\t%u message%s < minimum length\n",
		icmpstat.icps_tooshort, plural(icmpstat.icps_tooshort));
	printf("\t%u bad checksum%s\n",
		icmpstat.icps_checksum, plural(icmpstat.icps_checksum));
	printf("\t%u message%s with bad length\n",
		icmpstat.icps_badlen, plural(icmpstat.icps_badlen));
	for (first = 1, i = 0; i < ICMP_IREQREPLY + 1; i++)
		if (icmpstat.icps_inhist[i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %u\n", icmpnames[i],
				icmpstat.icps_inhist[i]);
		}
	printf("\t%u message response%s generated\n",
		icmpstat.icps_reflect, plural(icmpstat.icps_reflect));
}

/*
 * return the name of a network via the given IP address.
 * if not found query  the system routines
 */
char *
cachenetbyaddr(net, family)
	int net;
	int family;
{
	int i;
	struct ncache *np = NULL;
	struct netent *mp;

	for(i = 0, np = netcache ; i < netcache_free; i++, np++) {
		if(np->address == net)
			return(np->name);
	}

	/* try the name resolver */
	mp = getnetbyaddr(net, family);
	/*
	 * enter into cache
	 */
	if(mp) {
		if(netcache_free < MAXCACHE) {
			bzero(netcache[netcache_free].name, 32);
			strncpy(netcache[netcache_free].name , mp->n_name, 32);
			netcache[netcache_free].address = net;
			netcache_free++;
		}
		return(mp->n_name);
	}

	return((char *)0);
}

/*
 * return the name of a host via the given IP address.
 * if not found query  the system routines
 */
char *
cachehostbyaddr(in, len, family)
	struct in_addr *in;
	int len;
	int family;
{
	int i;
	struct ncache *np = NULL;
	struct hostent *hp;

	for(i = 0, np = hostcache ; i < hostcache_free; i++, np++) {
		if(np->address == in->s_addr)
			return(np->name);
	}

	/* try the name resolver */
	hp = gethostbyaddr(in, len, family);
	/*
	 * enter into cache
	 */
	if(hp) {
		if(hostcache_free < MAXCACHE) {
			bzero(hostcache[hostcache_free].name,32);
			strncpy(hostcache[hostcache_free].name , hp->h_name, 32);
			hostcache[hostcache_free].address = in->s_addr;
			hostcache_free++;
		}
		return(hp->h_name);
	}

	return((char *)0);
}



/*
 * Pretty print an Internet address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */
inetprint(in, port, proto)
	register struct in_addr *in;
	int port;
	char *proto;
{
	struct servent *sp = 0;
	char line[80], *cp, *index();
	int width;

	sprintf(line, "%.*s.", (Aflag && !nflag) ? 12 : 16, inetname(*in));
	cp = index(line, '\0');
	if (!nflag && port)
		sp = getservbyport(port, proto);
	if (sp || port == 0)
		sprintf(cp, "%.8s", sp ? sp->s_name : "*");
	else
		sprintf(cp, "%d", ntohs((u_short)port));
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give 
 * numeric value, otherwise try for symbolic name.
 */
char *
inetname(in)
	struct in_addr in;
{
	register char *cp, *cp2;
	static char line[50];
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;

	if (first && !nflag) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag && in.s_addr != INADDR_ANY) {
		int net = inet_netof(in);
		int lna = inet_lnaof(in);

		if (lna == INADDR_ANY) {
			cp = cachenetbyaddr(net, AF_INET);
		}
		if (cp == 0) {
			cp = cachehostbyaddr(&in, sizeof (in), AF_INET);
			if (cp) {
				if ((cp2 = index(cp, '.')) &&
				    !strcmp(cp2 + 1, domain))
					*cp2 = 0;
			}
		}
	}
	if (in.s_addr == INADDR_ANY)
		strcpy(line, "*");
	else if (cp)
		strcpy(line, cp);
	else {
		in.s_addr = ntohl(in.s_addr);
#define C(x)	((x) & 0xff)
		sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
	}
	return (line);
}
