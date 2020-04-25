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

# $Header: Makefile 1.6 87/03/27 $
#
OBJS	= htonl.o htons.o ntohl.o ntohs.o
LIB	= netlib

.s.o:
	$(CPP) -DPROF $(INCLUDE) $*.s | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CPP) -UPROF $(INCLUDE) $*.s | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../$(LIB)_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru $(LIB) $(OBJS)

parallel	:& $(OBJS)
$(OBJS)		: ../DEFS.h
profiled	:; mkdir profiled
clean		:; rm -rf $(OBJS) profiled $(LIB) $(LIB)_p
