#	Makefile -- for libc/port/sys (emulations)

INCLUDE	= $(ATT_INCLUDE)
CFLAGS	= -O $(INCLUDE)
OBJS	=\
	_nap.o alarm.o creat.o getpgrp.o gtty.o ioctl.o kill.o lockf.o lseek.o\
	mount.o nice.o open.o pause.o ptrace.o read.o setgid.o\
	setpgrp.o setuid.o stime.o stty.o time.o times.o ulimit.o umount.o\
	uname.o ustat.o utime.o wait.o write.o plock.o
LIB	= syslib syslib_p

.c.o:
		  $(CC) -p $(CFLAGS) -c $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC)    $(CFLAGS) -c $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../syslib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru syslib $(OBJS)

parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean clobber	:; rm -rf $(LIB) profiled $(OBJS)
