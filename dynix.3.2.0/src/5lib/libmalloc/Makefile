
#	@(#)libmalloc.mk	1.7
#
# makefile for libmalloc
#

DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
OBJS	= malloc.o

.c.o:
	$(CC) -p $(CFLAGS) -c $*.c; \
	$(LD) -X -r $*.o -o profiled/$*.o
	$(CC)    $(CFLAGS) -c $*.c; \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o

all:	libmalloc.a libmalloc_p.a

libmalloc.a libmalloc_p.a: profiled $(OBJS)
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 *.o profiled/$*.o
	@-chgrp daemon *.o profiled/$*.o > /dev/null 2>&1
	@-chown  root *.o profiled/$*.o > /dev/null 2>&1
	@echo "	building profiled $@"
	@cd profiled; $(AR) cru ../libmalloc_p.a $(OBJS)
	@echo "	building regular $@"
	@$(AR) cru libmalloc.a $(OBJS)

malloc.o: mallint.h
profiled:; mkdir profiled

##
# BTW: Lint library installation moved to lint Makefile.
##

install:
	install -m 644 libmalloc_p.a $(DESTDIR)/usr/lib/libmalloc_p.a
	$(RANLIB) $(DESTDIR)/usr/lib/libmalloc_p.a
	install -m 644 libmalloc.a $(DESTDIR)/usr/lib/libmalloc.a
	$(RANLIB) $(DESTDIR)/usr/lib/libmalloc.a

clean clobber: 
	rm -rf libmalloc.a libmalloc_p.a profiled *.o core
