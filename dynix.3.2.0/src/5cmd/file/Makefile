
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

all:	file

file:	file.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o file file.c

install:
	install -s file $(DESTDIR)/bin/file
	install -c -m 644 magic $(DESTDIR)/etc/magic

clean:
	rm -f *.o core

clobber: clean
	rm -f file
