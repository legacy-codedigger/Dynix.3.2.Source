# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.
#

#ident	"$Header: Makefile 1.2 1991/07/31 00:41:07 $"

# $Log: Makefile,v $
INCLUDES=-I../lib $(INCLUDE)
ETC=${DESTDIR}/usr/etc
CFLAGS= -O $(INCLUDES)
LIBSNMP= ../lib/libsnmp.a
SRCS=	snmpd.c 
LDFLAGS= $(LIBSNMP) -lseq
TARGET=snmpd

all: snmpd

snmpd: $(SRCS) $(LIBSNMP)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)


install:	all
	install -m 744 -s snmpd $(ETC)
	install -m 444 mib.txt $(ETC)
	install -m 444 snmpd.conf $(ETC)

clean:
	rm -f *.o core

clobber: clean
	rm -f $(TARGET)

depend: $(SRCS) FRC
	mkdep $(CFLAGS) $(SRCS)


FRC:

