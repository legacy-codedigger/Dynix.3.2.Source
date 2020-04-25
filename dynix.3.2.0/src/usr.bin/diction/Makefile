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

# $Header: Makefile 2.0 86/01/28 $
#
#	make file for STYLE and DICTION
#
#	BIN is the directory for storing the object code
#		for the 3 programs that make up style
#	DICT is the full pathname of the file dict.d, the
#		dictionary file used by DICTION
#
CFLAGS=	-O ${INCLUDE}
#

BIN= 	${DESTDIR}/usr/lib
DICT=	-DDICT=\"/usr/lib/dict.d\"

all:	diction style

style:	style1 style2 style3

install:
	install -s style1 ${DESTDIR}/usr/lib
	install -s style2 ${DESTDIR}/usr/lib
	install -s style3 ${DESTDIR}/usr/lib
	install -s dprog  ${DESTDIR}/usr/lib
	install -c style.sh   ${DESTDIR}/usr/bin/style
	install -c diction.sh ${DESTDIR}/usr/bin/diction
	install -c explain.sh ${DESTDIR}/usr/bin/explain
	install -c dict.d     ${DESTDIR}/usr/lib
	install -c explain.d  ${DESTDIR}/usr/lib

clean:
	rm -f prtlex.c wdslex.c endlex.c
	rm -f *.o style1 style2 style3 dprog

style3: prtlex.o pscan.o outp.o
	${CC} -o style3 prtlex.o pscan.o outp.o -ll

prtlex.c: part.l
	lex part.l
	mv lex.yy.c prtlex.c

prtlex.o:	names.h prtlex.c conp.h style.h names.h
pscan.o:	names.h conp.h pscan.c
outp.o:		names.h conp.h style.h outp.c
wdslex.o:	wdslex.c nhash.c dict.c ydict.c names.h abbrev.c
endlex.o:	names.h endlex.c ehash.c edict.c

style1: wdslex.o
	${CC} -o style1 wdslex.o -ll

wdslex.c: nwords.l
	lex nwords.l
	mv lex.yy.c wdslex.c

style2: endlex.o
	${CC} -o style2 endlex.o -ll

endlex.c: end.l
	lex end.l
	mv lex.yy.c endlex.c

diction: dprog
dprog:	diction.c
	${CC} ${CFLAGS} ${DICT} diction.c -o dprog
