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

# $Header: Makefile 2.7 89/07/25 $
#
FPFLAGS	= -DDEBUG
CFLAGS	= -g $(INCLUDE) $(FPFLAGS)
#remember to change -g back to -O
#
# NFS routines obsolete these BSD modules
#
BSD	= gethostbyname.o getnetbyaddr.o getnetbyname.o \
	  getproto.o getprotoname.o gethostbyaddr.o getservbyname.o \
	  getservbyport.o herror.o
NFS	= ether_addr.o getnetgrent.o getrpcent.o innetgr.o
OBJS	= gethostent.o getnetent.o getprotoent.o getservent.o herror.o \
	  rcmd.o rexec.o ruserpass.o res_comp.o res_debug.o res_init.o \
	  res_mkquery.o res_query.o res_send.o $(NFS)
SED	= sed -f ../$(MACHINE)/rofix.sed
LIB	= netlib netlib_p

.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o


all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../netlib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru netlib $(OBJS)

parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean		:; rm -rf errs a.out core netlib netlib_p $(OBJS) profiled
