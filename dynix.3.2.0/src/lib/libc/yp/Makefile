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

# $Header: Makefile 1.2 87/03/27 $
#
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)
#
OBJS	=\
	yp_all.o yp_bind.o yp_enum.o yp_master.o yp_match.o yp_order.o \
	yperr_string.o ypprot_err.o ypv1_xdr.o ypxdr.o
SED	= sed -f ../$(MACHINE)/rofix.sed
LIB	= yplib yplib_p

.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../yplib_p $(OBJS) && \
		  echo "	buiding normal $@" && \
		  cd .. && $(AR) cru yplib $(OBJS)
parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean		:; rm -rf yplib yplib_p profiled $(OBJS)
