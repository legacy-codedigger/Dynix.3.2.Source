/*
 * $Copyright:	$
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

#include "../h/types.h"

#ifndef lint
static char rcsid[] = "$Header: conf_pci.c 2.1 86/09/13 $";
#endif

 
/*
 * Binary configuration information for pci pseudo-driver.
 */

int pci_debug = 0;	/* Debug level */
int pci_nqsizes = 8;	/* number of entries in pci_qsizes */
int pci_qsizes[] = {
	2,		/* unit 0: unused */
	8,		/* unit 1: pci disk */
	2,		/* unit 2: pci mail */
	2,		/* unit 3: pci printer */
	2,		/* unit 4: unused */
	8,		/* unit 5: pci admin */
	2,		/* unit 6: unused */
	2		/* unit 7: unused, and default */
};
