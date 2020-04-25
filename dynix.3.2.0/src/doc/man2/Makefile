# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.40 1991/05/06 15:58:56 $
#
.SUFFIXES:
.SUFFIXES: .2 .Z .P

M	= $(DESTDIR)/usr/man/cat2
C	= 2
ITPF	= /usr/imagen/bin/itpf -x

# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

PAGES	=\
	accept.$C access.$C acct.$C adjtime.$C bind.$C brk.$C chdir.$C\
	chmod.$C chown.$C chroot.$C close.$C connect.$C creat.$C\
	csymlink.$C dup.$C execve.$C exit.$C fcntl.$C flock.$C fork.$C\
	fsync.$C getdirentries.$C getdomainname.$C getdtablemax.$C\
	getdtablesize.$C getgid.$C getgroups.$C gethostid.$C gethostname.$C\
	getitimer.$C getpagesize.$C getpeername.$C getpgrp.$C getpid.$C\
	getpriority.$C getrlimit.$C getrusage.$C getsockname.$C\
	getsockopt.$C gettimeofday.$C getuid.$C intro.$C ioctl.$C\
	kill.$C killpg.$C link.$C listen.$C lseek.$C mkdir.$C mknod.$C\
	mmap.$C mount.$C msgctl.$C msgget.$C msgop.$C munmap.$C\
	open.$C pipe.$C proc_ctl.$C profil.$C ptrace.$C quotactl.$C\
	read.$C readclink.$C readlink.$C reboot.$C recv.$C rename.$C\
	rmdir.$C select.$C semctl.$C semget.$C semop.$C send.$C\
	setdtablesize.$C setgroups.$C setpgrp.$C setregid.$C setreuid.$C\
	shutdown.$C sigblock.$C sigpause.$C sigsetmask.$C sigstack.$C\
	sigvec.$C socket.$C socketpair.$C stat.$C statfs.$C swapon.$C\
	symlink.$C sync.$C syscall.$C tmp_affinity.$C tmp_ctl.$C truncate.$C\
	umask.$C universe.$C unlink.$C unmount.$C utimes.$C vfork.$C\
	vhangup.$C vm_ctl.$C wait.$C write.$C getvers.$C

.2.Z	:
	cd ..; umask 111; nroff $(MANMACS) man2/$< | compress > $M/$<.Z
.2.P	:
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; install -c -m 444 $(PAGES) $(DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z pages

pages	:& $(PAGES)

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages
