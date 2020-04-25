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

/* $Header: emon.h 2.6 87/04/30 $ */

/*
 * $Log:	emon.h,v $
 */

#include <sys/param.h>
#include <stdio.h>
#include <nlist.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <net/promisc.h>

#include "ll.h"

#define XNS
#ifdef XNS
#include "ns/ns.h"
#include "ns/idp.h"
#include "ns/sp.h"
#include "ns/spidp.h"
#endif XNS

#define VFILES
#ifdef VFILES
#include "smb/types.h"
#include "smb/command.h"
#include "smb/message.h"
#include "smb/errors.h"
#endif VFILES

#define AT
#ifdef AT
#include <netat/atalk.h>
#include <netat/atp.h>
#endif AT

#define	SAME		0		/* for strcmp usage */
#define	XPERLINE	16		/* output number per line */
#define	OFF		0
#define	ON		1

#undef WNUMBUFS
#define WNUMBUFS wnumbufs

extern int wnumbufs;

extern struct promiscstat *promiscstatp;

#define total_packets promiscstatp->promisc_packets
#define total_broad_packets promiscstatp->promisc_bcast
#define total_ip_packets promiscstatp->promisc_ip
#define total_trail_packets promiscstatp->promisc_trailers
#define total_arp_packets promiscstatp->promisc_arp

#define total_bytes promiscstatp->promisc_bytes
#define total_beelions promiscstatp->promisc_beelions

#define counter promiscstatp->promisc_kept
#define bytes_kept promiscstatp->promisc_bytes_kept
#define total_rwho_packets promiscstatp->promisc_rwho
#define unknown_broad promiscstatp->promisc_unk_broad
#define total_udp_packets promiscstatp->promisc_udp
#define total_tcp_packets promiscstatp->promisc_tcp
#define total_icmp_packets promiscstatp->promisc_icmp
#define total_xns_packets promiscstatp->promisc_xns
#define total_at_packets promiscstatp->promisc_at

#define total_crc_errors promiscstatp->promisc_crc
#define total_align_errors promiscstatp->promisc_align
#define total_lost_errors promiscstatp->promisc_lost
#define total_runt_errors promiscstatp->promisc_runt
#define total_toobig_errors promiscstatp->promisc_toobig
#define total_unrecognized promiscstatp->promisc_unk
#define total_sequentE promiscstatp->promisc_sequentE

#define trapees	promiscstatp->promisc_trapees
#define trapx promiscstatp->promisc_trapx
#define trapping promiscstatp->promisc_trapping

extern char * enames[];

/* filter flags */

#define bkeep promiscstatp->promisc_bkeep	/* broadcast keeper flag */
#define keepall promiscstatp->promisc_keepall	/* all packet keeper flag */
#define keeparp promiscstatp->promisc_keeparp	/* arp packet keeper flag */
#define keeptrail promiscstatp->promisc_keeptrail/* trail packet keeper flag */
#define keeptrap promiscstatp->promisc_keeptrap	/* trap packet keeper flag */
#define keepbogus promiscstatp->promisc_keepbogus/* bogus packet keeper flag */
#define keepxns promiscstatp->promisc_keepxns	/* xns packet keeper flag */
#define keepat promiscstatp->promisc_keepat	/* at packet keeper flag */

#define wbuf_index (promiscstatp->promisc_kept % WNUMBUFS)

extern struct wbuf * wbuf_begin;	/* *local* offset into wbuf */

extern int reverse;
extern int dumpit;
extern int autoprint;
extern char savechar;
extern int intson;

extern int smbprintlots;

char getandocommands();
char * enameof();

u_char	atox();

int	/* command entry points */
	do_do_autoprint(),
	do_do_bkeep(),
	do_do_buck(),
	do_do_count(),
	do_do_clear(),
	do_do_debug(),
	do_do_dumpit(),
	do_do_help(),
	do_do_intsoff(),
	do_do_intson(),
	do_do_k();
	do_do_keepall(),
	do_do_keeparp(),
	do_do_keepat(),
	do_do_keepbogus(),
	do_do_keeptrail(),
	do_do_keeptrap(),
	do_do_keepwho(),
	do_do_keepxns(),
	do_do_lookit(),
	do_do_quit(),
	do_do_reverse(),
	do_do_stats(),
	do_do_summ(),
	do_do_test(),	/* to add new commands */
	do_do_trap(),	/* to add new trapees */
	do_do_trapwho(),/* to report trapees */
	do_do_huh();

extern char tinput[];
extern int tindex;

extern int if_debug;

#define NUM_BUX 6
