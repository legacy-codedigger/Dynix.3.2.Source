
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
D	= $(DESTDIR)
IFILES	= $(D)/usr/lib/calprog $(D)/usr/bin/calendar
I	= install -s $? $@
IC	= install -c $? $@

all:	calprog

calprog:
	$(CC) $(CFLAGS) -o calprog calprog.c

install:& $(IFILES)

$(D)/usr/lib/calprog: calprog; $(I)
$(D)/usr/bin/calendar: calendar.sh; $(IC)

clean clobber:
	rm -f calprog *.o core
