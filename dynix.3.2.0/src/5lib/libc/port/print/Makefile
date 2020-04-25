#	@(#)makefile	1.4
#
# makefile for libc/port/print
#
#

INCLUDE	= $(ATT_INCLUDE)
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)
OBJS	= doprnt.o fprintf.o printf.o sprintf.o vfprintf.o vprintf.o vsprintf.o
LIB	= printlib printlib_p

.c.o:
		  $(CC) -p $(CFLAGS) -c $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC)    $(CFLAGS) -c $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../printlib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru printlib $(OBJS)

parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean clobber	:; rm -rf $(LIB) profiled $(OBJS)
