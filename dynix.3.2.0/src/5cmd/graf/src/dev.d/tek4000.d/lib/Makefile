
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

I =	../../../../include
HDR1 =	$(I) tek.h  ../include/gedstructs.h
HDR2 = $(HDR1)  ../include/termodes.h

LFILES = addrbuf.o chplot.o circarc.o clip.o \
	getacur.o getcraw.o getline.o getpoint.o getpos.o inarea.o locacur.o\
	scaleoff.o setmode.o\
	tekalpha.o tekarc.o tekflush.o tekinit.o teklines.o tekmove.o\
	tektext.o tekxy.o unscaleoff.o xytek.o xytekl.o

tek:	tek.a
	touch tek

tek.a:	$(LFILES)
	$(AR) cr tek.a $(LFILES)
	$(RANLIB) tek.a

clean clobber:
	-rm -f *.o tek.a tek
