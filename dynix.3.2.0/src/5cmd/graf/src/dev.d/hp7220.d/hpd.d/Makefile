
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
SOURCE =	devarc.c devbuff.c devcursor.c devfinal.c devinit.c devfill.c \
		devline.c devtext.c mba.c mbn.c mbp.c newcolor.c sbn.c
LFILES =	devarc.o devbuff.o devcursor.o devfinal.o devinit.o devfill.o \
		devline.o devtext.o mba.o mbn.o mbp.o newcolor.o sbn.o

HDR1 =	$(I) ../../include/dev.h dev.h

hp:	dev.c dev.a $(UPLIB)/uplib.a $(LOLIB)/lolib.a  $(L)/glib.a
	$(CC) $(CFLAGS) dev.c $(UPLIB)/uplib.a dev.a $(LOLIB)/lolib.a  $(L)/glib.a -lm -o hpd

install:
	install -c -m 755 hpd $(BIN)/hpd

dev.a:	$(LFILES)
	$(AR) cr dev.a $(LFILES)
	$(RANLIB) dev.a

clean clobber:
	-rm -f *.o hpd dev.a
