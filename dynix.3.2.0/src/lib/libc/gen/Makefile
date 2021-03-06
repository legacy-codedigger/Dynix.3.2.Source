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

# $Header: /usr/src/dynix.3.2.0/src/lib/libc/gen/RCS/Makefile,v 1.2 1993/01/19 10:44:55 bruce Exp $
#
CRYPT	=
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(CRYPT) $(FPFLAGS)
#
# NFS routines obsolete these BSD modules
#
BSD	= getpwnam.o getpwuid.o getgrnam.o getgrgid.o
NFS	= mntent.o
OBJS	=\
	$(NFS) _getpwent.o alarm.o atoi.o atol.o calloc.o closedir.o\
	crypt.o ctime.o ctype_.o errlst.o execvp.o fakcu.o fstab.o\
	gcvt.o getenv.o getgrent.o getlogin.o getpass.o getpseudotty.o\
	getpwent.o getusershell.o getwd.o initgroups.o isatty.o malloc.o\
	mkstemp.o mktemp.o ndbm.o nlist.o opendir.o perror.o popen.o psignal.o\
	qsort.o random.o readdir.o regex.o rindex.o scandir.o seekdir.o\
	setegid.o setenv.o seteuid.o setgid.o setrgid.o setruid.o setuid.o\
	siglist.o signal.o sleep.o strcasecmp.o swab.o syslog.o system.o\
	telldir.o time.o timezone.o ttyname.o ttyslot.o valloc.o add_utmp.o\
	getnumusers.o getmaxusers.o strdup.o strerror.o

SED	= sed -f ../$(MACHINE)/rofix.sed
LIB	= genlib genlib_p

.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../genlib_p $(OBJS) && \
		  echo "	buiding normal $@" && \
		  cd .. && $(AR) cru genlib $(OBJS)

parallel	:& $(OBJS) misc
misc		: DEScrypt.c
		  -if [ "$(EXPORT)" != TRUE ]; then \
			make $(MFLAGS) $(MRULES) CRYPT="-DCRYPT" FPFLAGS="$(FPFLAGS)" DEScrypt.o; \
		  fi

DEScrypt.c	: crypt.c
		  -if [ "$(EXPORT)" != TRUE ]; then \
			  ln -s $? $@; \
		  fi

# Special handling to make data shared and readonly
siglist.o ctype_.o:
	cd profiled; $(CC) -R $(CFLAGS) -c ../$*.c; cp $*.o ../$*.o

errlst.o	: errlst.c
		  $(CC) -SO $(CFLAGS) $*.c | \
	  	  sed -e 's/^\.data/.text/' \
		      -e 's/^_sys_nerr:/.data;.align 2;&/' | \
	  	  $(AS) -o profiled/$*.o; cp profiled/$*.o ./$*.o
profiled	:; mkdir profiled

clean		:
		-if [ -f crypt.c ]; then \
			rm -rf genlib genlib_p core a.out errs DEScrypt.[oc] \
			$(OBJS) profiled; \
		else \
			rm -rf genlib genlib_p core a.out errs DEScrypt.[oc] \
			`find $(OBJS) profiled ! -name crypt.o -print`; \
		fi

# crypt.c is not included on the utilities (export) source distribution
crypt.c		:

crypt.o		: crypt.c
	-if [ "$(EXPORT)" != TRUE ]; then \
	$(CC) -SO -p -DPROF $(CFLAGS) -c crypt.c | $(SED) | $(AS) -o crypt.o && \
	$(LD) -X -r crypt.o -o profiled/crypt.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c crypt.c | $(SED) | $(AS) -o crypt.o && \
	$(LD) -x -r crypt.o -o crypt.O && mv crypt.O crypt.o; \
	fi
