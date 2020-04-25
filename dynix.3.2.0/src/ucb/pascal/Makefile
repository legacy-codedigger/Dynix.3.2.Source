# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.2 89/03/13 $

# make sure P is reserved (defined to "&" in command line for parallel)
P=

all install install.headers clean: FRC
	cd $(MACHINE); make $(MFLAGS) $(MRULES) DESTDIR=${DESTDIR} P='$P' $@

FRC:
