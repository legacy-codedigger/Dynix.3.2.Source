# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.5 91/03/27 $
#
# Franz Lisp
#

all \
install \
clean:
	if [ "$(MACHINE)" = "ns32000" ] ;\
	then \
		(cd lisplib && make $(MFLAGS) $(MRULES) $@); \
		(cd franz && make $(MFLAGS) $(MRULES) $@); \
		(cd liszt && make $(MFLAGS) $(MRULES) $@); \
	else \
		echo "Not installing lisp on $(MACHINE)."; \
	fi
