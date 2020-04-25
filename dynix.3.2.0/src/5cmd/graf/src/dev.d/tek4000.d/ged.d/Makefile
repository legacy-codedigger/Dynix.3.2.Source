
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../../../include
L =	../../../../lib
BIN =	$(DESTDIR)/usr/bin/graf
LIB =	../lib
HDRS =	$(I) ged.h ../include/gedstructs.h
LFILES = bufged.o clipu.o cmdfuncs.o cmdproc.o copymove.o \
	deleteobj.o dispbuf.o drbox.o \
	ged.o getfile.o \
	objects.o outwindow.o points.o \
	rotarea.o scalearea.o setwindow.o text.o \
	unixesc.o window.o xymxmn.o

ged:	ged.a $(LIB)/tek.a $(L)/glib.a
	$(CC) $(CFLAGS) ged.a $(LIB)/tek.a $(L)/glib.a -lm -o ged

install:
	install -c -m 755 ged $(BIN)/ged

ged.a:	$(LFILES)
	$(AR) cr ged.a $(LFILES)
	$(RANLIB) ged.a

clean clobber:
	-rm -f *.o ged.a ged
