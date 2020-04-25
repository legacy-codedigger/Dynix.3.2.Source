# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile.m4 2.1 86/04/28 $
#
#  SENDMAIL Makefile.
#
include(../md/config.m4)dnl

SLIBS=	../lib/libsys.a m4LIBS
OBJS1=	conf.o main.o collect.o parseaddr.o alias.o deliver.o \
	savemail.o err.o readcf.o stab.o headers.o recipient.o \
	stats.o daemon.o usersmtp.o srvrsmtp.o queue.o \
	macro.o util.o clock.o trace.o envelope.o
OBJS2=	sysexits.o bmove.o arpadate.o convtime.o
OBJS=	$(OBJS1) $(OBJS2)
SRCS1=	conf.h sendmail.h \
	conf.c deliver.c main.c parseaddr.c err.c alias.c savemail.c \
	sysexits.c util.c bmove.c arpadate.c version.c collect.c \
	macro.c headers.c readcf.c stab.c recipient.c stats.c daemon.c \
	usersmtp.c srvrsmtp.c queue.c clock.c trace.c envelope.c
SRCS2=	TODO convtime.c
SRCS=	Version.c $(SRCS1) $(SRCS2)
O=	-O

COPTS=
SDEFINE=-Dbmove=bcopy -Dclear=bzero
CCONFIG=-I../`include' m4CONFIG $(SDEFINE)
CFLAGS=	$O $(COPTS) $(CCONFIG) ${INCLUDE}
ASMSED=	../`include'/asm.sed
ARFLAGS=rvu
LINT=	lint
XREF=	ctags -x
CP=	cp
MV=	mv
INSTALL=install -c
M4=	m4
TOUCH=	touch
ABORT=	false

OBJMODE=755

# .c.o:
# 	cc -S ${CFLAGS} $*.c
# 	sed -f $(ASMSED) $*.s | as -o $*.o
# 	rm -f $*.s

all:	sendmail

sendmail:& $(OBJS1) $(OBJS2) Version.o
	$(CC) $(COPTS) -o sendmail Version.o $(OBJS1) $(OBJS2) $(SLIBS)
	${SIZE} sendmail; ls -l sendmail; ifdef(`m4SCCS', `$(WHAT) < Version.o')

install: all
	install -s -m 4755 sendmail ${DESTDIR}/usr/lib/sendmail

version: newversion $(OBJS) Version.c

newversion:
	@rm -f SCCS/p.version.c version.c
	@$(GET) $(REL) -e SCCS/s.version.c
	@$(DELTA) -s SCCS/s.version.c
	@$(GET) -t -s SCCS/s.version.c

fullversion: $(OBJS) dumpVersion Version.o

dumpVersion:
	rm -f Version.c

ifdef(`m4SCCS',
Version.c: version.c
	@echo generating Version.c from version.c
	@cp version.c Version.c
	@chmod 644 Version.c
	@echo "" >> Version.c
	@echo "`# ifdef' COMMENT" >> Version.c
	@$(PRT) SCCS/s.version.c >> Version.c
	@echo "" >> Version.c
	@echo "code versions:" >> Version.c
	@echo "" >> Version.c
	@$(WHAT) $(OBJS) >> Version.c
	@echo "" >> Version.c
	@echo "`#' endif COMMENT" >> Version.c
)dnl

$(OBJS1): sendmail.h
$(OBJS): conf.h

sendmail.h util.o: ../`include'/useful.h

#
#  Auxiliary support entries
#

clean:
	rm -f core sendmail rmail usersmtp uucp a.out XREF sendmail.cf
	rm -f *.o Makefile

print: $(SRCS)
	@ls -l | pr -h "sendmail directory"
	@$(XREF) *.c | pr -h "cross reference listing"
	@${SIZE} *.o | pr -h "object code sizes"
	@pr Makefile *.m4 *.h *.[cs]

lint:
	$(LINT) $(CCONFIG) $(SRCS1)
