
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= $(ATT_INCLUDE) -O
OFILES	= sno1.o sno2.o sno3.o sno4.o

all:	sno

sno:&	$(OFILES)
	$(CC) -o sno $(OFILES)

$(OFILES):	sno.h

install:
	install -s sno $(DESTDIR)/usr/bin/sno

clean clobber:
	rm -f *.o core sno
