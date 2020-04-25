
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

I =	../../../include
LIB =	../../../lib/glib.a
SOURCE =  arcfill.c clipseg.c compwind.c dispfile.c dispprim.c  inarea.c \
	linefill.c softarc.c softtext.c utodcoord.c
OBJS =  arcfill.o clipseg.o compwind.o dispfile.o dispprim.o  inarea.o \
	linefill.o softarc.o softtext.o utodcoord.o
HDR1 =	$(I) ../include/dev.h
HDR2 = $(HDR1) ../include/fill.h

uplib:  uplib.a  $(LIB)
	touch uplib

uplib.a:  $(HDR1) $(OBJS)
	  $(AR) cr uplib.a $(OBJS)
	  $(RANLIB) uplib.a

clean clobber:
	-rm -f uplib.a $(OBJS)
