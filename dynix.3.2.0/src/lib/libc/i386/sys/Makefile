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

# $Header: Makefile 2.20 90/06/06 $
#
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)
SED	= sed -f ../rofix.sed

# These are SEQUENT added system calls and routines
SEQOBJS= att_mount.o att_setuid.o csymlink.o getdtablemax.o lockf.o mmap.o \
	 msgsys.o munmap.o offline_all.o proc_ctl.o readclink.o semsys.o \
	 setdtablesize.o tmp_affinity.o tmp_ctl.o universe.o ustat.o vm_ctl.o \
	 BmapCache.o DirectRead.o DirectWrite.o shfork.o shvfork.o \
	 affinitypid.o

# These are system calls added to support VFS and NFS
NFSOBJS= async_daemon.o fstatfs.o getdirentries.o \
	getdomainname.o getfh.o nfssvc.o setdomainname.o statfs.o unmount.o

# These are unsupported in this version of 4.2 BSD
NEVER	= quota.o setquota.o

OBJS	= \
	$(SEQOBJS) _exit.o accept.o access.o acct.o adjtime.o bind.o brk.o\
	cerror.o chdir.o chmod.o chown.o chroot.o close.o connect.o creat.o\
	cust0.o	cust1.o cust2.o cust3.o cust4.o cust5.o cust6.o cust7.o cust8.o\
	cust9.o dup.o dup2.o execl.o execle.o exect.o execv.o execve.o fchmod.o\
	fchown.o fcntl.o flock.o fork.o fstat.o fsync.o ftruncate.o\
	getdtablesize.o getegid.o geteuid.o getgid.o getgroups.o gethostid.o\
	gethostname.o getitimer.o getpagesize.o getpeername.o getpgrp.o\
	getpid.o getppid.o getpriority.o getrlimit.o getrusage.o getsockname.o\
	getsockopt.o gettimeofday.o getuid.o ioctl.o kill.o killpg.o link.o\
	listen.o lseek.o lstat.o mkdir.o mknod.o mount.o open.o pipe.o\
	profil.o ptrace.o quotactl.o read.o readlink.o readv.o reboot.o recv.o\
	recvfrom.o recvmsg.o rename.o rmdir.o sbrk.o select.o send.o sendmsg.o\
	sendto.o setgroups.o sethostid.o sethostname.o setitimer.o setpgrp.o\
	setpriority.o setregid.o setreuid.o setrlimit.o setsockopt.o\
	settimeofday.o shutdown.o sigblock.o sigpause.o sigsetmask.o\
	sigstack.o sigvec.o socket.o socketpair.o stat.o swapon.o symlink.o\
	sync.o syscall.o truncate.o umask.o umount.o unlink.o utimes.o vfork.o\
	vhangup.o wait.o wait3.o write.o writev.o Ovadvise.o $(NFSOBJS)

LIB	= syslib

.s.o:
	$(CPP) -E -DPROF $(INCLUDE) $*.s | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CPP) -E -UPROF $(INCLUDE) $*.s | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o
.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../$(LIB)_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru $(LIB) $(OBJS)

parallel	:& $(OBJS)
$(OBJS)		: SYS.h
profiled	:; mkdir profiled
clean		:; rm -rf $(OBJS) profiled $(LIB) $(LIB)_p
