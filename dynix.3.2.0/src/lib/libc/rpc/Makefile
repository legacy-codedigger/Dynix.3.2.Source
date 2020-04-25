# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.3 91/01/10 $
#
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)
#
OBJS	=\
	auth_none.o auth_unix.o authunix_prot.o clnt_perror.o\
	clnt_raw.o clnt_simple.o clnt_tcp.o clnt_udp.o pmap_clnt.o\
	pmap_getmaps.o pmap_getport.o pmap_prot.o pmap_rmt.o\
	rpc_prot.o svc.o svc_auth.o svc_auth_unix.o svc_raw.o\
	svc_simple.o svc_tcp.o svc_udp.o xdr.o xdr_array.o xdr_float.o\
	xdr_mem.o xdr_rec.o xdr_reference.o xdr_stdio.o rpcdtablesize.o
SED	= sed -f ../$(MACHINE)/rofix.sed
LIB	= rpclib rpclib_p

.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../rpclib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru rpclib $(OBJS)
parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean		:; rm -rf rpclib rpclib_p profiled $(OBJS)
