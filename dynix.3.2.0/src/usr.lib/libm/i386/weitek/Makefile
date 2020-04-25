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
# $Header: Makefile 1.4 1991/05/22 00:09:38 $
#

.SUFFIXES:
.SUFFIXES: .o .s .c

VPATH	= ../../common
CFLAGS	= -O -f1167 $(INCLUDE)
OBJS	= hypot.o jn.o j0.o j1.o gamma.o erf.o floor.o \
	  asin.o acos.o atan.o exp.o log.o pow.o tan.o sqrt.o \
	  values.o sin.o cos.o atan2.o matherr.o sinhcosh.o

.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $< && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC)    -UPROF $(CFLAGS) -c $< && \
	$(LD) -x -r $*.o -o $*.ld && mv $*.ld $*.o

.s.o:
	$(CPP) -DPROF -I.. $< | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CPP) -UPROF -I.. $< | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.ld && mv $*.ld $*.o

all	: profiled libfpam.a libfpam_p.a
libfpam.a \
libfpam_p.a:& $(OBJS)
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod 644 *.o profiled/*.o
	@-chgrp daemon *.o profiled/*.o > /dev/null 2>&1
	@-chown  root *.o profiled/*.o > /dev/null 2>&1
	cd profiled && $(AR) cru ../libfpam_p.a $(OBJS)
	$(AR) cru libfpam.a $(OBJS)

profiled:; mkdir profiled
install	:; install -m 644 libfpam.a $(DESTDIR)/usr/lib
	   $(RANLIB) $(DESTDIR)/usr/lib/libfpam.a
	   install -m 644 libfpam_p.a $(DESTDIR)/usr/lib
	   $(RANLIB) $(DESTDIR)/usr/lib/libfpam_p.a
values.o:; $(CPP) -DPROF -I.. ../values.s | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CPP) -UPROF -I.. ../values.s | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.ld && mv $*.ld $*.o
clean	:; rm -rf $(OBJS) libfpam.a libfpam_p.a profiled
