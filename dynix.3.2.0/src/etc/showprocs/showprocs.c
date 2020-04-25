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

#ident "$Header: showprocs.c 1.2 91/04/05 $"

/*
 * showprocs.c -- display processor info
 *
 * Usage:
 *	showprocs
 *
 * 'showprocs' displays a table containing the following information for
 * each processor: proc number (i.e. an index into the kernel engine table),
 * type (386/486/...), speed (MHz), SLIC address, state (online/offline),
 * and FPA info (yes/no/emulated).
 *
 *
 * $Log:	showprocs.c,v $
 */

#include <stdio.h>
#include <errno.h>
#include <sys/tmp_ctl.h>
#include <sys/file.h>
#include <nlist.h>

unsigned int tmp_ctl();
extern int errno;

struct nlist nl[] = {
	{"_version"},
	{"_weitek_emulation"},
	0
};

char	version[100];
char	boot_name[100];
char	buff[256];
int	emulation;

main(argc, argv)
int argc;
char **argv;
{
	register int i, n_engine, type;
	char	*name;
	int	mem;
	extern	char	*strchr();

	mem = open("/dev/kmem", O_RDONLY);
	(void)get_vers(0, 100, version);
	(void)get_vers(1, 100, boot_name);

	name = "/dynix";
	if (nlist(name, nl) == -1) {
		fprintf(stderr, "showprocs: unable to nlist /dynix\n");
		exit(1);
	}
	lseek(mem, nl[0].n_value, 0);
	read (mem, buff, 100);
	if (strncmp(buff, version, 99)) {
		if ((name = strchr(boot_name, ')')) == NULL) {
			fprintf(stderr, "showprocs: version string mismatch\n");
			exit(1);
		}
		if (*name + 1 != '/') {
			*name = '/';
		} else {
			name++;
		}
		if (nlist(name, nl) == -1) {
			fprintf(stderr, "showprocs: unable to nlist %s\n", name);
			exit(1);
		}
		lseek(mem, nl[0].n_value, 0);
		read (mem, buff, 100);
		if (strncmp(buff, version, 99)) {
			fprintf(stderr, "showprocs: version string mismatch in %s\n", name);
			exit(1);
		}
	} 
	if (nl[1].n_value) {
		lseek(mem, nl[1].n_value, 0);
		read (mem, &emulation, sizeof(int));
	}

	printf("proc #   type   MHz   SLIC    state    FPA\n");
	printf("------   ----   ---   ----   -------   ---\n");

	n_engine = tmp_ctl(TMP_NENG, 0);
	for (i = 0; i < n_engine; i++) {
		type = tmp_ctl(TMP_TYPE, i);
		switch (type) {
		case TMP_TYPE_NS32000:
			printf("%4d %8s %5d %5d %10s %5s\n",
			       i, "32000", tmp_ctl(TMP_RATE, i),
			       tmp_ctl(TMP_SLIC, i),
			       (tmp_ctl(TMP_QUERY, i) == TMP_ENG_ONLINE) ?
			       "online" : "offline",
			       (tmp_ctl(TMP_GETFLAGS, i) & TMP_FLAGS_FPA) ?
			       "yes" : "no");
			break;
		case TMP_TYPE_I386:
			printf("%4d %8s %5d %5d %10s %5s\n",
			       i, "386", tmp_ctl(TMP_RATE, i),
			       tmp_ctl(TMP_SLIC, i),
			       (tmp_ctl(TMP_QUERY, i) == TMP_ENG_ONLINE) ?
			       "online" : "offline",
			       (tmp_ctl(TMP_GETFLAGS, i) & TMP_FLAGS_FPA) ?
			       "yes" : emulation ? "emu" : "no" );
			break;
		case TMP_TYPE_I486:
			printf("%4d %8s %5d %5d %10s %5s\n",
			       i, "486", tmp_ctl(TMP_RATE, i),
			       tmp_ctl(TMP_SLIC, i),
			       (tmp_ctl(TMP_QUERY, i) == TMP_ENG_ONLINE) ?
			       "online" : "offline",
			       (tmp_ctl(TMP_GETFLAGS, i) & TMP_FLAGS_FPA) ?
			       "yes" : emulation ? "emu" : "no" );
			break;
		case TMP_TYPE_I586:
			printf("%4d %8s %5d %5d %10s %5s\n",
			       i, "586", tmp_ctl(TMP_RATE, i),
			       tmp_ctl(TMP_SLIC, i),
			       (tmp_ctl(TMP_QUERY, i) == TMP_ENG_ONLINE) ?
			       "online" : "offline",
			       (tmp_ctl(TMP_GETFLAGS, i) & TMP_FLAGS_FPA) ?
			       "yes" : emulation ? "emu" : "no" );
			break;
		}
	}
	exit(0);
}
