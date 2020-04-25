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

# $Header: Makefile 1.4 1991/08/02 16:27:00 $

# Someday some of this stuff should be separated out and given to the
# user, but at the moment its all intertwined such that to distribute
# this library would mean distributing the kernel interface routines.
# Since there's no documentation for such things, we punt and don't
# give them anything.  BTW, we don't want to distribute the kernel
# interface stuff so we don't have to deal with backward compatability
# issues when it comes to updating the mib, et.al., in future releases.
#
# Note the define for KERN3_2.  It was possible at the time this was
# first checked in to compile the entire agent and library under 3.1
# header files.  The agent is a little lacking in the things it can
# report, but a lot of things do work.  I even attempted to distribute
# such a daemon, but it meet with lots of internal resistance, so it
# never panned out.  But the define remains nonetheless.  At this point
# its good documentation as to what I had to do to the D3 kernel to
# fully support MIB-II

TARG=   libsnmp.a
CFLAGS=-O -DKERN3_2 -I. $(INCLUDE)

CSRCS=	snmp_client.c snmp.c snmp_auth.c asn1.c \
	mib.c parse.c snmp_api.c snmp_config.c var.c \
	var_system.c var_if.c var_at.c var_ip.c \
	var_icmp.c var_tcp.c var_udp.c var_egp.c \
	var_snmp.c var_generic.c \
	get_tcp.c get_icmp.c get_if.c \
	get_udp.c get_ip.c get_at.c \
	set_at.c set_ip.c set_if.c mmap.c 
OBJS=	snmp_client.o snmp.o snmp_auth.o asn1.o \
	mib.o parse.o snmp_api.o snmp_config.o var.o \
	var_system.o var_if.o var_at.o var_ip.o \
	var_icmp.o var_tcp.o var_udp.o var_egp.o \
	var_snmp.o var_generic.o \
	get_tcp.o get_icmp.o get_if.o \
	get_udp.o get_ip.o get_at.o \
	set_at.o set_ip.o set_if.o mmap.o 

all install: $(TARG)

libsnmp.a:&    ${OBJS}
	rm -f libsnmp.a
	$(AR) r libsnmp.a ${OBJS}
	$(RANLIB) libsnmp.a

clean:
	rm -f $(OBJS) core $(TARG)
