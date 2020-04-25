
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

all:& ptx eign

ptx:	ptx.o
	$(CC) $(CFLAGS) -o $* $*.o

eign	:;	cp eign.sh eign

install: 
	install ptx $(DESTDIR)/usr/bin/ptx 
	install -m 644 eign $(DESTDIR)/usr/lib/eign

clean clobber:
	-rm -f *.o eign ptx
