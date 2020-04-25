# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.3 1991/04/15 21:34:52 $

BIN=	${DESTDIR}/usr/bin

S=/usr/include/sys
I=/usr/include
A=../src
B=.

CDEFS= -DRMOTHERS
DEBUG=	-O
CFLAGS= -w ${DEBUG} ${CDEFS} -I${A} -I. $(INCLUDE)

EYACC=	/usr/ucb/eyacc

HDR = $(A)/0.h whoami.h $(A)/tree.h 0.h $(A)/yy.h \ $(A)/tree_ty.h

# Files which are specific to PXP

LOCSRC = Version.c call.c case.c const.c cset.c error.c fdec.c func.c \
	 lab.c lval.c main.c pmon.c pp.c proc.c rec.c rval.c stat.c \
	 type.c var.c yycomm.c yyerror.c yymain.c yyput.c rmothers.c version.c

LOCOBJ = Version.o call.o case.o const.o cset.o error.o fdec.o func.o \
	 lab.o lval.o main.o pmon.o pp.o proc.o rec.o rval.o stat.o \
	 type.o var.o y.tab.o yycomm.o yyerror.o yymain.o yyput.o rmothers.o

SRCUTIL = $(A)/pas.y

SRCOBJ = TRdata.o hash.o nl.o string.o subr.o tree.o treen.o yycopy.o \
	 yycosts.o yyget.o yyid.o yylex.o yypanic.o yyparse.o yyprint.o \
	 yyrecover.o yyseman.o yytree.o


all:	y.tab.h opcode.h pxp

pxp:$P	$(LOCOBJ) $(SRCOBJ)
	$(CC) $(CFLAGS) -o pxp $(LOCOBJ) $(SRCOBJ)

srcobj: ${SRCOBJ}

${SRCOBJ}:
	FILE=`basename $@ .o`.c ; export FILE;\
	ln -s $(A)/$$FILE . && $(CC) $(CFLAGS) -c $$FILE && rm -f $$FILE

y.tab.h y.tab.c: $(A)/pas.y
	$(EYACC) $(A)/pas.y >/dev/null
	-ex - y.tab.c < $(A)/gram

newversion: version
	./version > Version.c

version: version.c Version.o
	${CC} -o version version.c Version.o

opcode.h: $(A)/OPnames.h $(A)/opc.c
	rm -f opcode.h
	cc ${A}/opc.c -o opc
	./opc >opcode.h
	rm -f opc

clean:	FRC
	-rm -f a.out pxp core errs version *.bak
	-rm -f *.o x* y.tab.c y.tab.h yacc.acts yacc.tmp opcode.h
	-for FILE in ${SRCOBJ} ; do rm -f `basename $$FILE .o`.c ; done

install: all
	install -s pxp $(DESTDIR)/usr/ucb

FRC:

# DO NOT DELETE THIS LINE - maketd DEPENDS ON IT

Version.o: Version.c

call.o: $A/tree.h $I/stdio.h $S/types.h 0.h call.c

case.o: $A/tree.h $I/stdio.h $S/types.h 0.h case.c

const.o: $A/tree.h $I/stdio.h $S/types.h 0.h const.c

cset.o: $A/tree.h $I/stdio.h $S/types.h 0.h cset.c

error.o: $A/yy.h $I/stdio.h $S/types.h 0.h error.c whoami.h y.tab.h

fdec.o: $A/tree.h $I/stdio.h $S/types.h 0.h fdec.c

func.o: $A/tree.h $I/stdio.h $S/types.h 0.h func.c

lab.o: $I/stdio.h $S/types.h 0.h lab.c

lval.o: $A/tree.h $I/stdio.h $S/types.h 0.h lval.c

main.o: $I/stdio.h $S/types.h 0.h main.c

pmon.o: $I/stdio.h $S/types.h 0.h pmon.c

pp.o: $I/stdio.h $S/types.h 0.h pp.c

proc.o: $A/tree.h $I/stdio.h $S/types.h 0.h proc.c

rec.o: $I/stdio.h $S/types.h 0.h rec.c

rmothers.o: rmothers.c

rval.o: $A/tree.h $I/stdio.h $S/types.h 0.h rval.c

stat.o: $A/tree.h $I/stdio.h $S/types.h 0.h stat.c

type.o: $A/tree.h $I/stdio.h $S/types.h 0.h type.c

var.o: $A/tree.h $I/stdio.h $S/types.h 0.h var.c

version.o: $I/stdio.h $S/time.h version.c

y.tab.o: $A/tree.h $A/tree_ty.h $A/yy.h $I/stdio.h $S/types.h 0.h whoami.h \
	y.tab.c y.tab.h

yycomm.o: $A/yy.h $I/stdio.h $S/types.h 0.h whoami.h y.tab.h yycomm.c

yyerror.o: $A/yy.h $I/stdio.h $S/types.h 0.h whoami.h y.tab.h yyerror.c

yymain.o: $A/yy.h $I/stdio.h $S/types.h 0.h whoami.h y.tab.h yymain.c

yyput.o: $A/tree.h $A/yy.h $I/stdio.h $S/types.h 0.h whoami.h y.tab.h yyput.c

# *** Do not add anything here - It will go away. ***
