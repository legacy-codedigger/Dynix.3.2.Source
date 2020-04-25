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

# $Header: Makefile 1.2 86/12/12 $
#
CFLAGS	= -O $(INCLUDE)
#
OBJS	=\
	etherxdr.o mountxdr.o rquotaxdr.o rstatxdr.o rusersxdr.o \
	rwallxdr.o sprayxdr.o util.o yppasswdxdr.o
SED	= sed -f ../$(MACHINE)/rofix.sed
LIB	= librpcsvc.a librpcsvc_p.a

.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $*.c && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC)    -UPROF $(CFLAGS) -c $*.c && \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../librpcsvc_p.a $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru librpcsvc.a $(OBJS)
parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean		:; rm -rf librpcsvc.a librpcsvc_p.a $(OBJS) profiled
install		:; install -m 644 librpcsvc.a $(DESTDIR)/usr/lib/ && \
		   $(RANLIB) $(DESTDIR)/usr/lib/librpcsvc.a && \
		   install -m 644 librpcsvc_p.a $(DESTDIR)/usr/lib/ && \
		   $(RANLIB) $(DESTDIR)/usr/lib/librpcsvc_p.a
