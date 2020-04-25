
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

ILIB= t4014 t300 t300s t450 vplot
IDIR= tplot

all: t4014 t300 t300s t450 & vplot & tplot

t4014: driver.o
	$(CC) $(CFLAGS) -o t4014 driver.o -l4014 -lm

t300:	driver.o
	$(CC) $(CFLAGS) -o t300 driver.o -l300 -lm

t300s: driver.o
	$(CC) $(CFLAGS) -o t300s driver.o -l300s -lm

t450:	driver.o
	$(CC) $(CFLAGS) -o t450 driver.o -l450 -lm

vplot:	vplot.o banner.o
	$(CC) $(CFLAGS) -o vplot vplot.o banner.o

tplot:	tplot.sh
	cp tplot.sh tplot

install:
	 for i in ${IDIR}; do \
	 	install -m 755 $$i $(DESTDIR)/usr/bin; \
	 done
	 for i in ${ILIB}; do \
	 	install -m 644 $$i $(DESTDIR)/usr/lib; \
	 done

clean clobber:
	 -rm -f t4014 t300 t300s t450 vplot tplot *.o
