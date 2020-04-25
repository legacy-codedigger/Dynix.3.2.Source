
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

all:	diff3prog

diff3prog: diff3prog.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o diff3prog diff3prog.c

install:
	install -s diff3prog $(DESTDIR)/usr/lib/diff3prog
	install -c diff3.sh $(DESTDIR)/usr/bin/diff3

clean clobber:
	rm -f *.o diff3prog core
