
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
FILES	= dc.c

all:	dc

dc:	$(FILES)
	$(CC) $(CFLAGS) $(LDFLAG) -o dc $(FILES)

$(FILES): dc.h

install:
	install -s dc $(DESTDIR)/usr/bin/dc

clean:
	rm -f *.o core

clobber: clean
	rm -f dc
