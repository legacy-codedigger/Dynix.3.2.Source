
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

LFILES = getgedf.o getgeds.o gplalpha.o \
	gplarcs.o gplcomment.o gplinit.o gpllines.o  \
	gpltext.o putgedf.o putgeds.o \
	getgeds.o putgeds.o
LIB =	../../../lib

gpl:	$(LFILES)
	$(AR) cr $(LIB)/glib.a  $(LFILES)
	$(RANLIB) $(LIB)/glib.a

install:
	:

clean clobber:
	-rm -f *.o
