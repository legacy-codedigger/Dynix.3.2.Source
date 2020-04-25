
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

all:&	diff diffh

diff:	diff.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o diff diff.c

diffh:	diffh.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o diffh diffh.c

install:
	install -s diff $(DESTDIR)/bin/diff
	install -s diffh $(DESTDIR)/usr/lib/diffh

clean clobber:
	rm -f *.o diff diffh core
