
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= $(ATT_INCLUDE) -O
OFILES	= sed0.o sed1.o

all:	sed

sed:&	$(OFILES)
	$(CC) -o sed $(OFILES)

$(OFILES): sed.h

install:
	   install -s sed $(DESTDIR)/bin/sed

clean clobber:
	   rm -f *.o sed core
