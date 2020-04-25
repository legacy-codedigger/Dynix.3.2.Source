#	@(#)makefile	1.4
#
# makefile for libc/port/stdio
#
#

INCLUDE	= $(ATT_INCLUDE)
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)

# BTW: dropped "bufctl" for now since a zero length file
OBJS	=\
	clrerr.o ctermid.o cuserid.o data.o doscan.o\
	fdopen.o fgetc.o fgets.o filbuf.o findiop.o flsbuf.o\
	fopen.o fputc.o fputs.o fread.o fseek.o ftell.o fwrite.o\
	getchar.o getpass.o gets.o getw.o nfile.o popen.o putchar.o puts.o\
	putw.o rew.o scanf.o setbuf.o setvbuf.o system.o tempnam.o\
	tmpfile.o tmpnam.o ungetc.o
LIB	= stdiolib stdiolib_p

.c.o:
		  $(CC) -p $(CFLAGS) -c $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC)    $(CFLAGS) -c $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../stdiolib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru stdiolib $(OBJS)

parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean clobber	:; rm -rf $(LIB) profiled $(OBJS)
