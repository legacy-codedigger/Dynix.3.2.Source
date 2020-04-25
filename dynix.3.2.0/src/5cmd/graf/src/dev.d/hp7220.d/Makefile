
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../../include
DIRS =	hpd.d
LIB =	../../../lib/glib.a
BIN =	../../../bin

hp7220 all:	$(DIRS)
		cd hpd.d; make $(MFLAGS) $(MRULES)
		touch hp7220

install:
	cd hpd.d; make $(MFLAGS) $(MRULES) install

clean clobber:
	cd hpd.d; make $(MFLAGS) $(MRULES) clean
	-rm -f hp7220
