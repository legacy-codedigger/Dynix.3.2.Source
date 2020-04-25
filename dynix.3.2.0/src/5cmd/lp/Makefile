
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)
D	= $(DESTDIR)

ALLUSR = cancel disable enable lp lpstat
ALLLIB	= accept lpmove lpshut reject
ALLROOTLIB = lpsched lpadmin
LIB	= lib.a

ADMIN	= lp
GROUP	= daemon
SUSER	= root
SPOOL	= /usr/spool/lp
ADMDIR	= /usr/lib
USRDIR	= /usr/bin
DEFS	= -DSPOOL='"$(SPOOL)"' -DADMIN='"$(ADMIN)"'\
	  -DADMDIR='"$(ADMDIR)"' -DUSRDIR='"$(USRDIR)"'

LFILES	= dest.c destlist.c fullpath.c gwd.c enter.c fifo.c getname.c \
	  isclass.c isprinter.c isrequest.c outlist.c outputq.c pstatus.c \
	  date.c isdest.c eacross.c qstatus.c fatal.c lock.c request.c \
	  sendmail.c trim.c wrtmsg.c findtty.c
SFILES	= accept.c cancel.c disable.c enable.c lp.c lpadmin.c lpmove.c \
	  lpsched.c lpshut.c lpstatus.c reject.c
HFILES	= lp.h lpsched.h
LIBFILES= dest.o destlist.o fullpath.o gwd.o enter.o fifo.o getname.o\
	  isclass.o isprinter.o isrequest.o outlist.o outputq.o pstatus.o\
	  date.o isdest.o eaccess.o qstatus.o fatal.o lock.o request.o\
	  sendmail.o trim.o wrtmsg.o findtty.o

.c.a:

all:	$(ALLUSR) $(ALLLIB) $(ALLROOTLIB) filter_d

.PRECIOUS: $(LIB)

install:
	install -c -m 644 -o $(ADMIN) -g $(GROUP) /dev/null $(D)$(SPOOL)/pstatus
	install -c -m 644 -o $(ADMIN) -g $(GROUP) /dev/null $(D)$(SPOOL)/qstatus
	cd filter;  make $(MFLAGS) $(MRULES) install
	cd model;   make $(MFLAGS) $(MRULES) install
	for i in $(ALLUSR) ; do \
		install -m 6775 -o $(ADMIN) -g $(GROUP) $$i $(D)$(USRDIR)/$$i; \
	done
	for i in $(ALLLIB) ; do \
		install -m 6775 -o $(ADMIN) -g $(GROUP) $$i $(D)$(ADMDIR)/$$i; \
	done
	for i in $(ALLROOTLIB) ; do \
		install -m 6775 -o $(SUSER) -g $(GROUP) $$i $(D)$(ADMDIR)/$$i; \
	done

accept:	accept.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

cancel:	cancel.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

disable:	disable.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

enable:	enable.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

lp:	lp.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

lpadmin:	lpadmin.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

lpmove:	lpmove.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

lpsched:	lpsched.c lpsched.h lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

lpshut:	lpshut.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

lpstat:	lpstat.c lpsched.h lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

reject:	reject.c lp.h $(LIB)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $@.c $(LIB)

filter_d:
	cd filter; make $(MFLAGS) $(MRULES) all

$(LIB): $(LIBFILES)
	$(AR) cr $(LIB) $(LIBFILES)
	$(RANLIB) $(LIB)

clean clobber:
	rm -f $(ALLUSR) $(ALLLIB) $(ALLROOTLIB) $(LIB) *.o
	cd filter; make $(MFLAGS) $(MRULES) clobber
	cd model; make $(MFLAGS) $(MRULES) clobber
