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

# $Header: Makefile 1.52 1991/06/13 00:26:45 $
#
.SUFFIXES:
.SUFFIXES:	.3 .3c .3x .3n .3s .3m .3p .3r \
		.Z .Zc .Zx .Zn .Zs .Zm .Zp .Zr \
		.P .Pc .Px .Pn .Ps .Pm .Pp .Pr \
		.C

M	= $(DESTDIR)/usr/man/cat3
D	= $(CRYPT_DESTDIR)/usr/man/cat3
C	= 3
ITPF	= /usr/imagen/bin/itpf -x
IPATH	= $(DESTDIR)/usr/include

# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

# The following macro file has the fonts for the Imagen laser printer
#	and one new macro for displays (.Ps/.Pe).

MACS	= macs
MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

#This line no longer needed- SOS	= getdisk.3x getgrent.3 getpwent.3 time.3c times.3c

SED	= sed -e '/Copyright/,/^$$/ d' -e '/Log/,/^$$/ d' \
	-e 's/[$$]Header/$$\\\&Header/'

CPAGES	= crypt.$C

PAGES	=\
	getpseudotty.$C\
	addutmp.$C abort.$C abs.$C alarm.$Cc assert.$Cx atof.$C brk.$Cp\
	bstring.$C byteorder.$Cn cpus_online.$Cp ctime.$C ctype.$C\
	curses.$Cx dbm.$Cx directory.$C dis.$Cx ecvt.$C end.$C\
	ethers.$Cn execl.$C exit.$C exp.$Cm fclose.$Cs ferror.$Cs\
	floor.$Cm fopen.$Cs fread.$Cs frexp.$C fseek.$Cs gamma.$Cm\
	getc.$Cs getdisk.$Cx getenv.$C getfsent.$Cx getgrent.$C\
	gethostent.$Cn getlogin.$C getmntent.$C getnetent.$Cn\
	getnetgrent.$Cn getopt.$Cx getpass.$C getprotoent.$Cn getpw.$Cc\
	getpwent.$C getrpcent.$Cn getrpcport.$Cr gets.$Cs getservent.$Cn\
	getgeombyname.$C getscsiinfo.$C getzdinfo.$C \
	getusclk.$C getusershell.$C getwd.$C hypot.$Cm inet.$Cn\
	initgroups.$Cx insque.$C\
	intro.$C intro.$Cc intro.$Cm intro.$Cn intro.$Cp intro.$Cr intro.$Cs\
	intro.$Cx j0.$Cm lib2648.$Cx libsanity.$Cx lockf.$Cc m_fork.$Cp\
	m_get_myid.$Cp m_get_numprocs.$Cp m_kill_procs.$Cp m_lock.$Cp\
	m_next.$Cp m_park_procs.$Cp m_set_procs.$Cp m_single.$Cp\
	m_sync.$Cp malloc.$C mktemp.$C monitor.$C mount.$Cr nargs.$Cc\
	nice.$Cc nlist.$C offline_all.$C pause.$Cc perror.$C\
	plot.$Cx popen.$C printf.$Cs psignal.$C putc.$Cs puts.$Cs\
	pwcache.$Cx \
	qsort.$C rand.$Cc random.$C rcmd.$Cx regex.$C resolver.$C\
	rexec.$Cx rnusers.$Cr\
	rpc.$Cn rstat.$Cr rwall.$Cr s_lock.$Cp s_wait_barrier.$Cp scandir.$C\
	scanf.$Cs setbuf.$Cs setjmp.$C setuid.$C shbrk.$Cp shmalloc.$Cp\
	signal.$Cc sin.$Cm sinh.$Cm sleep.$C spray.$Cr string.$C stty.$Cc\
	swab.$C\
	syslog.$C system.$C termcap.$Cx time.$Cc times.$Cc ttyname.$C\
	ungetc.$Cs utime.$Cc valloc.$C varargs.$C vlimit.$Cc vtimes.$Cc\
	xdr.$Cn ypclnt.$Cn yppasswd.$Cr

.3.C	:
	cd ..; umask 111; nroff $(MANMACS) man3/$< | compress > $D/$<.Z
.3.Z .3c.Zc .3x.Zx .3n.Zn .3s.Zs .3m.Zm .3p.Zp .3r.Zr :
	cd ..; umask 111; nroff $(MANMACS) man3/$< | compress > $M/$<.Z
.3.P .3c.Pc .3x.Px .3n.Pn .3s.Ps .3m.Pm .3p.Pp .3r.Pr : 
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:& $(SOS); sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		install -c -m 444 $(CPAGES) $(CRYPT_DESTDIR)/usr/man/man$C/; \
		make $(MFLAGS) $(MRULES) MANMACS="$(MANMACS)" C=C cpages; \
	fi'
	install -c -m 444 $(PAGES) $(DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) MANMACS="$(MANMACS)" C=Z pages

pages	:& $(PAGES)
cpages	:& $(CPAGES)

#Special handling for pages that .so a header file
#getdisk.3x	: getdisk.3x.so; $(SED) < $(IPATH)/disktab.h > disktab.h && \
#		soelim getdisk.3x.so > $@
#getgrent.3	: getgrent.3.so
#		$(SED) < $(IPATH)/grp.h > grp.h && soelim getgrent.3.so > $@
#getpwent.3	: getpwent.3.so
#		$(SED) < $(IPATH)/pwd.h > pwd.h && soelim getpwent.3.so > $@
#time.3c		: time.3c.so; $(SED) < $(IPATH)/sys/timeb.h > timeb.h && \
#		soelim time.3c.so > $@
#times.3c	: times.3c.so; $(SED) < $(IPATH)/sys/times.h > times.h && \
#		soelim times.3c.so > $@

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages
