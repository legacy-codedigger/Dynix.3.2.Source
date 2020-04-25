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
static char rcsid[] = "$Header: showmount.c 1.2 90/01/03 $";
#endif

#ifndef lint
/* @(#)showmount.c	2.1 86/04/16 NFSSRC */
/*      showmount.c     1.1     86/02/05     */
#endif

/*
 * Copyright (c) 1984 Sun Microsystems, Inc.
 */

/*
 * showmount
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>

#define	STTBLSZ	2048
#define	TBLINC	1024
int sorthost();
int sortpath();
struct mountlist **table;

main(argc, argv)
	int argc;
	char **argv;
{
	
	int aflg = 0, dflg = 0, eflg = 0;
	int err, addr;
	int tablesize = STTBLSZ;
	struct mountlist *ml = NULL;
	struct mountlist **tb, **endtb;
	struct hostent *hp;
	char *host = NULL, hostbuf[256];
	char *last;

	while(--argc) {
		if (argv[1][0] == '-') {
			switch(argv[1][1]) {
				case 'a':
					aflg++;
					break;
				case 'd':
					dflg++;
					break;
				case 'e':
					eflg++;
					break;
				default:
					usage();
					exit(1);
			}
		}
		else if (host) {
			usage();
			exit(1);
		}
		else
			host = argv[1];
		argv++;
	}
	if (host == NULL) {
		if (gethostname(hostbuf, sizeof(hostbuf)) < 0) {
			perror("showmount: gethostname");
			exit(1);
		}
		host = hostbuf;
	}
	if (eflg) {
		printex(host);
		if (aflg + dflg == 0) {
			exit(0);
		}
	}
	if (err = callrpc(host, MOUNTPROG, MOUNTVERS, MOUNTPROC_DUMP,
	    xdr_void, 0, xdr_mountlist, &ml)) {
		fprintf(stderr, "showmount: ");
		clnt_perrno(err);
		fprintf(stderr, "\n");
		exit(1);
	}
	table = (struct mountlist **) malloc(tablesize * sizeof(table));
	if (!table) {
		fprintf(stderr, "showmount: no memory\n");
		exit(1);
	}
	tb = table;
	while (ml != NULL) {
		*tb++ = ml;
		ml = ml->ml_nxt;
		if (tb == &table[tablesize]) {
			tablesize += TBLINC;
			table = (struct mountlist **) realloc(table, tablesize *
								sizeof(table));
			if (!table) {
				fprintf(stderr, "showmount: no memory\n");
				exit(1);
			}
			tb = &table[tablesize - TBLINC];
		}
	}
	endtb = tb;
	if (dflg)
	    qsort(table, endtb - table, sizeof(struct mountlist *), sortpath);
	else
	    qsort(table, endtb - table, sizeof(struct mountlist *), sorthost);
	if (aflg) {
		for (tb = table; tb < endtb; tb++)
			printf("%s:%s\n", (*tb)->ml_name, (*tb)->ml_path);
	}
	else if (dflg) {
		last = "";
		for (tb = table; tb < endtb; tb++) {
			if (strcmp(last, (*tb)->ml_path))
				printf("%s\n", (*tb)->ml_path);
			last = (*tb)->ml_path;
		}
	}
	else {
		last = "";
		for (tb = table; tb < endtb; tb++) {
			if (strcmp(last, (*tb)->ml_name))
				printf("%s\n", (*tb)->ml_name);
			last = (*tb)->ml_name;
		}
	}
}

sorthost(a, b)
	struct mountlist **a,**b;
{
	return strcmp((*a)->ml_name, (*b)->ml_name);
}

sortpath(a, b)
	struct mountlist **a,**b;
{
	return strcmp((*a)->ml_path, (*b)->ml_path);
}

usage()
{
	fprintf(stderr, "showmount [-a] [-d] [-e] [host]\n");
}

printex(host)
	char *host;
{
	struct exports *ex = NULL;
	struct groups *gr;
	int err;

	if (err = callrpc(host, MOUNTPROG, MOUNTVERS, MOUNTPROC_EXPORT,
	    xdr_void, 0, xdr_exports, &ex)) {
		fprintf(stderr, "showmount: ");
		clnt_perrno(err);
		fprintf(stderr, "\n");
		exit(1);
	}

	fprintf(stdout, "export list for %s:\n", host);
	if (ex == NULL) {
		fprintf(stdout, "	No exported file systems\n");
	}
	while (ex) {
		fprintf(stdout, "%-20s", ex->ex_name);
		if (strlen(ex->ex_name) > 20) {
			fprintf(stdout, "\n                    ");
		}
		gr = ex->ex_groups;
		if (gr == NULL) {
			fprintf(stdout, "everyone");
		}
		while (gr) {
			fprintf(stdout, "%s ", gr->g_name);
			gr = gr->g_next;
		}
		fprintf(stdout, "\n");
		ex = ex->ex_next;
	}
}
