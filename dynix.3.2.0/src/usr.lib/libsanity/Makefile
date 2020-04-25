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

# $Header: Makefile 1.2 87/04/28 $
#
CFLAGS	= -DLIBSANITY=1 -O $(INCLUDE) $(DEBUG)
#
OBJS	= malloc.o
LIB	= libsanity.a

all		:  $(LIB)
$(LIB)		:& $(OBJS)
		   @chmod     644 $(OBJS)
		   @-chgrp daemon $(OBJS) 2> /dev/null; exit 0
		   @-chown   root $(OBJS) 2> /dev/null; exit 0
		   $(AR) cru $(LIB) $(OBJS)
install		:; install -m 644 $(LIB) $(DESTDIR)/usr/lib/$(LIB) && \
		   $(RANLIB) $(DESTDIR)/usr/lib/$(LIB)
clean clobber	:; rm -f $(LIB) $(OBJS) malloc.c

# grab a various sources to build sanity library
malloc.o	:  malloc.c
malloc.c	:  ../../lib/libc/gen/malloc.c
		   rm -f malloc.c && ln -s $?
