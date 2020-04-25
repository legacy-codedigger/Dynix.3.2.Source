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
static char rcsid[] = "$Header: main.c 2.9 91/03/11 $";
#endif

#include <sys/param.h>
#include <sys/vmmac.h>
#include <sys/socket.h>
#include <machine/pte.h>
#include <sys/file.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <stdio.h>

struct nlist nl[] = {
#define	N_MBSTAT	0
	{ "_mbstat" },
#define	N_IPSTAT	1
	{ "_ipstat" },
#define	N_TCB		2
	{ "_tcb" },
#define	N_TCPSTAT	3
	{ "_tcpstat" },
#define	N_UDB		4
	{ "_udb" },
#define	N_UDPSTAT	5
	{ "_udpstat" },
#define	N_RAWCB		6
	{ "_rawcb" },
#define	N_SYSMAP	7
	{ "_Sysmap" },
#define	N_IFNET		8
	{ "_ifnet" },
#define	N_RTHOST	9
	{ "_rthost" },
#define	N_RTNET		10
	{ "_rtnet" },
#define	N_ICMPSTAT	11
	{ "_icmpstat" },
#define	N_RTSTAT	12
	{ "_rtstat" },
#define N_RTHASHSIZE	13
	{ "_rthashsize" },

#define	N_NFILE		14
	{ "_nfile" },
#define	N_FILE		15
	{ "_file" },
#define	N_UNIXSW	16
	{ "_unixsw" },
#ifdef	notdef
#define N_IDP		17
	{ "_nspcb"},
#define N_IDPSTAT	18
	{ "_idpstat"},
#define N_SPPSTAT	19
	{ "_spp_istat"},
#define N_NSERR		20
	{ "_ns_errstat"},
#define	N_HOSTS		21
	{ "_hosts" },
#endif
#define	N_IPQUEUE	17
	{ "_ipintrq" },
	"",
};

extern	int protopr();
extern	int tcp_stats(), udp_stats(), ip_stats(), icmp_stats();

struct protox {
	u_char	pr_index;		/* index into nlist of cb head */
	u_char	pr_sindex;		/* index into nlist of stat block */
	u_char	pr_wanted;		/* 1 if wanted, 0 otherwise */
	int	(*pr_cblocks)();	/* control blocks printing routine */
	int	(*pr_stats)();		/* statistics printing routine */
	char	*pr_name;		/* well-known name */
} protox[] = {
	{ N_TCB,	N_TCPSTAT,	1,	protopr,
	  tcp_stats,	"tcp" },
	{ N_UDB,	N_UDPSTAT,	1,	protopr,
	  udp_stats,	"udp" },
	{ -1,		N_IPSTAT,	1,	0,
	  ip_stats,	"ip" },
	{ -1,		N_ICMPSTAT,	1,	0,
	  icmp_stats,	"icmp" },
	{ -1,		-1,		0,	0,
	  0,		0 }
};

struct	pte *Sysmap;

char	*system = "/dynix";
char	*kmemf = "/dev/kmem";
char	*defaultmem = "/dev/kmem";
int	kmem;
int	kflag;
int	Aflag;
int	aflag;
#ifdef	notdef
int	hflag;
#endif
int	iflag;
int	mflag;
int	nflag;
int	rflag;
int	sflag;
int	tflag;
int	interval;
char	*interface;
int	unit;
char	usage[] = "[ -Aaimnrst ] [ interval ] [ system ] [ core ]";

int	af = AF_UNSPEC;

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	char *cp, *name;
	register struct protoent *p;
	register struct protox *tp;

	name = argv[0];
	argc--, argv++;
  	while (argc > 0 && **argv == '-') {
		for (cp = &argv[0][1]; *cp; cp++)
		switch(*cp) {

		case 'A':
			Aflag++;
			break;

		case 'a':
			aflag++;
			break;

		case 'u':
			af = AF_UNIX;
			break;

		case 'f':
			argv++;
			argc--;
			if (strcmp(*argv, "inet") == 0)
				af = AF_NS;
#ifdef notyet
			else if (strcmp(*argv, "ns") == 0)
				af = AF_INET;
#endif
			else if (strcmp(*argv, "unix") == 0)
				af = AF_UNIX;
			else {
				fprintf(stderr, "%s: unknown address family\n",
					*argv);
				exit(10);
			}
			break;
#ifdef notyet

		/* when these are implemented, fix usage[] above */

			
		case 'I':
			iflag++;
			if (*(interface = cp + 1) == 0) {
				if ((interface = argv[1]) == 0)
					break;
				argv++;
				argc--;
			}
			for (cp = interface; isalpha(*cp); cp++)
				;
			unit = atoi(cp);
			*cp-- = 0;
			break;


		case 'h':
			hflag++;
			break;

#endif notyet
		case 'i':
			iflag++;
			break;

		case 'm':
			mflag++;
			break;

		case 'n':
			nflag++;
			break;

		case 'r':
			rflag++;
			break;

		case 's':
			sflag++;
			break;

		case 't':
			tflag++;
			break;

		default:
use:
			printf("usage: %s %s\n", name, usage);
			exit(1);
		}
		argv++, argc--;
	}
	if (argc > 0 && isdigit(argv[0][0])) {
		interval = atoi(argv[0]);
		if (interval <= 0)
			goto use;
		argv++, argc--;
		iflag++;
	}
	if (argc > 0) {
		system = *argv;
		argv++, argc--;
	}
	nlist(system, nl);
	if (nl[0].n_type == 0) {
		fprintf(stderr, "%s: no namelist\n", system);
		exit(1);
	}
	if (argc > 0) {
		kmemf = *argv;
		if (strcmp(defaultmem, kmemf) != 0)
			kflag++;
	}
	kmem = open(kmemf, 0);
	if (kmem < 0) {
		fprintf(stderr, "cannot open ");
		perror(kmemf);
		exit(1);
	}
	if (kflag) {
		off_t off;

		off = (off_t)nl[N_SYSMAP].n_value;
		if (lseek(kmem, off, 0) < 0) {
			fprintf(stderr, "cannot seek to symbol %s\n",
							nl[N_SYSMAP].n_name);
			perror(kmemf);
			exit(1);
		}
		if (read(kmem, (char *)&Sysmap, sizeof(u_int)) != sizeof(u_int)) {
			fprintf(stderr, "cannot read location of symbol %s\n",
							nl[N_SYSMAP].n_name);
			perror(kmemf);
			exit(1);
		}
	}
	if (mflag) {
		mbpr(nl[N_MBSTAT].n_value);
		exit(0);
	}
	/*
	 * Keep file descriptors open to avoid overhead
	 * of open/close on each call to get* routines.
	 */
	sethostent(1);
	setnetent(1);
	if (iflag) {
		intpr(interval, nl[N_IFNET].n_value);
		q_pr(nl[N_IPQUEUE].n_value);
		exit(0);
	}

#ifdef	notdef

	if (hflag) {
		hostpr(nl[N_HOSTS].n_value);
		exit(0);
	}
#endif
	if (rflag) {
		if (sflag)
			rt_stats(nl[N_RTSTAT].n_value);
		else
			routepr(nl[N_RTHOST].n_value, nl[N_RTNET].n_value,
				nl[N_RTHASHSIZE].n_value);
		exit(0);
	}
	if (af == AF_INET || af == AF_UNSPEC) {
		setprotoent(1);
		setservent(1);
		while (p = getprotoent()) {

			for (tp = protox; tp->pr_name; tp++)
				if (strcmp(tp->pr_name, p->p_name) == 0)
					break;
			if (tp->pr_name == 0 || tp->pr_wanted == 0)
				continue;
			if (sflag && tp->pr_stats) {
				(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
				    p->p_name);
				continue;
			}
			if (tp->pr_cblocks)
				(*tp->pr_cblocks)(nl[tp->pr_index].n_value,
				    p->p_name);
		}
		endprotoent();
	}
	if (af == AF_UNIX && !sflag)
		unixpr((off_t)nl[N_NFILE].n_value, (off_t)nl[N_FILE].n_value,
		    (struct protosw *)nl[N_UNIXSW].n_value);
}

#ifdef	ns32000
# define	PTBITS	0x1ff	/* 512 byte pages */
#endif
#ifdef	i386
# define	PTBITS	0xfff	/* 4096 byte pages */
#endif

/*
 * Seek into the kernel for a value.
 */
klseek(fd, base, off)
	int fd, base, off;
{
	int physaddr;

	if (kflag) {
		/* get kernel pte */
#ifdef	i386
		if (base > 8192) {
#endif
			physaddr = (int)((int *)Sysmap + (base / NBPG));
			lseek(fd, physaddr, 0);
			read(fd, &physaddr, sizeof physaddr);
			base = (physaddr & ~PTBITS) | (base & PTBITS);
#ifdef	i386
		}
#endif
	}
	lseek(fd, base, off);
}

char *
plural(n)
	int n;
{

	return (n != 1 ? "s" : "");
}
