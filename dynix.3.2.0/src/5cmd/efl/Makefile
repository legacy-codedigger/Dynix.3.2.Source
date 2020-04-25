
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
INSDIR = $(DESTDIR)/usr/bin
FILES =  Makefile efl.1 defs tokens main.c init.c tailor.c\
		gram.head gram.dcl gram.expr gram.exec\
		lex.l fixuplex symtab.c\
		dcl.c addr.c struct.c field.c misc.c alloc.c mk.c\
		exec.c blklab.c simple.c print.c temp.c io.c error.c\
		pass2.c icfile.c free.c dclgen.c namgen.c
OFILES = main.o init.o tailor.o gram.o lex.o symtab.o\
	dcl.o addr.o struct.o field.o blklab.o\
	mk.o misc.o alloc.o print.o simple.o exec.o temp.o io.o error.o\
	pass2.o icfile.o free.o dclgen.o namgen.o

all:	efl

efl:	$(OFILES)
	$(CC) -o efl $(CFLAGS) $(OFILES)

$(OFILES):	defs
lex.o init.o:	tokdefs

gram.c:	gram.head gram.dcl gram.expr gram.exec tokdefs
	(sed <tokdefs "s/#define/%token/" ;\
		cat gram.head gram.dcl gram.expr gram.exec) >gram.in;\
		$(YACC) $(YFLAGS) gram.in;  mv y.tab.c gram.c;  rm gram.in;

lex.c:	fixuplex lex.l
	lex lex.l
	sh ./fixuplex
	mv lex.yy.c lex.c

tokdefs: tokens
	grep -n "^[^#]" <tokens | sed "s/\([^:]*\):\(.*\)/#define \2 \1/" >tokdefs

.c.o:
	$(CC) -c $(CFLAGS) $< 2>&1 | sed "/arning/d"

clean clobber:
	rm -f *.o efl[cde].* gram.c lex.c tokdefs efl

install :
	install -c efl $(INSDIR)/efl
