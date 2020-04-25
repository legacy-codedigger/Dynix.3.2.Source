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
static char rcsid[] = "$Header: rpc.rstatd.c 1.5 90/06/04 $";
#endif

#ifndef lint
/* @(#)rpc.rstatd.c	2.2 86/05/15 NFSSRC */ 
static  char sccsid[] = "@(#)rpc.rstatd.c 1.1 86/02/05 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/* 
 * rstat demon:  called from inet
 *
 */

#include <signal.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <nlist.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/vmmeter.h>
#include <net/if.h>
#include <rpcsvc/rstat.h>
#ifdef	sequent
#include <sys/param.h>
#include <machine/vmparam.h>
#include <machine/pte.h>
#include <machine/plocal.h>
#include <machine/engine.h>
#endif

struct nlist nl[] = {
#define	X_IFNET		0
	{ "_ifnet" },
#define	X_BOOTTIME	1
	{ "_boottime" },
#define	X_AVENRUN	2
	{ "_avenrun" },
#define X_HZ		3
	{ "_hz" },
#ifdef	sequent
#define	X_ENGINE	4
	{ "_engine" },
#define	X_NENGINE	5
	{ "_Nengine" },
#define	X_DK_NDRIVES	6
	{ "_dk_ndrives" },
#define	X_DK		7
	{ "_dk" },
#else	sequent
#define	X_CPTIME	4
	{ "_cp_time" },
#define	X_SUM		5
	{ "_sum" },
#define	X_DKXFER	6
	{ "_dk_xfer" },
#endif	sequent
	"",
};
int kmem;
int firstifnet, numintfs;	/* chain of ethernet interfaces */
int stats_service();

int sincelastreq = 0;		/* number of alarms since last request */
#define CLOSEDOWN 20		/* how long to wait before exiting */

union {
    struct stats s1;
    struct statsswtch s2;
    struct statstime s3;
} stats;

int updatestat();
extern int errno;

#ifndef FSCALE
#define FSCALE (1 << 8)
#endif

#ifdef	sequent
long dk_data[DK_NDRIVE];
#endif

main(argc, argv)
	char **argv;
{
	SVCXPRT *transp;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	int readfds, port, readfdstmp;


#ifdef DEBUG
	{
	int s;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("inet: socket");
		return -1;
	}
	if (bind(s, &addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}
	if (getsockname(s, &addr, &len) != 0) {
		perror("inet: getsockname");
		(void)close(s);
		return -1;
	}
	pmap_unset(RSTATPROG, RSTATVERS_ORIG);
	pmap_set(RSTATPROG, RSTATVERS_ORIG, IPPROTO_UDP,ntohs(addr.sin_port));
	pmap_unset(RSTATPROG, RSTATVERS_SWTCH);
	pmap_set(RSTATPROG, RSTATVERS_SWTCH,IPPROTO_UDP,ntohs(addr.sin_port));
	pmap_unset(RSTATPROG, RSTATVERS_TIME);
	pmap_set(RSTATPROG, RSTATVERS_TIME,IPPROTO_UDP,ntohs(addr.sin_port));
	if (dup2(s, 0) < 0) {
		perror("dup2");
		exit(1);
	}
	}
#endif	
	if (getsockname(0, &addr, &len) != 0) {
		perror("rstat: getsockname");
		exit(1);
	}
	if ((transp = svcudp_bufcreate(0, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE))
	    == NULL) {
		fprintf("svc_rpc_udp_create: error\n");
		exit(1);
	}
	if (!svc_register(transp,RSTATPROG,RSTATVERS_ORIG,stats_service, 0)) {
		fprintf(stderr, "svc_rpc_register: error\n");
		exit(1);
	}
	if (!svc_register(transp,RSTATPROG,RSTATVERS_SWTCH,stats_service,0)) {
		fprintf(stderr, "svc_rpc_register: error\n");
		exit(1);
	}
	if (!svc_register(transp,RSTATPROG,RSTATVERS_TIME,stats_service,0)) {
		fprintf(stderr, "svc_rpc_register: error\n");
		exit(1);
	}
	setup();
	updatestat();
	alarm(1);
	signal(SIGALRM, updatestat);
	svc_run();
	fprintf(stderr, "svc_run should never return\n");
}

static int
stats_service(reqp, transp)
	 struct svc_req  *reqp;
	 SVCXPRT  *transp;
{
	int have;
	
#ifdef DEBUG
	fprintf(stderr, "entering stats_service\n");
#endif
	switch (reqp->rq_proc) {
		case RSTATPROC_STATS:
			sincelastreq = 0;
			if (reqp->rq_vers == RSTATVERS_ORIG) {
				if (svc_sendreply(transp, xdr_stats,
				    &stats.s1, TRUE) == FALSE) {
					fprintf(stderr,
					    "err: svc_rpc_send_results");
					exit(1);
				}
				return;
			}
			if (reqp->rq_vers == RSTATVERS_SWTCH) {
				if (svc_sendreply(transp, xdr_statsswtch,
				    &stats.s2, TRUE) == FALSE) {
					fprintf(stderr,
					    "err: svc_rpc_send_results");
					exit(1);
				    }
				return;
			}
			if (reqp->rq_vers == RSTATVERS_TIME) {
				if (svc_sendreply(transp, xdr_statstime,
				    &stats.s3, TRUE) == FALSE) {
					fprintf(stderr,
					    "err: svc_rpc_send_results");
					exit(1);
				    }
				return;
			}
		case RSTATPROC_HAVEDISK:
			have = havedisk();
			if (svc_sendreply(transp,xdr_long, &have, TRUE) == 0){
			    fprintf(stderr, "err: svc_sendreply");
			    exit(1);
			}
			return;
		case 0:
			if (svc_sendreply(transp, xdr_void, 0, TRUE)
			    == FALSE) {
				fprintf(stderr, "err: svc_rpc_send_results");
				exit(1);
			    }
			return;
		default: 
			svcerr_noproc(transp);
			return;
		}
}

updatestat()
{
	int off, i, hz;
	struct vmmeter sum;
	struct ifnet ifnet;
	double avrun[3];
	struct timeval tm, btm;
	
#ifdef DEBUG
	fprintf(stderr, "entering updatestat\n");
#endif
	if (sincelastreq >= CLOSEDOWN) {
#ifdef DEBUG
	fprintf(stderr, "about to closedown\n");
#endif
		exit(0);
	}
	sincelastreq++;
	if (lseek(kmem, (long)nl[X_HZ].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
	if (read(kmem, &hz, sizeof hz) != sizeof hz) {
		fprintf(stderr, "can't read hz from kmem\n");
		exit(1);
	}
#ifndef	sequent
	if (lseek(kmem, (long)nl[X_CPTIME].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (read(kmem, stats.s1.cp_time, sizeof (stats.s1.cp_time))
	    != sizeof (stats.s1.cp_time)) {
		fprintf(stderr, "can't read cp_time from kmem\n");
		exit(1);
	}
#else
	get_sum(&sum);
	for (i = 0; i < CPUSTATES; i++)
		stats.s1.cp_time[i] = sum.v_time[i];
#endif
	if (lseek(kmem, (long)nl[X_AVENRUN].n_value, 0) ==-1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
#ifdef vax
 	if (read(kmem, avrun, sizeof (avrun)) != sizeof (avrun)) {
		fprintf(stderr, "can't read avenrun from kmem\n");
		exit(1);
	}
	stats.s2.avenrun[0] = avrun[0] * FSCALE;
	stats.s2.avenrun[1] = avrun[1] * FSCALE;
	stats.s2.avenrun[2] = avrun[2] * FSCALE;
#else vax
#ifdef sun
 	if (read(kmem, stats.s2.avenrun, sizeof (stats.s2.avenrun))
	    != sizeof (stats.s2.avenrun)) {
		fprintf(stderr, "can't read avenrun from kmem\n");
		exit(1);
	}
#else sun
#ifdef	sequent
 	if (read(kmem, stats.s2.avenrun, sizeof (stats.s2.avenrun))
	    != sizeof (stats.s2.avenrun)) {
		fprintf(stderr, "can't read avenrun from kmem\n");
		exit(1);
	}
	stats.s2.avenrun[0] = (double)stats.s2.avenrun[0] / (FSCALE/256.);
	stats.s2.avenrun[1] = (double)stats.s2.avenrun[1] / (FSCALE/256.);
	stats.s2.avenrun[2] = (double)stats.s2.avenrun[2] / (FSCALE/256.);
#else	sequent
	put machine dependent code here
#endif	sequent
#endif sun
#endif vax

	if (lseek(kmem, (long)nl[X_BOOTTIME].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (read(kmem, &btm, sizeof (stats.s2.boottime))
	    != sizeof (stats.s2.boottime)) {
		fprintf(stderr, "can't read boottime from kmem\n");
		exit(1);
	}
	stats.s2.boottime = btm;


#ifdef DEBUG
	fprintf(stderr, "%d %d %d %d\n", stats.s1.cp_time[0],
	    stats.s1.cp_time[1], stats.s1.cp_time[2], stats.s1.cp_time[3]);
#endif

#ifndef	sequent
	if (lseek(kmem, (long)nl[X_SUM].n_value, 0) ==-1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (read(kmem, &sum, sizeof sum) != sizeof sum) {
		fprintf(stderr, "can't read sum from kmem\n");
		exit(1);
	}
#endif	sequent
	stats.s1.v_pgpgin = sum.v_pgpgin;
	stats.s1.v_pgpgout = sum.v_pgpgout;
	stats.s1.v_pswpin = sum.v_pswpin;
	stats.s1.v_pswpout = sum.v_pswpout;
	stats.s1.v_intr = sum.v_intr;
	stats.s2.v_swtch = sum.v_swtch;

#ifdef	sequent
	for (i = 0; i < DK_NDRIVE; i++)
		stats.s1.dk_xfer[i] = dk_data[i];
#else	sequent
	if (lseek(kmem, (long)nl[X_DKXFER].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (read(kmem, stats.s1.dk_xfer, sizeof (stats.s1.dk_xfer))
	    != sizeof (stats.s1.dk_xfer)) {
		fprintf(stderr, "can't read dk_xfer from kmem\n");
		exit(1);
	}
#endif	sequent
	stats.s1.if_ipackets = 0;
	stats.s1.if_opackets = 0;
	stats.s1.if_ierrors = 0;
	stats.s1.if_oerrors = 0;
	stats.s1.if_collisions = 0;
	for (off = firstifnet, i = 0; off && i < numintfs; i++) {
		if (lseek(kmem, off, 0) == -1) {
			fprintf(stderr, "can't seek in kmem\n");
			exit(1);
		}
		if (read(kmem, &ifnet, sizeof ifnet) != sizeof ifnet) {
			fprintf(stderr, "can't read ifnet from kmem\n");
			exit(1);
		}
		stats.s1.if_ipackets += ifnet.if_ipackets;
		stats.s1.if_opackets += ifnet.if_opackets;
		stats.s1.if_ierrors += ifnet.if_ierrors;
		stats.s1.if_oerrors += ifnet.if_oerrors;
		stats.s1.if_collisions += ifnet.if_collisions;
		off = (int) ifnet.if_next;
	}
	gettimeofday(&stats.s3.curtime, 0);
	alarm(1);
}

static 
setup()
{
	struct ifnet ifnet;
	int off, *ip;
	
	nlist("/dynix", nl);
	if (nl[0].n_value == 0) {
		fprintf (stderr, "Variables missing from namelist\n");
		exit (1);
	}
	if ((kmem = open("/dev/kmem", 0)) < 0) {
		fprintf(stderr, "can't open kmem\n");
		exit(1);
	}

	off = nl[X_IFNET].n_value;
	if (lseek(kmem, off, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
	if (read(kmem, &firstifnet, sizeof(int)) != sizeof (int)) {
		fprintf(stderr, "can't read firstifnet from kmem\n");
		exit(1);
	}
	numintfs = 0;
	for (off = firstifnet; off;) {
		if (lseek(kmem, off, 0) == -1) {
			fprintf(stderr, "can't seek in kmem\n");
			exit(1);
		}
		if (read(kmem, &ifnet, sizeof ifnet) != sizeof ifnet) {
			fprintf(stderr, "can't read ifnet from kmem\n");
			exit(1);
		}
		numintfs++;
		off = (int) ifnet.if_next;
	}
}

/* 
 * returns true if have a disk
 */
static
havedisk()
{
#ifdef	sequent
	return (1);
#else	sequent
	int i, cnt;
	long  xfer[DK_NDRIVE];

	nlist("/vmunix", nl);
	if (nl[X_DKXFER].n_value == 0) {
		fprintf (stderr, "Variables missing from namelist\n");
		exit (1);
	}
	if ((kmem = open("/dev/kmem", 0)) < 0) {
		fprintf(stderr, "can't open kmem\n");
		exit(1);
	}
	if (lseek(kmem, (long)nl[X_DKXFER].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
	if (read(kmem, xfer, sizeof xfer)!= sizeof xfer) {
		fprintf(stderr, "can't read kmem\n");
		exit(1);
	}
	cnt = 0;
	for (i=0; i < DK_NDRIVE; i++)
		cnt += xfer[i];
	return (cnt != 0);
#endif	sequent
}

#ifdef	sequent
/*
 * In DYNIX, the sum structure is per processor and part of
 * the plocal structure (see plocal.h).
 * We must read the per processor data and sum it.
 */

int Nengine, dk_ndrives;
struct engine	*engine;	/* array of engine structures */
struct engine	*pengine;	/* kernel addr of array of engine structures */
struct plocal	**pplocal;	/* array of plocal pointers */

#define	STEAL(where, var, size) \
	if (where == 0) { \
		bzero((char *)var, size); \
	} else { \
		(void) lseek(kmem, (off_t)where, 0); \
		if (read(kmem, (caddr_t)var, size) != size) { \
			fprintf(stderr, "read of kmem failed!\n"); \
			exit (1); \
		} \
	}

get_sum(vp)
	register struct vmmeter *vp;
{
	register int i;
	register unsigned *cp, *sp;
	struct plocal plocal;
	static int firsttime = 1;
	static struct dk *dk;

	if (firsttime) {
		firsttime = 0;
		STEAL(nl[X_DK_NDRIVES].n_value, &dk_ndrives, sizeof dk_ndrives);
		if (dk_ndrives > DK_NDRIVE)
			dk_ndrives = DK_NDRIVE;
		dk = (struct dk *) calloc(dk_ndrives, sizeof (struct dk));
		STEAL(nl[X_NENGINE].n_value, &Nengine, sizeof Nengine);
		if (Nengine <= 0)
			Nengine = 1;
		STEAL(nl[X_ENGINE].n_value, &pengine, sizeof pengine);
		engine = (struct engine *)calloc(Nengine, sizeof(struct engine));
		STEAL(pengine, &engine[0], Nengine*sizeof(struct engine));
		pplocal = (struct plocal **)calloc(Nengine, sizeof(struct plocal *));
		for (i = 0; i < Nengine; i++)
			pplocal[i] = (struct plocal *)&engine[i].e_local->pp_local[0][0];
	}
	bzero((caddr_t)vp, sizeof (struct vmmeter));
	for (i = 0; i < Nengine; i++) {
		STEAL(&pplocal[i]->cnt, &plocal.cnt, sizeof (struct vmmeter));
		for ( sp = &(vp->v_first), cp = &plocal.cnt.v_first;
		      cp <= &plocal.cnt.v_last; cp++, sp++ ) {
			*sp += *cp;
		}
	}
	/*
	 * Scale cpu state stats to 100 percent 
	 * by dividing by number of cpu's.
	 */
	for (i = 0; i < CPUSTATES; i++)
		vp->v_time[i] /= Nengine;
	/*
	 * Disk xfer data is stored in the
	 * dk[] structure so get and save it.
	 */
	bzero((caddr_t)dk_data, sizeof (dk_data));
	STEAL(nl[X_DK].n_value, dk, dk_ndrives * sizeof (struct dk));
	for (i = 0; i < dk_ndrives && i < DK_NDRIVE; i++)
		dk_data[i] = dk[i].dk_xfer;
}
#endif	sequent
