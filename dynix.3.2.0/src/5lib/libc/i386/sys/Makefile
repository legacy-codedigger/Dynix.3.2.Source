
SRCDIR	= ../../../../lib/libc/$(MACHINE)/sys
SRCHDIR	= ../../../..
UBAR	=
ATTFLG	=
COPTS	= $(UBAR) $(ATTFLG) $(INCLUDE)
CFLAGS	= -O $(COPTS)

##
# SHARED_OBJ is common system calls between DYNIX and System V
##
SHARED_OBJ =\
	_exit.o access.o cerror.o chdir.o chmod.o chroot.o\
	close.o csymlink.o dup.o dup2.o execl.o execle.o exect.o\
	execv.o execve.o fcntl.o fork.o getegid.o geteuid.o getgid.o getpid.o\
	getppid.o getuid.o link.o msgsys.o pipe.o profil.o readclink.o\
	sbrk.o semsys.o sync.o syscall.o umask.o universe.o\
	getdtablesize.o setdtablesize.o getdtablemax.o

SHARED_SRC =\
	_exit.s access.s cerror.s chdir.s chmod.s chroot.s\
	close.s csymlink.s dup.s dup2.s execl.s execle.s exect.s\
	execv.s execve.s fcntl.s fork.s getegid.s geteuid.s getgid.s getpid.s\
	getppid.s getuid.s link.s msgsys.c pipe.s profil.s readclink.s\
	sbrk.c semsys.c sync.s syscall.s umask.s universe.s\
	getdtablesize.s setdtablesize.s getdtablemax.s

##
# UBAR_OBJ are objects from DYNIX that the System V emulation uses
##
UBAR_OBJ =\
	_fchmod.o _fchown.o _flock.o _gethostname.o _getpagesize.o _getpgrp.o\
	_getpriority.o _getrlimit.o _getrusage.o _gettimeofda.o _ioctl.o\
	_kill.o _killpg.o _lseek.o _mmap.o _mount.o _munmap.o _ptrace.o\
	_rename.o\
	_select.o _setitimer.o _setpgrp.o _setpriority.o _setregid.o\
	_setreuid.o _setrlimit.o _settimeofda.o _sigblock.o _sigpause.o\
	_sigsetmask.o _sigvec.o _umount.o _utimes.o _wait.o _write.o _ustat.o\
	_att_mount.o _att_setuid.o

UBAR_SRC =\
	_fchmod.s _fchown.s _flock.s _gethostname.s _getpagesize.s _getpgrp.s\
	_getpriority.s _getrlimit.s _getrusage.s _gettimeofday.s _ioctl.s\
	_kill.s _killpg.s _lseek.s _mmap.s _mount.s _munmap.s _ptrace.s\
	_rename.s\
	_select.s _setitimer.s _setpgrp.s _setpriority.s _setregid.s\
	_setreuid.s _setrlimit.s _settimeofday.s _sigblock.s _sigpause.s\
	_sigsetmask.s _sigvec.s _umount.s _utimes.s _wait.s _write.s _ustat.s\
	_att_mount.s _att_setuid.s

##
# ATT_OBJ are special entry points for System V emulation that differ from DYNIX
##
ATT_OBJ	= acct.o chown.o fstat.o creat.o mknod.o _open.o _read.o stat.o unlink.o
ATT_SRC	= acct.s chown.s fstat.s creat.s mknod.s stat.s unlink.s

LIB	= syslib syslib_p

.s.o:
		$(CPP) -DPROF $(COPTS) $*.s | $(AS) -o $*.o && \
		$(LD) -X -r $*.o -o profiled/$*.o && \
		$(CPP) -UPROF $(COPTS) $*.s | $(AS) -o $*.o && \
		$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled SYS.h src pobjects
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../syslib_p $(SHARED_OBJ) \
		  	$(UBAR_OBJ) $(ATT_OBJ) brk.o _mmmap.o signal.o shmsys.o && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru syslib $(SHARED_OBJ) \
		  	$(UBAR_OBJ) $(ATT_OBJ) brk.o _mmmap.o signal.o shmsys.o

pobjects	:& shared_parallel ubar_objs att_objs brk.o _mmmap.o signal.o shmsys.o
src		:& $(SHARED_SRC) $(UBAR_SRC) $(ATT_SRC)

# Special rule because want '__'name and SYSV syscall number
_read.o		: _read.s
		  $(CPP) -DPROF $(COPTS) -DUBAR=1 $*.s | $(AS) -o $*.o && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CPP) -UPROF $(COPTS) -DUBAR=1 $*.s | $(AS) -o $*.o && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

_open.o		: _open.s
		  $(CPP) -DPROF $(COPTS) -DUBAR=1 $*.s | $(AS) -o $*.o && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CPP) -UPROF $(COPTS) -DUBAR=1 $*.s | $(AS) -o $*.o && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

_mmmap.o	: _mmmap.s

# Real C code so special rule
msgsys.o semsys.o sbrk.o:
		  $(CC) -c -p $(CFLAGS) $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC) -c    $(CFLAGS) $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

shmsys.o	: shmsys.c mman.h file.h
		  $(CC) -c -p -O -I../ $(ATT_INCLUDE) $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC) -c    -O -I../ $(ATT_INCLUDE) $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

brk.o		: brk.c mman.h 
		  $(CC) -c -p -O $(ATT_INCLUDE) $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC) -c    -O $(ATT_INCLUDE) $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

mman.h		: $(SRCHDIR)/sys/$(MACHINE)/h/mman.h
		  rm -f $@ && ln -s $? $@
file.h		: $(SRCHDIR)/sys/$(MACHINE)/h/file.h fcntl.h unistd.h
		  rm -f $@ 
		  ln -s $(SRCHDIR)/sys/$(MACHINE)/h/file.h $@
fcntl.h:	  $(SRCHDIR)/sys/$(MACHINE)/h/fcntl.h 
		  rm -f $@
		  ln -s $(SRCHDIR)/sys/$(MACHINE)/h/fcntl.h $@
unistd.h:	  $(SRCHDIR)/sys/$(MACHINE)/h/unistd.h
		  rm -f $@
		  ln -s $(SRCHDIR)/sys/$(MACHINE)/h/unistd.h $@

# Can't profile because that breaks the emulation code !!!
signal.o	: syscall.s syscall.h
		  $(CC) -c -O -I. $(ATT_INCLUDE) $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC) -c -O -I. $(ATT_INCLUDE) $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o
syscall.h	: $(SRCHDIR)/include/syscall.h; rm -f $@ && ln -s $?

# These files are >14 chars, so ...
_gettimeofda.o	: _gettimeofday.o
_settimeofda.o	: _settimeofday.o
_gettimeofda.o _settimeofda.o:
		  rm -f $@ profiled/$@ && ln -s $? $@ && \
		  cd profiled && ln -s $? $@

att_objs	:; make $(MFLAGS) $(MRULES) ATTFLG='-DATTFLG=1' att_parallel
ubar_objs	:; make $(MFLAGS) $(MRULES) UBAR='-DUBAR=1' ubar_parallel

att_parallel	:& $(ATT_OBJ)
ubar_parallel	:& $(UBAR_OBJ)
shared_parallel	:& $(SHARED_OBJ)
profiled	:; mkdir profiled
$(SHARED_SRC) 	:; rm -f $@ && ln -s $(SRCDIR)/$@
$(ATT_SRC) 	:; rm -f $@ && ln -s $(SRCDIR)/$@
SYS.h		:; rm -f $@ && ln -s $(SRCDIR)/$@
$(UBAR_SRC)	:; rm -f $@ && ln -s $(SRCDIR)/`expr $@ : '_\(.*\)'` $@
_read.s 	:; rm -f $@ && ln -s $(SRCDIR)/`expr $@ : '_\(.*\)'` $@
_open.s		:; rm -f $@ && ln -s $(SRCDIR)/`expr $@ : '_\(.*\)'` $@
clobber clean	:; rm -rf core a.out *.o profiled mman.h \
		  $(SHARED_SRC) $(UBAR_SRC) $(ATT_SRC) \
		  _open.s _read.s syscall.h SYS.h file.h fcntl.h \
		  unistd.h syslib syslib_p
