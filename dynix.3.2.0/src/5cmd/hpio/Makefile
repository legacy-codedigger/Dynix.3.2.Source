
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

all: hpio

hpio: hpio.o
	$(CC) $(CFLAGS) -o hpio hpio.o

install: all
	install -s hpio $(DESTDIR)/usr/bin/hpio

clean clobber:
	-rm -f hpio *.o
