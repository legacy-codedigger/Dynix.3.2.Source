
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
FILES	= main.o sub1.o sub2.o header.o parser.o

all:	lex

lex:&	$(FILES)
	$(CC) $(LDFLAGS) $(FILES) -o lex -ly

$(FILES): ldefs.c
main.o:	  once.c

install:
	 install -s lex $(DESTDIR)/usr/bin/lex

clean clobber:
	 -rm -f *.o y.tab.c lex
