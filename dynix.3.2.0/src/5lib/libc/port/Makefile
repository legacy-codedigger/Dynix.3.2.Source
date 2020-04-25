#
# Makefile for libc/port
# 
SUBDIRS	= gen print stdio sys libdir
LIB	= portlib portlib_p
FPFLAGS	=

all		: $(LIB)
$(LIB)		: subdirs
		  rm -rf tmp; mkdir tmp
		  for i in $(SUBDIRS); do \
			(cd tmp; $(AR) x ../$$i/$${i}lib); done
		  ls tmp/*.o | sort -t/ +1 > t1
		  $(AR) cru portlib `cat t1`
		  rm -rf tmp; mkdir tmp
		  for i in $(SUBDIRS); do \
			(cd tmp; $(AR) x ../$$i/$${i}lib_p); done
		  ls tmp/*.o | sort -t/ +1 > t1
		  $(AR) cru portlib_p `cat t1`
		  rm -rf tmp t1
subdirs		:& $(SUBDIRS)
$(SUBDIRS)	: FRC; cd $@; make $(MFLAGS) -P4 $(MRULES) FPFLAGS=$(FPFLAGS)
FRC		:
clean clobber	:; for i in $(SUBDIRS); do \
		       (cd $$i; make $(MFLAGS) -P8 $(MRULES) $@); done
		   rm -rf t1 tmp $(LIB)
