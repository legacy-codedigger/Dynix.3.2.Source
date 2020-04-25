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

# $Header: Makefile 1.30 1991/07/03 16:41:27 $
#
# Build kernels
# install target moves binary kernel
# install.headers target installs sys headers
#
CONFIG	= config
I	= install -c -m 444 $? $@
D	= $(DESTDIR)
S	= $(DESTDIR)/sys
C	= ../common
H_DIRS	= ./h ./machine ./custA ./net ./netinet ./netat ./ufs \
	  $C/netif $C/mbad $C/sec $C/ssm $C/zdc $C/rpc $C/nfs $C/balance \
	  $C/mirror $C/stripe
H_DEST	= $S/DYNIX $S/GENERIC $S/conf $S/custA $S/h $S/machine $S/mbad $S/net \
	  $S/netif $S/netinet $S/netat $S/sec $S/ssm $S/sys $S/zdc $S/ufs  \
	  $S/rpc $S/nfs $S/mirror $S/stripe
FILES	= conf_*.c cust_panics.c controllers.balance devices.balance files \
	  files.balance newvers.sh param.c symbols.raw
NFSCONF = conf_nfs.c conf_vfs.c
NFS	= $(NFS_DESTDIR)
NFSSYS	= $(NFS)/sys
N	= $(NFSSYS)
NFSH_DEST= $N/NFS $N/conf $N/custA $N/h $N/machine $N/mbad $N/net $N/ssm\
	  $N/netif $N/netinet $N/netat $N/sec $N/sys $N/zdc $N/ufs $N/rpc\
	  $N/nfs $N/mirror $N/stripe
MIR	= $(MIRROR_DESTDIR)
MIRSYS	= $(MIR)/sys
M	= $(MIRSYS)
MIRH_DEST= $M/mirror
STR	= $(STRIPE_DESTDIR)
STRSYS	= $(STR)/sys
ST	= $(STRSYS)
STRH_DEST= $(ST)/stripe

all:& dynix mfgdynix nfsdynix

dynix:
	(cd conf && $(CONFIG) -src DYNIX)
	(cd DYNIX; make $(MFLAGS) $(MRULES) all binarylibs)
	(cd DYNIX; make $(MFLAGS) $(MRULES) lint)>>../../../makelog.sys.lint 2>&1
	cp DYNIX/*.o GENERIC
	(cd conf && $(CONFIG) -src GENERIC)
	(cd GENERIC; make $(MFLAGS) $(MRULES) all) 
	(cd GENERIC; make $(MFLAGS) $(MRULES) lint)>>../../../makelog.sys.lint 2>&1

mfgdynix:
nfsdynix:
	if [ ! -d ../../../src.nfs ]; then exit 0; else \
		cd conf ; $(CONFIG) -src NFS; \
		cd ../NFS;\
		make $(MFLAGS) $(MRULES) all binarylibs; \
	        make $(MFLAGS) $(MRULES) lint >>../../../makelog.sys.lint 2>&1 ; \
	fi
clean:
	rm -f GENERIC/* & rm -f DYNIX/* & wait
		   sh -c "if [ ! -d ../../../src.nfs ]; then exit 0; else \
		   		cd NFS && rm -f *; \
			  fi"
install:& install1 install2 install3 install3A install4 install5 install6
		   chmod 644 $(DESTDIR)/sys/conf/OBJSlib.a
		   $(RANLIB) $(DESTDIR)/sys/conf/OBJSlib.a
		   sh -c "if [ ! -d ../../../src.nfs ]; then exit 0; else \
		   		chmod 644 $(NFSSYS)/conf/OBJSlib.a && \
		   		$(RANLIB) $(NFSSYS)/conf/OBJSlib.a; \
			  fi"
install1:
	cd DYNIX && \
		   install -c -m 444 dynix_wd dynix_sd dynix_zd $(DESTDIR)/ && \
		   install -c -m 444 OBJSlib.a $(DESTDIR)/sys/conf && \
		   install -c -s genassym.x $(DESTDIR)/sys/machine/genassym
install2:
	cd GENERIC && install -c -m 444 gendynix $(DESTDIR)/
install3:
	-cd conf && \
		   install -c -m 444 DYNIX GENERIC $(FILES) \
			$(DESTDIR)/sys/conf && \
		   for f in $(NFSCONF); do \
			rm -f N$$f $(DESTDIR)/sys/conf/$$f ; \
			unifdef -UNFS $$f > N$$f ; \
			install -c -m 444 N$$f $(DESTDIR)/sys/conf/$$f ; \
			rm -f N$$f ; \
		   done && \
		   install -c -m 444 Makefile.balance.obj  \
				$(DESTDIR)/sys/conf/Makefile.balance
install3A:
	-cd conf && \
		   rm -f Ncust_syscalls.c && \
		   unifdef -UMFG cust_syscalls.c > Ncust_syscalls.c ; \
		   install -c -m 444 Ncust_syscalls.c  \
			$(DESTDIR)/sys/conf/cust_syscalls.c && \
		   rm -f Ncust_syscalls.c
install4:
	cd machine && install -c -m 444 asm.sed $(DESTDIR)/sys/machine
install5:
	cd custA && install -c -m 444 custA.c $(DESTDIR)/sys/custA
install6:
		   sh -c "if [ ! -d ../../../src.nfs ]; then exit 0; else \
		   (cd conf && \
		   install -c -m 444 NFS $(FILES) $(NFSSYS)/conf && \
		   install -c -m 444 Makefile.balance.obj \
			$(NFSSYS)/conf/Makefile.balance); \
		   sh -c 'cd conf &&  rm -f Ncust_syscalls.c && \
		   unifdef -UMFG cust_syscalls.c > Ncust_syscalls.c; \
		   install -c -m 444 Ncust_syscalls.c  \
			$(NFSSYS)/conf/cust_syscalls.c && \
		   rm -f Ncust_syscalls.c; exit 0'; \
		   (cd NFS && \
		   install -c -m 444 dynix_wd dynix_sd dynix_zd $(NFS)/ && \
		   install -c -m 444 OBJSlib.a $(NFSSYS)/conf && \
		   install -c -s genassym.x $(NFSSYS)/machine/genassym); \
		   (cd machine && \
		   install -c -m 444 asm.sed $(NFSSYS)/machine); \
		   (cd custA && \
		   install -c -m 444 custA.c $(NFSSYS)/custA); \
		   fi"

install.headers:  make_dirs copy_headers
		   -cd $(DESTDIR)/sys && \
		   if [ ! -d $(MACHINE) ]; then \
			mv machine $(MACHINE) && \
			ln -s $(MACHINE) machine; \
		   fi
		   sh -c "if [ ! -d ../../../src.nfs ]; then exit 0; else \
				cd $(NFSSYS) && \
				if [ ! -d $(MACHINE) ]; then \
					mv machine $(MACHINE) && \
					ln -s $(MACHINE) machine; \
				fi; \
				exit 0; \
			  fi"
make_dirs:& $(H_DEST) $(MH_DEST) $(NFSH_DEST) $(MIRH_DEST) $(STRH_DEST)\
			make_arch_dirs \
			make_mfg_dirs
$(H_DEST) \
$(MIRH_DEST) \
$(STRH_DEST) \
$(MH_DEST):
	umask 022 && mkdir $@
$(NFSH_DEST):
	sh -c "if [ ! -d ../../../src.nfs ]; then exit 0; else \
				umask 022 && mkdir $@; \
				exit 0; \
			  fi"
make_arch_dirs:
	-cd $(DESTDIR)/sys && if [ ! -d balance ]; then ln -s machine balance; fi
		   sh -c "if [ ! -d ../../../src.nfs ]; then exit 0; else \
		   		cd $(NFSSYS) && \
				if [ ! -d balance ]; then ln -s machine balance; fi; \
				exit 0; \
			  fi"
copy_headers:FRC
		for i in ${H_DIRS}; do \
			(cd $$i && install -c -m 444 *.h \
			$(DESTDIR)/sys/`expr $$i : '.*/\(.*\)'` \
			); \
		done
		for i in ${H_DIRS}; do \
			if [ -d $(MIRSYS)/`expr $$i : '.*/\(.*\)'` ]; then \
				(cd $$i && install -c -m 444 *.h \
				$(MIRSYS)/`expr $$i : '.*/\(.*\)'` \
				); \
			fi; \
			if [ -d $(STRSYS)/`expr $$i : '.*/\(.*\)'` ]; then \
				(cd $$i && install -c -m 444 *.h \
				$(STRSYS)/`expr $$i : '.*/\(.*\)'` \
				); \
			fi; \
		done
		sh -c 'if [ ! -d ../../../src.nfs ]; then exit 0; else \
			for i in ${H_DIRS}; do \
				(cd $$i && install -c -m 444 *.h \
				$(NFSSYS)/`expr $$i : ".*/\(.*\)"` \
			); done; \
		       fi'
FRC:
make_mfg_dirs:
