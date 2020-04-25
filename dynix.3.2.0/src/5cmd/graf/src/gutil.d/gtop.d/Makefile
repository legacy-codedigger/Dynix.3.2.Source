
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../../include
LIB =	../../../lib/glib.a
PLOTLIB = $(DESTDIR)/usr/lib/libplot.a
LFILES = arcrad.o gtop.o inarea.o plotarc.o setspace.o windowon.o
HDR0 =	$(I)/gsl.h
HDR1 =	$(HDR0) $(I)/util.h $(I)/errpr.h $(I)/setop.h $(I)/gpl.h

gtop:	gtop.a $(LIB)
	$(CC) $(CFLAGS) gtop.a $(PLOTLIB) $(LIB) -lm -o gtop

install:
	install -c -m 755 gtop $(DESTDIR)/usr/bin/graf/gtop

gtop.a:	$(LFILES)
	$(AR) cr gtop.a $(LFILES)
	$(RANLIB) gtop.a

clean clober:
	-rm -f gtop gtop.a *.o
