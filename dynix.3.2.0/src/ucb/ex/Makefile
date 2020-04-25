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

# ADA support used by permission of Verdix Corporation
# Given Sept 25, 1985 by Steve Zeigler
# Author: Ben Priest

# $Header: Makefile 2.3 86/05/07 $
#
VERSION=3.7
#
# Ex skeletal makefile for VAX VM/Unix 4.2BSD
#
# NB: This makefile doesn't indicate any dependencies on header files.
#
# Ex is very large - this version will not fit on PDP-11's without overlay
# software.  Things that can be turned off to save
# space include LISPCODE (-l flag, showmatch and lisp options), UCVISUAL
# (visual \ nonsense on upper case only terminals), CHDIR (the undocumented
# chdir command.)  CRYPT includes the code to edit encrypted files (the -x
# option, like ed.)  VMUNIX makes ex considerably larger, raising many limits
# and improving speed and simplicity of maintenance.  It is suitable only
# for a VAX or other large machine, and then probably only in a paged system.
#
# Don't define VFORK unless your system has the VFORK system call,
# which is like fork but the two processes have only one data space until the
# child execs. This speeds up ex by saving the memory copy.
#
# If your system expands tabs to 4 spaces you should -DTABS=4 below
#
BINDIR=	/usr/ucb
NBINDIR=/usr/new
LIBDIR=	/usr/lib
FOLD=	${BINDIR}/fold
CTAGS=	${BINDIR}/ctags
XSTR=	${BINDIR}/xstr
DEBUGFLAGS= -DTRACE -g
NONDEBUGFLAGS= -O
DEB= ${NONDEBUGFLAGS}	# or ${DEBUGFLAGS} to to debug
OPTIONS=-DLISPCODE -DCHDIR -DUCVISUAL -DVFORK -DVMUNIX -DFASTTAG
CFLAGS=	-DTABS=8 ${OPTIONS} ${DEB} ${INCLUDE}
LDFLAGS= 		# -i or -n or -z (default)
TERMLIB=-ltermcap
MKSTR=	${BINDIR}/mkstr
CXREF=	${BINDIR}/cxref
INC=	${DESTDIR}/usr/include
PR=	pr
#

OBJS=	ex_addr.o ex_cmds.o ex_cmds2.o ex_cmdsub.o \
	ex_data.o ex_get.o ex_put.o ex_re.o \
	ex_set.o ex_subr.o ex_tagio.o ex_tty.o ex_unix.o \
	ex_v.o ex_vadj.o ex_vget.o ex_vmain.o ex_voper.o \
	ex_vops.o ex_vops2.o ex_vops3.o ex_vput.o ex_vwind.o \
	printf.o a.vi.iskey.o
NOCRYPT=ex_extern.o ex.o ex_io.o ex_temp.o
CRYPT=	ex_extern.x ex.x ex_io.x ex_temp.x
HEADRS=	ex.h ex_argv.h ex_re.h ex_temp.h ex_tty.h ex_tune.h ex_vars.h ex_vis.h
SRC1=	ex.c ex_addr.c ex_cmds.c ex_cmds2.c ex_cmdsub.c
SRC2=	ex_data.c ex_get.c ex_io.c ex_put.c ex_re.c
SRC3=	ex_set.c ex_subr.c ex_tagio.c ex_temp.c ex_tty.c ex_unix.c
SRC4=	ex_v.c ex_vadj.c ex_vget.c ex_vmain.c ex_voper.c
SRC5=	ex_vops.c ex_vops2.c ex_vops3.c ex_vput.c ex_vwind.c
SRC6=	printf.c bcopy.c expreserve.c exrecover.c
MISC=	makefile READ_ME
VGRIND=	csh /usr/ucb/vgrind
VHDR=	"Ex Version ${VERSION}"

.SUFFIXES: .c .o .x

.c.x:
	${CC} -E ${CFLAGS} -DCRYPT $*.c | ${XSTR} -c -; \
	sed '/rcsid\[\]/d' < x.c > $*.x.c; \
	${CC} ${CFLAGS} -c $*.x.c; \
	mv $*.x.o $*.x; \
	rm x.c $*.x.c

.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -; \
	sed '/rcsid\[\]/d' < x.c > $*.x.c; \
	${CC} ${CFLAGS} -c $*.x.c; \
	mv $*.x.o $*.o; \
	rm x.c $*.x.c

all:	ex exrecover expreserve
	sh -c 'if [ "${EXPORT}" = TRUE ]; then :; else \
		make ${MFLAGS} ${MRULES} ex.crypt; \
	fi'

ex: 	strings.o
	${CC} ${LDFLAGS} -o ex ${OBJS} strings.o ${NOCRYPT} ${TERMLIB}

ex.crypt: strings.x
	${CRYPT_CC} ${LDFLAGS} -o ex.crypt ${OBJS} strings.x ${CRYPT} ${TERMLIB}

tags:	FRC
	${CTAGS} -w ex.[hc] ex_*.[hc]

${OBJS} ${CRYPT} ${NOCRYPT}: ${HEADRS}

# ex_vars.h:
# 	csh makeoptions ${CFLAGS}

strings.x: $(OBJS) $(CRYPT)
	$(XSTR); $(CC) -c -R xs.c; mv xs.o $@

strings.o: $(OBJS) $(NOCRYPT)
	$(XSTR); $(CC) -c -R xs.c; mv xs.o $@

# These do not want to share strings so special rules
exrecover: exrecover.o ex_extern.o
	${CC} ${CFLAGS} -o exrecover exrecover.o ex_extern.o

exrecover.o: exrecover.c
	${CC} ${CFLAGS} -c exrecover.c

expreserve: expreserve.o
	${CC} expreserve.o -o expreserve

expreserve.o:
	${CC} ${CFLAGS} -c expreserve.c

clean:
	rm -f ex ex.crypt exrecover expreserve strings core errs trace
	rm -f *.[xo] x*.[cs] 

# install a new version for testing in /usr/new
ninstall:
	-rm -f ${DESTDIR}${NBINDIR}/ex ${DESTDIR}${NBINDIR}/vi ${DESTDIR}${NBINDIR}/view
	cp ex ${DESTDIR}${NBINDIR}/ex
#	-cp ex${VERSION}strings ${LIBDIR}/ex${VERSION}strings
	ln ${DESTDIR}${NBINDIR}/ex ${DESTDIR}${NBINDIR}/vi
	ln ${DESTDIR}${NBINDIR}/ex ${DESTDIR}${NBINDIR}/view
	chmod 1755 ${DESTDIR}${NBINDIR}/ex

# install in standard place (/usr/ucb)
install:
	rm -f ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/vi \
		${DESTDIR}/${BINDIR}/view ${DESTDIR}/${BINDIR}/edit \
		${DESTDIR}/${BINDIR}/e ${DESTDIR}/${BINDIR}/a.vi \
		${DESTDIR}/usr/bin/ex
	install -s -m 1755 ex ${DESTDIR}/${BINDIR}/ex
#	install -c -m 644 ex${VERSION}strings \
			  ${DESTDIR}/${LIBDIR}/ex${VERSION}strings
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/edit
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/e
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/vi
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/view
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/a.vi
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/usr/bin/ex
	install -s -m 4755 exrecover ${DESTDIR}/${LIBDIR}/ex${VERSION}recover
	install -s -m 4755 expreserve ${DESTDIR}/${LIBDIR}/ex${VERSION}preserve
#	-mkdir ${DESTDIR}/usr/preserve
	sh -c 'if [ "${EXPORT}" = TRUE ]; then :; else \
		rm -f ${CRYPT_DESTDIR}/${BINDIR}/ex \
			${CRYPT_DESTDIR}/${BINDIR}/vi \
			${CRYPT_DESTDIR}/${BINDIR}/view \
			${CRYPT_DESTDIR}/${BINDIR}/edit\
			${CRYPT_DESTDIR}/${BINDIR}/e \
			${CRYPT_DESTDIR}/${BINDIR}/a.vi \
			${CRYPT_DESTDIR}/usr/bin/ex; \
		install -s -m 1755 ex.crypt ${CRYPT_DESTDIR}/${BINDIR}/ex; \
		ln ${CRYPT_DESTDIR}/${BINDIR}/ex ${CRYPT_DESTDIR}/${BINDIR}/edit; \
		ln ${CRYPT_DESTDIR}/${BINDIR}/ex ${CRYPT_DESTDIR}/${BINDIR}/e; \
		ln ${CRYPT_DESTDIR}/${BINDIR}/ex ${CRYPT_DESTDIR}/${BINDIR}/vi; \
		ln ${CRYPT_DESTDIR}/${BINDIR}/ex ${CRYPT_DESTDIR}/${BINDIR}/view; \
		ln ${CRYPT_DESTDIR}/${BINDIR}/ex ${CRYPT_DESTDIR}/${BINDIR}/a.vi; \
		ln ${CRYPT_DESTDIR}/${BINDIR}/ex ${CRYPT_DESTDIR}/usr/bin/ex; \
	fi'

# move from /usr/new to /usr/ucb
newucb: ex
	-rm -f ${DESTDIR}/${BINDIR}/ex
	-rm -f ${DESTDIR}/${BINDIR}/vi
	-rm -f ${DESTDIR}/${BINDIR}/edit
	-rm -f ${DESTDIR}/${BINDIR}/e
	-rm -f ${DESTDIR}/usr/bin/ex
	mv ${DESTDIR}/${NBINDIR}/ex ${DESTDIR}/${NBINDIR}/ex
	-rm -f ${DESTDIR}/${NBINDIR}/vi
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/edit
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/e
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/vi
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/usr/bin/ex
	chmod 1755 ${DESTDIR}/${BINDIR}/ex

lint:
	lint ${CFLAGS} ex.c ex_?*.c
	lint ${CFLAGS} -u exrecover.c
	lint ${CFLAGS} expreserve.c

print:
	@${PR} READ* BUGS
	@${PR} makefile*
	@${PR} /etc/termcap
	@(size -l ex ; size *.o) | ${PR} -h sizes
	@${PR} -h errno.h ${INC}/errno.h
	@${PR} -h setjmp.h ${INC}/setjmp.h
	@${PR} -h sgtty.h ${INC}/sgtty.h
	@${PR} -h signal.h ${INC}/signal.h
	@${PR} -h sys/stat.h ${INC}/sys/stat.h
	@${PR} -h sys/types.h ${INC}/sys/types.h
	@ls -ls | ${PR}
	@${CXREF} *.c | ${PR} -h XREF
	@${PR} *.h *.c
vgrind:
	tee index < /dev/null
	${VGRIND} -h ${VHDR} ${HEADRS}
	${VGRIND} -h ${VHDR} ${SRC1}
	${VGRIND} -h ${VHDR} ${SRC2}
	${VGRIND} -h ${VHDR} ${SRC3}
	${VGRIND} -h ${VHDR} ${SRC4}
	${VGRIND} -h ${VHDR} ${SRC5}
	${VGRIND} -h ${VHDR} ${SRC6}
	${VGRIND} -n -h ${VHDR} ${MISC}
	${VGRIND} -i -h ${VHDR} index

FRC:
