
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O

## NOTE: sgetl is from libld.a ##
FILES =\
	main.o \
	doname.o \
	misc.o \
	files.o \
	rules.o \
	dosys.o \
	gram.o \
	dyndep.o \
	prtmem.o \
	sgetl.o

all:	make

make:&  $(FILES)
	$(CC) -o make $(LDFLAGS) $(FILES)

gram.c:	gram.y

gram.o: gram.c

$(FILES):  defs

install:
	install -s make $(DESTDIR)/bin/make

clean clobber:
	rm -f *.o gram.c core
