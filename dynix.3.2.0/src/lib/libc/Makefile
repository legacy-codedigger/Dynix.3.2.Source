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

# $Header: /usr/src/dynix.3.2.0/src/lib/libc/RCS/Makefile,v 1.1 1993/01/19 07:53:26 bruce Exp $
#
# Machine dependent routines are located in a subtree which parallels
# the top directories.  This subtree is identified by the machine name.
#
# Compatibility routines are kept in directories with a prefixing
# ``compat'' (so they all sort together).
# 
# The C run-time startup code is always machine dependent and expected
# to be located in $(MACHINE)/csu
#
FPA	=
FPFLAGS =
COMPAT	= compat-4.1 compat-sys5
COMPATD	= compat-4.1.dir compat-sys5.dir
LIB	= lib$(FPA)c.a lib$(FPA)c_p.a lib$(FPA)crypt.a lib$(FPA)crypt_p.a
ALL	= $(COMPAT) gen inet net stdio rpc yp $(MACHINE)
ALLDIR	= $(COMPATD) gen.dir inet.dir net.dir stdio.dir \
	  rpc.dir yp.dir $(MACHINE).dir
FP_OBJS	= $(MACHINE)/gen/fabs.o \
	  $(MACHINE)/gen/frexp.o \
	  $(MACHINE)/gen/modf.o \
	  $(MACHINE)/gen/atof.o \
	  $(MACHINE)/gen/ldexp.o \
	  $(MACHINE)/gen/ecvt.o \
	  $(MACHINE)/stdio/doprnt.o \
	  stdio/doscan.o

all 		: $(MACHINE)all
ns32000all	: lib
i386all		: rm_fp_objs lib
	make $(MFLAGS) $(MRULES) FPA="fpa" FPFLAGS="-f1167" rm_fp_objs lib
lib		: pbuild
	rm -rf tmp; mkdir tmp
	for i in $(ALL); do (cd tmp; $(AR) x ../$$i/$${i}lib); done
	ls tmp/*.o | sort -t/ +1 > t1
	chmod    644 `cat t1`
	-chgrp daemon `cat t1` > /dev/null 2>&1
	-/etc/chown  root `cat t1` > /dev/null 2>&1
	$(AR) cr lib$(FPA)c.a `$(LORDER) \`cat t1\` | tsort`
	$(AR) ma flsbuf.o lib$(FPA)c.a exit.o
	$(AR) ma exit.o lib$(FPA)c.a fakcu.o
	rm -f t1 `cat t1`
	for i in $(ALL); do (cd tmp; $(AR) x ../$$i/$${i}lib_p); done
	ls tmp/*.o | sort -t/ +1 > t1
	chmod    644 `cat t1`
	-chgrp daemon `cat t1` > /dev/null 2>&1
	-/etc/chown  root `cat t1` > /dev/null 2>&1
	$(AR) cr lib$(FPA)c_p.a `$(LORDER) \`cat t1\` | tsort`
	$(AR) ma flsbuf.o lib$(FPA)c_p.a exit.o
	$(AR) ma exit.o lib$(FPA)c_p.a fakcu.o
	rm -rf t1 tmp
	make $(MFLAGS) $(MRULES) FPA="$(FPA)" $(EXPORT)exportlib

TRUEexportlib	:
exportlib	:
	cp lib$(FPA)c.a lib$(FPA)crypt.a
	cp lib$(FPA)c_p.a lib$(FPA)crypt_p.a
	cp gen/DEScrypt.o crypt.o
	chmod 644 crypt.o
	-chgrp daemon crypt.o
	-/etc/chown root crypt.o > /dev/null 2>&1
	$(AR) rv lib$(FPA)crypt.a crypt.o
	cp gen/profiled/DEScrypt.o crypt.o
	chmod 644 crypt.o
	-chgrp daemon crypt.o
	-/etc/chown root crypt.o > /dev/null 2>&1
	$(AR) rv lib$(FPA)crypt_p.a crypt.o
	rm -f crypt.o

rm_fp_objs		:; rm -f $(FP_OBJS)
pbuild			:& $(MACHINE)/csu.dir $(ALLDIR)
$(MACHINE)/csu.dir	:; cd $(MACHINE)/`basename $@ .dir` && \
			   make $(MFLAGS) -P3 $(MRULES) FPFLAGS="$(FPFLAGS)"
$(ALLDIR)		:; cd `basename $@ .dir` && \
			   make $(MFLAGS) -P3 $(MRULES) FPFLAGS="$(FPFLAGS)"
clean			:; for i in $(MACHINE)/csu $(ALL); \
			     do (cd $$i; make $(MFLAGS) $(MRULES) clean); done
			   rm -rf t1 tmp *.a

install			: $(MACHINE)install
	cd $(MACHINE)/csu && make $(MFLAGS) $(MRULES) install
install_template	:
	install -m 644 lib$(FPA)c.a $(DESTDIR)/lib
	$(RANLIB) $(DESTDIR)/lib/lib$(FPA)c.a
	install -m 644 lib$(FPA)c_p.a $(DESTDIR)/usr/lib
	$(RANLIB) $(DESTDIR)/usr/lib/lib$(FPA)c_p.a
	make $(MRULES) $(MFLAGS) FPA="$(FPA)" $(EXPORT)installexport

TRUEinstallexport	:
installexport		:
	  install -m 644 lib$(FPA)crypt.a $(CRYPT_DESTDIR)/lib/lib$(FPA)c.a
	  $(RANLIB) $(CRYPT_DESTDIR)/lib/lib$(FPA)c.a
	  install -m 644 lib$(FPA)crypt_p.a $(CRYPT_DESTDIR)/usr/lib/lib$(FPA)c_p.a
	  $(RANLIB) $(CRYPT_DESTDIR)/usr/lib/lib$(FPA)c_p.a

ns32000install:
	make $(MFLAGS) $(MRULES) install_template

i386install:
	make $(MFLAGS) $(MRULES) check_387 install_template
	make $(MFLAGS) $(MRULES) FPA="fpa" FPFLAGS="-f1167" \
				 check_1167 install_template

#
#  Check the see libraries are pure (only 387 or only 1167 code)
#
check_387:
	$(NM) $(LIB) | awk '/_1167_flt/ { print; exit 1 } END { exit 0 }'
check_1167:
	$(NM) $(LIB) | awk '/_387_flt/  { print; exit 1 } END { exit 0 }'
