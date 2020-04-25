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
static	char rcsid[] = "$Header: vmtune.c 2.1 86/09/04 $";
#endif

/*
 * vmtune.c
 *	Read/modify VM tunable parameters.
 */

#include <sys/types.h>
#include <sys/vmsystm.h>
#include <machine/param.h>

extern	int	errno;
extern	char	*gets();

struct	{
	char	sw_char;
	char	*sw_name;
	char	sw_type;   /* p = physical, v = virtual or logical, o = other */
} swtab[] = {
	'r',	"minRS",	'v',
	'R',	"maxRS",	'v',
	'e',	"RSexecslop",	'p',
	'm',	"RSexecmult",	'o',
	'd',	"RSexecdiv",	'o',
	'L',	"dirtylow",	'p',
	'H',	"dirtyhigh",	'p',
	'K',	"klout_look",	'v',
	'v',	"PFFvtime",	'o',
	'D',	"PFFdecr",	'v',
	'l',	"PFFlow",	'o',
	'I',	"PFFincr",	'v',
	'h',	"PFFhigh",	'o',
	's',	"minfree",	'p',
	'S',	"desfree",	'p',
	'M',	"maxdirty",	'p',
};

#define	NSW	(sizeof(swtab) / sizeof(swtab[0]))

#define	PHYSTOK(x)	((x)*NBPG/1024)
#define	VIRTTOK(x)	((x)*pagesize/1024)

#define KTOPHYS(k)	(((k)*1024+(NBPG-1))/NBPG)
#define KTOVIRT(k)	(((k)*1024+(pagesize-1))/pagesize)

char	*prog;
int	pagesize;


usage()
{
	register int i;

	printf("Usage: %s [-f] [options], where options are:\n", prog);
	for(i = 0; i < NSW; i++)
		printf("	-%c or -%s\n", swtab[i].sw_char, swtab[i].sw_name);

	exit(1);
}

main(argc, argv)
	int	argc;
	char	*argv[];
{
	register char *argp;
	register long *vmval;
	register int i;
	int	force = 0;
	struct vm_tune vmtune;
	char	stuff[128];

	pagesize = getpagesize();
	prog = argv[0];
	vmval = (long *)&vmtune;

	if (vm_ctl(VM_GETPARAM, &vmtune) < 0) {
		perror(prog);
		exit(1);
	}

	if (argc < 2) {
		dump_vmtune(&vmtune);
		exit(0);
	}

	while(--argc) {
		argp = *++argv;
		if (*argp == '-') {
			argp++;
			if (*argp == 'f') {
				++force;
				continue;
			}
			for(i = 0; i < NSW; i++) {
				if ((strcmp(&swtab[i].sw_char, argp) == 0) ||
				   (strcmp(argp, swtab[i].sw_name) == 0)) {
					if (--argc == 0)
						usage();
					vmval[i] = atoi(*++argv);
					if (swtab[i].sw_type == 'p')
						vmval[i] = KTOPHYS(vmval[i]);
					else if (swtab[i].sw_type == 'v')
						vmval[i] = KTOVIRT(vmval[i]);
					break;
				}
			}
			if (i == NSW)
				usage();
		}
		else
			usage();
	}

	/*
	 * Ask him about it (if appropriate) and re-set the parameters.
	 */

	if (!force) {
		dump_vmtune(&vmtune);
		printf("Ok? ");
		if (*gets(stuff) != 'y')
			exit(0);
	}

	if (vm_ctl(VM_SETPARAM, &vmtune) < 0) {
		perror(prog);
		exit(1);
	}

	exit(0);
}

dump_vmtune(vm)
	struct vm_tune *vm;
{
	register long *vmval = (long *)vm;
	register int i;
	register long tmp;

	for(i = 0; i < NSW; i++) {
		if (swtab[i].sw_type == 'p')
			tmp = PHYSTOK(vmval[i]);
		else if (swtab[i].sw_type == 'v')
			tmp = VIRTTOK(vmval[i]);
		else
			tmp = vmval[i];
		printf("%6d %s\n", tmp, swtab[i].sw_name);
	}
}

#ifdef	vax
vm_ctl() {}
#endif	vax
