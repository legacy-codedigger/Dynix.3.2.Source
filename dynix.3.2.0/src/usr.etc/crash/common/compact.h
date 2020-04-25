/* $Copyright: $
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

#ident "$Header: compact.h 1.1 1991/04/16 20:22:54 $"

/*
 * compact.h--definitions for tuning crash's cacheing of uncompressed
 * data segments when looking at a compressed dump.
 */

#define PERM_CACHE	(1*1024*1024)	/* Amount permanently cached in-core */
#define LOW_WATER	(1*1024*1024)	/* Need at least this much cache */
#define CACHE_SZ	(5*1024*1024)	/* Total size cache can grow to */
