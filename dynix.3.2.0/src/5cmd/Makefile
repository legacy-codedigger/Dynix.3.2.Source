
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
D	= $(DESTDIR)

SUBDIRS= .adm acct awk bc bdiff bfs bs calendar cflow cpp cron ctrace cxref\
	 dc diff diff3 ed efl file graf hpio ipc lex lint lp m4 mailx make man\
	 ptx ratfor sccs sed sh sno spell text tplot uucp vi yacc

NOTYET= killall sa

RFILES= 300 300s 4014 450 arcv asa banner cal cat cb checkeq chgrp chmod chown\
	chroot cmp col comm cpio cpset crypt csplit ct cu cut date dd\
	deroff devnm du echo env expr fgrep find fsplit getopt getty grep grpck\
	hp hyphen id init join kill line link logname mail manprog\
	mesg mkdir mknod mount mv mv_dir newform news nice nl nohup od pack\
	paste pr pwck pwd rm rmdir sdiff setmnt sleep sort spline split\
	stty sum sync tail tar tee time touch tr tsort tty umount uname uniq\
	units unlink unpack wc wall who write xargs

YFILES	= egrep
SFILES	= factor graph login ls makekey passwd pg regcmp su tabs tput

BIN = 	cat chgrp chmod chown cmp cpio date dd du echo env expr find \
	grep kill line login ls mesg mv nice nohup od pr pwd rm sleep\
	sort stty sum sync tail tee time touch tty uname wall wc who write
USRBIN= 300 300s 4014 450 asa banner cal cb checkeq col comm cpset\
	csplit cut deroff egrep factor fgrep fsplit getopt graph hp hyphen\
	id join logname newform news nl pack paste pg regcmp sdiff\
	split spline tabs tar tput tr tsort uniq units unpack xargs
ETC=	devnm getty grpck init link mount pwck setmnt umount\
	unlink

all:	parallel $(YFILES) 

parallel:& $(SUBDIRS) $(RFILES) $(SFILES)

$(SUBDIRS): FRC
	cd $@; make $(MFLAGS) $(MRULES)

$(RFILES):
	-if [ "$@" != crypt  -o  "$(EXPORT)" != TRUE ]; then \
		$(CC) $(CFLAGS) -o $@ $@.c; \
	fi

$(YFILES):
	$(YACC) $(YFLAGS) $@.y
	$(CC) -c $(CFLAGS) y.tab.c
	mv y.tab.o $@.o
	$(CC) -o $@ $@.o
	rm -f $@.o y.tab.c

## commands that need special rules to link in libraries ##
makekey: makekey.c
	$(CC) $(CFLAGS) -o makekey makekey.c -lcrypt

su:	su.c
	$(CC) $(CFLAGS) -o su su.c -lcrypt

ls:	ls.c
	$(CC) $(CFLAGS) -o ls ls.c -lcurses

login:	login.c
	$(CC) $(CFLAGS) -o login login.c -lcrypt

passwd:	passwd.c
	$(CC) $(CFLAGS) -o passwd passwd.c -lcrypt

factor:	factor.c
	$(CC) $(CFLAGS) -o factor factor.c -lm

graph:	graph.c
	$(CC) $(CFLAGS) -o graph graph.c -lplot -lm

tabs:	tabs.c
	$(CC) $(CFLAGS) -o tabs tabs.c -lcurses

tput:	tput.c
	$(CC) $(CFLAGS) -o tput tput.c -lcurses

pg:	pg.c
	$(CC) $(CFLAGS) -o pg pg.c -lcurses

regcmp:	regcmp.c
	$(CC) $(CFLAGS) -o regcmp regcmp.c -lPW

# Temp, for ps
OCC = $(TOOLS)/xcc -A$(TOOLS)/ -B$(TOOLS)/ -G$(CCS_LIBS)/
ps: ps.c
	$(OCC) $(INCLUDE) -O -o ps ps.c -lseq

ps.install:
	$(OCC) $(INCLUDE) -O -o ps ps.c -lseq
	install -s -m 2755 -o root -g daemon ps $(ATT_DESTDIR)/bin

install: FRC
	@for i in $(SUBDIRS); do \
		echo "	cd $$i; make $(MFLAGS) $(MRULES) install"; \
		      ( cd $$i; make $(MFLAGS) $(MRULES) install ) done
	install -s $(BIN) $(D)/bin
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		install -s crypt $(ATT_CRYPT_DESTDIR)/bin; \
	fi'
	install -s $(USRBIN) $(D)/usr/bin
	install -s $(ETC) $(D)/etc
	install -s -m 4755 -o root mail $(D)/bin
	install -s -m 4755 -o root mkdir rmdir su $(D)/bin
	install -s -m 4755 -o root -g daemon ct $(D)/usr/bin
	install -s -m 4755 -o uucp -g daemon cu $(D)/usr/bin
	install -s -m 4755 -o root -g daemon passwd $(D)/bin
	install -s mv_dir makekey $(D)/usr/lib
	install -s chroot $(D)/etc/chroot
	install -s mknod $(D)/etc/mknod
	install -c basename.sh $(D)/bin/basename
	install -c dirname.sh $(D)/bin/dirname
	install -c false.sh $(D)/bin/false
	install -c true.sh $(D)/bin/true
	install -c install.sh $(D)/etc/install
	install -c diffmk.sh $(D)/usr/bin/diffmk
	install -c dircmp.sh $(D)/usr/bin/dircmp
	install -c greek.sh $(D)/usr/bin/greek
	install -c uupick.sh $(D)/usr/bin/uupick
	install -c uuto.sh $(D)/usr/bin/uuto
	install -c unittab.sh $(D)/usr/lib/unittab
	install -c whodo.sh $(D)/etc/whodo
	rm -f $(D)/bin/pdp11; ln $(D)/bin/false $(D)/bin/pdp11
	rm -f $(D)/bin/u370; ln $(D)/bin/false $(D)/bin/u370
	rm -f $(D)/bin/u3b10; ln $(D)/bin/false $(D)/bin/u3b10
	rm -f $(D)/bin/u3b2; ln $(D)/bin/false $(D)/bin/u3b2
	rm -f $(D)/bin/u3b5; ln $(D)/bin/false $(D)/bin/u3b5
	rm -f $(D)/bin/u3b; ln $(D)/bin/false $(D)/bin/u3b
	rm -f $(D)/bin/vax; ln $(D)/bin/false $(D)/bin/vax
	rm -f $(D)/bin/ns32000; ln $(D)/bin/false $(D)/bin/ns32000
	rm -f $(D)/bin/i386; ln $(D)/bin/false $(D)/bin/i386
	rm -f $(D)/bin/i486; ln $(D)/bin/false $(D)/bin/i486
	rm -f $(D)/bin/$(MACHINE); ln $(D)/bin/true $(D)/bin/$(MACHINE)
	rm -f $(D)/bin/rmail; ln $(D)/bin/mail $(D)/bin/rmail
	rm -f $(D)/bin/cp; ln $(D)/bin/mv $(D)/bin/cp
	rm -f $(D)/bin/ln; ln $(D)/bin/mv $(D)/bin/ln
	rm -f $(D)/usr/bin/pcat; ln $(D)/usr/bin/unpack $(D)/usr/bin/pcat
	rm -f $(D)/bin/telinit; ln $(D)/etc/init $(D)/bin/telinit

clean clobber:
	@for i in $(SUBDIRS); do \
		echo "	cd $$i; make $(MFLAGS) $(MRULES) $@"; \
		      ( cd $$i; make $(MFLAGS) $(MRULES) $@ ) done
	rm -f *.o core $(RFILES) $(YFILES) $(SFILES)

FRC:
