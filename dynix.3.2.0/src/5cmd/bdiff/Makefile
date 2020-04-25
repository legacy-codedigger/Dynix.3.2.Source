
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= $(ATT_INCLUDE) -O

all:	bdiff

bdiff:&	bdiff.o
	$(CC) $(LDFLAGS) -o bdiff bdiff.o -lPW

install:
	install -s bdiff $(DESTDIR)/usr/bin/bdiff

clean clobber:
	rm -f bdiff core *.o
