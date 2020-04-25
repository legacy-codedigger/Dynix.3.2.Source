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
static char rcsid[] = "$Header: showcfg.c 2.19 91/03/29 $";
#endif

/*
 * showcfg.c -- show system configuration
 *
 * usage:
 *	showcfg [-s|-d] [filename]
 *
 * flags:
 *	-s	print summary
 *	-d	print system description information
 *
 * Showcfg prints system configuration in a format
 * that mimics the SCED firmware 'rc' command.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>

#define	FLAG_CHAR(v)	( (v) ? '*' : ' ' )
#define	CD_SIZE		(CD_STAND_ADDR - (int)CD_LOC)
#define	M		(1024*1024)

char	*memfile = "/dev/kmem";	/* file containing config tables */
int	fd;			/* file descriptor of above */
int	sflag;			/* non-zero if summary option */
int	dflag;			/* non-zero if descrip option */
void	nprint();
extern	int errno;
int	pr_proc032_config(), pr_proc386_config();
int	pr_mem1_config(), pr_mem2_config(), pr_sced_config();
int	pr_mbad_config(), pr_cadm_config(), pr_dcc_config();
int	pr_ssm_config(), pr_oem_config();

char *bus_mode_names[] = {
	"invalid", "compatibility", "narrow", "wide"
};

char *cache_mode_names[] = {
	"cache_off", "write-thru cache", "copy-back cache",
#ifdef CUSTOM_CACHE_MODE
	"ask about cache init", "leave cache as is",
#endif CUSTOM_CACHE_MODE
	"illegal cache mode"
};

struct {
	int type;
	char *name;
} sys_types[] = {

{	SYSTYP_B8,	"B8"			},
{	SYSTYP_B21,	"B21"			},
{	SYSTYP_S27,	"S27"			},
{	SYSTYP_S81,	"S81"			},
{       0,		"Unknown System Type"	}

};

int nsys_types = (sizeof(sys_types) / sizeof(sys_types[0])) - 1;

/* 
 * pad text to at least CD_STAND_ADDR so 
 * later mmap() does not trash real text space.
 */
pad()
{ 

#if CD_STAND_ADDR != 0x4000
	@@@ ERROR: "CD_STAND_ADDR" has changed, check mmap() assumptions
#endif

#ifdef	i386
	/*
	 * ASMQ, ASMEND workaround is necessary for now because
	 * 386 optimizer moves pseudo operands out of a routine
	 * and into data space.  Remove this comment and the "#ifdef i386"
	 * code when optimizer is fixed.
	 */
	asm("/ASMQ");
	asm(" .space 16*1024 ");
	asm("/ASMEND");
#else
	asm(" .space 16*1024 ");
#endif
}

/*
 * Print usage line
 */
usage()	
{ 
	printf("Usage: showcfg [-s|-d] [file]\n");
	exit(1);
}

main(argc, argv)
	int argc;
	char **argv;
{

#ifdef	lint
	pad();
#endif

	/*
	 * Sanity check text padding at runtime
	 */
	if ((unsigned int)usage < CD_STAND_ADDR) {
		fprintf(stderr,
		"showcfg: linked incorrectly, correct operation impossible.\n");
		exit(1);
	}

	/*
	 * read command line
	 */
	argc--; argv++;
	while (argc) {
		char *cp = *argv;
		if (*cp == '-') {
			while(*(++cp) != '\0')
				switch(*cp) {
				case 's':
					sflag++;
					break;
				case 'd':
					dflag++;
					break;
				default:
					usage();
					break;
				}
		} else {
			memfile = cp;
		}
		argv++;
		argc--;
	}

	/*
	 * Open file containing config data.
	 */
	fd = open(memfile, O_RDONLY, 0);
	if (fd < 0) {
		perror(memfile);
		exit(1);
	}

	/*
	 * mmap() config data to CD_LOC in my virtual address
	 * so config data pointers can be followed directly.
	 *
	 * This requires the pad function above to pad text space 
	 * so when we mmap() over it, we don't trash real code space.
	 */
	if ( mmap((caddr_t)CD_LOC, CD_SIZE, 
	     PROT_READ, MAP_SHARED, fd, (off_t)CD_LOC) < 0) {
		perror("mmap");
		exit(1);
	}

	if (sflag)
		print_summary();
	else if (dflag)
		print_descrip();
	else
		print_verbose();

	exit(0);
}

/*
 * Summarize from table of contents
 */
print_summary()
{
	register int type, n;
	static int proc, sced, mbad, cadm, dcc, ssm, foundation, gfde, unknown;

	for (type = 0; type < 256; type++) {

		if ((n = CD_LOC->c_toc[type].ct_count) == 0)
			continue;

		switch (type) {

		case SLB_PROCBOARD:	/* 032 processor board */
		case SLB_SGSPROCBOARD:	/* 386 processor board */
		case SLB_SGS2PROCBOARD:	/* 486 processor board */
		case SLB_KXXBOARD: 	/* 386 prototype processor board */
			proc += n;
			break;

		case SLB_SCSIBOARD:	/* SCED board */
			sced += n;
			break;

		case SLB_MBABOARD:	/* MULTIBUS adapter board */
			mbad += n;
			break;
			
		case SLB_CLKARBBOARD:	/* CADM board */
			cadm += n;
			break;
		
		case SLB_ZDCBOARD:	/* DCC board (once known as ZDC) */
			dcc += n;
			break;
		
		case SLB_FOUNDBOARD:	/* Foundation board */
			foundation += n;
			break;

		case SLB_GFDEBOARD:	/* Teradyne Accelerator board */
			gfde += n;
			break;
		
		case SLB_SSMBOARD:
		case SLB_SSM2BOARD:
			ssm += n;
			break;
		default:
			unknown += n;
			break;
		}
	}
	/*
	 * Print summary of system hardware
	 */
	printf("%d processors", proc);
	printf(", %dM", CD_LOC->c_memsize / 1024);
	nprint("sced", sced);
	nprint("mbad", mbad);
	nprint("cadm", cadm);
	nprint("dcc", dcc);
	nprint("foundation", foundation);
	nprint("gfde", gfde);
	nprint("ssm", ssm);
	printf(".\n");
}

/*
 * Generate a output string for a device (if any).
 */
void
nprint(what, count)
	char *what;
	int count;
{
	if (count == 0)
		return;
	if (count == 1)
		printf(", %s", what);
	else
		printf(", %d %ss", count, what);
}


print_descrip()
{
	register int i;
	register struct config_desc *cd = CD_LOC;
	struct sys_mode sm;
	struct sys_desc sd;

	sd = CD_LOC->c_sys;
	for (i = 0; i < nsys_types; i++) {
		if (sys_types[i].type == sd.sd_type)
			break;
	}
	printf("%s with", sys_types[i].name);
#ifndef ns32000
	printf(" %d Mhz bus clock rate and ", cd->c_clock_rate);
#endif ns32000
	printf(" %dMb memory:\n", cd->c_memsize / 1024);

	printf(" maxmem=%dMb, bottom=0x%x, mmap_size=%d\n",
		cd->c_maxmem/M, cd->c_bottom, cd->c_mmap_size);

	printf(" boot flags: 0x%x\n", cd->c_boot_flag);
	printf(" boot name: ");
	printf("%s\n", cd->c_boot_name);

	printf(" console: erase="); pr_char(cd->c_erase);
	printf("  kill="); pr_char(cd->c_kill);
	printf("  int="); pr_char(cd->c_interrupt);

	sm = CD_LOC->c_sys_mode;
	printf("\n current system mode: ");
	print_sys_mode(&sm);
	putchar('\n');
}

print_sys_mode(smp)
	register struct sys_mode *smp;
{
	printf("%s, tsize %d, bsize %d, %s",
		bus_mode_names[smp->sm_bus_mode], smp->sm_tsize, smp->sm_bsize,
		cache_mode_names[smp->sm_cache]);
}

pr_char(ch)
	char ch;
{
	if (ch < ' ')
		printf("^%c", ch + '@');
	else if (ch < 0x7F)
		putchar(ch);
	else
		printf("^?");
}

/*
 * Print a detailed description of each device
 * by looping through the configuration tables.
 */
print_verbose()
{
	register struct ctlr_desc *cdp;
	register struct ctlr_toc *tocp;

	printf("System Configuration:\n type  no slic  flags  revision\n");
	for (cdp = CD_LOC->c_ctlrs; cdp < CD_LOC->c_end_ctlrs;) {
		tocp = &CD_LOC->c_toc[cdp->cd_type];
		print_config(cdp, (int)tocp->ct_count);
		cdp += tocp->ct_count;
	}
}

/*
 * print a configuration
 */
print_config(cdp, count)
	struct ctlr_desc *cdp;
	int count;
{
	struct winsize ws;
	short width;

	if (ioctl(fileno(stdout), TIOCGWINSZ, &ws) == 0 && ws.ws_row != 0)
		width = ws.ws_col;
	else
		width = 80;

	switch (cdp->cd_type) {

	case SLB_MEMBOARD:	/* 1st gen MEM board */
		pr_mem1_config(cdp, count);
		break;
	
	case SLB_SGSMEMBOARD:	/* 2nd gen MEM board */
		pr_mem2_config(cdp, count);
		break;

	case SLB_PROCBOARD:	/* 032 processor board */
	case SLB_KXXBOARD: 	/* 386 prototype board */
		pr_proc032_config(cdp, count, width);
		break;

	case SLB_SGSPROCBOARD:	/* 386 processor board */
	case SLB_SGS2PROCBOARD:	/* 486 processor board */
		pr_proc386_config(cdp, count, width);
		break;

	case SLB_SCSIBOARD:	/* SCED board */
		pr_sced_config(cdp, count);
		break;

	case SLB_MBABOARD:	/* MULTIBUS adapter board */
		pr_mbad_config(cdp, count);
		break;

	case SLB_CLKARBBOARD:	/* CADM board */
		pr_cadm_config(cdp, count);
		break;

	case SLB_ZDCBOARD:	/* ZDC board */
		pr_dcc_config(cdp, count);
		break;

	case SLB_SSMBOARD:	/* SSM board */
	case SLB_SSM2BOARD:
		pr_ssm_config(cdp, count);
		break;

	case SLB_FOUNDBOARD:	/* Foundation board */
	case SLB_GFDEBOARD:	/* Teradyne Accelerator board */
	default:
		pr_oem_config(cdp, count);
		break;
	}
}

char gen_config_fmt[] = "%c%-6s%2d%4d %08x %02x.%02x.%02x";

/*
 * Print the generic part of a configuration
 */
print_generic_config(cdp)
	register struct ctlr_desc *cdp;
{
	printf(gen_config_fmt, DIAG_FLAGS(cdp)? '*' : ' ',
		cdp->cd_name, cdp->cd_i,
		cdp->cd_slic, DIAG_FLAGS(cdp),
		cdp->cd_var, cdp->cd_hrev, cdp->cd_srev);
}

/*
 * pr_mem1_config() - print config table for MEM controllers.
 */
static int
pr_mem1_config(basecdp, count)
	struct ctlr_desc *basecdp;
	int count;
{
	register struct ctlr_desc *cdp;

	for(cdp = basecdp; cdp < basecdp+count; cdp++) {
		print_generic_config(cdp);
		printf(" type=%s size=%d.%cMb",
			cdp->cd_m_type? "64k" : "256k",
			cdp->cd_m_psize/M, cdp->cd_m_psize & (M/2)? '5' : '0');
		if (FAILED(cdp))
			printf(" disabled\n");
		else {
			printf(" base=0x%08x %sileave%s\n",
				cdp->cd_m_base, cdp->cd_m_ileave? "" : "no-",
				cdp->cd_m_ileave == ILEAVE_LO? "-lo":
					cdp->cd_m_ileave == ILEAVE_HI? "-hi": "");
		}
	}
}

/*
 * pr_mem2_config prints config table for mem2 controllers
 */
static int
pr_mem2_config(basecdp, count)
	struct ctlr_desc *basecdp;
	int count;
{
	register struct ctlr_desc *cdp;

	for (cdp = basecdp; cdp < basecdp + count; cdp++) {
		print_generic_config(cdp);
		printf(" size=%d.0Mb", cdp->cd_m_psize/M);
		if (FAILED(cdp))
			printf(" disabled\n");
		else {
			printf(" base=0x%08x %sileave%s\n",
				cdp->cd_m_base, cdp->cd_m_ileave? "" : "no-",
				cdp->cd_m_ileave == ILEAVE_LO? "-lo":
					cdp->cd_m_ileave == ILEAVE_HI? "-hi": "");
		}
	}
}

short line_eaten;

/*
 * pr_proc032_config() - print config table for PROC controllers.  Remember
 * how much of the screen line got eaten.
 */
static
pr_proc032_head(cdp, cflags)
	register struct ctlr_desc *cdp;
	register int cflags;
{
	static char procfmt[] = "%c%-8s     %08x %02x.%02x.%02x no. %d(slic %d)";
	char line[512];

	sprintf(line, procfmt, cflags? '*' : ' ', cdp->cd_name, cflags,
		cdp->cd_var, cdp->cd_hrev, cdp->cd_srev, cdp->cd_i,
		cdp->cd_slic);
	line_eaten = strlen(line);
	printf(line);
}

/*
 * pr_proc032_config() - some squirling about is done to insure that
 * only as many procs as the screen can handle are printed out (minimum
 * of 1 for small screens).  Solution is adaptive, so format can change.
 */
#define	cd_p_printed	cd_p_width	/* negative if printed */
#define	MAXSLICID	64

int
pr_proc032_config(basecdp, count, width)
	struct ctlr_desc *basecdp;
	int count;
	short width;
{
	register struct ctlr_desc *cdp, *cmpcdp;
	register int cflags, cmpflags, line_rem;
	u_char printed[MAXSLICID];
	char line[512];

	bzero((char *)printed, sizeof printed);

	for (cdp = basecdp; cdp < basecdp + count; cdp++) {
		if (printed[cdp->cd_slic])
			continue;
		cflags = DIAG_FLAGS(cdp);
		pr_proc032_head(cdp, cflags);
		line_rem = width - line_eaten;

		for (cmpcdp = cdp+1; cmpcdp < basecdp + count; cmpcdp++) {
			if (printed[cmpcdp->cd_slic])
				continue;
			cmpflags = DIAG_FLAGS(cmpcdp);
			if (cflags == cmpflags
			    && cdp->cd_var == cmpcdp->cd_var
			    && cdp->cd_hrev == cmpcdp->cd_hrev
			    && cdp->cd_srev == cmpcdp->cd_srev) {
				if (line_rem <= 0) {
					printf("\n");
					pr_proc032_head(cmpcdp, cflags);
					line_rem = width - line_eaten;
				} else {
					sprintf(line, ", %d(%d)", cmpcdp->cd_i,
						cmpcdp->cd_slic);
					line_rem -= strlen(line);
					if (line_rem <= 0) {
						printf("\n");
						pr_proc032_head(cmpcdp, cflags);
						line_rem = width - line_eaten;
					} else
						printf(line);
				}
				printed[cmpcdp->cd_slic] = 1;
			}
		}
		putchar('\n');
	}
}

/*
 * pr_proc386_config() - print config table for PROC controllers.  Remember
 * how much of the screen line got eaten.
 */
static
pr_proc386_head(cdp, cflags)
	register struct ctlr_desc *cdp;
	register int cflags;
{
	static char procfmt[] = "%c%-9s    %08x %02x.%02x.%02x %dMHz %d*%dK%s%s: %d(slic %d)";
	char line[512];
	char bsize[8];

	if (cdp->cd_p_bsize)
		sprintf(bsize, "/%d", cdp->cd_p_bsize);
	else
		sprintf(bsize, "");

	sprintf(line, procfmt, cflags? '*' : ' ', cdp->cd_name, cflags,
		cdp->cd_var, cdp->cd_hrev, cdp->cd_srev, cdp->cd_p_speed,
		cdp->cd_p_nsets, CDP_SETSIZE(cdp->cd_p_setsize) >> 10,
		bsize, (cdp->cd_p_fp & SLP_FPA)? " FPA" : "",
		cdp->cd_i, cdp->cd_slic);
	line_eaten = strlen(line);
	printf(line);
}

/*
 * pr_proc386_config() - some squirling about is done to insure that
 * only as many procs as the screen can handle are printed out (minimum
 * of 1 for small screens).  Solution is adaptive, so format can change.
 */
static int
pr_proc386_config(basecdp, count, width)
	struct ctlr_desc *basecdp;
	int count;
	short width;
{
	register struct ctlr_desc *cdp, *cmpcdp;
	register int cflags, cmpflags, line_rem;
	u_char printed[MAXSLICID];
	char line[512];

	bzero((char *)printed, sizeof printed);

	for (cdp = basecdp; cdp < basecdp + count; cdp++) {
		if (printed[cdp->cd_slic])
			continue;
		cflags = DIAG_FLAGS(cdp);
		pr_proc386_head(cdp, cflags);
		line_rem = width - line_eaten;

		for (cmpcdp = cdp+1; cmpcdp < basecdp + count; cmpcdp++) {
			if (printed[cmpcdp->cd_slic])
				continue;
			cmpflags = DIAG_FLAGS(cmpcdp);
			if (cflags == cmpflags
			    && cdp->cd_var == cmpcdp->cd_var
			    && cdp->cd_hrev == cmpcdp->cd_hrev
			    && cdp->cd_p_speed == cmpcdp->cd_p_speed
			    && cdp->cd_p_fp == cmpcdp->cd_p_fp
			    && cdp->cd_p_width == cmpcdp->cd_p_width
			    && cdp->cd_p_nsets == cmpcdp->cd_p_nsets
			    && cdp->cd_p_setsize == cmpcdp->cd_p_setsize
			    && cdp->cd_p_bsize == cmpcdp->cd_p_bsize) {
				if (line_rem <= 0) {
					printf("\n");
					pr_proc386_head(cmpcdp, cmpflags);
					line_rem = width - line_eaten;
				} else {
					sprintf(line, ", %d(%d)", cmpcdp->cd_i,
						cmpcdp->cd_slic);
					line_rem -= strlen(line);
					if (line_rem <= 0) {
						printf("\n");
						pr_proc386_head(cmpcdp, cmpflags);
						line_rem = width - line_eaten;
					} else
						printf(line);
				}
				printed[cmpcdp->cd_slic] = 1;
			}
		}
		putchar('\n');
	}
}

/*
 * pr_sced_config() - print config table for SCED controllers.
 */
static int
pr_sced_config(basecdp, count)
	struct ctlr_desc *basecdp;
	int count;
{
	register struct ctlr_desc *cdp;
	register int i;

	for(cdp = basecdp; cdp < basecdp+count; cdp++) {
		print_generic_config(cdp);
		printf(" ver=%d host=%d enet=",
			cdp->cd_sc_version, cdp->cd_sc_host_num);
		for (i = 0; i < 6; i++)
			printf("%02x", cdp->cd_sc_enet_addr[i]);
		if (cdp->cd_sc_cons == CDSC_LOCAL)
			printf(" local");
		else if (cdp->cd_sc_cons == CDSC_REMOTE)
			printf(" remote");
		putchar('\n');
	}
}

/*
 * pr_mbad_config() - print config table for MBAD controllers.
 */
static int
pr_mbad_config(basecdp, count)
	struct ctlr_desc *basecdp;
	int count;
{
	register struct ctlr_desc *cdp;

	for (cdp = basecdp; cdp < basecdp+count; cdp++) {
		print_generic_config(cdp);
		printf(" f/w version=%d\n", cdp->cd_mb_version);
	}
}

/*
 * pr_cadm_config() - print config table for the CADM.
 */
static int
pr_cadm_config(basecdp, count)
	struct ctlr_desc *basecdp;
	int count;
{
	register struct ctlr_desc *cdp;

	for(cdp = basecdp; cdp < basecdp+count; cdp++) {
		print_generic_config(cdp);
		printf(" sysid 0x%06x front panel type %d\n",
			cdp->cd_ca_sysid, cdp->cd_ca_fptype);
	}
}

/*
 * pr_dcc_config() - print config table for DCC controllers.
 */
static int
pr_dcc_config(basecdp, count)
	struct ctlr_desc *basecdp;
	int count;
{
	register struct ctlr_desc *cdp;

	for(cdp = basecdp; cdp < basecdp+count; cdp++) {
		print_generic_config(cdp);
		printf(" f/w version=%d\n", cdp->cd_dc_version);
	}
}

/*
 * pr_oem_config() - print config table for OEM controllers.
 */
int
pr_oem_config(basecdp, count)
	struct ctlr_desc *basecdp;
	int count;
{
	register struct ctlr_desc *cdp;

	for(cdp = basecdp; cdp < basecdp+count; cdp++) {
		print_generic_config(cdp);
		printf(" type=0x%x\n", cdp->cd_type);
	}
}

/*
 * pr_ssm_config() - print config table for SSM controllers.
 */
int
pr_ssm_config(basecdp, count)
	struct ctlr_desc *basecdp;
	int count;
{
	register struct ctlr_desc *cdp;

	for(cdp = basecdp; cdp < basecdp+count; cdp++) {
		print_generic_config(cdp);
		printf(" sysid=0x%x ver=%X.%X.%X ",
			cdp->cd_ssm_sysid, cdp->cd_ssm_version[0],
			cdp->cd_ssm_version[1], cdp->cd_ssm_version[2]);
		if (cdp->cd_ssm_cons != CDSC_NOT_CONS)
			printf(" console");
		putchar('\n');
	}
}
