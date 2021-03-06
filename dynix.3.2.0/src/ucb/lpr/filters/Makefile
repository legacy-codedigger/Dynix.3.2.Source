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
# makefile for line printer filters and associated programs
#

CFLAGS=	-O ${INCLUDE}
LIBDIR=	/usr/lib
CXREF=	/usr/ucb/ctags -x
PRINTER=lpf necf
VARIAN=	rvsort rvcat vsort vcat vpf vpsf vdmp vpltdmp vplotf vfontinfo vwidth \
	vfw fcvt rotate rotprt

all:	printer varian

printer:& 	${PRINTER}

varian:&	${VARIAN}

lpf:	lpf.c
	${CC} -o lpf ${CFLAGS} lpf.c

necf:	necf.c
	${CC} -o necf ${CFLAGS} necf.c

rvsort:	rvsort.c
	${CC} -o rvsort ${CFLAGS} rvsort.c

rvcat:	rvcat.c
	${CC} -o rvcat ${CFLAGS} rvcat.c

vsort:	vsort.c
	${CC} -o vsort ${CFLAGS} vsort.c

vcat:	vcat.c
	${CC} -o vcat ${CFLAGS} vcat.c

vpf:	vpf.c
	${CC} -o vpf ${CFLAGS} vpf.c

vpsf:	vpsf.c
	${CC} -o vpsf ${CFLAGS} vpsf.c

vdmp: vdmp.c
	${CC} -o vdmp ${CFLAGS} vdmp.c

vpltdmp: vpltdmp.c
	${CC} -o vpltdmp ${CFLAGS} vpltdmp.c

vplotf: vplotf.c chrtab.o
	${CC} -o vplotf ${CFLAGS} vplotf.c chrtab.o

vfontinfo: vfontinfo.c
	${CC} -o vfontinfo ${CFLAGS} vfontinfo.c

vwidth: vwidth.c
	${CC} -o vwidth ${CFLAGS} vwidth.c

vfw: vfw.c
	${CC} -o vfw ${CFLAGS} vfw.c

fcvt: fcvt.c
	${CC} -o fcvt ${CFLAGS} fcvt.c

rotate: rotate.c
	${CC} -o rotate ${CFLAGS} rotate.c

rotprt: rotprt.c
	${CC} -o rotprt ${CFLAGS} rotprt.c

install:
	install -s lpf ${DESTDIR}/${LIBDIR}/lpf
	install -s necf ${DESTDIR}/${LIBDIR}/necf

installv:
	install -s vfontinfo ${DESTDIR}/${LIBDIR}/vfontinfo
	install -s vwidth ${DESTDIR}/${LIBDIR}/vwidth
	install -s vfw ${DESTDIR}/${LIBDIR}/vfw
	install -s fcvt ${DESTDIR}/${LIBDIR}/fcvt
	install -s rotate ${DESTDIR}/${LIBDIR}/rotate
	install -s rotprt ${DESTDIR}/${LIBDIR}/rotprt
	install -s rvsort ${DESTDIR}/${LIBDIR}/rvsort
	install -s rvcat ${DESTDIR}/${LIBDIR}/rvcat
	install -s vsort ${DESTDIR}/${LIBDIR}/vsort
	install -s vcat ${DESTDIR}/${LIBDIR}/vcat
	rm -f ${DESTDIR}/${LIBDIR}/vpfW
	install -s vpf ${DESTDIR}/${LIBDIR}/vpf
	ln ${DESTDIR}/${LIBDIR}/vpf ${DESTDIR}/${LIBDIR}/vpfW
	rm -f ${DESTDIR}/${LIBDIR}/vpsfW
	install -s vpsf ${DESTDIR}/${LIBDIR}/vpsf
	ln ${DESTDIR}/${LIBDIR}/vpsf ${DESTDIR}/${LIBDIR}/vpsfW
	install -s vdmp ${DESTDIR}/${LIBDIR}/vdmp
	install -s vpltdmp ${DESTDIR}/${LIBDIR}/vpltdmp
	install -s vplotf ${DESTDIR}/${LIBDIR}/vplotf

clean:
	rm -f ${PRINTER} ${VARIAN} *.o

print:
	@pr makefile
	@${CXREF} *.c | pr -h XREF
	@pr *.h *.c
