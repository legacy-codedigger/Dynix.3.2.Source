
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

LIB =	../../../lib
LFILES = box.o gsl.o gslgsl.o gslswing.o \
	putx0y0.o region.o textbox.o textcent.o \
	textright.o xymaxmin.o

gsl:	$(LFILES)
	$(AR) cr $(LIB)/glib.a $(LFILES)
	$(ATT_RANLIB) $(LIB)/glib.a

clean clobber:
	-rm -f *.o
