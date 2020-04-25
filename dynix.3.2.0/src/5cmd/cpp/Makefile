
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O -DFLEXNAMES
YACC	= $(ATT_YACC)
CC	= $(ATT_CC)

all:	cpp

cpp:&	cpp.o cpy.o
	$(CC) $(LDFLAGS) -o cpp cpp.o rodata.o cpy.o

cpy.c : cpy.y
	$(YACC) cpy.y; \
	sh :yyfix >rodata.c; \
	mv y.tab.c cpy.c; \
	$(CC) $(CFLAGS) -S rodata.c; \
	sh :rofix rodata.s; \
	$(CC) $(CFLAGS) -c rodata.s

cpy.o :	cpy.c yylex.c
	$(CC) $(CFLAGS) -c cpy.c

cpp.o : cpp.c
	$(CC) $(CFLAGS) -Dunix=1 -D$(MACHINE)=1 -Dsequent=1 -c cpp.c

install:
	install -s cpp $(DESTDIR)/lib/cpp

clean clobber:
	rm -f *.s *.o cpp rodata.c cpy.c core
