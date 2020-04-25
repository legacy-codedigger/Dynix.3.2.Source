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

# $Header

#	Makefile	4.4	86/02/12

DEBUG=
CFLAGS= -O $(INCLUDE) ${DEBUG}

OBJS= bsdtcp.o df2.o df12.o dk.o dn.o hys.o hysq.o mic.o nov.o pen.o pnet.o \
	rvmacs.o sy.o unet.o va212.o va811.o va820.o vad.o vent.o vmacs.o \
	hys24.o cds224.o att2224.o

aculib.a:& $(OBJS)
	$(AR) cr aculib.a $(OBJS)
	$(RANLIB) aculib.a

$(OBJS): ../uucp.h ../condevs.h

clean:
	rm -f *.o aculib.a
