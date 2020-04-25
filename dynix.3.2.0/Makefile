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

# $Header: Makefile 2.64 1991/11/12 01:16:31 $

# For a complete build of entire system, type
# "make build MACHINE=machine VERSION=n.n"
# or optionally, for a complete build with the intention of making a tape, type
# "make build MACHINE=machine VERSION=n.n TMPOS_ROOT=zd1a \
#   TMPOS_USR=zd1h ROOT_DISKTYPE=m2333k USR_DISKTYPE=m2333k"
# and then to actually make the distribution tape, type
# "make tape MACHINE=machine MINIROOT=zd1b TMPOS_ROOT=zd1a TMPOS_USR=zd1h"

# Note: any partition on any disk can be used for MINIROOT, TMPOS_ROOT,
#       and TMPOS_USR.  These no longer always have to be the
#	b, a, and h partitions.

# PARALLEL limits

PLOW=4
PMED=6
PHIGH=8

# directories to be cleaned

CLEAN= src.k src.nfs.k tools.src.k compilers.k

# files which must be updated with the new DYNIX version number

VERSIONFILES=	\
		src/sys/$(MACHINE)/conf/newvers.sh \
		src/etc/version/version.c \
		src/dist/proto.sh \
		src/5cmd/.adm/issue.sh

# tools used on the native machine. (dynix3)
PSTAT=/etc/pstat
AWK=awk
PWD=/bin/pwd
DF=/bin/df
NMAKE=/bin/make
NINSTALL=/usr/bin/install
WHOAMI=/usr/ucb/whoami
# tools used on the native machine. (ptx)
#PSTAT=/etc/pstat
#AWK=awk
#PWD=/bin/pwd
#DF=/bin/df
#NMAKE=/bin/make
#NINSTALL=/usr/bin/install
#WHOAMI=/usr/bin/logname
#
# other tools assumed native
# echo
# csh,unsetenv,setenv,set
# sh,set,cd
# exit
# time
# date
# rm
# install
# make
# cp
# mount
# umount


error:
	@echo "Sorry, this makefile not meant to be invoked without target"
	@exit 1;

local_build: toolprep
	@rm -f makelog*
	@(eval `(set | sed -e 's/=.*/=/')>/dev/null 2>&1` ;\
	date; \
	umask 022; \
	time /ccsbin/ccsmake -M $(MACHINE) $(MFLAGS) \
		TMPOS_ROOT=$(TMPOS_ROOT) \
		TMPOS_USR=$(TMPOS_USR) \
		ROOT_DISKTYPE=$(ROOT_DISKTYPE) \
		USR_DISKTYPE=$(USR_DISKTYPE) \
		-V `$(PWD)` VERSION=$(VERSION) all ) >makelog 2>&1
	echo ----------------------------- 
	echo Dont forget to run make clean 
	echo -----------------------------' 

build: buildcheck toolprep
	@exec csh -c '\
	date && umask 022 && \
	set a = ( `printenv | sed -e "/^USER=/d" -e "s/=.*//"` ) && \
	unsetenv $$a && \
	setenv PATH "" && \
	time /ccsbin/ccsmake -M $(MACHINE) $(MFLAGS) \
	TMPOS_ROOT=$(TMPOS_ROOT) \
	TMPOS_USR=$(TMPOS_USR) \
	ROOT_DISKTYPE=$(ROOT_DISKTYPE) \
	USR_DISKTYPE=$(USR_DISKTYPE) \
	-V `$(PWD)` VERSION=$(VERSION) all && \
	echo ----------------------------- && \
	echo Dont forget to run make clean && \
	echo -----------------------------' >> makelog 2>&1

buildcheck:
	@sh -c 'if [ "root" != `$(WHOAMI)` ]; then \
		echo error: you are not root!; \
		echo usage: make build MACHINE=machine VERSION=n.n; \
		exit 1; \
	else \
		exit 0; \
	fi' >> makelog 2>&1
	@sh -c 'if [ "$(VERSION)" = "" ]; then \
		echo error: VERSION omitted!; \
		echo usage: make build MACHINE=machine VERSION=n.n; \
		exit 1; \
	else \
		exit 0; \
	fi' >> makelog 2>&1
	@sh -c 'if [ "$(MACHINE)" = "" ]; then \
		echo error: MACHINE omitted!; \
		echo usage: make build MACHINE=machine VERSION=n.n; \
		exit 1; \
	else \
		exit 0; \
	fi' >> makelog 2>&1
	@sh -c 'if [ "$(DISK)" != "" ]; then \
		echo -n "error: DISK is no longer supported.  Use" ; \
		echo " MINIROOT, TMPOS_USR, and TMPOS_ROOT. "; \
		exit 1; \
	else \
		exit 0; \
	fi' >> makelog 2>&1
	@sh -c 'if [ "$(DISKTYPE)" != "" ]; then \
		echo -n "error: DISKTYPE is no longer supported.  Use" ; \
		echo " USR_DISKTYPE and ROOT_DISKTYPE. "; \
		exit 1; \
	else \
		exit 0; \
	fi' >> makelog 2>&1
	@sh -c 'if [ "$(TMPOS_ROOT)" != "" -o "$(TMPOS_USR)" != "" ]; then \
			if [ "$(TMPOS_ROOT)" = "" -o "$(TMPOS_USR)" = "" ]; then \
		echo "error: if one of TMPOS_USR or TMPOS_ROOT"; \
		echo " is specified, then both must be given"; \
				exit 1; \
			else \
				exit 0; \
			fi; \
		exit 0; \
	else \
		exit 0; \
	fi' >> makelog 2>&1
	@sh -c "if [ \"$(TMPOS_ROOT)\" = \"\" ]; then \
		$(DF) . | $(AWK) 'NR == 2 \
		{ if (\$$4 < 40000) { \
			print \"There is not enough space in \" \$$6 \" for a build\"; \
			exit 1; \
		  } \
		  exit 0; \
		}'; \
	else \
		exit 0; \
	fi" >> makelog 2>&1
	@$(PSTAT) -T | sed 's;.*/;;' > build.pstat
	@$(AWK) 'BEGIN { s = "is too small for a build"; } \
	/mfiles/	{ if ($$1 <   200) { printf "DYNIX mfile table %s (%d < 500)\n", s, $$1; exit 1; } next; } \
	/files/		{ if ($$1 <   500) { printf "DYNIX file table %s (%d < 500)\n", s, $$1; exit 1; } next; } \
	/inodes/	{ if ($$1 <   250) { printf "DYNIX inode table %s (%d < 250)\n", s, $$1; exit 1; } next; } \
	/processes/	{ if ($$1 <   400) { printf "DYNIX proc table %s (%d < 400)\n", s, $$1; exit 1; } next; } \
	/swap/		{ if ($$1 < 80000) { printf "DYNIX swap space %s (%d < 80000)\n", s, $$1; exit 1; } next; } \
	' build.pstat >> makelog 2>&1
	@rm -f build.pstat

toolprep:
	@sh -c '\
	if [ "$(TMPOS_USR)" != ""  -a "$(TMPOS_ROOT)" != "" ]; then \
		/etc/umount /dev/$(TMPOS_USR); \
		/etc/umount /dev/$(TMPOS_ROOT); \
	fi; \
	cd tools.src && make hostarchvers && cd .. && \
	A_V=`tools.src/hostarchvers` && \
	rm -fr .ccsmark $(MACHINE) && \
	umask 022 && mkdir $(MACHINE) $(MACHINE)/tools $(MACHINE)/tools/$$A_V $(MACHINE)/tools/$$A_V/lib && \
	cp  $(NMAKE) $(MACHINE)/tools/$$A_V/make && \
	cp  $(NINSTALL) $(MACHINE)/tools/$$A_V/install && \
	cp  tools.src/makerules.src $(MACHINE)/tools/$$A_V/makerules && \
	chmod 644 $(MACHINE)/tools/$$A_V/makerules' >> makelog 2>&1

all: version dist.i headers tools dynix & system5 & documentation whatis & dynix.nfs & doc.nfs & disttools

dist.i	\
headers	\
libraries	:; cd src && make     $(MFLAGS) $(MRULES) $@
		   sh -c "if [ ! -d src.nfs ]; then exit 0; else \
		   		cd src.nfs && make $(MFLAGS) $(MRULES) `basename $@ .i`; \
			  fi"

whatis		:; cd src/doc && make $(MFLAGS) $(MRULES) whatis >> ../../makelog.doc 2>&1

dynix		:; -cd src && make -P$(PMED) $(MFLAGS) $(MRULES) dynix.libraries && \
			make -P$(PLOW) $(MFLAGS) $(MRULES) dynix.cmds 
dynix.nfs	:; sh -c "if [ ! -d src.nfs ]; then exit 0; else \
				cd src.nfs && make -P$(PLOW) $(MFLAGS) $(MRULES) nfs.cmds; \
			  fi"

system5		:; -cd src && make -P$(PHIGH) $(MFLAGS) $(MRULES) 5lib && \
			make -P$(PLOW) $(MFLAGS) $(MRULES) 5cmd.i

documentation	:; -cd src/usr.bin/troff && make $(MFLAGS) $(MRULES) tmac && \
		   	cd  ../.. && make -P$(PMED) $(MFLAGS) $(MRULES) doc.i

doc.nfs		:; sh -c "if [ ! -d src.nfs ]; then exit 0; else \
				cd src.nfs && make $(MFLAGS) $(MRULES) doc.i; \
			  fi"

# see "src/Makefile" target "dynix.cmds"

compilersandtools:; cd compilers && \
			time make -P$(PLOW) $(MFLAGS) $(MRULES) all install  >>../makelog.compilers 2>&1

tools		:; cd tools.src && \
			time make $(MFLAGS) $(MRULES) INSTALL clean >>../makelog.tools 2>&1

disttools	:; cd tools.src && \
			time make $(MFLAGS) $(MRULES) TINSTALL clean >>../makelog.tools 2>&1

## Update the version numbers.

version:
	@echo VERSION=$(VERSION); test $(VERSION)
	@for file in $(VERSIONFILES); do \
		echo ...updating $$file; \
		rm -f $$file.old; \
		cp $$file $$file.old; \
		chmod +w $$file; \
		sed \
		    -e "s/DYNIX(R) \$${VERSION}/DYNIX(R) V$(VERSION)/" \
		    -e "s/DYNIX(R) *V[^ ]*/DYNIX(R) V$(VERSION)/" \
		    -e "s/DYNIX(R)\(  *[^V]\)/DYNIX(R) V$(VERSION)\1/" \
		    -e "s/- DYNIX\(  *\)V[^ ]*/- DYNIX\1V$(VERSION)/" \
		    -e "s/- DYNIX\(  *[^V]\)/- DYNIX V$(VERSION)\1/" \
		    $$file.old >$$file; \
		rm -f $$file.old; \
	done

## Build from the source distribution

srcbuild	: headers dynix & system5 & documentation whatis & dynix.nfs & doc.nfs &

##  Set up headers and libraries in a source distribution.

srcdist		:; cd src && \
			make $(MFLAGS) $(MRULES) dist.i headers libraries

clean		:;
		/ccsbin/ccsmake -M $(MACHINE) $(MFLAGS) -V `$(PWD)` cleanup

cleanup		:& $(CLEAN)
$(CLEAN)	:; sh -c "if [ ! -d `basename $@ .k` ]; then exit 0; else \
				cd `basename $@ .k` && \
					make $(MFLAGS) $(MRULES) clean; \
			  fi"

tape		:
	@exec sh -c '/ccsbin/ccsmake -V `$(PWD)` -M $(MACHINE) $(MFLAGS) \
	MINIROOT=$(MINIROOT) TMPOS_ROOT=$(TMPOS_ROOT) TMPOS_USR=$(TMPOS_USR) \
	maketape' | tee makelog.tape 2>&1

maketape	:
	@exec sh -c '\
	if [ "$(MINIROOT)" != "" -a "$(TMPOS_ROOT)" != "" -a \
	     "$(TMPOS_USR)" != "" ]; then \
		$(DESTDIR)/etc/buildmini $(MINIROOT) $(DESTDIR) && \
		/bin/cp $(DESTDIR)/etc/maketape /etc/maketape$$$$ && \
		/etc/maketape$$$$ $(MINIROOT) $(TMPOS_ROOT) $(TMPOS_USR) \
			$(DESTDIR) && \
		rm -f /etc/maketape$$$$; \
	else \
		echo "error: MINIROOT, TMPOS_ROOT, or TMPOS_USR omitted!"; \
		echo example usage: make tape MACHINE=machine MINIROOT=zd1b TMPOS_ROOT=zd1a TMPOS_USR=zd1h; \
		exit 1; \
	fi'
