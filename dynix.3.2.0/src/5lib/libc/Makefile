
DESTDIR	= $(ATT_DESTDIR)
FPA	=
FPFLAGS	=
ALL	= port $(MACHINE)
LIB	= lib$(FPA)c.a lib$(FPA)c_p.a
FP_OBJS	= port/gen/atof.o \
	  port/gen/drand48.o \
	  port/gen/ecvt.o \
	  port/gen/frexp.o \
	  port/gen/ldexp.o \
	  port/gen/modf.o \
	  port/gen/strtod.o \
	  port/print/doprnt.o \
	  port/stdio/doscan.o

all		: $(MACHINE)all
ns32000all	: lib
i386all		: rm_fp_objs lib
	make $(MFLAGS) $(MRULES) FPA="fpa" FPFLAGS="-f1167" rm_fp_objs lib
lib		: $(MACHINE)/csu $(ALL)
		  rm -rf tmp; mkdir  tmp
		  for i in $(ALL); do (cd tmp; $(AR) x ../$$i/$${i}lib); done
		  ls tmp/*.o | sort -t/ +1 > t1
		  chmod    644 `cat t1`
		  -chgrp daemon `cat t1` > /dev/null 2>&1
		  -chown   root `cat t1` > /dev/null 2>&1
		  $(AR) cru lib$(FPA)c.a `$(LORDER) \`cat t1\` | tsort`
		  $(AR) ma flsbuf.o lib$(FPA)c.a exit.o
		  $(AR) ma exit.o lib$(FPA)c.a fakcu.o
		  rm -f t1 `cat t1`
		  for i in $(ALL); do (cd tmp; $(AR) x ../$$i/$${i}lib_p); done
		  ls tmp/*.o | sort -t/ +1 > t1
		  chmod    644 `cat t1`
		  -chgrp daemon `cat t1` > /dev/null 2>&1
		  -chown  root `cat t1` > /dev/null 2>&1
		  $(AR) cr lib$(FPA)c_p.a `$(LORDER) \`cat t1\` | tsort`
		  $(AR) ma flsbuf.o lib$(FPA)c_p.a exit.o
		  $(AR) ma exit.o lib$(FPA)c_p.a fakcu.o
		  rm -rf t1 tmp

rm_fp_objs	:; rm -f $(FP_OBJS)
$(MACHINE)/csu	: FRC; cd $@; make $(MFLAGS) $(MRULES) FPFLAGS="$(FPFLAGS)"
$(ALL)		: FRC; cd $@; make $(MFLAGS) $(MRULES) FPFLAGS="$(FPFLAGS)"
FRC		:
clean clobber	:; for i in $(MACHINE)/csu $(ALL); do \
		       (cd $$i; make $(MFLAGS) -P8 $(MRULES) $@); done
		   rm -rf t1 tmp *.a
install		: $(MACHINE)install
		  cd $(MACHINE)/csu; make $(MFLAGS) $(MRULES) install

ns32000install	:
		  make $(MFLAGS) $(MRULES) install_template

i386install	:
		  make $(MFLAGS) $(MRULES) check_387 install_template
		  make $(MFLAGS) $(MRULES) FPA="fpa" FPFLAGS="-f1167"  \
			  check_1167 install_template

install_template:
		  install -c -m 644 lib$(FPA)c.a $(DESTDIR)/lib/lib$(FPA)c.a
		  $(RANLIB) $(DESTDIR)/lib/lib$(FPA)c.a
		  -if [ "$(EXPORT)" != TRUE ]; then \
			  install -m 644 lib$(FPA)c.a $(ATT_CRYPT_DESTDIR)/lib/lib$(FPA)c.a; \
			  $(RANLIB) $(ATT_CRYPT_DESTDIR)/lib/lib$(FPA)c.a; \
		  fi
		  install -m 644 lib$(FPA)c_p.a $(DESTDIR)/usr/lib/lib$(FPA)c_p.a
		  $(RANLIB) $(DESTDIR)/usr/lib/lib$(FPA)c_p.a

#
#	Make sure the libraries are pure (only 387 or only 1167 code)
#
check_387	:
	$(NM) $(LIB) | awk '/_1167_flt/ { print; exit 1 } END { exit 0 }'
check_1167	:
	$(NM) $(LIB) | awk '/_387_flt/ { print; exit 1 } END { exit 0 }'
