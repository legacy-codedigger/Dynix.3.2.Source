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

# $Header: Makefile 2.1 86/04/06 $
#
CFLAGS	= -O $(DEBUG) $(INCLUDE)
OBJS	= t0.o t1.o t2.o t3.o t4.o t5.o t6.o t7.o t8.o t9.o\
	  tb.o tc.o te.o tf.o tg.o ti.o tm.o ts.o tt.o tu.o tv.o

all		:  tbl
tbl		:& $(OBJS)
		   $(CC) $(CFLAGS) -o tbl $(OBJS)
$(OBJS)		:  t..c
install		:; install -s tbl $(DESTDIR)/usr/bin/tbl
clean		:; rm -f tbl $(OBJS)
