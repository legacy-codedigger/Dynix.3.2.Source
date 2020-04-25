
DESTDIR	= $(ATT_DESTDIR)
BIN	= $(DESTDIR)/usr/bin
LIB	= $(DESTDIR)/usr/lib/ctrace
RUN	= /usr/lib/ctrace
YACC	= $(ATT_YACC)

# setting preprocessor symbols:
# set for UNIX/370:
# U370 = -b1,0
CC	= $(ATT_CC)
CFLAGS	= -O -DLIB=\"$(RUN) $(U370) $(ATT_INCLUDE)
I	= install -s $? $@
IC	= install -c $? $@
IFILES	=\
	$(BIN)/ctrace\
	$(BIN)/ctcr\
	$(LIB)/runtime.c

YFLAGS	= -d

SOURCE	= constants.h global.h main.c parser.y scanner.l lookup.c trace.c \
	   runtime.c ctcr
OTHER	= ctrace.mk
OBJECTS = main.o parser.o scanner.o lookup.o trace.o

all: ctrace

ctrace: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

install:& $(IFILES)
	chmod 644 $(LIB)/runtime.c
	rm -f $(BIN)/ctc
	ln $(BIN)/ctcr $(BIN)/ctc

$(BIN)/ctrace: ctrace; $(I)
$(BIN)/ctcr: ctcr; $(IC)
$(LIB)/runtime.c: runtime.c; $(IC)

clean:
	rm -f *.o y.tab.h y.output

clobber: clean
	rm -f ctrace
