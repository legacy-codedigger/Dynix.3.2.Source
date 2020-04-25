
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)
CMDS= cron at crontab batch

.c.o:
	$(CC) $(CFLAGS) -c $<

all:	$(CMDS)

install:
	install -c -m 700 -o root -g daemon cron $(DESTDIR)/etc
	install -c -m 4755 -o root -g daemon crontab $(DESTDIR)/usr/bin
	install -c -m 4755 -o root -g daemon at $(DESTDIR)/usr/bin
	install -c -m 755 batch $(DESTDIR)/usr/bin

libelm.a: elm.o
	$(AR) cr libelm.a elm.o
	$(RANLIB) libelm.a

cron:	cron.o funcs.o libelm.a
	$(CC) $(CFLAGS) cron.o funcs.o libelm.a -o cron

crontab:	crontab.o permit.o funcs.o
	$(CC) $(CFLAGS) crontab.o permit.o funcs.o -o crontab

at:	at.o att1.o att2.o funcs.o permit.o
	$(CC) $(CFLAGS) at.o att1.o att2.o funcs.o permit.o -o at

batch:	batch.sh
	cp batch.sh batch

att1.c att1.h:	att1.y
	$(YACC) $(YFLAGS) att1.y
	mv y.tab.c att1.c
	mv y.tab.h att1.h

att2.c:	att2.l
	lex att2.l
	ed - lex.yy.c < att2.ed
	mv lex.yy.c att2.c

att2.o:	att1.h

cron.o:	cron.c cron.h
crontab.o:	crontab.c cron.h
at.o:	at.c cron.h

clean clobber:
	rm -f *.o libelm.a $(CMDS)
