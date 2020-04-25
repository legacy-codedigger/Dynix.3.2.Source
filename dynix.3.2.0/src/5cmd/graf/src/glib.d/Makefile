
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../include
LIB =	../../lib

glib:
	-cd gsl.d; make $(MFLAGS) $(MRULES)
	-cd gpl.d; make $(MFLAGS) $(MRULES)
	touch glib

install:
	:

clean clobber:
	-cd gsl.d; make $(MFLAGS) $(MRULES) clean
	-cd gpl.d; make $(MFLAGS) $(MRULES) clean
	-rm -f glib
