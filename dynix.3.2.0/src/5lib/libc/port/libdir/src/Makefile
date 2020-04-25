
INCLUDE	= $(ATT_INCLUDE)
CFLAGS	= -O $(INCLUDE)
LIB	= ../libdirlib ../libdirlib_p
OBJS	= closedir.o opendir.o readdir.o seekdir.o telldir.o

.c.o:
		  $(CC) -p $(CFLAGS) -c $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC)    $(CFLAGS) -c $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled objects
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../../libdirlib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru ../libdirlib $(OBJS)

objects		:& $(OBJS)
profiled	:; mkdir profiled
clean clobber	:; rm -rf $(LIB) profiled $(OBJS)
