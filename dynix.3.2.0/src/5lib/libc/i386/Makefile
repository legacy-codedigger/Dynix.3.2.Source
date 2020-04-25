#
# Makefile for libc/i386
# 

ALL	= sys gen
LIB	= $(MACHINE)lib $(MACHINE)lib_p

all		: $(LIB)
$(LIB)		:& $(ALL)
		  rm -rf tmp; mkdir tmp
		  for i in $(ALL); do (cd tmp; $(AR) x ../$$i/$${i}lib); done
		  ls tmp/*.o | sort -t/ +1 > t1
		  $(AR) cru $(MACHINE)lib `cat t1`
		  rm -rf tmp; mkdir tmp
		  for i in $(ALL); do (cd tmp; $(AR) x ../$$i/$${i}lib_p); done
		  ls tmp/*.o | sort -t/ +1 > t1
		  $(AR) cru $(MACHINE)lib_p `cat t1`
		  rm -rf tmp t1

$(ALL)		: FRC; cd $@ && make $(MFLAGS) $(MRULES)
FRC		:
clean clobber	:; for i in $(ALL); do \
		       	  (cd $$i && make $(MFLAGS) -P8 $(MRULES) $@); done
		   rm -rf t1 tmp $(LIB)
