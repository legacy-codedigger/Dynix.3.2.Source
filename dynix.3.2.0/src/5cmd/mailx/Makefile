
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
VERSION	= 2.14
HDR	= hdr
HELP	= help
DESTLIB	= $(DESTDIR)/usr/lib/mailx
D	= $(DESTDIR)
I	= install -s $? $@
IG	= install -s -m 2755 -o root $? $@
IC	= install -c -m 644 $? $@

DEFINES	= -DUSG -DUSG_TTY
CFLAGS	= -O -I${HDR} ${ATT_INCLUDE} ${DEFINES}
LDFLAGS	=
LIBES	=
IFILES	=\
	$(D)/usr/bin/mailx\
	$(D)/usr/lib/mailx/rmmail\
	$(D)/usr/lib/mailx/mailx.help\
	$(D)/usr/lib/mailx/mailx.help.~

OBJS=	version.o \
	aux.o \
	cmd1.o \
	cmd2.o \
	cmd3.o \
	cmd4.o \
	cmdtab.o \
	collect.o \
	config.o \
	edit.o \
	fio.o \
	getname.o \
	hostname.o \
	head.o \
	usg.local.o \
	lock.o \
	lex.o \
	list.o \
	main.o \
	names.o \
	optim.o \
	popen.o \
	quit.o \
	send.o \
	stralloc.o \
	temp.o \
	tty.o \
	vars.o \
	sigretro.o \
	lpaths.o \
	extern.o \
	str.o		# must be last

SRCS=	aux.c \
	cmd1.c \
	cmd2.c \
	cmd3.c \
	cmd4.c \
	collect.c \
	config.c \
	edit.c \
	fio.c \
	getname.c \
	head.c \
	hostname.c \
	usg.local.c \
	lock.c \
	lex.c \
	list.c \
	main.c \
	names.c \
	optim.c \
	popen.c \
	quit.c \
	send.c \
	stralloc.c \
	temp.c \
	tty.c \
	vars.c \
	sigretro.c \
	lpaths.c \
	extern.c

MHDRS=	${HDR}/rcv.h \
	${HDR}/configdefs.h \
	${HDR}/def.h \
	${HDR}/glob.h \
	${HDR}/usg.local.h \
	${HDR}/local.h \
	${HDR}/uparm.h \
	${HDR}/sigretro.h

#
# Special massaging of C files for sharing of strings
#
.c.o:
	$(CC) $(CFLAGS) -c x$*.c; mv x$*.o $*.o; rm x$*.c

all:	mailx rmmail

mailx:	mkxstrings parallel
	${CC} ${LDFLAGS} -o mailx ${OBJS} ${LIBES}

mkxstrings: $(SRCS)
	for i in $?; do \
		$(CC) -E $(CFLAGS) $$i | xstr -c -; \
		mv x.c x$$i; \
	done
	touch mkxstrings

parallel:& $(OBJS)

rmmail:
	${CC} ${CFLAGS} ${LDFLAGS} -o rmmail rmmail.c

install:& $(IFILES)

$(D)/usr/bin/mailx: mailx; $(I)
$(D)/usr/lib/mailx/rmmail: rmmail; $(IG)
$(D)/usr/lib/mailx/mailx.help: $(HELP)/mailx.help; $(IC)
$(D)/usr/lib/mailx/mailx.help.~: $(HELP)/mailx.help.~; $(IC)

version.o: version.c
	${CC} -c version.c
	rm -f version.c

version.c:
	echo "char *version=\"mailx version ${VERSION}\";" > version.c

clean:
	rm -f *.o mkxstrings
	rm -f version.c a.out x.c xs.c core strings

clobber:	clean
	rm -f mailx rmmail

lint:	version.c
	${LINT} ${CFLAGS} ${SRCS}
	rm -f version.c
str.o:	strings
	xstr
	${CC} -c xs.c
	mv xs.o str.o
	rm xs.c

cmdtab.o:	cmdtab.c
	${CC} -c ${CFLAGS} cmdtab.c

listing:
	pr mailx.mk hdr/*.h [a-l]*.c | lpr
	pr [m-z]*.c | lpr
