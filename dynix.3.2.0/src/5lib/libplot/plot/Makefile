
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
LD	= $(ATT_LD)

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

# $Header: Makefile 1.1 87/01/15 $
#
OBJS=	arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linmod.o move.o open.o point.o putsi.o space.o

.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $*.c; \
	$(LD) -X -r $*.o -o profiled/$*.o; \
	$(CC)    -UPROF $(CFLAGS) -c $*.c; \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o

../libplot ../libplot_p: profiled ${OBJS}
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 *.o profiled/*.o
	@-chgrp daemon *.o profiled/*.o > /dev/null 2>&1
	@-chown  root *.o profiled/*.o > /dev/null 2>&1
	cd profiled; ${AR} cru ../../libplot_p ${OBJS}
	${AR} cru ../libplot ${OBJS}

profiled:
	mkdir profiled

clean:
	rm -rf ${OBJS} errs a.out core profiled
