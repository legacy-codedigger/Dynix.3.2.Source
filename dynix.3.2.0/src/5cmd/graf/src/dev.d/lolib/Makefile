
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

I =	../../../include
L =	../../../lib
SOURCE = adjarc.c adjline.c  adjtext.c
OBJS = adjarc.o adjline.o  adjtext.o
HDR1 =	$(I) ../include/dev.h

dev:  lolib.a  $(L)/glib.a
	touch dev

lolib.a:   $(HDR1) $(OBJS)
	   $(AR) cr lolib.a $(OBJS)
	   $(RANLIB) lolib.a

clean clobber:
	-rm -f lolib.a $(OBJS)
