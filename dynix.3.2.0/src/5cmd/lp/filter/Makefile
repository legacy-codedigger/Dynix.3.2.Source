
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

FILES	= hp2631a.o prx.o pprx.o

all: hp2631a prx pprx
	:

hp2631a:	hp2631a.o
	$(CC) $(CFLAGS) -o hp2631a hp2631a.o

prx:	prx.o
	$(CC) $(CFLAGS) -o prx prx.o

pprx:	pprx.o
	$(CC) $(CFLAGS) -o pprx pprx.o

install:
	install -c -m 755 hp2631a $(DESTDIR)/usr/lib/hp2631a
	install -m 755 prx $(DESTDIR)/usr/lib/prx
	install -m 755 pprx $(DESTDIR)/usr/lib/pprx

clean clobber:
	rm -f $(FILES) hp2631a prx pprx
