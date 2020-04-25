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

#ifndef	lint
static	char	rcsid[] = "$Header: emon_lookit.c 2.6 87/05/08 $";
#endif

/*
 * $Log:	emon_lookit.c,v $
 */

#include "emon.h"

#define ETHERPUP_XNSTYPE 0x600

int scan;		/* global for do_do_lookit and getindex */

char	get1char();

do_do_lookit()		/* look at buffers received */
{
	int cc;
	char arg[8];

	spl7();		/* stop receiving packets while looking */
	getarg(arg, 8);
	if(arg[0] != '\0') {
		cc = atoi(arg);
		scan = cc;
		cc = printpacket(scan);
	} else {

		/*
		 * no particular packet specified
		 */

		scan = -1;
		do_do_stats();	/* print stats */

		printf("summary(CR)?");
		cc = get1char();
		if(cc == 'y' || cc == '\n')
			do_do_summ();	/* summarize all packets */
	}

	/*
	 * print out packets
	 */

	if(cc != 'x')
		while((cc = getindex()) >= 0)
			if(printpacket(cc) == 'x') break;

	spl0();		/* re-enable interrupts */
	return(0);
}

/*----------------------------------------------------*/

printpacket(cc)		/* print contents of e-packet */
	int cc;
{
	register struct il_rpacket	*il; 
	struct wbuf			*wptr;
	u_char				*w;	/* w -> byte string */
	int				length;
	char				c = '\0';
	u_short				type;

	w = (u_char *) ((int) wbuf_begin + (sizeof(struct wbuf) * cc));
	printf("&wbuf[%d] = 0x%x\n\n", cc, w);

	il = (struct il_rpacket *) w;	/** point to received packet **/
	wptr = (struct wbuf *) il;

	printf("u_char\tether_status - %x\tu_char ether_fill1 %x\n",
		il->ether_status, il->ether_fill1);
	printf("int\tether_length - %x (%d)\tN.B. NOT ntohs\n",
		wptr->wbuf_len, wptr->wbuf_len);

	printf("u_char\tether_dhost[6] = %x %x %x %x %x %x -- %s\n",
		il->ether_dhost[0], il->ether_dhost[1],
		il->ether_dhost[2], il->ether_dhost[3],
		il->ether_dhost[4], il->ether_dhost[5],
		enameof((caddr_t)il->ether_dhost));

	printf("u_char\tether_shost[6] = %x %x %x %x %x %x - %s\n",
		il->ether_shost[0], il->ether_shost[1],
		il->ether_shost[2], il->ether_shost[3],
		il->ether_shost[4], il->ether_shost[5],
		enameof((caddr_t)il->ether_shost));

	printf("u_short\tether_type %x ntohs = 0x%x (%d)\n",
		il->ether_type,
		ntohs((u_short) il->ether_type),
		ntohs((u_short) il->ether_type));

	length = wptr->wbuf_len;

	printf("length -- %d\n", length);

	type = ntohs((u_short)il->ether_type);

	switch(type) {
	case ETHERPUP_IPTYPE:
		c = printiphdr((struct ip *)(&il->ip_tos));
		break;
	case ETHERPUP_ARPTYPE:
		printarp((struct ether_arp *) &il->ip_tos);
		break;
	case ETHERPUP_XNSTYPE:
		printxns((struct idp *)(&il->ip_tos));
		break;
	case ETHERPUP_PUPTYPE:
		printpup(il);
		break;
	case ETHERPUP_ATALKTYPE:
		printat((struct lap *) (&il->ip_tos));
		break;
	default:
		if (type >= ETHERPUP_TRAIL &&
		    type < ETHERPUP_TRAIL + ETHERPUP_NTRAILER)
			printrail(il);
		else
			printunk(il);
	}

	if(c != 'n')
		printw((u_char *)il, (length+8));

	printf("wbuf_index is %d\n", wbuf_index);
	printf("that was buffer %d\n", cc);
	return(c);
}

/*----------------------------------------------------*/

printiphdr(il)
	struct ip * il;
{
	int		length, cksum;
	char		c;

	printf("IP_PACKET header ->\n");

	printf("u_char\tip_hl:4, = \t0x%x - 4-bit nibble\n", il->ip_hl);
	printf("\tip_v:4 = \t0x%x - 4-bit nibble\n", il->ip_v);
	printf("u_char\tip_tos =\t0x%x\n", il->ip_tos);
	printf("short\tip_len =\t0x%x\tntohs(ip_len) =\t0x%x (%d)\n",
		il->ip_len, ntohs((u_short) il->ip_len),
		ntohs((u_short) il->ip_len));
	printf("u_short\tip_id = \t0x%x\tntohs(ip_id) =\t0x%x\n",
		il->ip_id, ntohs(il->ip_id));
	printf("short\tip_off =\t0x%x\tntohs(ip_off) =\t0x%x (%d)\n",
		il->ip_off, ntohs((u_short) il->ip_off),
		ntohs((u_short) il->ip_off));
	printf("short\tip_ttl =\t0x%x\tntohs(ip_ttl) =\t0x%x (%d)\n",
		il->ip_ttl, ntohs((u_short) il->ip_ttl),
		ntohs((u_short) il->ip_ttl));
	printf("u_char\tip_p =\t0x%x\n", il->ip_p);
	printf("u_short\tip_sum =\t0x%x ntohs(ip_sum) =\t0x%x\t",
		il->ip_sum, ntohs((u_short) il->ip_sum));
		if((cksum = in_cksum((u_short *)il, 20)))
			if(il->ip_sum)
				printf("IP CKSUM FAILS -> 0x%x\n", cksum);
			else
				printf("IP NOT CKSUMED -> 0x%x\n", cksum);
		else 
			printf("IP CKSUM OK\n");
	printf("struct\tin_addr ip_src =\t0x%x\n", il->ip_src);
	printf("\tntohl(ip_src.s_addr) =\t0x%x\n",
			ntohl((u_long)il->ip_src.s_addr));
	printf("struct\tin_addr ip_dst =\t0x%x\n", il->ip_dst);
	printf("\tntohl(ip_dst.s_addr) =\t0x%x\n",
			ntohl((u_long) il->ip_dst.s_addr));

	length = ntohs((u_short) il->ip_len) + 8;

	printf("\t\t\t%d bytes in ", ntohs((u_short) il->ip_len));

#define	DUMPIT (dumpit ? (c == 'n') : ((c = getchar()) == 'n'))

	switch(il->ip_p) {
	case IPPROTO_TCP:
		printf("TCP PACKET - tcphdr =>\n");
		if(!DUMPIT)
			printtcp((struct tcpiphdr *) il);
		break;
	case IPPROTO_UDP:
		printf("UDP PACKET - udphdr =>\n");
		if(!DUMPIT)
			printudp((struct udpiphdr *) il);
		break;
	case IPPROTO_ICMP:
		printf("ICMP PACKET ->\n");
		if(!DUMPIT)
			printicmp((struct icmp *)
				(((int) &il->ip_dst)+sizeof(il->ip_dst)));
		break;
	default:
		printf("\t\tUNKNOWN IP PACKET TYPE??\n");
		if(!DUMPIT)
			printw((u_char *)il, length);
	}
	putchar('\n');
	return(c);
}

/*----------------------------------------------------*/

printtcp(ti)
	struct tcpiphdr *ti;
{
	int cksum;
	caddr_t save_ti_next, save_ti_prev;
	u_char save_ti_x1;
	short save_ti_len;
	int tlen;
	u_char flags;

	printf("u_short\tth_sport =\t%x\tntohs =\t%x (%d)\n", ti->ti_sport,
			ntohs((u_short) ti->ti_sport),
			ntohs((u_short) ti->ti_sport));
	printf("u_short\tth_dport =\t%x\tntohs =\t%x (%d)\n", ti->ti_dport,
			ntohs((u_short) ti->ti_dport),
			ntohs((u_short) ti->ti_dport));
	printf("FYI typedef u_long	tcp_seq\n");
	printf("tcp_seq\tth_seq =\t%x \tntohl =\t%x\n", ti->ti_seq,
			ntohl(ti->ti_seq));		/* sequence number */
	printf("tcp_seq\tth_ack =\t%x \tntohl =\t%x\n", ti->ti_ack,
			ntohl(ti->ti_ack));	/* acknowledgement number */
	printf("u_char\tth_x2:4, /* (unused) */\n");
	printf("\tth_off:4\t%x\n", ti->ti_off); /* data offset */
	printf("u_char\tth_flags =\t%x\t", ti->ti_flags);

		flags = ti->ti_flags;
		if(flags & TH_URG) printf("TH_URG ");
		if(flags & TH_ACK) printf("TH_ACK ");
		if(flags & TH_PUSH) printf("TH_PUSH ");
		if(flags & TH_RST) printf("TH_RST ");
		if(flags & TH_SYN) printf("TH_SYN ");
		if(flags & TH_FIN) printf("TH_FIN ");
		printf("\n");

	printf("u_short\tth_win =\t%x\tntohs(tth_win) 0x%x (%d)\n",
		ti->ti_win, ntohs((u_short) ti->ti_win),
		ntohs((u_short) ti->ti_win));
	printf("u_short\tth_sum =\t%x\t", ti->ti_sum);	/* checksum */

	save_ti_next = ti->ti_next;
	save_ti_prev = ti->ti_prev;
	save_ti_x1 = ti->ti_x1;
	save_ti_len = ti->ti_len;

	tlen = (u_short) ((struct ip*)ti)->ip_len;
	tlen = ntohs((u_short) tlen);
	ti->ti_len = (u_short)tlen - sizeof(struct ip);
	ti->ti_len = htons((u_short) ti->ti_len);

	ti->ti_next = ti->ti_prev = 0;
	ti->ti_x1 = 0;

	if(cksum = in_cksum((u_short *)ti, tlen))
		printf("TCP CKSUM FAILS -> 0x%x\n", cksum);
	else
		printf("TCP CKSUM OK\n");

	ti->ti_next = save_ti_next;
	ti->ti_prev = save_ti_prev;
	ti->ti_x1 = save_ti_x1;
	ti->ti_len = save_ti_len;

	printf("u_short\tth_urp =\t%x\n", ti->ti_urp);  /* urgent pointer */
	return;
}

/*----------------------------------------------------*/

printudp(ui)
	struct udpiphdr * ui;
{
	caddr_t save_ui_next, save_ui_prev;
	u_char save_ui_x1;
	short save_ui_len;
	int ulen, cksum;

	printf("u_short\tuh_sport =\t0x%x\tntohs =\t0x%x (%d)\n",
		ui->ui_sport, ntohs((u_short) ui->ui_sport),
		ntohs((u_short) ui->ui_sport));
	printf("u_short\tuh_dport =\t0x%x\tntohs =\t0x%x (%d)\n",
		ui->ui_dport, ntohs((u_short) ui->ui_dport),
		ntohs((u_short) ui->ui_dport));
	printf("short\tuh_ulen =\t0x%x\tntohs =\t0x%x (%d)\n",
		ui->ui_ulen, ntohs((u_short) ui->ui_ulen),
		ntohs((u_short) ui->ui_ulen));
	printf("u_short\tuh_sum =\t0x%x\tntohs =\t0x%x (%d)\t",
		ui->ui_sum, ntohs((u_short) ui->ui_sum),
		ntohs((u_short) ui->ui_sum));

	save_ui_next = ui->ui_next;
	save_ui_prev = ui->ui_prev;
	save_ui_x1 = ui->ui_x1;
	save_ui_len = ui->ui_len;

	ulen = ntohs((u_short)ui->ui_ulen);

	ui->ui_next = ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_len = htons((u_short)ulen);

	if(cksum = in_cksum((u_short *) ui, (ulen + sizeof(struct ip))))
		if(ui->ui_sum)
			printf("UDP CKSUM FAILS -> 0x%x\n", cksum);
		else
			printf("UDP NOT CKSUMED -> 0x%x\n", cksum);
	else
		printf("UDP CKSUM OK\n");

	ui->ui_next = save_ui_next;
	ui->ui_prev = save_ui_prev;
	ui->ui_x1 = save_ui_x1;
	ui->ui_len = save_ui_len;
	return;
}

/*----------------------------------------------------*/

printicmp(icmpp)
	struct icmp * icmpp;
{
	int	code = icmpp->icmp_code;

	printf("u_char	icmp_type = 0x%x, ", icmpp->icmp_type);
	printf("icmp_code = 0x%x ", code);
	switch(icmpp->icmp_type) {
	case	ICMP_ECHOREPLY:
		printf("ICMP_ECHOREPLY");
		break;
	case	ICMP_UNREACH:
		printf("ICMP_UNREACH -- ");
		switch(code) {
		case	ICMP_UNREACH_NET:
			printf("ICMP_UNREACH_NET");
			break;
		case	ICMP_UNREACH_HOST:
			printf("ICMP_UNREACH_HOST");
			break;
		case	ICMP_UNREACH_PROTOCOL:
			printf("ICMP_UNREACH_PROTOCOL");
			break;
		case	ICMP_UNREACH_PORT:
			printf("ICMP_UNREACH_PORT");
			break;
		case	ICMP_UNREACH_NEEDFRAG:
			printf("ICMP_UNREACH_NEEDFRAG");
			break;
		case	ICMP_UNREACH_SRCFAIL:
			printf("ICMP_UNREACH_SRCFAIL");
			break;
		default:
			printf("???");
		}
		break;
	case	ICMP_SOURCEQUENCH:
		printf("ICMP_SOURCEQUENCH");
		break;
	case	ICMP_REDIRECT:
		printf("ICMP_REDIRECT -- ");
		switch(code) {
		case	ICMP_REDIRECT_NET:
			printf("ICMP_REDIRECT_NET");
			break;
		case	ICMP_REDIRECT_HOST:
			printf("ICMP_REDIRECT_HOST");
			break;
		case	ICMP_REDIRECT_TOSNET:
			printf("ICMP_REDIRECT_TOSNET");
			break;
		case	ICMP_REDIRECT_TOSHOST:
			printf("ICMP_REDIRECT_TOSHOST");
			break;
		default:
			printf("???");
		}
		break;
	case	ICMP_ECHO:
		printf("ICMP_ECHO");
		break;
	case	ICMP_TIMXCEED:
		printf("ICMP_TIMXCEED -- ");
		switch(code) {
		case	ICMP_TIMXCEED_INTRANS:
			printf("ICMP_TIMXCEED_INTRANS");
			break;
		case	ICMP_TIMXCEED_REASS:
			printf("ICMP_TIMEXCEED_REASS");
			break;
		default:
			printf("???");
		}
		break;
	case	ICMP_PARAMPROB:
		printf("ICMP_PARAMPROB");
		break;
	case	ICMP_TSTAMP:
		printf("ICMP_TSTAMP");
		break;
	case	ICMP_TSTAMPREPLY:
		printf("ICMP_TSTAMPREPLY");
		break;
	case	ICMP_IREQ:
		printf("ICMP_IREQ");
		break;
	case	ICMP_IREQREPLY:
		printf("ICMP_IREQREPLY");
		break;
	case	ICMP_MASKREQ:
		printf("ICMP_MASKREQ");
		break;
	case	ICMP_MASKREPLY:
		printf("ICMP_MASKREPLY");
		break;
	default:
		printf("???");
	}
	printf("\n");
	printf("u_short	icmp_cksum = 0x%x ntohs(0x%x)\n",
			icmpp->icmp_cksum, ntohs((u_short) icmpp->icmp_cksum));
	printf("icmp_pptr = 0x%x ntohs(0x%x) - ",
			icmpp->icmp_pptr, ntohs((u_short) icmpp->icmp_pptr));
	printf("icmp_gwaddr = 0x%x ntohl(0x%x)\n",
			icmpp->icmp_gwaddr, ntohl(icmpp->icmp_gwaddr));
	printf("icmp_id = 0x%x ntohs(0x%x) - ",
			icmpp->icmp_id, ntohs((u_short) icmpp->icmp_id));
	printf("icmp_seq = 0x%x ntohs(0x%x)\n",
			icmpp->icmp_seq, ntohs((u_short) icmpp->icmp_seq));
	printf("icmp_void = 0x%x ntohl(0x%x)\n", icmpp->icmp_void,
			ntohl((u_long) icmpp->icmp_void));
	printf("icmp_otime = 0x%x ntohl(0x%x)\n", icmpp->icmp_otime,
			ntohl((u_long) icmpp->icmp_otime));
	printf("icmp_rtime = 0x%x ntohl(0x%x)\n", icmpp->icmp_rtime,
			ntohl((u_long) icmpp->icmp_rtime));
	printf("icmp_ttime = 0x%x ntohl(0x%x)\n", icmpp->icmp_ttime,
			ntohl((u_long) icmpp->icmp_ttime));

#ifdef removed
	printf("ICMP_IP ->\n");

	printf("u_char\tip_hl:4, = \t0x%x - 4-bit nibble\n",
			icmpp->icmp_ip.ip_hl);
	printf("\tip_v:4 = \t0x%x - 4-bit nibble\n",
			icmpp->icmp_ip.ip_v);
	printf("u_char\tip_tos =\t0x%x\n",
			icmpp->icmp_ip.ip_tos);
	printf("short\tip_len =\t0x%x\tntohs(ip_len) =\t0x%x (%d)\n",
		icmpp->icmp_ip.ip_len,
		ntohs((u_short) icmpp->icmp_ip.ip_len),
		ntohs((u_short) icmpp->icmp_ip.ip_len));
	printf("u_short\tip_id =\t0x%x\tntohs(ip_id) =\t0x%x\n",
		icmpp->icmp_ip.ip_id, ntohs((u_short) icmpp->icmp_ip.ip_id));
	printf("short\tip_off =\t0x%x\tntohs(ip_off) =\t0x%x (%d)\n",
		icmpp->icmp_ip.ip_off,
		ntohs((u_short) icmpp->icmp_ip.ip_off),
		ntohs((u_short) icmpp->icmp_ip.ip_off));
	printf("short\tip_ttl =\t0x%x\tntohs(ip_ttl) =\t0x%x (%d)\n",
		icmpp->icmp_ip.ip_ttl,
		ntohs((u_short) icmpp->icmp_ip.ip_ttl),
		ntohs((u_short) icmpp->icmp_ip.ip_ttl));
	printf("u_char\tip_p =\t0x%x\n", icmpp->icmp_ip.ip_p);
	printf("u_short\tip_sum =\t0x%x ntohs(ip_sum) =\t0x%x\n",
		icmpp->icmp_ip.ip_sum,
		ntohs((u_short) icmpp->icmp_ip.ip_sum));
	printf("struct\tin_addr ip_src =\t0x%x\n",
		icmpp->icmp_ip.ip_src);
	printf("\tntohl(ip_src.s_addr) =\t0x%x\n",
		ntohl(icmpp->icmp_ip.ip_src.s_addr));
	printf("struct\tin_addr ip_dst =\t0x%x\n",
		icmpp->icmp_ip.ip_dst);
	printf("\tntohl(ip_dst.s_addr) =\t0x%x\n",
		ntohl(icmpp->icmp_ip.ip_dst.s_addr));
	summip(&icmpp->icmp_ip);
#endif removed
	printiphdr(&icmpp->icmp_ip);
	return;
}

/*----------------------------------------------------*/

printrail(il)
	struct il_rpacket * il;
{
	int len;
	int off, resid;
	short type;
	struct ip * trailip;
	struct wbuf* wptr;

	wptr = (struct wbuf *)il;

	printf("TRAILER PACKET ->\n");

	/*
	 * Deal with trailer protocol: if type is PUP trailer
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */

	type = ntohs((u_short)il->ether_type);

/* careful - &il->ip_tos hack */

#define	ildataaddr(il, off, type) ((type)(((caddr_t)(&il->ip_tos)+(off))))

	if (type >= ETHERPUP_TRAIL &&
	    type < ETHERPUP_TRAIL+ETHERPUP_NTRAILER) {
		off = (type - ETHERPUP_TRAIL) * 512;
		printf("calculated offset = 0x%x (%d)\n", off, off);
		if (off >= ETHERMTU) {
			printf("bad trailer off = %d\n", off);
			return;
		}
		type = ntohs(*ildataaddr(il, off, u_short *));
		printf("il_rpacket begins at 0x%x\n", il);
		printf("ildataaddr = 0x%x\n", ildataaddr(il, off, u_short *));
		printf("PACKET TYPE -> %x\n", type);
		resid = ntohs(*(ildataaddr(il, off+2, u_short *)));
		printf("resid = 0x%x (%d)\n", resid, resid);

		len = wptr->wbuf_len;

		if (off + resid > len){
			printf("bad trailer length? = %d\n", len);
			return;
		}
		len = off + resid;
	} else {
		printf("bad trailer type = 0x%x\n", il->ether_type);
		off = 0;
		return;
	}
	if (len == 0) {
		printf("bad trailer length? = %d\n", len);
		return;
	}
	if (off) {
		trailip = ildataaddr(il, off+4, struct ip *);
		if(type == ETHERPUP_IPTYPE)
			(void) printiphdr(trailip);
		else{
			printf("UNKNOWN (not IP) trailer\n");
			printw((u_char *) trailip, resid);
		}
	}
	return;
}

/*--------------------------------------------------------*/

/*ARGSUSED*/
printunk(il)
	struct il_rpacket * il;
{
	printf("UNKNOWN PACKET TYPE - raw data ->\n");
	return;
}

/*--------------------------------------------------------*/

char ffSMB[] = {0xff, 'S', 'M', 'B'};

printxns(idpp)
	struct idp * idpp;
{
	register struct sphdr * spp;
	register struct spidp * spi;
	register struct pxidp * pxi;
	register struct pxhdr * pxp;
	register struct smbidphdr * smbip;
	register struct smb_msg * smbp;
	int flags;

	printf("struct iphdr ----------------------------------\n");
	printf("u_short	idp_sum %x ntohs(%x)\n",idpp->idp_sum,
						ntohs((u_short)idpp->idp_sum));
	printf("u_short	idp_len %x ntohs(0x%x - %d)\n",idpp->idp_len,
						ntohs((u_short)idpp->idp_len),
						ntohs((u_short)idpp->idp_len));
	printf("u_char	idp_tc %x\n", idpp->idp_tc);
	printf("u_char	idp_pt %x - ", idpp->idp_pt);
	switch(idpp->idp_pt) {
	case NSPROTO_RI:
		printf("NSPROTO_RI\n");
		break;
	case NSPROTO_ECHO:
		printf("NSPROTO_ECHO\n");
		break;
	case NSPROTO_ERROR:
		printf("NSPROTO_ERROR\n");
		break;
	case NSPROTO_PE:
		printf("NSPROTO_PE\n");
		break;
	case NSPROTO_SPP:
		printf("NSPROTO_SPP\n");
		break;
	default:
		printf("NSPROTO_???\n");
		break;
	}
 printf("struct ns_addr	idp_dna net-%x %x %x %x  host-%x %x %x %x %x %x\n", 
				(u_char)idpp->idp_dna.c_net[0],
				(u_char)idpp->idp_dna.c_net[1],
				(u_char)idpp->idp_dna.c_net[2],
				(u_char)idpp->idp_dna.c_net[3],
				(u_char)idpp->idp_dna.c_host[0],
				(u_char)idpp->idp_dna.c_host[1],
				(u_char)idpp->idp_dna.c_host[2],
				(u_char)idpp->idp_dna.c_host[3],
				(u_char)idpp->idp_dna.c_host[4],
				(u_char)idpp->idp_dna.c_host[5]);
	printf("\t\tidp_dna.x_port %x ntohs(0x%x - %d)\n",
				(u_short)idpp->idp_dna.x_port,
				(u_short)ntohs((u_short)idpp->idp_dna.x_port),
				(u_short)ntohs((u_short)idpp->idp_dna.x_port));
 printf("struct ns_addr	idp_sna net-%x %x %x %x  host-%x %x %x %x %x %x\n", 
				(u_char)idpp->idp_sna.c_net[0],
				(u_char)idpp->idp_sna.c_net[1],
				(u_char)idpp->idp_sna.c_net[2],
				(u_char)idpp->idp_sna.c_net[3],
				(u_char)idpp->idp_sna.c_host[0],
				(u_char)idpp->idp_sna.c_host[1],
				(u_char)idpp->idp_sna.c_host[2],
				(u_char)idpp->idp_sna.c_host[3],
				(u_char)idpp->idp_sna.c_host[4],
				(u_char)idpp->idp_sna.c_host[5]);
	printf("\t\tidp_sna.x_port %x ntohs(0x%x - %d)\n",
				(u_short)idpp->idp_sna.x_port,
				(u_short)ntohs((u_short)idpp->idp_sna.x_port),
				(u_short)ntohs((u_short)idpp->idp_sna.x_port));
	switch(idpp->idp_pt) {

	case NSPROTO_RI:
		printf("Routing Information Packet NSPROTO_RI\n");
		break;
	case NSPROTO_ECHO:
		printf("Echo Protocol Packet NSPROTO_ECHO\n");
		break;
	case NSPROTO_ERROR:
		printf("Error Protocol Packet NSPROTO_ERROR\n");
		break;
	case NSPROTO_PE:
		printf("Packet Exchange Packet NSPROTO_PE\n");

		pxi = (struct pxidp *) idpp;
		pxp = (struct pxhdr *)pxi->pi_p;

		printf("u_long px_id %x ntohl(%x)\n", pxp->px_id,
							ntohl(pxp->px_id));
		printf("u_short px_type %x ntohs(%x)\n", pxp->px_type,
						ntohs((u_short)pxp->px_type));
		break;

	case NSPROTO_SPP:

		printf("Sequence Packet Protocol Packet NSPROTO_SPP-----\n");

		spi = (struct spidp *) idpp;
		spp = &spi->si_s;

		printf("u_char	sp_cc %x ", spp->sp_cc);
		flags = spp->sp_cc;
		if(flags & SP_SP) printf("SP_SP ");
		if(flags & SP_SA) printf("SP_SA ");
		if(flags & SP_OB) printf("SP_OB ");
		if(flags & SP_EM) printf("SP_EM ");

		if(flags & SP_SP) printf("system|");
		if(flags & SP_SA) printf("ack|");
		if(flags & SP_OB) printf("out of band|");
		if(flags & SP_EM) printf("end message|");

		printf("\n");

		printf("u_char	sp_dt %x\n", spp->sp_dt);
		printf("u_short	sp_sid %x ntohs(%x)\n", spp->sp_sid,
						ntohs((u_short) spp->sp_sid));
		printf("u_short	sp_did %x ntohs(%x)\n", spp->sp_did,
 						ntohs((u_short) spp->sp_did));
		printf("u_short	sp_seq %x ntohs(%x)\n", spp->sp_seq,
 						ntohs((u_short) spp->sp_seq));
		printf("u_short	sp_ack %x ntohs(%x)\n", spp->sp_ack,
 						ntohs((u_short) spp->sp_ack));
		printf("u_short	sp_alo %x ntohs(%x)\n", spp->sp_alo,
 						ntohs((u_short) spp->sp_alo));

		smbip = (struct smbidphdr *) idpp;
		smbp = &smbip->smbi_s;

	if((ntohs((u_short) idpp->idp_len) > 42) &&
	     (bcmp((caddr_t) smbp->smb_idf,
		(caddr_t) ffSMB, sizeof(ffSMB)) == 0))
			printsmb(smbp);
		break;
	}

	printf("XNS PACKET - raw data ->\n");

	return;
}

/*--------------------------------------------------------*/

/*ARGSUSED*/
printpup(il)
	struct il_rpacket * il;
{
	printf("PUP PACKET - raw data ->\n");
	return;
}

/*--------------------------------------------------------*/

printarp(arper)
	struct ether_arp * arper;
{

	printf("ARP PACKET - raw data ->\n");

	printf("u_short	arp_hrd = %x ntohs(%x) = ARPHRD_ETHER\n",
			arper->arp_hrd, ntohs((u_short) arper->arp_hrd));
	printf("u_short	arp_pro = %x ntohs(%x)\n",
			arper->arp_pro, ntohs((u_short) arper->arp_pro));
	printf("u_char	arp_hln = %x\n", arper->arp_hln);
	printf("u_char	arp_pln = %x\n", arper->arp_pln);
	printf("u_short	arp_op = %x noths(%x)\t",
			arper->arp_op, ntohs((u_short) arper->arp_op));

	if(ntohs((u_short) arper->arp_op) == ARPOP_REQUEST)
		printf("ARPOP_REQUEST\n");

	else if(ntohs((u_short) arper->arp_op) == ARPOP_REPLY)
		printf("ARPOP_REPLY\n");

	else printf("ARPOP_??\n");

	printf("sender h/w address u_char arp_sha[6] = %x %x %x %x %x %x\n",
		arper->arp_sha[0], arper->arp_sha[1], arper->arp_sha[2],
		arper->arp_sha[3], arper->arp_sha[4], arper->arp_sha[5]);

	printf("protocol address u_char	arp_spa[4] = %x %x %x %x\n",
			arper->arp_spa[0], arper->arp_spa[1],
			arper->arp_spa[2], arper->arp_spa[3]);

	printf("target h/w address u_char arp_tha[6] = %x %x %x %x %x %x\n",
		arper->arp_tha[0], arper->arp_tha[1], arper->arp_tha[2],
		arper->arp_tha[3], arper->arp_tha[4], arper->arp_tha[5]);

	printf("protocol address u_char	arp_tpa[4] = %x %x %x %x\n",
			arper->arp_tpa[0], arper->arp_tpa[1],
			arper->arp_tpa[2], arper->arp_tpa[3]);
	return;
}


/*----------------------------------------------------*/

		/*  utility routines */

getindex()
{
	printf("\ninput wbuf index < %d (CR or n)?", WNUMBUFS);
	(void) getline(tinput, 80);
	tindex = 0;
	while(tinput[0] == 's') {
		do_do_summ();
		printf("\ninput wbuf index < %d (CR or n)?", WNUMBUFS);
		(void) getline(tinput, 80);
		tindex = 0;
	}
	if(tinput[0] != 'n' && tinput[1] != 'x') {
		if(tinput[0] != '\n') {
			scan = atoi(tinput);
			if(scan > WNUMBUFS) {
				printf("WRONGO\n");
				scan = -1;
			}
		}else{
			scan = (scan + reverse) % WNUMBUFS;

			if (scan < 0) {	/* reverse wraparound */
				if(counter < WNUMBUFS)
					/* wbufs not filled */
					scan = wbuf_index;
				else
					/* wbufs filled */
					scan = WNUMBUFS - 1; /* i.e. 99 */
			}
		}
	}else		/* no more indices */
		scan = -1;
	return(scan);
}

/*--------------------------------------------------------*/

int translate();

printw(w, length)
	u_char	*w;
	int	length;
{
	char c;
	int i, j;
	u_char *wsave;

	if(!autoprint) {
		printf("[printit(y)?] ");
		c = get1char();
		if(c == '\n')
			c = savechar;
		else {
			savechar = c;
			putchar('\n');
		}
	} else
		c = savechar;

	if(c == 'y')
		for (i=0; i < length; i += XPERLINE) { /** note 16 per line */
			printf("%x ", w);
			wsave = w;
			for(j = 0; j < XPERLINE; j++)
				printf(" %x", *w++);
			printf("\t");
			translate(wsave, XPERLINE); /* try printing ascii */
			printf("\n");
		}
	return;
}

#define LINZPER 23

do_do_summ()		/* print summary records */
{
	struct il_rpacket * il;
	u_char * w;
	int i, cc;
	short screen;
	u_short type;
	int off, len, resid;
	struct ip * trailip;
	char mark;
	char arg[8];
	struct wbuf * wptr;

	screen = 0;
	getarg(arg,8);
	if(arg[0] != '\0')
		i = atoi(arg);
	else
		i = 0;

	for(; i < WNUMBUFS; i++) {
		if(i == counter) {
			printf("NO MORE DATA\n");
			break;
		}
		if(screen >= LINZPER) {
			printf("current index = %d ", wbuf_index);
			cc = getline(tinput, 80);
			tindex = 0;
			cc = tinput[0];
			if(cc == 'n' || cc == 'x') break;
			if(cc != '\n') {
				getarg(arg, 8);
				i = atoi(arg);
			}
			screen = 0;
		}

		w = (u_char *) ((int) wbuf_begin + (sizeof(struct wbuf) * i));

		/* point to received packet */

		il = (struct il_rpacket *) w;

		wptr = (struct wbuf *) il;

		if(i == wbuf_index)
			mark = '*';
		else
			mark = ' ';

		printf("[%2d] %c %03d to: %s   ", i, mark,
			(((ntohl(wptr->wbuf_time))/10) % 1000),
			enameof((caddr_t)il->ether_dhost));
		printf("fr: %s.%d l=%d\t",
			enameof((caddr_t)il->ether_shost),
			wptr->wbuf_unit,
			wptr->wbuf_len);

		type = ntohs((u_short) il->ether_type);

		if (type >= ETHERPUP_TRAIL &&
		   type < ETHERPUP_TRAIL+ETHERPUP_NTRAILER) {
			printf("TRAIL.");
			off = (type - ETHERPUP_TRAIL) * 512;
			if (off >= ETHERMTU) {
				printf("BADMTU\n");
				off = 0;
			}
			type=ntohs(*ildataaddr(il, off, u_short *));
			resid=ntohs(*(ildataaddr(il, off+2,u_short *)));

			len = wptr->wbuf_len;

			if (off + resid > len){
				printf("BADLEN\n");
				off = 0;
			}
			len = off + resid;
			if (len == 0) {
				printf("BAD0\n");
				off = 0;
			}
			if (off) {
				trailip = ildataaddr(il, off+4, struct ip *);
				if(type == ETHERPUP_IPTYPE)
					summip(trailip);
				else
					printf("0x%x\n",type);
			}

		}else switch(type) {

		case ETHERPUP_IPTYPE:
			summip((struct ip *)&il->ip_tos);
			break;
		case ETHERPUP_ARPTYPE:
			printf("ARP\n");
			break;
		case ETHERPUP_XNSTYPE:
			summxns((struct idp *)&il->ip_tos);
			break;
		case ETHERPUP_ATALKTYPE:
			summat((struct lap*)&il->ip_tos);
			break;
		case ETHERPUP_PUPTYPE:
			printf("PUP\n");
			break;
		default:
			printf("UNK\n");
		}
		screen++;
	}
	return;
}

summip(ipp)
	struct ip * ipp;
{
	struct tcphdr * tpp;
	struct udphdr * upp;

	printf("IP.");
	switch(ipp->ip_p) {
	case IPPROTO_TCP:
		printf("TCP.");
		tpp = (struct tcphdr*)((int)&ipp->ip_dst + sizeof(ipp->ip_dst));
		printf("%d.%d\n", ntohs((u_short) tpp->th_sport),
			ntohs((u_short) tpp->th_dport));
		break;
	case IPPROTO_UDP:
		printf("UDP.");
		upp = (struct udphdr*)((int)&ipp->ip_dst + sizeof(ipp->ip_dst));
		printf("%d.%d\n", ntohs((u_short) upp->uh_sport),
					ntohs((u_short) upp->uh_dport));
		break;
	case IPPROTO_ICMP:
		printf("ICMP\n");
		break;
	default:
		printf("UNK\n");
	}
	return;
}

/*ARGSUSED*/
summat(lapp)
	struct lap * lapp;
{
	printf("DDP.");
	printf("\n");
}

summxns(idpp)
	struct idp * idpp;
{
	printf("XNS.");
	switch(idpp->idp_pt) {
	case NSPROTO_RI:
		printf("RI.");
		break;
	case NSPROTO_ECHO:
		printf("ECHO.");
		break;
	case NSPROTO_ERROR:
		printf("ERROR.");
		break;
	case NSPROTO_PE:
		printf("PE.");
		break;
	case NSPROTO_SPP:
		printf("SPP.");
		break;
	default:
		printf("???.");
		break;
	}
	printf("%d.", (u_short)ntohs((u_short) idpp->idp_sna.x_port));
	printf("%d\n", (u_short)ntohs((u_short) idpp->idp_dna.x_port));
	return;
}

do_do_stats()		/* print statistics */
{
	short i;

	printf(" total_packets -\t %d\t", total_packets);
	printf(" total_ip_packets -\t %d\n", total_ip_packets);
	printf(" total_udp_packets -\t %d\t", total_udp_packets);
	printf(" total_tcp_packets -\t %d\n", total_tcp_packets);
	printf(" total_icmp_packets -\t %d\t", total_icmp_packets);
	printf(" total_trail_packets -\t %d\n", total_trail_packets);
	printf(" total_arp_packets -\t %d\t", total_arp_packets);
	printf(" total_bytes - %d Beelions and %d\n",
				total_beelions, total_bytes);

	/* do_do_buck(); */

	printf("packet counts per bucket ->\n");

	for (i = 0; i < NUM_BUX; i++)
		printf("\t\tbuck[%d] = %d\n", i, promiscstatp->buck[i]);

	printf(" total_crc_errors -\t %d\t", total_crc_errors);
	printf(" total_align_errors -\t %d\n", total_align_errors);
	printf(" total_lost_errors -\t %d\t", total_lost_errors);
	printf(" total_runt_errors -\t %d\n", total_runt_errors);
	printf(" total_toobig_errors -\t %d\n", total_toobig_errors);

	printf("total unrecognized packets = %d\t", total_unrecognized);
	printf("packets kept = %d\n", counter);

	printf("\tbytes kept = %d\n", bytes_kept);
	printf("total broadcasts packets -> %d\t", total_broad_packets);
	printf(" total_xns_packets -\t %d\n", total_xns_packets);
	printf(" total_at_packets -\t %d ", total_at_packets);
	printf("rwho packets (ntohs(uh_sport) = 0x%x (%d)-> %d\n",
			RWHOPORT, RWHOPORT, total_rwho_packets);

	printf("\ntotal sequent Ether = %d\t", total_sequentE);

	printf("----- wbuf_index %d\n", wbuf_index);

	return;
}

/*--------------------------------------------------------*/

printat(lapp)
	struct lap * lapp;
{
	register struct ddp *di;
	register struct ddpshort *ds;

	printf("APPLETALK PACKET - header ->\n");
	printf("struct lap u_char dst = %x ", lapp->dst); 
	printf("u_char src = %x ", lapp->src); 
	printf("u_char type = %x ", lapp->type); 

	di = (struct ddp*) ((int)lapp + SIZEOFLAP);	/* GAG! */
	ds = (struct ddpshort*) di;

	switch(lapp->type) {
	case LT_SHORTDDP:
		printf("LT_SHORTDDP\n");
		printf("struct ddpshort u_short d_length = %x ntohs(%x[%d])\n",
			ds->d_length, ntohs((u_short) ds->d_length),
					ntohs((u_short) ds->d_length));
		printf("u_char D_dsno = %x(%d), ", ds->D_dsno, ds->D_dsno); 
		printf("u_char D_ssno = %x(%d), ", ds->D_ssno, ds->D_ssno); 
		printf("u_char D_type = %x(%d) ", ds->D_type, ds->D_type); 
		printddptype(lapp, ds->D_type, SIZEOFDDPSHORT);
		break;
	case LT_DDP:
		printf("LT_DDP\n");
		printf("struct ddp u_short d_length = %x ntohs(%x[%d]) ,",
			di->d_length, ntohs((u_short) di->d_length),
				ntohs((u_short) di->d_length));
		printf("u_short d_cksum = %x ntohs(%x[%d])\n",
			di->d_cksum, ntohs((u_short) di->d_cksum),
			ntohs((u_short) di->d_cksum));
		printf("u_short d_dnet = %x ntohs(%x[%d]) ,",
			di->d_dnet, ntohs((u_short) di->d_dnet),
				ntohs((u_short) di->d_dnet));
		printf("u_short d_snet = %x ntohs(%x[%d])\n",
			di->d_snet, ntohs((u_short) di->d_snet),
				ntohs((u_short) di->d_snet));
		printf("u_char d_dnode = %x(%d), ", di->d_dnode, di->d_dnode); 
		printf("u_char d_snode = %x(%d)\n", di->d_snode, di->d_snode); 
		printf("u_char d_dsno = %x(%d) ,", di->d_dsno, di->d_dsno); 
		printf("u_char	d_ssno = %x(%d)\n", di->d_ssno, di->d_ssno); 
		printf("u_char	d_type = %x(%d) ", di->d_type, di->d_type); 
		printddptype(lapp, di->d_type, SIZEOFDDP);
		break;
	case LT_IP:
		printf("LT_IP\n");
		break;
	case LT_ARP:
		printf("LT_ARP\n");
		break;
	default:
		printf("TYPE???\n");
	}
	return;
}

printddptype(lapp, type, offset)
	u_short		type;
	struct lap*	lapp;
	int		offset;
{
	struct rtmp		*rtmpp;
	struct nbp_header	*nbph;
	struct nbp_theader	*nbpt;
	struct nbp_name	*nbpname;
	int		i;
	u_char		*id;
	u_char		ctl;

	switch(type) {
	case ddpTypeRTMP:
		printf("ddpTypeRTMP\n");
		rtmpp = (struct rtmp*) ((int)lapp + SIZEOFLAP + offset);
		printf("struct rtmp u_short r_net = %x ntohs(%x [%d]) ",
			rtmpp->r_net, ntohs((u_short) rtmpp->r_net),
				ntohs((u_short) rtmpp->r_net));
		printf("u_char	r_idlen = %x(%d) bits\n",
			rtmpp->r_idlen, rtmpp->r_idlen); 
		printf("u_char	r_id = "); 
		i = rtmpp->r_idlen/8+1;
		id = &rtmpp->r_id;
		for(;i>0;i--)
			printf("%x ", *id++);
		printf("\n");
		break;
	case ddpTypeNBP:
		printf("ddpTypeNBP\n");
		nbph = (struct nbp_header *) ((int) lapp + SIZEOFLAP + offset);
		printf("u_char nb_cntrl = %x ", nbph->nb_cntrl);
		ctl = nbph->nb_cntrl & 0xf0 >> 4;
		switch(ctl) {
		case	BrRq:
			printf("BrRq ");
			break;
		case	LkUp:
			printf("LkUp ");
			break;
		case	LkUp_Reply:
			printf("LkUp_Reply ");
			break;
		default:
			printf("ctl?? ");
		}
		ctl = nbph->nb_cntrl & 0x0f;
		printf("tuple count = %x (%d) ", ctl, ctl);
		printf("u_char nb_id = %x(%d)\n", nbph->nb_id, nbph->nb_id); 
		nbpt = (struct nbp_theader *) ((int) nbph + 2);
		for(i = 0;i < (int) ctl; i++) {
			printf("NBP Tuple (%d)\n", i);
			printf("u_short nb_netnum = %x ntohs(%x)[%d] ",
				nbpt->nb_netnum,
				ntohs((u_short) nbpt->nb_netnum),
				ntohs((u_short) nbpt->nb_netnum));
			printf("u_char nb_node = %x (%d)\n",
				nbpt->nb_node, nbpt->nb_node);
			printf("u_char nb_socket = %x (%d) ",
				nbpt->nb_socket, nbpt->nb_socket);
			printf("u_char nb_enum = %x (%d)\n",
				nbpt->nb_enum, nbpt->nb_enum);
			nbpname = nbpNameLoc(nbpt);
			printnbpname(nbpname);
		}
		break;
	case ddpTypeATP:
		printf("ddpTypeATP\n");
		break;
	case ddpTypeEP:
		printf("ddpTypeEP\n");
		break;
	case ddpTypeRTMPReq:
		printf("ddpTypeRTMPReq\n");
		break;
	case ddpTypeZIP:
		printf("ddpTypeZIP\n");
		break;
	case ddpTypeData:
		printf("ddpTypeData\n");
		break;
	default:
		printf("ddp-type ???\n");
	}
}

struct nbp_field * printfield();

printnbpname(nbpname)
	struct nbp_name*	nbpname;
{
	struct nbp_field	*nbpfield = &nbpname->nb_object;

	nbpfield = printfield(nbpfield, "OBJECT");
	nbpfield = printfield(nbpfield, "TYPE");
	nbpfield = printfield(nbpfield, "ZONE");
}

struct nbp_field *
printfield(nbpfield, name)
	struct nbp_field*	nbpfield;
	char*	name;
{
	int	len, j;

	len = (int) nbpfield->nb_nlength;
	printf("%s: u_char len = %x [%d])\n", name, len, len);
	for(j = 0; j < len; j++)
		printf("%x ", nbpfield->nb_string[j]);
	printf("\n");
	for(j = 0; j < len; j++)
		printf("%c ", nbpfield->nb_string[j]);
	printf("\n");
	nbpnxtfield(nbpfield);
	return(nbpfield);
}
