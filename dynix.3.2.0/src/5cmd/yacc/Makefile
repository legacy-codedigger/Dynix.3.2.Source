DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= -O $(ATT_INCLUDE) $(DEF) $(CCS) $(DEBUG)
OBJS	= y1.o y2.o y3.o y4.o
CMD	= yacc
TARGET	= $P$N$(CMD)
M	= make $(MFLAGS) $(MRULES)
C	= CCS=-DCCS=1 N=ccs
X	= P=x CC=cc "`cat DEFS`" ATT_INCLUDE=

native		:; $M clean all
nativeccs	:; $M $C clean all
cross		:; $M $X clean all
crossccs	:; $M $X $C clean all
cross crossccs	:  DEFS
everything	:  native nativeccs cross crossccs clean
#################################################################
#  we have to be real careful to not undef something to		#
#  cpp that we may want to define again since it is "sticky"	#
#################################################################
DEFS		:FRC
		  -if [ "$(NATIVE)" != "$(MACHINE)" ]; then \
			echo "DEF=-U$$(NATIVE) -D$$(MACHINE) -Dsequent" >DEFS; \
		  else \
			echo "DEF=" >DEFS ; \
		  fi
all		:  $(TARGET)
$(TARGET)	:& $(OBJS); $(CC) $(CFLAGS) -o $@ $(OBJS); rm -f DEFS
$(OBJS)		: dextern files
install		:; install -s yacc $(DESTDIR)/usr/bin/yacc
	   	   install -c -m 644 yaccpar $(DESTDIR)/usr/lib/yaccpar
clean 		:; rm -f $(OBJS)
clobber		:; rm -f $(OBJS) DEFS *yacc
FRC		:
