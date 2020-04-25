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

#ifndef lint
static char rcsid[] = "$Header: mkioconf.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "y.tab.h"
#include "config.h"

/*
 * build the ioconf.c file
 */

extern	int	bmp_cnt, cmp_cnt;		/* count(s) of mono-P drivers */
extern	int 	seen_custA;			/* flag if custom boards */
balance_ioconf()
{
	register struct device *dp, *mp;
	register int count;
	register char *namep;
	register struct proto *ptp;
	register struct p_entry *entry;
	int i;
	FILE *fp;
	int bin_table[8];
	int	flag;

	for (count=0; count < 8; count++)
		bin_table[count] = 0;
	fp = fopen(path("ioconf.c"), "w");
	if (fp == NULL) {
		perror(path("ioconf.c"));
		exit(1);
	}
	fprintf(fp, "#include	\"../h/param.h\"\n");
	fprintf(fp, "#include	\"../h/mutex.h\"\n");
	fprintf(fp, "#include	\"../h/systm.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include	\"../machine/ioconf.h\"\n");
	fprintf(fp, "\n");
	/*
	 * This is only temporary!!!
	 */
	fprintf(fp, 
		"u_long\tMBAd_IOwindow =\t\t3*256*1024;\t/* top 1/4 Meg */\n");
	fprintf(fp, "\n");

	for (ptp = ptab; ptp != NULL; ptp = ptp->p_next) {
		if (!(ptp->p_seen_flag))
			continue;

		fprintf(fp, "/*\n");
		fprintf(fp, " * %s device configuration.\n", ptp->p_name);
		fprintf(fp, " */\n\n");
		fprintf(fp, "\n");
		fprintf(fp, "#include	\"../%s/ioconf.h\"\n", ptp->p_name);
		fprintf(fp, "\n");

		/*
		 * Generate dev structures for this controller
		 */
		for (dp = dtab, namep = NULL; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS ||
			   !eq(mp->d_name, ptp->p_name) ||
			   (namep != NULL && eq(dp->d_name, namep)) )
				continue;
			fprintf(fp, "extern\tstruct\t%s_driver\t%s_driver;\n",
			    ptp->p_name, namep = dp->d_name);
		}

		flag = 0;
		for (dp = dtab, namep = NULL; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS ||
			   !eq(mp->d_name, ptp->p_name))
				continue;
			if (namep == NULL || !eq(namep, dp->d_name)) {
				count = 0;
				if (namep != NULL) 
					fprintf(fp, "};\n");
				flag = 1;
				fprintf(fp, 
				  "\nstruct\t%s_dev %s_%s[] = {\n",
				  ptp->p_name,
				  ptp->p_name,
				  namep = dp->d_name);
				fprintf(fp, "/*");
				entry = ptp->p_fields;
				for (i = 0; i < ptp->p_nentries; i++)
					fprintf(fp, "\t%s",entry[i].p_name);
				fprintf(fp, " */\n");
			}
			if (dp->d_bin != UNKNOWN)
				bin_table[dp->d_bin]++;
			fprintf(fp, "{");
			for (i = 0; i < ptp->p_nentries; i++)  
				if (eq(ptp->p_fields[i].p_name,"index"))
					fprintf(fp, "\t%d,", mp->d_unit);
				else
					fprintf(fp, "\t%d,", dp->d_fields[i]);
			fprintf(fp, "\t},\t/* %s%d */\n", dp->d_name, count++);
		}
		if (flag)
			fprintf(fp, "};\n\n");

		/*
	 	* Generate conf array
	 	*/
		fprintf(fp, "/*\n");
		fprintf(fp, " * %s_conf array collects all %s devices\n", 
			ptp->p_name, ptp->p_name);
		fprintf(fp, " */\n\n");
		fprintf(fp, "struct\t%s_conf %s_conf[] = {\n", 
			ptp->p_name, ptp->p_name);
		fprintf(fp, "/*\tDriver\t\t#Entries\tDevices\t\t*/\n");
		for (dp = dtab, namep = NULL; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS ||
			   !eq(mp->d_name, ptp->p_name))
				continue;
			if (namep == NULL || !eq(namep, dp->d_name)) {
				if (namep != NULL)
					fprintf(fp, 
			"{\t&%s_driver,\t%d,\t\t%s_%s,\t},\t/* %s */\n",
			namep, count, ptp->p_name, namep, namep);
				count = 0;
				namep = dp->d_name;
			}
			++count;
		}
		if (namep != NULL) {
			fprintf(fp, 
			  "{\t&%s_driver,\t%d,\t\t%s_%s,\t},\t/* %s */\n",
			  namep, count, ptp->p_name, namep, namep);
		}
		fprintf(fp, "\t{ 0 },\n");
		fprintf(fp, "};\n\n");

	}
	/*
	 * Pseudo's
	 * Includes sleep/wakeup init entry if there are mono-P drivers.
	 */

	fprintf(fp, "/*\n");
	fprintf(fp, " * Pseudo-device configuration\n");
	fprintf(fp, " */\n\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_type == PSEUDO_DEVICE) {
			fprintf(fp, "extern\tint\t%sboot();\n", dp->d_name);
		}
	}
	if (bmp_cnt || cmp_cnt)
		fprintf(fp, "extern\tint\t%sboot();\n", "slp");
	fprintf(fp, "\nstruct\tpseudo_dev pseudo_dev[] = {\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_type == PSEUDO_DEVICE) {
			fprintf(fp, "\t{ \"%s\",\t%d,\t%sboot,\t},\n",
				dp->d_name, 
				dp->d_slave == UNKNOWN ? 32 : dp->d_slave, 
				dp->d_name);
		}
	}
	if (bmp_cnt || cmp_cnt)
		fprintf(fp, "\t{ \"%s\",\t%d,\t%sboot,\t},\n",
			"sleep/wakeup", 32, "slp");
	fprintf(fp, "\t{ 0 },\n");
	fprintf(fp, "};\n\n");

	/*
	 * Bin interrupt table and misc
	 */
	fprintf(fp, "/*\n");
	fprintf(fp, " * Interrupt table\n");
	fprintf(fp, " */\n\n");
	fprintf(fp, "int\tbin_intr[8] = {\n");
	fprintf(fp, "\t\t0,\t\t\t\t/* bin 0, always zero */\n");
	for (count=1; count < 8; count++) {
		fprintf(fp, "\t\t%d,\t\t\t\t/* bin %d */\n", 
			bin_table[count], count);
	}
	fprintf(fp, "};\n");

	/*
	 * b8k_cntlrs[]
	 */

	fprintf(fp, "/*\n");
	fprintf(fp, " * b8k_cntlrs array collects all controller entries\n");
	fprintf(fp, " */\n\n");
	for (ptp = ptab; ptp != NULL; ptp = ptp->p_next) {
		if (!(ptp->p_seen_flag))
			continue;
		
fprintf(fp, "extern int  conf_%s(),\tprobe_%s_devices(),\t%s_map();\n",
			ptp->p_name, ptp->p_name, ptp->p_name);
	}
	fprintf(fp, "\n\nstruct\tcntlrs b8k_cntlrs[] = {\n");
	fprintf(fp, "/*\tconf\t\tprobe_devs\t\tmap\t*/\n");
	for (ptp = ptab; ptp != NULL; ptp = ptp->p_next) {
		if (!(ptp->p_seen_flag))
			continue;
		
		fprintf(fp, "{\tconf_%s,\tprobe_%s_devices,\t%s_map\t}, \n",
			ptp->p_name, ptp->p_name, ptp->p_name);
	}
	fprintf(fp, "{\t0,\t},\n");
	fprintf(fp, "};\n");

	(void) fclose(fp);
}

