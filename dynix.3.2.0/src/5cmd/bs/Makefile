
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
FILES	= atof.o bs.o string.o

all:	bs

bs:&	$(FILES)
	$(CC) -o bs $(FILES) -lm 

install:
	install -s bs $(DESTDIR)/bin/bs

clean clobber:
	rm -f *.o core bs
