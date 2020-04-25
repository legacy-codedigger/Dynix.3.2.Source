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

/*
 * $Header: vm.h 2.0 86/01/28 $
 *
 * vm.h
 *	#include "../h/vm.h"
 * or	#include <vm.h>		 in a user program
 * is a quick way to include all the vm header files.
 */

/* $Log:	vm.h,v $
 */

#ifdef KERNEL
#include "../h/vmparam.h"
#include "../h/vmmac.h"
#include "../h/vmmeter.h"
#include "../h/vmsystm.h"
#else
#include <sys/vmparam.h>
#include <sys/vmmac.h>
#include <sys/vmmeter.h>
#include <sys/vmsystm.h>
#endif
