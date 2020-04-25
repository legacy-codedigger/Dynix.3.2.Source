
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

OWNER=uucp
INSDIR=$(DESTDIR)/usr/lib/uucp
BIN=$(DESTDIR)/usr/bin
PUBDIR=$(DESTDIR)/usr/spool/uucppublic
SPOOL=$(DESTDIR)/usr/spool/uucp
XQTDIR=$(DESTDIR)/usr/lib/uucp/.XQTDIR
OLDLOG=$(DESTDIR)/usr/lib/uucp/.OLD
UPATH=.
PKON=pkon.o
PKONSRC=pkon.c
LINE=line.o
LINESRC=line.c
IOCTL=
GETOPT=
COMMANDS=uucp uux uuxqt uucico uulog uuclean uuname uustat uusub
OFILES=assert.o cpmv.o expfile.o gename.o getpwinfo.o \
	prefix.o shio.o ulockf.o xqt.o logent.o versys.o gnamef.o rkill.o
LFILES=assert.c cpmv.c expfile.c gename.c getpwinfo.c \
	prefix.c shio.c ulockf.c xqt.c logent.o
OUUCP=uucpdefs.o uucp.o gwd.o chkpth.o getargs.o logent.o uucpname.o\
	versys.o us_crs.o
LUUCP=uucpdefs.c uucp.c gwd.c chkpth.c getargs.c logent.c uucpname.c\
	versys.c us_crs.c
OUUX=uucpdefs.o uux.o gwd.o anyread.o chkpth.o getargs.o getprm.o\
	logent.o uucpname.o versys.o us_crs.o
LUUX=uucpdefs.c uux.c gwd.c anyread.c chkpth.c getargs.c getprm.c\
	logent.c uucpname.c versys.c us_crs.c
OUUXQT=uucpdefs.o uuxqt.o mailst.o getprm.o gnamef.o logent.o uucpname.o \
	chkpth.o getargs.o anyread.o cpmv.o
LUUXQT=uucpdefs.c uuxqt.c mailst.c getprm.c gnamef.c logent.c uucpname.c \
	chkpth.c getargs.c anyread.c cpmv.c
OUUCICO=uucpdefs.o cico.o conn.o cntrl.o pk0.o pk1.o gio.o xio.o anyread.o \
	anlwrk.o chkpth.o getargs.o gnamef.o gnsys.o gnxseq.o \
	pkdefs.o imsg.o fwdchk.o  logent.o sysacct.o systat.o \
	gtcfile.o us_crs.o mailst.o uucpname.o us_rrs.o us_sst.o ub_sst.o $(LINE)
LUUCICO=uucpdefs.c cico.c conn.c cntrl.c pk0.c pk1.c gio.c xio.c anyread.c \
	anlwrk.c chkpth.c getargs.c gnamef.c gnsys.c gnxseq.c \
	$(PKONSRC) pkdefs.c imsg.c fwdchk.c logent.c sysacct.c systat.c \
	gtcfile.o us_crs.o mailst.c uucpname.c us_rrs.c us_sst.c ub_sst.c $(LINESRC)
OUULOG=uucpdefs.o uulog.o prefix.o xqt.o ulockf.o gnamef.o assert.o logent.o
LUULOG=uucpdefs.c uulog.c prefix.c xqt.c ulockf.c gnamef.c assert.c logent.o
OUUCLEAN=uucpdefs.o uuclean.o gnamef.o prefix.o mailst.o getpwinfo.o\
	 getargs.o assert.o
LUUCLEAN=uucpdefs.c uuclean.c gnamef.c prefix.c mailst.c getpwinfo.c\
	 getargs.c
OUUNAME=uuname.o uucpname.o uucpdefs.o getpwinfo.o
LUUNAME=uuname.c uucpname.c uucpdefs.c getpwinfo.c
OUUSTAT=uucpdefs.o uustat.o gnamef.o getpwinfo.o $(GETOPT) \
	cpmv.o ulockf.o assert.o logent.o
LUUSTAT=uucpdefs.c uustat.c gnamef.c getpwinfo.c \
	cpmv.c ulockf.c assert.c logent.o
OUUSUB=uucpdefs.o uusub.o getpwinfo.o xqt.o $(GETOPT)
LUUSUB=uucpdefs.c uusub.c getpwinfo.c xqt.c 
INIT=init

all:	$(INIT) $(COMMANDS) 

install:	mkdirs cpfiles
	$(CH)chown $(OWNER) $(INSDIR)
	$(CH)chmod 755 $(INSDIR)
	install -m 4111 -o $(OWNER) uucp $(BIN)/uucp
	install -m 4111 -o $(OWNER) uux $(BIN)/uux
	install -m 4111 -o $(OWNER) uuxqt $(INSDIR)/uuxqt
	install -m 4111 -o $(OWNER) uucico $(INSDIR)/uucico
	install -m 4111 -o $(OWNER) uulog $(BIN)/uulog
	install -m 4111 -o $(OWNER) uuclean $(INSDIR)/uuclean
	install -m 4111 -o $(OWNER) uuname $(BIN)/uuname
	install -m 4111 -o $(OWNER) uustat $(BIN)/uustat
	install -m 100 -o $(OWNER) uusub $(INSDIR)/uusub

clean clobber:
	-rm -f *.o $(COMMANDS)

init:	anlwrk.o anyread.o chkpth.o cpmv.o expfile.o gename.o \
	getargs.o getprm.o getpwinfo.o gnamef.o gnsys.o \
	gnxseq.o gwd.o imsg.o logent.o \
	prefix.o mailst.o shio.o sysacct.o \
	$(GETOPT) systat.o ulockf.o uucpname.o versys.o xqt.o

uucp:	$(OUUCP) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUCP) $(OFILES) $(LIBS) -o uucp

uux:	$(OUUX) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUX) $(OFILES) $(LIBS) -o uux

uuxqt:	$(OUUXQT) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUXQT) $(OFILES) $(LIBS) -o uuxqt

uucico:	$(OUUCICO) $(OFILES) $(IOCTL) $(PKON)
	$(CC) $(IFLAG) $(LDFLAGS) $(OUUCICO) $(OFILES) $(IOCTL) $(PKON) $(LIBS)\
	-o uucico

uulog:	$(OUULOG)
	$(CC) $(LDFLAGS) $(OUULOG) $(LIBS) -o uulog

uuclean:  $(OUUCLEAN) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUCLEAN) $(OFILES) $(LIBS) -o uuclean

uuname:	$(OUUNAME)
	$(CC) $(LDFLAGS) $(OUUNAME) $(LIBS) -o uuname
 
uustat:	$(OUUSTAT) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUSTAT) $(OFILES) $(LIBS) -o uustat

uusub:	$(OUUSUB)
	$(CC) $(LDFLAGS) $(OUUSUB) $(LIBS) -o uusub


ub_sst.o uusub.o:	uusub.h

cico.o:	uusub.h uust.h uucp.h

anlwrk.o cntrl.o us_crs.o us_rrs.o\
	uuclean.o rkill.o us_sst.o uucp.o uustat.o:	uust.h uucp.h

anyread.o assert.o chkpth.o cico.o conn.o cpmv.o expfile.o gename.o\
	getpwinfo.o gio.o fwdch.o xio.o gnamef.o gnsys.o gnxseq.o gwd.o imsg.o ioctl.o\
	logent.o mailst.o sdmail.o $(LINE) shio.o\
	systat.o ulockf.o uuclean.o uucpdefs.o uucpname.o uulog.o uuname.o\
	uux.o uuxqt.o versys.o xqt.o:	uucp.h

FRC:

mkdirs: # done in proto.sh
#	-mkdir $(INSDIR)
#	$(CH)chmod 755 $(INSDIR)
#	-mkdir $(SPOOL)
#	$(CH)chmod 777 $(SPOOL)
#	$(CH)chown $(OWNER) $(SPOOL)
#	-mkdir $(PUBDIR)
#	$(CH)chmod 777 $(PUBDIR)
#	$(CH)chown $(OWNER) $(PUBDIR)
#	-mkdir $(XQTDIR)
#	$(CH)chmod 777 $(XQTDIR)
#	$(CH)chown $(OWNER) $(XQTDIR)
#	-mkdir $(OLDLOG)
#	$(CH)chmod 777 $(OLDLOG)
#	$(CH)chown $(OWNER) $(OLDLOG)

cpfiles:
	cp $(UPATH)/uudemon* $(INSDIR)
	$(CH)chmod 755 $(INSDIR)/uudemon*
	$(CH)chown $(OWNER) $(INSDIR)/*

