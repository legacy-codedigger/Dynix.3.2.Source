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

#ident	"$Header: Makefile 1.10 90/12/12 $


# format makefile
ROOT=$(DESTDIR)
ETC=etc

INS = install
OWN = -o root -g daemon
INSDIR = $(ROOT)/$(ETC)
INSTFDIR = $(ROOT)/$(ETC)
DINSDIR = $(ROOT)/$(ETC)/diskinfo
#INSTFDIR = $(ROOT)/usr/admin.psx/admbin
CFLAGS = -O  $(INCLUDE) -DBSD
LIBS = -lseq

GEOMS =	vertex.geom fujitsu.geom maxtor.geom wren3.geom wren4.geom \
	m2333k.geom m2351a.geom m2344k.geom m2382k.geom xt8380s.geom \
	xt8380sm.geom cdc9720-850.geom \
	m2249s.geom \
	sabre5-1230.geom cdc9720-1230.geom
SCSIS = wren3.scsi wren4.scsi xt8380s.scsi xt8380sm.scsi \
	m2249s.scsi
ZDS = m2333k.zd m2351a.zd m2344k.zd m2382k.zd cdc9720-850.zd \
	sabre5-1230.zd cdc9720-1230.zd

PRODUCTS = format 
OBJS = formatlib.o format_conf.o scsi.o scsi_conf.o \
	zd.o zdformat.o zdlists.o zd_conf.o zdaddbad.o \
	zdverify.o zdwrdgn.o zdcdd.o
SOURCES = format.c formatlib.c format_conf.c scsi.c scsi_conf.c \
	  zd.c zdformat.c zdlists.c zd_conf.c zdaddbad.c \
	  zdverify.c zdwrdgn.c zdcdd.c \
	  formatck.c

all: $&	$(PRODUCTS)

format: format.o $(OBJS)
	$(CC) $(CFLAGS) -o format format.o $(OBJS) $(LIBS)

formatck: formatck.o $(OBJS) 
	$(CC) $(CFLAGS) -o formatck formatck.o $(OBJS) $(LIBS)

install: all DISKINFO
	$(INS) -c $(OWN) -s format $(INSDIR)
	: ln -s ../$(ETC)/diskinfo $(DESTDIR)/etc/diskinfo

DISKINFO: 
	rm -f ${DESTDIR}/etc/diskinfo/*
	cd diskinfo;$(INS) -c -m 444 $(OWN) * $(DINSDIR)

FRC:

lint:
	$(LINT) $(SOURCES)

clean clobber:
	-rm -f *.o
	-rm -f $(PRODUCTS)

format.o:	format.h
formatlib.o:	format.h
format_conf.o:	format.h
scsi.o:		format.h scsiformat.h
scsi_conf.o:	format.h
zd.o:		format.h
zdformat.o:	zdformat.h format.h
zdcdd.o:	zdformat.h format.h
zdlists.o:	zdformat.h
zd_conf.o:	format.h zdformat.h
zdaddbad.o:	zdformat.h format.h
zdverify.o:	zdformat.h format.h
zdwrdgn.o:	zdformat.h format.h
formatck.o:	format.h
