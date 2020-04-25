
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

I =	../../../include
LIB =	../../../lib/glib.a
SOURCE = chcknds.c getentry.c getmax.c gpsfmt.c idcmp.c \
	levcal.c maxwordlen.c position.c setkids.c setwtst.c sizefac.c\
	squish.c treebldr.c upperstr.c validid.c vcmp.c vmap.c vtoc.c 
HDR =	$(I)/setop.h $(I)/util.h $(I)/errpr.h 
LFILES = vtoc.o treebldr.o vmap.o levcal.o setkids.o sizefac.o \
	squish.o gpsfmt.o getentry.o position.o vcmp.o idcmp.o \
	chcknds.o getmax.o setwtst.o validid.o maxwordlen.o upperstr.o

vtoc:	vtoc.a vtoc.h $(I)/gsl.h $(LIB)
	$(CC) $(CFLAGS) -o vtoc vtoc.a $(LIB) -lm

install:
	install -c -m 755 vtoc $(DESTDIR)/usr/bin/graf/vtoc

vtoc.a:	 $(LFILES) $(HDR)
	$(AR) cr vtoc.a $(LFILES)
	$(RANLIB) vtoc.a

clean clobber:
	-rm -f *.o vtoc vtoc.a
