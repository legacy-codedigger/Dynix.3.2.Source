
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

INSDIR = $(DESTDIR)/usr/bin
SFILES = t..c t[0-9].c t[bcefgimrstuv].c
OFILES = t0.o t1.o t2.o t3.o t4.o t5.o t6.o t7.o t8.o t9.o tb.o tc.o\
	te.o tf.o tg.o ti.o tm.o tr.o ts.o tt.o tu.o tv.o

compile all: tbl
	:

tbl:	$(OFILES) 
	$(CC) $(CFLAGS) -o tbl $(OFILES)

$(OFILES):: t..c
	:

install:
	install -c -m 755 tbl $(INSDIR)/tbl

clean clobber:
	rm -f *.o
	rm -f tbl
