
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../../include
LFILES = ptog.o linemod.o ptogarcpt.o
LIB =	../../../lib/glib.a
HDR =	$(I)/util.h $(I)/gpl.h $(I)/gsl.h $(I)/errpr.h

ptog:	ptog.a $(LIB)
	$(CC) $(CFLAGS) ptog.a $(LIB) -lm -o ptog

install:
	install -c -m 755 ptog $(DESTDIR)/usr/bin/graf/ptog

ptog.a:	$(LFILES)
	$(AR) cr ptog.a $(LFILES)
	$(RANLIB) ptog.a

clean clobber:
	-rm -f *.o ptog ptog.a
