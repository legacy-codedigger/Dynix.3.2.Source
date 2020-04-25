
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

all:
	cd src; make $(MFLAGS) $(MRULES)

install:
	cd src; make $(MFLAGS) $(MRULES) install

clean clobber:
	cd src; make $(MFLAGS) $(MRULES) clean
