
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

LIB =	../../../lib/glib.a
BIN =	../../../bin
DIRS =	lolib uplib hp7220.d tek4000.d

all:
	cd lolib; make $(MFLAGS) $(MRULES)
	cd uplib; make $(MFLAGS) $(MRULES)
	cd hp7220.d; make $(MFLAGS) $(MRULES)
	cd tek4000.d; make $(MFLAGS) $(MRULES)

install:
	cd hp7220.d; make $(MFLAGS) $(MRULES) install
	cd tek4000.d; make $(MFLAGS) $(MRULES) install

clean clobber:
	cd lolib; make $(MFLAGS) $(MRULES) clean
	cd uplib; make $(MFLAGS) $(MRULES) clean
	cd hp7220.d; make $(MFLAGS) $(MRULES) clean
	cd tek4000.d; make $(MFLAGS) $(MRULES) clean
