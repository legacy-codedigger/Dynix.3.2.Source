# $Copyright:	$
# Copyright (c) 1984, 1985, 1986 Sequent Computer Systems, Inc.
# All rights reserved
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.2 89/03/12 $

DEST=${DESTDIR}/usr/lib

# tell make to use the common directory, also
VPATH=../common

C=../common
I=/usr/include
S=/usr/include/sys

CO=/usr/local/bin/co
RCSCLEAN=/usr/local/bin/rcsclean
MAKETD=/usr/local/bin/maketd

DEBUG=	-O
CDEFS= 
CFLAGS=	${DEBUG} ${CDEFS} ${INCLUDE}

#
# common sources
#
HDRc=	$C/h00vars.h $C/libpc.h $C/whoami.h

SRCc=	$C/actfile.c $C/addt.c $C/append.c $C/argv.c $C/asrt.c \
	$C/asrts.c $C/atan.c $C/buff.c $C/card.c $C/caserng.c $C/chr.c \
	$C/clck.c $C/cos.c $C/cttot.c $C/date.c $C/defname.c \
	$C/dfdispose.c $C/dispose.c $C/error.c $C/except.c $C/exp.c \
	$C/expo.c $C/fcall.c $C/flush.c $C/fnil.c $C/frtn.c $C/fsav.c \
	$C/get.c $C/getname.c $C/halt.c $C/in.c $C/inct.c $C/iosync.c \
	$C/lino.c $C/llimit.c $C/ln.c $C/max.c $C/mult.c $C/nam.c \
	$C/new.c $C/nil.c $C/pack.c $C/page.c $C/pcexit.c $C/pclose.c \
	$C/pcstart.c $C/perror.c $C/pfclose.c $C/pflush.c $C/pmflush.c \
	$C/pred.c $C/put.c $C/random.c $C/rang4.c $C/read4.c $C/read8.c \
	$C/readc.c $C/reade.c $C/readln.c $C/releq.c $C/relne.c \
	$C/relsge.c $C/relsgt.c $C/relsle.c $C/relslt.c $C/reltge.c \
	$C/reltgt.c $C/reltle.c $C/reltlt.c $C/remove.c $C/reset.c \
	$C/rewrite.c $C/round.c $C/rsng4.c $C/sclck.c $C/seed.c \
	$C/seek.c $C/sin.c $C/sqrt.c $C/stlim.c $C/subsc.c $C/subscz.c \
	$C/subt.c $C/succ.c $C/tell.c $C/teof.c $C/teoln.c $C/time.c \
	$C/trunc.c $C/unit.c $C/unpack.c $C/unsync.c $C/writec.c \
	$C/writef.c $C/writes.c $C/writln.c

ASMc=	$C/_asrt.s $C/_asrts.s $C/_card.s $C/_caserng.s $C/_chr.s \
	$C/_cos.s $C/_exp.s $C/_half.s $C/_halt.s $C/_lino.s \
	$C/_llimit.s $C/_ln.s $C/_max.s $C/_nil.s $C/_rang4.s $C/_sin.s \
	$C/_sqrt.s $C/_stlim.s $C/_subsc.s

#
# common objects
#
OBJc=   actfile.o addt.o append.o argv.o asrt.o asrts.o atan.o buff.o \
	card.o caserng.o chr.o clck.o cos.o cttot.o date.o defname.o \
	dfdispose.o dispose.o error.o except.o exp.o expo.o fcall.o \
	flush.o fnil.o frtn.o fsav.o get.o getname.o halt.o in.o inct.o \
	iosync.o lino.o llimit.o ln.o max.o mult.o nam.o new.o nil.o \
	pack.o page.o pcexit.o pclose.o pcstart.o perror.o pfclose.o \
	pflush.o pmflush.o pred.o put.o random.o rang4.o read4.o \
	read8.o readc.o reade.o readln.o releq.o relne.o relsge.o \
	relsgt.o relsle.o relslt.o reltge.o reltgt.o reltle.o reltlt.o \
	remove.o reset.o rewrite.o round.o rsng4.o sclck.o seed.o \
	seek.o sin.o sqrt.o stlim.o subsc.o subscz.o subt.o succ.o \
	tell.o teof.o teoln.o time.o trunc.o unit.o unpack.o unsync.o \
	writec.o writef.o writes.o writln.o \
	_asrt.o _asrts.o _card.o _caserng.o _chr.o _cos.o _exp.o \
	_half.o _halt.o _lino.o _llimit.o _ln.o _max.o _nil.o _rang4.o \
	_sin.o _sqrt.o _stlim.o _subsc.o

#
# i386 sources
#
ASMl=	_fltsupport.s

HDRl=

SRCl=	

#
# i386 objects
#
OBJl=	_fltsupport.o

ASM=	${ASMc} ${ASMl}
HDR=	${HDRc} ${HDRl}
SRC=	${SRCc} ${SRCl}
OBJ=	${OBJc} ${OBJl}
SOURCE=	Makefile ${HDR} ${ASM} ${SRC}

all:	libpc.a libpc_p.a

.c.o:
	${CC} ${CFLAGS} -c -p $*.c && \
	${LD} -X -r $*.o -o profiled/$*.o && \
	${CC} ${CFLAGS} -c $*.c && \
	${LD} -x -r $*.o -o $*.O && mv $*.O $*.o 

.s.o:
	${AS} -o $*.o $*.s && \
	cp $*.o profiled/$*.o

libpc.a libpc_p.a:& profiled ${OBJ}
	chmod 644 ${OBJ}
	${AR} cru libpc.a ${OBJ}
	${RANLIB} libpc.a
	cd profiled && chmod 644 ${OBJ}
	cd profiled && ${AR} cru ../libpc_p.a ${OBJ}
	${RANLIB} libpc_p.a

profiled:
	mkdir $@

clean: FRC
	rm -fr Makefile.bak libpc.a libpc_p.a *.o profiled/*.o a.out core errs tags profiled

#
# we go to lengths to make sure all the dot o's depend on their source -- this
# makes the dependencies work for the right reasons and also allows RCS to find
# the common sources
#
depend: ${ASM} ${SRC} ${HDR} FRC
	${MAKETD} -a ${CDEFS} ${INCLUDE} ${SRC}
	( \
		echo "/^# DO NOT DELETE THIS LINE/+1a"; \
		echo ${ASMc} | sed \
			-e 's@  *@ @g' \
			-e 's@[^ ]*/\([^ ]*\).s@\1.o:#$$C/\1.s @g' \
		| tr ' #'  '\012 '; \
		echo ${ASMl} | sed \
			-e 's@  *@ @g' \
			-e 's@\([^ /]*\).s@\1.o:#\1.s @g' \
		| tr ' #'  '\012 '; \
		echo "."; \
		echo "w"; \
		echo "q"; \
	) | ed Makefile

install: all FRC
	install -c -m 644 libpc.a ${DEST} && ${RANLIB} ${DEST}/libpc.a
	install -c -m 644 libpc_p.a ${DEST} && ${RANLIB} ${DEST}/libpc_p.a

lint: ${SRC} ${HDR} FRC
	${LINT} -hnx ${CDEFS} ${INCLUDE} ${SRC}

print: source FRC
	lpr -J'libpc source' ${SOURCE}

source: ${SOURCE}

spotless: clean
	${RCSCLEAN} ${SOURCE}

tags: ${SRC} ${HDR}
	${CTAGS} -t ${SRC} ${HDR}

${SOURCE}:
	${CO} $@

FRC:

# DO NOT DELETE THIS LINE - maketd DEPENDS ON IT

_asrt.o: $C/_asrt.s

_asrts.o: $C/_asrts.s

_card.o: $C/_card.s

_caserng.o: $C/_caserng.s

_chr.o: $C/_chr.s

_cos.o: $C/_cos.s

_exp.o: $C/_exp.s

_half.o: $C/_half.s

_halt.o: $C/_halt.s

_lino.o: $C/_lino.s

_llimit.o: $C/_llimit.s

_ln.o: $C/_ln.s

_max.o: $C/_max.s

_nil.o: $C/_nil.s

_rang4.o: $C/_rang4.s

_sin.o: $C/_sin.s

_sqrt.o: $C/_sqrt.s

_stlim.o: $C/_stlim.s

_subsc.o: $C/_subsc.s

_fltsupport.o: _fltsupport.s

actfile.o: $C/actfile.c $C/h00vars.h $C/whoami.h $I/stdio.h

addt.o: $C/addt.c

append.o: $C/append.c $C/h00vars.h $C/whoami.h $I/stdio.h

argv.o: $C/argv.c $C/h00vars.h $C/whoami.h $I/stdio.h

asrt.o: $C/asrt.c

asrts.o: $C/asrts.c

atan.o: $C/atan.c $I/math.h

buff.o: $C/buff.c $C/h00vars.h $C/whoami.h $I/stdio.h

card.o: $C/card.c $C/h00vars.h $C/whoami.h $I/stdio.h

caserng.o: $C/caserng.c

chr.o: $C/chr.c

clck.o: $C/clck.c

cos.o: $C/cos.c $I/math.h

cttot.o: $C/cttot.c $C/h00vars.h $C/whoami.h $I/stdio.h

date.o: $C/date.c

defname.o: $C/defname.c $C/h00vars.h $C/whoami.h $I/stdio.h

dfdispose.o: $C/dfdispose.c $C/h00vars.h $C/libpc.h $C/whoami.h $I/stdio.h

dispose.o: $C/dispose.c $C/h00vars.h $C/whoami.h $I/stdio.h

error.o: $C/error.c $I/signal.h $I/stdio.h

except.o: $C/except.c $I/signal.h

exp.o: $C/exp.c $I/math.h

expo.o: $C/expo.c

fcall.o: $C/fcall.c $C/h00vars.h $C/whoami.h $I/stdio.h

flush.o: $C/flush.c $C/h00vars.h $C/whoami.h $I/stdio.h

fnil.o: $C/fnil.c $C/h00vars.h $C/whoami.h $I/stdio.h

frtn.o: $C/frtn.c $C/h00vars.h $C/whoami.h $I/stdio.h

fsav.o: $C/fsav.c $C/h00vars.h $C/whoami.h $I/stdio.h

get.o: $C/get.c $C/h00vars.h $C/whoami.h $I/stdio.h

getname.o: $C/getname.c $C/h00vars.h $C/libpc.h $C/whoami.h $I/stdio.h

halt.o: $C/h00vars.h $C/halt.c $C/whoami.h $I/stdio.h

in.o: $C/h00vars.h $C/in.c $C/whoami.h $I/stdio.h

inct.o: $C/h00vars.h $C/inct.c $C/whoami.h $I/stdio.h

iosync.o: $C/h00vars.h $C/iosync.c $C/whoami.h $I/stdio.h

lino.o: $C/h00vars.h $C/lino.c $C/whoami.h $I/stdio.h

llimit.o: $C/h00vars.h $C/llimit.c $C/whoami.h $I/stdio.h

ln.o: $C/ln.c $I/math.h

max.o: $C/h00vars.h $C/max.c $C/whoami.h $I/stdio.h

mult.o: $C/mult.c

nam.o: $C/h00vars.h $C/nam.c $C/whoami.h $I/stdio.h

new.o: $C/h00vars.h $C/new.c $C/whoami.h $I/stdio.h

nil.o: $C/h00vars.h $C/nil.c $C/whoami.h $I/stdio.h

pack.o: $C/pack.c

page.o: $C/h00vars.h $C/page.c $C/whoami.h $I/stdio.h

pcexit.o: $C/h00vars.h $C/pcexit.c $C/whoami.h $I/stdio.h

pclose.o: $C/h00vars.h $C/libpc.h $C/pclose.c $C/whoami.h $I/stdio.h

pcstart.o: $C/h00vars.h $C/libpc.h $C/pcstart.c $C/whoami.h $I/signal.h \
	$I/stdio.h

perror.o: $C/perror.c $I/signal.h $I/stdio.h

pfclose.o: $C/h00vars.h $C/libpc.h $C/pfclose.c $C/whoami.h $I/stdio.h

pflush.o: $C/h00vars.h $C/pflush.c $C/whoami.h $I/stdio.h

pmflush.o: $C/h00vars.h $C/pmflush.c $C/whoami.h $I/stdio.h

pred.o: $C/pred.c

put.o: $C/h00vars.h $C/put.c $C/whoami.h $I/stdio.h

random.o: $C/h00vars.h $C/random.c $C/whoami.h $I/stdio.h

rang4.o: $C/rang4.c

read4.o: $C/h00vars.h $C/read4.c $C/whoami.h $I/errno.h $I/stdio.h $S/errno.h

read8.o: $C/h00vars.h $C/read8.c $C/whoami.h $I/errno.h $I/stdio.h $S/errno.h

readc.o: $C/h00vars.h $C/readc.c $C/whoami.h $I/stdio.h

reade.o: $C/h00vars.h $C/reade.c $C/whoami.h $I/stdio.h

readln.o: $C/h00vars.h $C/readln.c $C/whoami.h $I/stdio.h

releq.o: $C/h00vars.h $C/releq.c $C/whoami.h $I/stdio.h

relne.o: $C/h00vars.h $C/relne.c $C/whoami.h $I/stdio.h

relsge.o: $C/h00vars.h $C/relsge.c $C/whoami.h $I/stdio.h

relsgt.o: $C/h00vars.h $C/relsgt.c $C/whoami.h $I/stdio.h

relsle.o: $C/h00vars.h $C/relsle.c $C/whoami.h $I/stdio.h

relslt.o: $C/h00vars.h $C/relslt.c $C/whoami.h $I/stdio.h

reltge.o: $C/h00vars.h $C/reltge.c $C/whoami.h $I/stdio.h

reltgt.o: $C/h00vars.h $C/reltgt.c $C/whoami.h $I/stdio.h

reltle.o: $C/h00vars.h $C/reltle.c $C/whoami.h $I/stdio.h

reltlt.o: $C/h00vars.h $C/reltlt.c $C/whoami.h $I/stdio.h

remove.o: $C/h00vars.h $C/remove.c $C/whoami.h $I/stdio.h

reset.o: $C/h00vars.h $C/reset.c $C/whoami.h $I/stdio.h

rewrite.o: $C/h00vars.h $C/rewrite.c $C/whoami.h $I/stdio.h

round.o: $C/round.c

rsng4.o: $C/rsng4.c

sclck.o: $C/sclck.c

seed.o: $C/h00vars.h $C/seed.c $C/whoami.h $I/stdio.h

seek.o: $C/h00vars.h $C/seek.c $C/whoami.h $I/stdio.h

sin.o: $C/sin.c $I/math.h

sqrt.o: $C/sqrt.c $I/math.h

stlim.o: $C/h00vars.h $C/stlim.c $C/whoami.h $I/stdio.h

subsc.o: $C/subsc.c

subscz.o: $C/subscz.c

subt.o: $C/subt.c

succ.o: $C/succ.c

tell.o: $C/h00vars.h $C/tell.c $C/whoami.h $I/stdio.h

teof.o: $C/h00vars.h $C/teof.c $C/whoami.h $I/stdio.h

teoln.o: $C/h00vars.h $C/teoln.c $C/whoami.h $I/stdio.h

time.o: $C/time.c

trunc.o: $C/trunc.c

unit.o: $C/h00vars.h $C/unit.c $C/whoami.h $I/stdio.h

unpack.o: $C/unpack.c

unsync.o: $C/h00vars.h $C/unsync.c $C/whoami.h $I/stdio.h

writec.o: $C/h00vars.h $C/whoami.h $C/writec.c $I/stdio.h

writef.o: $C/h00vars.h $C/whoami.h $C/writef.c $I/stdio.h

writes.o: $C/h00vars.h $C/whoami.h $C/writes.c $I/stdio.h

writln.o: $C/h00vars.h $C/whoami.h $C/writln.c $I/stdio.h

# *** Do not add anything here - It will go away. ***
