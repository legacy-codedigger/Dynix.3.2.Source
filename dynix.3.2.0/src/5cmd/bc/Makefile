
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= $(ATT_INCLUDE) -O
YACC	= $(ATT_YACC)
FILES	= bc.o

all:	bc

bc:&	$(FILES)
	$(CC) $(CFLAGS) $(LDFLAG) -o bc $(FILES)

install:
	install -s bc $(DESTDIR)/usr/bin/bc
	install -c -m 644 lib.b.sh $(DESTDIR)/usr/lib/lib.b

clean clobber:
	rm -f bc core *.o
