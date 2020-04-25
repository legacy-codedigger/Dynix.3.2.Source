
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
FILES	= awk.lx.o b.o main.o tran.o lib.o run.o parse.o freeze.o awk.g.o

##
# proctab.o and token.o must be built first so as not to get /bin/cc versions.
##
all:	proctab.o token.o awk

awk:&	$(FILES)
	$(CC) $(FILES) token.o proctab.o awk.g.o -o awk -lm

y.tab.h:	awk.g.o

awk.h:	y.tab.h
	cp y.tab.h awk.h

$(FILES) token.o proctab.o:	awk.h awk.def

token.c:	awk.h tokenscript
	ed - <tokenscript

lint:
	lint -spu b.c main.c token.c tran.c run.c lib.c parse.c -lm |\
		egrep -v '^(error|free|malloc)'

proctab.c:	./makeprctab
	./makeprctab >proctab.c

./makeprctab:	awk.h makeprctab.c token.c
	/bin/cc -o ./makeprctab makeprctab.c token.c; rm -f makeprctab.o token.o

clean clobber:
	rm -f a.out *.o t.* *temp* *.out *junk* y.tab.* awk.h makeprctab proctab.c awk

install: 
	install -s awk $(DESTDIR)/usr/bin/awk
