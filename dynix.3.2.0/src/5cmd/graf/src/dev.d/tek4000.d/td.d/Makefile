
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

I =	../../../../include
L =	../../../../lib
BIN = $(DESTDIR)/usr/bin/graf
UPLIB = ../../uplib
LOLIB = ../../lolib
LFILES =	devarc.o devcursor.o devfinal.o devinit.o devline.o \
		devtext.o xytek.o xytekl.o devfill.o devbuff.o

HDR1 =	$(I) ../../include/dev.h dev.h

dev:	dev.c dev.a $(UPLIB)/uplib.a $(LOLIB)/lolib.a  $(L)/glib.a
	$(CC) $(CFLAGS) dev.c $(UPLIB)/uplib.a dev.a $(LOLIB)/lolib.a  $(L)/glib.a -lm -o td

install:
	install -c -m 755 td $(BIN)/td

dev.a:	$(LFILES)
	$(AR) cr dev.a $(LFILES)
	$(RANLIB) dev.a

clean clobber:
	-rm -f *.o td dev.a
