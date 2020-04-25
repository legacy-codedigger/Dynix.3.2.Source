
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
OBJS	= allprint.o main.o reject.o yyless.o yywrap.o

all:	libl.a

libl.a:& $(OBJS)
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 $(OBJS)
	@-chgrp daemon $(OBJS) > /dev/null 2>&1
	@-chown  root $(OBJS) > /dev/null 2>&1
	@echo "	building $@"
	@$(AR) rcu libl.a $(OBJS)

install:
	install -m 644 libl.a $(DESTDIR)/usr/lib/libl.a
	$(RANLIB) $(DESTDIR)/usr/lib/libl.a
	install -c -m 644 lib/ncform $(DESTDIR)/usr/lib/lex/ncform
	install -c -m 644 lib/nrform $(DESTDIR)/usr/lib/lex/nrform

clean clobber:
	rm -f *.o libl.a core

allprint.o:	lib/allprint.c
	$(CC) -c $(CFLAGS) lib/allprint.c
main.o:		lib/main.c
	$(CC) -c $(CFLAGS) lib/main.c
reject.o:		lib/reject.c
	$(CC) -c $(CFLAGS) lib/reject.c
yyless.o:		lib/yyless.c
	$(CC) -c $(CFLAGS) lib/yyless.c
yywrap.o:		lib/yywrap.c
	$(CC) -c $(CFLAGS) lib/yywrap.c
