
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../include
FILES = abs af bar bucket ceil cor cusum exp floor\
	gamma gas hilo hist label\
	list log lreg mean mod\
	pair pie plot point power prime prod qsort rand\
	rank root round siline subset sin title total var
BASIC =	s.h s.a
GRAPHICS = plot.h $(I)/gsl.h $(LIB)/glib.a
HDR =	ae.h title.h plot.h s.h 
LIB =	../../lib
LIBS =	ctos.o doubcmp.o filbuf.o getdoub.o getint.o\
	getlabel.o getline.o getx0y0.o minmax.o pow1.o scale.o
LIBAE =	aeeval.o aeparse.o

stat all: $(BASIC) $(GRAPHICS) $(FILES)
	touch stat

install:
	-for i in $(FILES); do \
		install -c $$i $(DESTDIR)/usr/bin/graf/$$i; \
	done

$(FILES):
	$(CC) $(CFLAGS) -o $@ $@.c  s.a ../../lib/glib.a -lm

s.h:	$(I) title.h
	:

s.a:	$(LIBS) $(LIBAE)
	$(AR) cr s.a $(LIBS) $(LIBAE)
	$(RANLIB) s.a

$(GRAPHICS):
		@:

clean clobber:
	-rm -f stat $(FILES) s.a *.o
