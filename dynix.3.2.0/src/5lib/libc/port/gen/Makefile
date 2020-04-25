#	@(#)makefile	1.5
#
# makefile for libc/port/gen
#
#

FPFLAGS	=
CFLAGS	= $(ATT_INCLUDE) -O $(FPFLAGS)

NOTYET	=\
	mon.o

OBJS	=\
	a64l.o abort.o abs.o assert.o atof.o atoi.o atol.o bsearch.o calloc.o\
	clock.o ctime.o ctype.o dial.o drand48.o ecvt.o errlst.o execvp.o\
	fakcu.o frexp.o ftok.o ftw.o gcvt.o getcwd.o getenv.o getgrent.o\
	getgrgid.o getgrnam.o getlogin.o getopt.o getpw.o getpwent.o getpwnam.o\
	getpwuid.o getut.o hsearch.o isatty.o l3.o l64a.o ldexp.o lfind.o\
	lsearch.o malloc.o memccpy.o memchr.o memcmp.o memcpy.o memset.o\
	mktemp.o modf.o nlist.o perror.o putenv.o putpwent.o qsort.o rand.o\
	sleep.o ssignal.o strcat.o strchr.o strcmp.o strcpy.o strcspn.o\
	strlen.o strncat.o strncmp.o strncpy.o strpbrk.o strrchr.o strspn.o\
	strtod.o strtok.o strtol.o swab.o tell.o tfind.o tolower.o toupper.o\
	tsearch.o ttyname.o ttyslot.o
LIB	= genlib genlib_p

.c.o:
	$(CC) -p $(CFLAGS) -c $*.c && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC)    $(CFLAGS) -c $*.c && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../genlib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru genlib $(OBJS)
parallel	:& $(OBJS)

##
# pick up DYNIX header files (for utmp.h)
##
ttyslot.o	: ttyslot.c
		  $(CC) -p -DUTMP_FILE=\"/etc/utmp\" $(INCLUDE) -O -c $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC)    -DUTMP_FILE=\"/etc/utmp\" $(INCLUDE) -O -c $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

getlogin.o	: getlogin.c
		  $(CC) -p -DUTMP_FILE=\"/etc/utmp\" $(INCLUDE) -O -c $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC)    -DUTMP_FILE=\"/etc/utmp\" $(INCLUDE) -O -c $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

# special rule so we can grab nlist
nlist.c		: ../../../../lib/libc/gen/nlist.c; ln -s $? $@
	
# put "pow1_25" table in text ...
atof.o		: atof.c
		  $(CC) -SO -p $(CFLAGS) -c $*.c | sed 's/^_pow1_25/.text;.align 2;&/' | $(AS) -o $*.o && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC) -SO    $(CFLAGS) -c $*.c | sed 's/^_pow1_25/.text;.align 2;&/' | $(AS) -o $*.o && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o
profiled	:; mkdir profiled
clean clobber	:; rm -rf $(LIB) profiled nlist.c $(OBJS)
