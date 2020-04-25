
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
FILES	= m4.o m4ext.o m4macs.o m4y.o

all:	m4

m4:&	$(FILES)
	$(CC) $(LDFLAGS) -o m4 $(FILES) -ly

install:
	install -s m4 $(DESTDIR)/usr/bin/m4

clean clobber:
	rm -f *.o m4 y.tab.c core
