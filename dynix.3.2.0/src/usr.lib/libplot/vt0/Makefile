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

# $Header: Makefile 2.1 87/03/30 $
#
FPA	=
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)
#
OBJS	= arc.o box.o circle.o close.o dot.o erase.o frame.o \
	  label.o line.o linmod.o move.o open.o point.o space.o \
	  subr.o
LIB	= ../lib$(FPA)vt0 ../lib$(FPA)vt0_p
FP_OBJS	= space.o subr.o

.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $*.c && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC)    -UPROF $(CFLAGS) -c $*.c && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(MACHINE)all
ns32000all	: lib
i386all		: rm_fp_objs lib
	make $(MFLAGS) $(MRULES) FPA="fpa" FPFLAGS="-f1167" rm_fp_objs lib
lib		: profiled objects
	@chmod 644 *.o profiled/*.o
	@-chgrp daemon *.o profiled/*.o > /dev/null 2>&1
	@-/etc/chown root *.o profiled/*.o > /dev/null 2>&1
	cd profiled && $(AR) cru ../../lib$(FPA)vt0_p $(OBJS)
	$(AR) cru ../lib$(FPA)vt0 $(OBJS)

objects		:& $(OBJS)
rm_fp_objs	:; rm -f $(FP_OBJS)
profiled	:; mkdir profiled
clean		:; rm -rf $(OBJS) profiled
