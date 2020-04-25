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

# $Header: Makefile 2.3 87/03/27 $
#
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)
#
OBJS	=\
	inet_addr.o inet_lnaof.o inet_makeaddr.o inet_netof.o\
	inet_network.o inet_ntoa.o
SED	= sed -f ../$(MACHINE)/rofix.sed
LIB	= inetlib inetlib_p

.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../inetlib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru inetlib $(OBJS)

parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean		:; rm -rf inetlib inetlib_p $(OBJS) profiled
