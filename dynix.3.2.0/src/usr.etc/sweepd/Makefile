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

# $Header: Makefile 1.1 89/07/24 $
#

UTILITY = sweepd
SOURCE = sweepd.c
OBJECTS = sweepd.o

CFLAGS= ${INCLUDE}

all: $(UTILITY)

clean :; rm -f $(OBJECTS) ${UTILITY}

lint:; $(LINT) $(CFLAGS) -hbx $(SOURCE)

install	:
	install -m 750 -s ${UTILITY}  $(DESTDIR)/usr/etc
	rm -f $(OBJECTS)

$(UTILITY): $(SOURCE)
	$(CC) $(CFLAGS) -o $(UTILITY) $(SOURCE)
	
