
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= -O $(ATT_INCLUDE)
LDFLAGS	=

all:	 bfs

bfs:
	$(CC) $(CFLAGS) $(LDFLAGS) -o bfs bfs.c

install:
	install -s bfs $(DESTDIR)/usr/bin/bfs

clean:
	rm -f *.o core

clobber:  clean
	  rm -f bfs
