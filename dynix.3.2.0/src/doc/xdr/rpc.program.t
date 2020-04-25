.V= $Header: rpc.program.t 1.2 87/07/22 $
.Ct "" "" "Remote Procedure Call Programming"
.Ah "Introduction"
This document is intended for programmers
who want to write network applications
that use remote procedure calls,
thereby avoiding low-level system primitives based on sockets.
The reader must be familiar with the C programming language
and should have a working knowledge of network theory.
.Pa
Programs that communicate over a network
need a paradigm for communication.
For example, a low-level mechanism might
send a signal on the arrival of incoming packets,
causing a network signal handler to execute.
A high-level mechanism would be the Ada
\f2rendezvous\fP.  NFS uses the
Remote Procedure Call (RPC) paradigm,
in which a client communicates with a server.
In this process,
the client first calls a procedure to send a data packet to the server.
When the packet arrives, the server calls a dispatch routine,
performs whatever service is requested, and sends back the reply.
The procedure call then returns to the client.
.Bh "Layers of RPC"
The RPC interface is divided into three layers.
The highest layer is totally transparent to the programmer.
For example, at this level a program can contain a call to
\f3rnusers()\fP, which returns the number of users on a remote machine.
You don't have to be aware that RPC is being used.
You simply make the call in a program,
just as you would call \f3malloc()\fP.
.Pa
At the middle layer, the routines \f3registerrpc()\fP
and \f3callrpc()\fP are used to make RPC calls.  \f3registerrpc()\fP
obtains a unique, system-wide number, while
\f3callrpc()\fP executes a remote procedure call.
These two routines are used to implement the \f3rnusers()\fP call. 
The middle-layer routines are designed for most common applications
and shield the user from knowing about sockets.
.Pa
The lowest layer is for more sophisticated applications,
such as altering the defaults of the routines.
At this layer, you can explicitly manipulate
sockets that transmit RPC messages.
This level should be avoided if possible.
.Pa
The two higher layers are discussed in the section \*QHigher Layers of 
RPC\*U
later in this document.  The low-level interface is described in the section
\*QLowest Layer of RPC.\*U
This document also includes a discussion of miscellaneous topics and a
summary of all of the entry points into the RPC system.
.Pa
Although this document discusses only the interface to C,
remote procedure calls can be made from any language.
In the examples in this document, RPC
is used to communicate
between processes on different machines.
However, it works just as well for communication
between different processes on the same machine.
.bp
.Bh "The RPC Paradigm"
The following diagram illustrates the RPC paradigm:
.PS
L1: arrow down 1i "client " rjust "program " rjust
L2: line right 1.5i "\f3callrpc()\fP" "function"
move up 1.5i; line dotted down 6i; move up 4.5i
arrow right 1i
L3: arrow down 1i "execute " rjust "request " rjust
L4: arrow right 1.5i "call" "service"
L5: arrow down 1i " service" ljust " executes" ljust
L6: arrow left 1.5i "\f2return\fP" "answer"
L7: arrow down 1i "request " rjust "completed " rjust
L8: line left 1i
arrow left 1.5i "\f2return\fP" "reply"
L9: arrow down 1i "program " rjust "continues " rjust
line dashed down from L2 to L9
line dashed down from L4 to L7
line dashed up 1i from L3 "service " rjust "daemon " rjust
arrow dashed down 1i from L8
move right 1i from L3
box invis "Machine B"
move left 1.2i from L2; move down
box invis "Machine A"
.PE
.bp
.Ah "Higher Layers of RPC"
.Bh "Highest Layer"
Imagine you're writing a program that needs to know
how many users are logged into a remote machine.
You can do this by calling the library routine
\f3rnusers()\fP, as shown below:
.Ps
#include <stdio.h>
.sp.5
main(argc, argv)
	int argc;
	char **argv;
{
	int num;
.sp.5
	if (argc < 2) {
		fprintf(stderr, "usage: rnusers hostname\en");
		exit(1);
	}
	if ((num = rnusers(argv[1])) < 0) {
		fprintf(stderr, "error: rnusers\en");
		exit(-1);
	}
	printf("%d users on %s\en", num, argv[1]);
	exit(0);
}
.Pe
Because RPC library routines such as \f3rnusers()\fP
are found in the RPC services library \f2librpcsvc.a\fP,
this program should be compiled as follows:
.Ps
% \ \c
.UL "cc \f2program\fP.c -lrpcsvc"
.Pe
The following RPC service library routines
are available to the C programmer:
.Ts 1-1 "RPC Service Library Routines"
.TS
box, center;
c c
l l.
\f3RPC Routine	Description\fP
_
rnusers()	return number of users on remote machine
rusers()	return information about users on remote machine
havedisk()	determine if remote machine has disk
rstat() 	get performance data from remote kernel
rwall() 	write to specified remote machines
getmaster()	get name of YP master
getrpcport()	get RPC port number
yppasswd()	update user password in yellow pages
.TE
.Te
.Pa
The other RPC services (\f3ether\fP, \f3mount\fP, \f3rquota\fP,
and \f3spray\fP) are not available to the C programmer as library routines.
However, because they have RPC program numbers, they can be invoked with
\f3callrpc()\fP, which will be discussed in the next section.
.Bh "Intermediate Layer"
The simplest interface that explicitly makes RPC
calls, the intermediate layer uses the functions 
\f3callrpc()\fP and \f3registerrpc()\fP.
Using this layer of RPC, the number of remote users can be obtained by:
.Ps
#include <stdio.h>
#include <rpcsvc/rusers.h>
#include <rpc/rpc.h>
.sp.5
main(argc, argv)
	int argc;
	char **argv;
{
	unsigned long nusers;
.sp.5
	if (argc < 2) {
		fprintf(stderr, "usage: nusers hostname\en");
		exit(-1);
	}
	if (callrpc(argv[1],
	  RUSERSPROG, RUSERSVERS, RUSERSPROC_NUM,
	  xdr_void, 0, xdr_u_long, &nusers) != 0) {
		fprintf(stderr, "error: callrpc\en");
		exit(1);
	}
	printf("%d users on %s\en", nusers, argv[1]);
	exit(0);
}
.Pe
Each RPC procedure is defined by a program number, a version number, 
and a procedure number.
The program number defines a group
of related remote procedures, each of which has a different
procedure number.  Because each program also has a version number,
when a minor change is made to a remote service
(e.g., adding a new procedure),
a new program number doesn't have to be assigned.
When you want to call a procedure to
find the number of remote users, you look up the appropriate
program, version, and procedure numbers
in this article, just as you look up the name of a memory
allocator when you want to allocate memory.
.Pa
\f3callrpc()\fP is the simplest routine used to make remote procedure
calls.  It has eight parameters.
The first parameter is the name of the remote machine.
The next three parameters are the program, version, and procedure numbers.
The next two parameters
define the argument of the RPC call, and the final two parameters
are for the return value of the call.
If the call completes successfully, \f3callrpc()\fP
returns zero. If the call is not successful, nonzero is
returned.
The exact meanings of the return codes are found in
\f2<rpc/clnt.h>\fP.  They are, in fact, an
\f2enum clnt_stat\fP cast into an integer.
.Pa
Because data types can be represented differently on different machines,
\f3callrpc()\fP needs both the type of the RPC argument, as well as
a pointer to the argument itself (and similarly for the result).  For
example, the return value for \f2RUSERSPROC_NUM\fP is
\f2unsigned long\fP.  Therefore, \f3callrpc()\fP has
\f2xdr_u_long\fP as its first return parameter, which says
that the result is of type \f2unsigned long\fP.  The second return
parameter is \f2&nusers\fP,
which is a pointer to where the long result will be placed.  Because
\f2RUSERSPROC_NUM\fP takes no argument, the argument parameter of
\f3callrpc()\fP is \f2xdr_void\fP.
.Pa
After trying several times to deliver a message and receiving no answer, 
\f3callrpc()\fP returns an error code.
The delivery mechanism is UDP, which stands for User Datagram Protocol.
Methods for adjusting the number of retries
or for using a different protocol require that you use the bottom layer
of the RPC library, which is discussed later in this article.
.Pa
The remote server procedure
corresponding to our example might look like this:
.Ps
char *
nuser(indata)
	char *indata;
{
	static int nusers;
.sp.5
	/*
	 * code here to compute the number of users
	 * and place result in variable nusers
	 */
	return((char *)&nusers);
}
.Pe
It takes one argument, which is a pointer to the input
of the remote procedure call (ignored in our example),
and it returns a pointer to the result.
In the current version of C,
character pointers are the generic pointers,
so both the input argument and the return value are cast to
\f2char *\fP.
.Pa
Normally, a server registers all of the RPC calls it plans
to handle, and then goes into an infinite loop waiting to service requests.
In this example, there is only a single procedure
to register, so the main body of the server would look like this:
.sp
.As
#include <stdio.h>
#include <rpcsvc/rusers.h>
.sp.5
char *nuser();
.sp.5
main()
{
	registerrpc(RUSERSPROG, RUSERSVERS, RUSERSPROC_NUM,
		nuser, xdr_void, xdr_u_long);
	svc_run();		/* never returns */
	fprintf(stderr, "Error: svc_run returned!\en");
	exit(1);
}
.Ae
.sp
The \f3registerrpc()\fP routine establishes the C procedure
that corresponds to each RPC procedure number.
The first three parameters, \f2RUSERPROG\fP, \f2RUSERSVERS\fP,
and \f2RUSERSPROC_NUM\fP, are the program, version, and procedure numbers
of the remote procedure to be registered.
\f3nuser()\fP is the name of the C procedure implementing the procedure.
\f2xdr_void\fP is the type of the input to the procedure,
and \f2xdr_u_long\fP is the type of the output from the procedure.
.Pa
Because only the UDP transport mechanism can use \f3registerrpc()\fP,
it is always safe in conjunction with calls generated by
\f3callrpc()\fP.
.Ns W
The UDP transport mechanism can deal only with
arguments and results less than 8K bytes in length.
.Ne
.Bh "Assigning Program Numbers"
Program numbers are assigned in groups of 0x20000000 (536870912)
according to the following list:
.Ps
       0 - 1fffffff	defined by sun
20000000 - 3fffffff	defined by user
40000000 - 5fffffff	transient
60000000 - 7fffffff	reserved
80000000 - 9fffffff	reserved
a0000000 - bfffffff	reserved
c0000000 - dfffffff	reserved
e0000000 - ffffffff	reserved
.Pe
Sun Microsystems administers the first group of numbers,
which should be identical for all Sun customers.
If a customer develops an application that might be of general interest,
that application should be given an assigned number in the first range.
The second group of numbers is reserved for specific customer applications.
This range is intended primarily for debugging new programs.
The third group is reserved for applications that
generate program numbers dynamically.
The final groups are reserved for future use. 
.Pa
To register a protocol specification,
send a request by network mail to \f2sun!rpc\fP, or write to:
.sp
.br
	RPC Administrator
.br
	Sun Microsystems
.br
	2550 Garcia Ave.
.br
	Mountain View, CA 94043
.Pa
Please include a complete protocol specification.
You will be given a unique program number in return.
.Bh "Passing Arbitrary Data Types"
In the previous example, the RPC call passes a single
\f2unsigned long\fP.
RPC can handle arbitrary data structures, regardless of
the byte orders or structure layout conventions of different machines.
This is done by converting the data structures into a network standard, called
eXternal Data Representation (XDR), before
sending them over the wire.
The process of converting from a particular machine representation
to XDR format is called \f2serializing\fP;
the reverse process is called \f2deserializing\fP.
The parameters for the \f2type\fP fields in \f3callrpc()\fP and
\f3registerrpc()\fP can be a built-in procedure 
(such as \f3xdr_u_long()\fP in the previous example),
or can be supplied by the user.
XDR contains these built-in type routines:
.Ps
xdr_int()      xdr_u_int()      xdr_enum()
xdr_long()     xdr_u_long()     xdr_bool()
xdr_short()    xdr_u_short()    xdr_string()
.Pe
As an example of a user-defined type routine,
if you wanted to send the structure:
.Ps
struct simple {
	int a;
	short b;
} simple;
.Pe
you would call \f3callrpc()\fP as:
.Ps
callrpc(hostname, PROGNUM, VERSNUM, PROCNUM,
        xdr_simple, &simple ...);
.Pe
where \f3xdr_simple()\fP is written as:
.Ps
#include <rpc/rpc.h>
.sp.5
xdr_simple(xdrsp, simplep)
	XDR *xdrsp;
	struct simple *simplep;
{
	if (!xdr_int(xdrsp, &simplep->a))
		return (0);
	if (!xdr_short(xdrsp, &simplep->b))
		return (0);
	return (1);
}
.Pe
An XDR routine returns nonzero (true in the sense of C)
if it completes successfully, and zero otherwise.
(For more information about XDR, refer to the article \f2XDR Protocol
Specification\fP in the \f2DYNIX Programmer's Manual, Vol. II\fP.)
.Pa
In addition to the built-in primitives,
RPC also contains the following prefabricated building blocks:
.Ps
xdr_array()       xdr_bytes()
xdr_reference()   xdr_union()
.Pe
To send a variable array of integers,
you might package them up as a structure like this:
.Ps
struct varintarr {
	int *data;
	int arrlnth;
} arr;
.Pe
.bp
Then make an RPC call such as:
.Ps
callrpc(hostname, PROGNUM, VERSNUM, PROCNUM,
        xdr_varintarr, &arr...);
.Pe
with \f3xdr_varintarr()\fP defined as:
.sp
.As
xdr_varintarr(xdrsp, arrp)
	XDR *xdrsp;
	struct varintarr *arrp;
{
	xdr_array(xdrsp, &arrp->data, &arrp->arrlnth, MAXLEN,
		sizeof(int), xdr_int);
}
.Ae
.Pa
This routine takes as parameters the XDR handle,
a pointer to the array, a pointer to the size of the array,
the maximum allowable array size,
the size of each array element,
and an XDR routine for handling each array element.
.Pa
If the size of the array is known in advance, 
the following method could also be used to send
out an array of length \f2SIZE\fP:
.Ps
int intarr[SIZE];
.sp.5
xdr_intarr(xdrsp, intarr)
	XDR *xdrsp;
	int intarr[];
{
	int i;
.sp.5
	for (i = 0; i < SIZE; i++) {
		if (!xdr_int(xdrsp, &intarr[i]))
			return (0);
	}
	return (1);
}
.Pe
When deserializing, XDR always converts quantities to 4-byte multiples. 
Therefore, if either of the previous examples involved characters
instead of integers, each character would occupy 32 bits.
The XDR routine \f3xdr_bytes()\fP
packs characters.
It has four parameters that are similar to the first
four parameters of \f3xdr_array()\fP.
The \f3xdr_string()\fP routine is used for null-terminated strings.  
It is the same as
\f3xdr_bytes()\fP without the length parameter.
On serializing, it obtains the string length from
\f3strlen()\fP, and on deserializing it creates a null-terminated string.
.Pa
Following is a final example that calls 
\f3xdr_simple()\fP as well as the built-in functions
\f3xdr_string()\fP and \f3xdr_reference()\fP
(which chases pointers):
.sp
.As
struct finalexample {
	char *string;
	struct simple *simplep;
} finalexample;
.sp.5
xdr_finalexample(xdrsp, finalp)
	XDR *xdrsp;
	struct finalexample *finalp;
{
	int i;
.sp.5
	if (!xdr_string(xdrsp, &finalp->string, MAXSTRLEN))
		return (0);
	if (!xdr_reference(xdrsp, &finalp->simplep,
	  sizeof(struct simple), xdr_simple);
		return (0);
	return (1);
}
.Ae
.Ah "Lowest Layer of RPC"
In the examples given so far,
RPC takes care of many details automatically.
In this section, we'll show you how you can change the defaults
by using the lowest layer of the RPC library.
It is assumed that you are familiar with sockets
and with the system calls for dealing with them.
.Pa
There are several occasions when you might need to use the lowest layer of RPC.
First, you might want to use TCP, which permits RPC calls to send long 
streams of data.  (The higher layers use UDP,
which restricts RPC calls to 8K bytes of data.)
For an example, see the section entitled \*QTCP\*U later in this document.
.Pa
Second, you might want to allocate and free memory
while serializing or deserializing with XDR routines.
There is no call at the higher levels to let you free memory explicitly.
For more information, refer to \*QMemory Allocation with XDR\*U 
later in this document.
.Pa
Third, you might need to perform authentication
on either the client or the server side.
This is done either by supplying credentials or by verifying them.
The \*QAuthentication\*U section of this document explains this procedure.
.Bh "The Server Side"
The server for the following \f3nusers\fP
program does the same thing as the server using
\f3registerrpc()\fP in the previous example.  However, the program is
written using the lowest layer of the RPC package:
.sp
.As
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/rusers.h>
.sp.5
main()
{
	SVCXPRT *transp;
	int nuser();
.sp.5
	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL){
		fprintf(stderr, "can't create an RPC server\en");
		exit(1);
	}
	pmap_unset(RUSERSPROG, RUSERSVERS);
	if (!svc_register(transp, RUSERSPROG, RUSERSVERS,
			  nuser, IPPROTO_UDP)) {
		fprintf(stderr, "can't register RUSER service\en");
		exit(1);
	}
	svc_run();  /* never returns */
	fprintf(stderr, "should never reach this point\en");
}
.sp.5
nuser(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	unsigned long nusers;
.sp.5
	switch (rqstp->rq_proc) {
	case NULLPROC:
		if (!svc_sendreply(transp, xdr_void, 0)) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		}
		return;
	case RUSERSPROC_NUM:
		/*
		 * code here to compute the number of users
		 * and put in variable nusers
		 */
		if (!svc_sendreply(transp, xdr_u_long, &nusers) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		}
		return;
	default:
		svcerr_noproc(transp);
		return;
	}
}
.Ae
.sp
First, the server obtains a transport handle, which is used
for sending out RPC messages.
\f3registerrpc()\fP uses \f3svcudp_create()\fP
to obtain a UDP handle.
If you require a reliable protocol, call
\f3svctcp_create()\fP instead.
If the argument to \f3svcudp_create()\fP is
\f2RPC_ANYSOCK\fP, the RPC library creates a socket
on which to send out RPC calls.
Otherwise, \f3svcudp_create()\fP
expects its argument to be a valid socket number.
If you specify your own socket, it can be bound or unbound.
If it is bound to a port by the user, the port numbers of
\f3svcudp_create()\fP and \f3clntudp_create()\fP
(the low-level client routine) must match.
.Pa
If the user specifies \f2RPC_ANYSOCK\fP
or gives an unbound socket,
the system determines port numbers in the following way.
When a server starts up, it advertises to a port mapper
daemon on its local machine.
If the socket specified to \f3svcudp_create()\fP
isn't already bound, the daemon picks a port number for the RPC
procedure.
When the \f3clntudp_create()\fP
call is made with an unbound socket,
the system queries the port mapper on
the machine to which the call is being made
and then obtains the appropriate port number.
If the port mapper is not running
or has no port corresponding to the RPC call,
the RPC call fails.
Users can make RPC calls to the port mapper themselves.
The appropriate procedure numbers are in the include file
\f2<rpc/pmap_prot.h>\fP.
.Pa
After creating an \f2SVCXPRT\fP, the next step is to call
\f3pmap_unset()\fP, which erases the entry for \f2RUSERSPROG\fP
from the port mapper's tables.  This ensures that if the \f3nusers\fP
server crashed earlier, any trace of it is erased before
restarting.
.Pa
Finally, the program number for
\f3nusers\fP is associated with the procedure \f3nuser()\fP.
The final argument to \f3svc_register()\fP
is normally the protocol being used.
In this case, the protocol is \f2IPPROTO_UDP\fP.
Unlike \f3registerrpc()\fP,
there are no XDR routines involved
in the registration process.
Also, registration is done on the program level
rather than on the procedure level.
.Pa
Based on the procedure number, the user routine \f3nuser()\fP
must call and dispatch the appropriate XDR routines.
\f3nuser()\fP handles two procedures that
\f3registerrpc()\fP handles automatically.
First, the \f2NULLPROC\fP procedure
(currently zero) returns with no arguments.
This can be used as a simple test
to determine if a remote program is running.
Second, \f3nuser\fP checks for invalid procedure numbers.
If an invalid number is detected, \f3svcerr_noproc()\fP
is called to handle the error.
.Pa
The user service routine serializes the results and returns
them to the RPC caller via \f3svc_sendreply()\fP.
Its first parameter is the \f2SVCXPRT\fP
handle, the second is the XDR routine,
and the third is a pointer to the data to be returned.
.Pa
The next example illustrates how a server
handles an RPC program that passes data.
We have included the 
\f2RUSERSPROC_BOOL\fP procedure, which has the argument
\f2nusers\fP and returns TRUE or FALSE
depending on whether there are nusers logged on.
.Ps
case RUSERSPROC_BOOL: {
	int bool;
	unsigned nuserquery;
.sp.5
	if (!svc_getargs(transp, xdr_u_int, &nuserquery)) {
		svcerr_decode(transp);
		return;
	}
	/*
	 * code to set nusers = number of users
	 */
	if (nuserquery == nusers)
		bool = TRUE;
	else
		bool = FALSE;
	if (!svc_sendreply(transp, xdr_bool, &bool){
		 fprintf(stderr, "can't reply to RPC call\en");
		 exit(1);
	}
	return;
}
.Pe
The relevant routine is \f3svc_getargs()\fP.  As arguments, this routine
takes an \f2SVCXPRT\fP handle, the XDR routine,
and a pointer to where the input is to be placed. 
.Bh "Memory Allocation with XDR"
In addition to performing input and output, XDR routines 
also do memory allocation.
This is why the second parameter of \f3xdr_array()\fP
is a pointer to an array, rather than the array itself.
If the parameter is NULL, then \f3xdr_array()\fP
allocates space for the array and returns a pointer to it,
putting the size of the array in the third argument.
As an example, consider the following XDR routine,
\f3xdr_chararr1()\fP, which deals with a fixed array of bytes with length
\f2SIZE\fP:
.Ps
xdr_chararr1(xdrsp, chararr)
	XDR *xdrsp;
	char chararr[];
{
	char *p;
	int len;
.sp.5
	p = chararr;
	len = SIZE;
	return (xdr_bytes(xdrsp, &p, &len, SIZE));
}
.Pe
If \f3chararr\fP has already allocated space, the routine could be 
called from a server in this manner:
.Ps
char chararr[SIZE];
.sp.5
svc_getargs(transp, xdr_chararr1, chararr);
.Pe
If you want XDR to do the allocation,
you would have to rewrite this routine in the following way:
.sp
.As
xdr_chararr2(xdrsp, chararrp)
	XDR *xdrsp;
	char **chararrp;
{
	int len;
.sp.5
	len = SIZE;
	return (xdr_bytes(xdrsp, charrarrp, &len, SIZE));
}
.Ae
.sp
Then the RPC call might look like this:
.Ps
char *arrptr;
.sp.5
arrptr = NULL;
svc_getargs(transp, xdr_chararr2, &arrptr);
/*
 * use the result here
 */
svc_freeargs(transp, xdr_chararr2, &arrptr);
.Pe
After the character array has been used, it can be freed with
\f3svc_freeargs()\fP.
In the routine \f3xdr_finalexample()\fP that was shown earlier, if
\f2finalp->string\fP was NULL in the call:
.Ps
svc_getargs(transp, xdr_finalexample, &finalp);
.Pe
then:
.Ps
svc_freeargs(xdrsp, xdr_finalexample, &finalp);
.Pe
frees the array allocated to hold
\f2finalp->string\fP; otherwise, it frees nothing.
The same is true for \f2finalp->simplep\fP.
.Pa
To summarize, each XDR routine is responsible
for serializing, deserializing, and allocating memory.
When an XDR routine is called from \f3callrpc()\fP,
the serializing part is used.
When called from \f3svc_getargs()\fP,
the deserializer is used.
And when called from \f3svc_freeargs()\fP,
the memory deallocator is used.
When building simple examples like the ones in this section,
a user doesn't have to worry about these three modes of operation.
The article \f2XDR Protocol Specification\fP in the \f2DYNIX Programmer's
Manual, Vol II\fP contains examples of more
sophisticated XDR routines that must
determine in which of the three modes they are operating in order
to function correctly.
.Bh "The Calling Side"
When you use \f3callrpc()\fP,
you have no control over the RPC delivery
mechanism or the socket used to transport the data.
To illustrate the layer of RPC that lets you adjust these
parameters, consider the following code to call the
\f3nusers\fP service:
.sp
.As
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/rusers.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
.sp.5
main(argc, argv)
	int argc;
	char **argv;
{
	struct hostent *hp;
	struct timeval pertry_timeout, total_timeout;
	struct sockaddr_in server_addr;
	int addrlen, sock = RPC_ANYSOCK;
	register CLIENT *client;
	enum clnt_stat clnt_stat;
	unsigned long nusers;
.sp.5
	if (argc < 2) {
		fprintf(stderr, "usage: nusers hostname\en");
		exit(-1);
	}
	if ((hp = gethostbyname(argv[1])) == NULL) {
		fprintf(stderr, "can't get addr for %s\en",argv[1]);
		exit(-1);
	}
	pertry_timeout.tv_sec = 3;
	pertry_timeout.tv_usec = 0;
	addrlen = sizeof(struct sockaddr_in);
	bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr,
		hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;
	if ((client = clntudp_create(&server_addr, RUSERSPROG,
	  RUSERSVERS, pertry_timeout, &sock)) == NULL) {
		clnt_pcreateerror("clntudp_create");
		exit(-1);
	}
	total_timeout.tv_sec = 20;
	total_timeout.tv_usec = 0;
	clnt_stat = clnt_call(client, RUSERSPROC_NUM, xdr_void,
		0, xdr_u_long, &nusers, total_timeout);
	if (clnt_stat != RPC_SUCCESS) {
		clnt_perror(client, "rpc");
		exit(-1);
	}
	clnt_destroy(client);
}
.Ae
.sp
The low-level version of \f3callrpc()\fP is
\f3clnt_call()\fP, which takes a \f2CLIENT\fP
pointer rather than a host name.  The parameters to
\f3clnt_call()\fP are a \f2CLIENT\fP pointer, the procedure number,
the XDR routine for serializing the argument,
a pointer to the argument,
the XDR routine for deserializing the return value,
a pointer to where the return value will be placed,
and the time in seconds to wait for a reply.
.Pa
The \f2CLIENT\fP
pointer is encoded with the transport mechanism.
Because \f3callrpc()\fP uses UDP, it calls
\f3clntudp_create()\fP to get a \f2CLIENT\fP
pointer.  To use TCP (Transport Control Protocol) instead, you would call
\f3clnttcp_create()\fP.
.Pa
The parameters to \f3clntudp_create()\fP are the server address,
the length of the server address,
the program number, the version number,
a timeout value (between tries), and a pointer to a socket.
The final argument to \f3clnt_call()\fP
is the total time to wait for a response.
Therefore, the number of tries is the
\f3clnt_call()\fP timeout divided by the
\f3clntudp_create()\fP timeout.
.Pa
When using the \f3clnt_destroy()\fP call,
keep in mind that it deallocates any space associated with the
\f2CLIENT\fP handle, but it does not close the socket associated with it,
which was passed as an argument to \f3clntudp_create()\fP.
If there are multiple client handles using the same socket,
it is possible to close one handle
without destroying the socket. 
.Pa
To make a stream connection, the call to \f3clntudp_create()\fP
is replaced with a call to \f3clnttcp_create()\fP:
.Ps
clnttcp_create(&server_addr, prognum, versnum, &socket,
               inputsize, outputsize);
.Pe
There is no timeout argument.  Instead, the receive and send buffer
sizes must be specified.  When the \f3clnttcp_create()\fP
call is made, a TCP connection is established.
All RPC calls with that \f2CLIENT\fP
handle would use this connection.
On the server side of an RPC call that is using TCP, 
\f3svcudp_create()\fP is replaced by \f3svctcp_create()\fP.
.Ah "Other RPC Features"
.Bh "Select on the Server Side"
Suppose a process is carrying out RPC requests
while performing some other activity.
If the other activity involves periodically updating a data structure,
the process can set an alarm signal before calling \f3svc_run()\fP.
But if the other activity involves waiting on a file descriptor, the
\f3svc_run()\fP call won't work.
The code for \f3svc_run()\fP is as follows:
.sp
.As
void
svc_run()
{
	int readfds;
.sp.5
	for (;;) {
		readfds = svc_fds;
		switch (select(32, &readfds, NULL, NULL, NULL)) {
.sp.5
		case -1:
			if (errno == EINTR)
				continue;
			perror("rstat: select");
			return;
		case 0:
			break;
		default:
			svc_getreq(readfds);
		}
	}
}
.Ae
.sp
You can bypass \f3svc_run()\fP and call \f3svc_getreq()\fP
yourself.  To do this, you need to know the file descriptors
of the socket(s) associated with the programs that you are waiting on.
You can then have your own \f3select()\fP
that waits on both the RPC socket and on your own descriptors.
.Bh "Broadcast RPC"
The \f2portmapper\fP is a daemon that converts RPC program numbers
into DARPA protocol port numbers (see \f2portmap\fP(8)).
You can't do broadcast RPC without the portmapper
(\f3pmap\fP) in conjunction with standard RPC protocols.
The main differences between
broadcast RPC calls and normal RPC calls are:
.Ls 1
.Li
Normal RPC expects one answer, whereas
broadcast RPC expects many answers
(one or more answer from each responding machine).
.Li
Broadcast RPC can be supported only by packet-oriented (connectionless)
transport protocols such as UPD/IP.
.Li
The implementation of broadcast RPC
treats all unsuccessful responses as garbage by filtering them out.
If there is a version mismatch between the
broadcaster and a remote service,
the user of broadcast RPC never knows.
.Li
All broadcast messages are sent to the portmap port.
Only services that register themselves with their portmapper
are accessible via the broadcast RPC mechanism.
.Le
.Ch "Broadcast RPC Synopsis"
.As
#include <rpc/pmap_clnt.h>
.sp.5
enum clnt_stat	clnt_stat;
.sp.5
clnt_stat =
clnt_broadcast(prog, vers, proc, xargs, argsp, xresults,
	resultsp, eachresult)
u_long		prog;		/* program number */
u_long		vers;		/* version number */
u_long		proc;		/* procedure number */
xdrproc_t	xargs;		/* xdr routine for args */
caddr_t		argsp;		/* pointer to args */
xdrproc_t	xresults;	/* xdr routine for results */
caddr_t		resultsp;	/* pointer to results */
bool_t (*eachresult)();	/* call with each result gotten */
.Ae
.sp
The procedure \f3eachresult()\fP
is called each time a valid result is obtained.
It returns a boolean that indicates
if the client wants more responses:
.sp
.As
bool_t			done;
.sp.5
done =
eachresult(resultsp, raddr)
caddr_t resultsp;
struct sockaddr_in *raddr;  /* addr of responding machine */
.Ae
.sp
If \f2done\fP is TRUE, then broadcasting stops and
\f3clnt_broadcast()\fP returns successfully.
Otherwise, the routine waits for another response.
The request is rebroadcast after a few seconds of waiting.
If no responses come back, the routine returns 
RPC_TIMEDOUT.  To interpret \f3clnt_stat\fP
errors, feed the error code to \f3clnt_perrno()\fP.
.Bh "Batching"
The RPC architecture is designed so that clients send a call message
and then wait for servers to reply that the call succeeded.
This implies that clients do not compute
while servers are processing a call.
If the client does not want or need
an acknowledgement for every message sent, this can be an
inefficient use of time.  The RPC batch facilities make it
possible for clients to continue computing
while waiting for a response. 
.Pa
RPC messages can be placed in a \*Qpipeline\*U of calls
to a desired server.  This is called \f2batching\fP.
Batching makes the following assumptions:
.Ls 1
.Li
Each RPC call in the pipeline requires no response from the server,
and the server does not send a response message. 
.Li
The pipeline of calls is transported on a reliable
byte stream transport such as TCP/IP.
.Le
.Pa
Since the server does not respond to every call,
the client can generate new calls in parallel
with the server executing previous calls.
Furthermore, the TCP/IP implementation can buffer up
many call messages and then send them to the server in one
\f3write()\fP system call.  This overlapped execution
greatly decreases the interprocess communication overhead of
the client and server processes, as well as
the total elapsed time of a series of calls.
.Pa
Because the batched calls are buffered,
the client should eventually do a legitimate call
in order to flush the pipeline.
.Pa
A contrived example of batching follows.
Assume a string rendering service (like a window system)
has two similar calls; one call renders a string and returns void results,
while the other call renders a string and remains silent.
The service (using the TCP/IP transport) might look like this:
.sp
.As
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/windows.h>
.sp.5
void windowdispatch();
.sp.5
main()
{
	SVCXPRT *transp;
.sp.5
	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL){
		fprintf(stderr, "can't create an RPC server\en");
		exit(1);
	}
	pmap_unset(WINDOWPROG, WINDOWVERS);
	if (!svc_register(transp, WINDOWPROG, WINDOWVERS,
	  windowdispatch, IPPROTO_TCP)) {
		fprintf(stderr, "can't register WINDOW service\en");
		exit(1);
	}
	svc_run();  /* never returns */
	fprintf(stderr, "should never reach this point\en");
}
.sp.5
void
windowdispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char *s = NULL;
.sp.5
	switch (rqstp->rq_proc) {
	case NULLPROC:
		if (!svc_sendreply(transp, xdr_void, 0)) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		}
		return;
	case RENDERSTRING:
		if (!svc_getargs(transp, xdr_wrapstring, &s)) {
			fprintf(stderr, "can't decode arguments\en");
			/*
			 * tell caller he screwed up
			 */
			svcerr_decode(transp);
			break;
		}
		/*
		 * call here to render the string s
		 */
		if (!svc_sendreply(transp, xdr_void, NULL)) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		}
		break;
	case RENDERSTRING_BATCHED:
		if (!svc_getargs(transp, xdr_wrapstring, &s)) {
			fprintf(stderr, "can't decode arguments\en");
			/*
			 * we are silent in the face of protocol errors
			 */
			break;
		}
		/*
		 * call here to render string s, but send no reply!
		 */
		break;
	default:
		svcerr_noproc(transp);
		return;
	}
	/*
	 * now free string allocated while decoding arguments
	 */
	svc_freeargs(transp, xdr_wrapstring, &s);
}
.Ae
.sp
Of course, the service could have one procedure
that takes the string and a boolean
to indicate if the procedure should respond.
.Pa
To take advantage of batching,
the client must perform RPC calls on a TCP-based transport.
The actual calls must have the following attributes:
.Ls 1
.Li
The XDR routine of the result must be zero (NULL). 
.Li
The timeout for the RPC call must be zero.
.Le
.Pa
Following is an example of a client that uses batching
to render several strings.  
The batching is flushed when the client receives a null string.
.sp
.As
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/windows.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
.sp.5
main(argc, argv)
	int argc;
	char **argv;
{
	struct hostent *hp;
	struct timeval pertry_timeout, total_timeout;
	struct sockaddr_in server_addr;
	int addrlen, sock = RPC_ANYSOCK;
	register CLIENT *client;
	enum clnt_stat clnt_stat;
	char buf[1000], *s = buf;
.sp.5
	/* initial as in example 3.3
	 */
	if ((client = clnttcp_create(&server_addr,
	  WINDOWPROG, WINDOWVERS, &sock, 0, 0)) == NULL) {
		perror("clnttcp_create");
		exit(-1);
	}
	total_timeout.tv_sec = 0;
	total_timeout.tv_usec = 0;
	while (scanf("%s", s) != EOF) {
		clnt_stat = clnt_call(client, RENDERSTRING_BATCHED,
			xdr_wrapstring, &s, NULL, NULL, total_timeout);
		if (clnt_stat != RPC_SUCCESS) {
			clnt_perror(client, "batched rpc");
			exit(-1);
		}
	}
	/* now flush the pipeline
	 */
	total_timeout.tv_sec = 20;
	clnt_stat = clnt_call(client, NULLPROC, xdr_void, NULL,
		xdr_void, NULL, total_timeout);
	if (clnt_stat != RPC_SUCCESS) {
		clnt_perror(client, "rpc");
		exit(-1);
	}
	clnt_destroy(client);
}
.Ae
.sp
Because the server does not send a message,
a client cannot be notified of any failures that may occur.
Therefore, clients are on their own when it comes to handling errors.
.Pa
The previous example rendered
all of the (2000) lines in the file \f2/etc/termcap\fP.
The rendering service did nothing but throw the lines away.
The example was run in the following four configurations:
.Ls 1
.Li
Machine to itself, regular RPC
.Li
Machine to itself, batched RPC
.Li
Machine to another, regular RPC
.Li
Machine to another, batched RPC
.Le
The results are as follows:
.Ls 1
.Li
50 seconds
.Li
16 seconds
.Li
52 seconds
.Li
10 seconds
.Le
Running \f3fscanf()\fP on \f2/etc/termcap\fP
requires only six seconds.
Although protocols that allow for overlapped execution
are often hard to design, these timings show the advantage
of using them.
.Bh "Authentication"
In the examples presented so far,
the caller never identified itself to the server,
and the server never required an ID from the caller.
Clearly, some network services, such as a network filesystem,
require stronger security than what has been shown so far.
.Pa
In reality, every RPC call is authenticated by
the RPC package on the server.  Similarly,
the RPC client package generates and sends authentication parameters.
Just as different transports (TCP/IP or UDP/IP)
can be used when creating RPC clients and servers,
different forms of authentication can be associated with RPC clients.
The default authentication type is \f2none\fP.
.Pa
The authentication subsystem of the RPC package is open-ended.
That is, numerous types of authentication are easy to support.
However, this section deals just with UNIX-type 
authentication.  (Only 
\f2none\fP and UNIX-type authentication are supported.) 
.bp
.Ch "The Client Side"
When a caller creates a new RPC client handle, such as:
.Ps
clnt = clntudp_create(address, prognum, versnum,
		      wait, sockp)
.Pe
the appropriate transport instance defaults
the associated authentication handle to be:
.Ps
clnt->cl_auth = authnone_create();
.Pe
After creating the RPC client handle, the RPC client can choose to use UNIX-
style authentication by setting \f2clnt->cl_auth\fP as follows:
.Ps
clnt->cl_auth = authunix_create_default();
.Pe
This causes each RPC call associated with \f2clnt\fP
to carry the following authentication credentials structure:
.sp
.As
/*
 * Unix style credentials.
 */
struct authunix_parms {
	u_long 	aup_time;	/* credentials creation time */
	char *aup_machname;	/* host name where client is */
	int 	aup_uid;	/* client's UNIX effective uid */
	int 	aup_gid;	/* client's current group id */
	u_int	aup_len;	/* element length of aup_gids */
	int 	*aup_gids;	/* array of groups user is in */
};
.Ae
.sp
These fields are set by \f3authunix_create_default()\fP,
which invokes the appropriate system calls.
Because the RPC user created this new style of authentication,
the user is responsible for destroying it.  To do this, enter:  
.Ps
auth_destroy(clnt->cl_auth);
.Pe
This should be done, in all cases, to conserve memory.
.Ch "The Server Side"
Service implementors have a harder time dealing with authentication issues.
This is because the RPC package passes the service dispatch routine a request
that has an arbitrary authentication style associated with it.
Consider the fields of a request handle passed to a service dispatch routine:
.sp
.As
/*
 * An RPC Service request
 */
struct svc_req {
	u_long	rq_prog;		/* service program number */
	u_long	rq_vers;		/* service protocol vers num */
	u_long	rq_proc;		/* desired procedure number */
	struct opaque_auth
			rq_cred;		/* raw credentials from wire */
	caddr_t rq_clntcred;	/* credentials (read only) */
};
.Ae
.sp
\f3rq_cred\fP is mostly opaque, except for the field containing
the style of authentication credentials:
.sp
.As
/*
 * Authentication info.  Mostly opaque to the programmer.
 */
struct opaque_auth {
	enum_t	oa_flavor;	/* style of credentials */
	caddr_t	oa_base;	/* address of more auth stuff */
	u_int	oa_length;	/* not to exceed MAX_AUTH_BYTES */
};
.Ae
.sp
The RPC package guarantees the following
to the service dispatch routine:
.Ls 1
.Li
That the request's \f3rq_cred\fP
is well-formed.  The service implementor can inspect the request's
\f2rq_cred.oa_flavor\fP to determine which style of authentication
the caller used.
The service implementor might also want to inspect the other fields of
\f3rq_cred\fP
if the style is not supported by the RPC package.
.Li
That the request's \f2rq_clntcred\fP field is either
NULL or points to a well-formed structure
that corresponds to a supported style of authentication credentials.
Because only UNIX-type is currently supported, 
\f2rq_clntcred\fP should be cast to a pointer to an
\f2authunix_parms\fP structure.  If \f2rq_clntcred\fP
is NULL, the service implementor might want to inspect the other
(opaque) fields of \f3rq_cred\fP
in case the service knows about a new type of authentication
that is not familiar to the RPC package.
.Le
.Pa
Our example of a remote users service can be extended so that
it computes results for all users except UID 16:
.sp
.As
nuser(rqstp, tranp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct authunix_parms *unix_cred;
	int uid;
	unsigned long nusers;
.sp.5
	/*
	 * we don't care about authentication for null proc
	 */
	if (rqstp->rq_proc == NULLPROC) {
		if (!svc_sendreply(transp, xdr_void, 0)) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		 }
		 return;
	}
	/*
	 * now get the uid
	 */
	switch (rqstp->rq_cred.oa_flavor) {
	case AUTH_UNIX:
		unix_cred = (struct authunix_parms *)rqstp->rq_clntcred;
		uid = unix_cred->aup_uid;
		break;
	case AUTH_NULL:
	default:
		svcerr_weakauth(transp);
		return;
	}
	switch (rqstp->rq_proc) {
	case RUSERSPROC_NUM:
		/*
		 * make sure caller is allowed to call this proc
		 */
		if (uid == 16) {
			svcerr_systemerr(transp);
			return;
		}
		/*
		 * code here to compute the number of users
		 * and put in variable nusers
		 */
		if (!svc_sendreply(transp, xdr_u_long, &nusers) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		}
		return;
	default:
		svcerr_noproc(transp);
		return;
	}
}
.Ae
.sp
A few things should be noted here.
First, it is customary not to check
the authentication parameters associated with the
\f2NULLPROC\fP (procedure number zero).
Second, if the authentication parameter type is not suitable
for your service, you should call \f3svcerr_weakauth()\fP.
And finally, the service protocol itself should return status
for access denied.  In the case of our example, the protocol
does not have such a status, so we call the service primitive
\f3svcerr_systemerr()\fP instead.
.Pa
The last point underscores the relationship between
the RPC authentication package and the services.
RPC deals only with authentication and not with
individual service access control.
The services themselves must implement their own access control policies
and reflect these policies as return statuses in their protocols.
.Bh "Using Inetd"
An RPC server can be started from \f3inetd\fP.
Because \f3inet\fP passes a socket as file descripter 0, 
\f3svcudp_create()\fP should be called as:
.Ps
transp = svcudp_create(0);
.Pe
Also, \f3svc_register()\fP should be called as:
.Ps
svc_register(transp, PROGNUM, VERSNUM, service, 0);
.Pe
Because the program would already be registered by
\f3inetd\fP, the final flag should be 0.  Remember, if you want to exit
from the server process and return control to
\f3inet\fP, you must exit explicitly because
\f3svc_run()\fP never returns.
.Pa
The format of entries for RPC services in \f2/etc/servers\fP is:
.Ps
rpc udp \f2server \0program \0version\fP
.Pe
\f2server\fP is the C code implementing the server;
\f2program\fP and \f2version\fP
are the program and version numbers of the service.
For TPC-based RPC services, the word \f3udp\fP can be replaced 
by \f3tcp\fP.
.Pa
If the same program handles multiple versions,
then the version number can be a range,
as in this example:
.Ps
rpc udp /usr/etc/rstatd 100001 1-2
.Pe
.bp
.Ah "More Examples"
.Bh "Versions"
By convention, the first version number of program \f2PROG\fP
is \f2PROGVERS_ORIG\fP; the most recent version is
\f2PROGVERS\fP.
Suppose there is a new version of the \f2user\fP
program that returns an \f2unsigned short\fP
rather than a \f2long\fP.
If we name this version
\f2RUSERSVERS_SHORT\fP,
then a server that wants to support both versions
would do a double register:
.sp
.As
if (!svc_register(transp, RUSERSPROG, RUSERSVERS_ORIG,
  nuser, IPPROTO_TCP)) {
	fprintf(stderr, "can't register RUSER service\en");
	exit(1);
}
if (!svc_register(transp, RUSERSPROG, RUSERSVERS_SHORT,
  nuser, IPPROTO_TCP)) {
	fprintf(stderr, "can't register RUSER service\en");
	exit(1);
}
.Ae
.sp
Both versions can be handled by the same C procedure:
.sp
.As
nuser(rqstp, tranp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	unsigned long nusers;
	unsigned short nusers2
.sp.5
	switch (rqstp->rq_proc) {
	case NULLPROC:
		if (!svc_sendreply(transp, xdr_void, 0)) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		}
		return;
	case RUSERSPROC_NUM:
		/*
		 * code here to compute the number of users
		 * and put in variable nusers
		 */
		nusers2 = nusers;
		if (rqstp->rq_vers != RUSERSVERS_ORIG)
			return;
		if (!svc_sendreply(transp, xdr_u_long, &nusers) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		} else
		if (!svc_sendreply(transp, xdr_u_short, &nusers2) {
			fprintf(stderr, "can't reply to RPC call\en");
			exit(1);
		}
		return;
	default:
		svcerr_noproc(transp);
		return;
	}
}
.Ae
.sp
.Bh "TCP"
This example is essentially \f3rcp\fP.
The initiator of the RPC \f3snd()\fP
call takes its standard input and sends it to the server
\f3rcv()\fP, which prints it on standard output.
The RPC call uses TCP.
This example also illustrates an XDR procedure that behaves differently
on serialization than on deserialization.
.sp
.As
/*
 * The xdr routine:
 *		on decode, read from wire, write onto fp
 *		on encode, read from fp, write onto wire
 */
#include <stdio.h>
#include <rpc/rpc.h>
.sp.5
xdr_rcp(xdrs, fp)
	XDR *xdrs;
	FILE *fp;
{
	unsigned long size;
	char buf[BUFSIZ], *p;
.sp.5
	if (xdrs->x_op == XDR_FREE)/* nothing to free */
		return 1;
	while (1) {
		if (xdrs->x_op == XDR_ENCODE) {
			if ((size = fread(buf, sizeof(char), BUFSIZ,
			  fp)) == 0 && ferror(fp)) {
				fprintf(stderr, "can't fread\en");
				exit(1);
			}
		}
		p = buf;
		if (!xdr_bytes(xdrs, &p, &size, BUFSIZ))
			return 0;
		if (size == 0)
			return 1;
		if (xdrs->x_op == XDR_DECODE) {
			if (fwrite(buf, sizeof(char), size,
			  fp) != size) {
				fprintf(stderr, "can't fwrite\en");
				exit(1);
			}
		}
	}
}
.sp.5
/*
 * The sender routines
 */
#include <stdio.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/time.h>
.sp.5
main(argc, argv)
	int argc;
	char **argv;
{
	int err;
.sp.5
	if (argc < 2) {
		fprintf(stderr, "usage: %s servername\en", argv[0]);
		exit(-1);
	}
	if ((err = callrpctcp(argv[1], RCPPROG, RCPPROC_FP,
	  RCPVERS, xdr_rcp, stdin, xdr_void, 0) != 0)) {
		clnt_perrno(err);
		fprintf(stderr, "can't make RPC call\en");
		exit(1);
	}
}
.sp.5
callrpctcp(host, prognum, procnum, versnum,
           inproc, in, outproc, out)
	char *host, *in, *out;
	xdrproc_t inproc, outproc;
{
	struct sockaddr_in server_addr;
	int socket = RPC_ANYSOCK;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	register CLIENT *client;
	struct timeval total_timeout;
.sp.5
	if ((hp = gethostbyname(host)) == NULL) {
		fprintf(stderr, "can't get addr for '%s'\en", host);
		exit(-1);
	}
	bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr,
		hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;
	if ((client = clnttcp_create(&server_addr, prognum,
	  versnum, &socket, BUFSIZ, BUFSIZ)) == NULL) {
		perror("rpctcp_create");
		exit(-1);
	}
	total_timeout.tv_sec = 20;
	total_timeout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum,
		inproc, in, outproc, out, total_timeout);
	clnt_destroy(client)
	return (int)clnt_stat;
}
.sp.5
/*
 * The receiving routines
 */
#include <stdio.h>
#include <rpc/rpc.h>
.sp.5
main()
{
	register SVCXPRT *transp;
.sp.5
	if ((transp = svctcp_create(RPC_ANYSOCK,
	  BUFSIZ, BUFSIZ)) == NULL) {
		fprintf("svctcp_create: error\en");
		exit(1);
	}
	pmap_unset(RCPPROG, RCPVERS);
	if (!svc_register(transp,
	  RCPPROG, RCPVERS, rcp_service, IPPROTO_TCP)) {
		fprintf(stderr, "svc_register: error\en");
		exit(1);
	}
	svc_run();  /* never returns */
	fprintf(stderr, "svc_run should never return\en");
}
.sp.5
rcp_service(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	switch (rqstp->rq_proc) {
	case NULLPROC:
		if (svc_sendreply(transp, xdr_void, 0) == 0) {
			fprintf(stderr, "err: rcp_service");
			exit(1);
		}
		return;
	case RCPPROC_FP:
		if (!svc_getargs(transp, xdr_rcp, stdout)) {
			svcerr_decode(transp);
			return;
		}
		if (!svc_sendreply(transp, xdr_void, 0)) {
			fprintf(stderr, "can't reply\en");
			return;
		}
		exit(0);
	default:
		svcerr_noproc(transp);
		return;
	}
}
.Ae
.sp
.Bh "Callback Procedures"
Occasionally, you might want to have a server become a client
and then make a callback RPC call to the process that is its client.
For example, in remote debugging
the client can be a window system program
and the server can be a debugger running on the remote machine.
Most of the time,
the user clicks a mouse button at the debugging window,
which converts the click to a debugger command
and then makes an RPC call to the server
(where the debugger is actually running)
telling it to execute that command.
However, when the debugger hits a breakpoint, the roles are reversed.
The debugger wants to make an RPC call to the window program
so that it can inform the user that a breakpoint has been reached.
.Pa
To do an RPC callback,
you need a program number on which to make the RPC call. 
Because this will be a dynamically generated program number,
it should be in the transient range 0x40000000 - 0x5fffffff.
The routine \f3gettransient()\fP
returns a valid program number in this range
and registers it with the portmapper.
It talks only to the portmapper running on the same machine as the
\f3gettransient()\fP routine itself.  The call to
\f3pmap_set()\fP is a test and set operation,
in that it indivisibly tests whether a program number
has already been registered,
and if it has not, then reserves it.
On return, the \f2sockp\fP argument will contain a socket that can be used
as the argument to an \f3svcudp_create()\fP or
\f3svctcp_create()\fP call.
.sp
.As
#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
.sp.5
gettransient(proto, vers, sockp)
	int proto, vers, *sockp;
{
	static int prognum = 0x40000000;
	int s, len, socktype;
	struct sockaddr_in addr;
.sp.5
	switch(proto) {
		case IPPROTO_UDP:
			socktype = SOCK_DGRAM;
			break;
		case IPPROTO_TCP:
			socktype = SOCK_STREAM;
			break;
		default:
			fprintf(stderr, "unknown protocol type\en");
			return 0;
	}
	if (*sockp == RPC_ANYSOCK) {
		if ((s = socket(AF_INET, socktype, 0)) < 0) {
			perror("socket");
			return (0);
		}
		*sockp = s;
	}
	else
		s = *sockp;
	addr.sin_addr.s_addr = 0;
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	len = sizeof(addr);
	/*
	 * may be already bound, so don't check for error
	 */
	bind(s, &addr, len);
	if (getsockname(s, &addr, &len)< 0) {
		perror("getsockname");
		return (0);
	}
	while (!pmap_set(prognum++, vers, proto, addr.sin_port))
		continue;
	return (prognum-1);
}
.Ae
.sp
The following pair of programs illustrate how to use the
\f3gettransient()\fP routine.
The client makes an RPC call to the server,
passing it a transient program number.
Then the client waits to receive a callback
from the server at that program number.
The server registers the program
\f2EXAMPLEPROG\fP
so that it can receive an RPC call
informing it of the callback program number.
Then at some random time (on receiving an
\f2ALRM\fP signal in this example), the server sends a callback RPC call,
using the program number it received earlier.
.sp
.As
/*
 * client
 */
#include <stdio.h>
#include <rpc/rpc.h>
.sp.5
int callback();
char hostname[256];
.sp.5
main(argc, argv)
	char **argv;
{
	int x, ans, s;
	SVCXPRT *xprt;
.sp.5
	gethostname(hostname, sizeof(hostname));
	s = RPC_ANYSOCK;
	x = gettransient(IPPROTO_UDP, 1, &s);
	fprintf(stderr, "client gets prognum %d\en", x);
	if ((xprt = svcudp_create(s)) == NULL) {
	  fprintf(stderr, "rpc_server: svcudp_create\en");
		exit(1);
	}
	/* protocol is 0 - gettransient() does registering
	 */
	(void)svc_register(xprt, x, 1, callback, 0);
	ans = callrpc(hostname, EXAMPLEPROG, EXAMPLEVERS,
		EXAMPLEPROC_CALLBACK, xdr_int, &x, xdr_void, 0);
	if (ans != RPC_SUCCESS) {
		fprintf(stderr, "call: ");
		clnt_perrno(ans);
		fprintf(stderr, "\en");
	}
	svc_run();
	fprintf(stderr, "Error: svc_run shouldn't return\en");
}
.sp.5
callback(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	switch (rqstp->rq_proc) {
		case 0:
			if (!svc_sendreply(transp, xdr_void, 0)) {
				fprintf(stderr, "err: rusersd\en");
				exit(1);
			}
			exit(0);
		case 1:
			if (!svc_getargs(transp, xdr_void, 0)) {
				svcerr_decode(transp);
				exit(1);
			}
			fprintf(stderr, "client got callback\en");
			if (!svc_sendreply(transp, xdr_void, 0)) {
				fprintf(stderr, "err: rusersd");
				exit(1);
			}
	}
}
.sp.5
/*
 * server
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/signal.h>
.sp.5
char *getnewprog();
char hostname[256];
int docallback();
int pnum;		/* program number for callback routine */
.sp.5
main(argc, argv)
	char **argv;
{
	gethostname(hostname, sizeof(hostname));
	registerrpc(EXAMPLEPROG, EXAMPLEVERS,
	  EXAMPLEPROC_CALLBACK, getnewprog, xdr_int, xdr_void);
	fprintf(stderr, "server going into svc_run\en");
	signal(SIGALRM, docallback);
	alarm(10);
	svc_run();
	fprintf(stderr, "Error: svc_run shouldn't return\en");
}
.sp.5
char *
getnewprog(pnump)
	char *pnump;
{
	pnum = *(int *)pnump;
	return NULL;
}
.sp.5
docallback()
{
	int ans;
.sp.5
	ans = callrpc(hostname, pnum, 1, 1, xdr_void, 0,
		xdr_void, 0);
	if (ans != 0) {
		fprintf(stderr, "server: ");
		clnt_perrno(ans);
		fprintf(stderr, "\en");
	}
}
.Ae
.bp
.Ah "Synopsis of RPC Routines"
\f3auth_destroy()\fP
.sp
.As
void
auth_destroy(auth)
	AUTH *auth;
.Ae
.sp
A macro that destroys the authentication information associated with
\f2auth\fP.  Destruction usually involves deallocation
of private data structures.  After calling \f3auth_destroy()\fP, 
the use of \f2auth\fP is undefined. 
.sp 2
\f3authnone_create()\fP
.sp
.As
AUTH *
authnone_create()
.Ae
.sp
Creates and returns an RPC authentication handle that passes no
usable authentication information with each remote procedure call.
.sp 2
\f3authunix_create()\fP
.sp
.As
AUTH *
authunix_create(host, uid, gid, len, aup_gids)
	char *host;
	int uid, gid, len, *aup_gids;
.Ae
.sp
Creates and returns an RPC authentication handle that contains
UNIX authentication information.
The parameter \f2host\fP is the name of the machine on which the
information was created, \f2uid\fP is the appropriate user ID, and
\f2gid\fP is the user's current group ID.
\f2len\fP and \f2aup_gids\fP refer to a counted array of groups
to which the user belongs.
It is easy to impersonate a user.
.sp 2
\f3authunix_create\%_default()\fP
.sp
.As
AUTH *
authunix_create_default()
.Ae
.sp
Calls \f3authunix_create()\fP
with the appropriate parameters.
.sp 2
.bp
\f3callrpc()\fP
.sp
.As
callrpc(host,prognum,versnum,procnum,inproc,in,outproc,out)
	char *host;
	u_long prognum, versnum, procnum;
	char *in, *out;
	xdrproc_t inproc, outproc;
.Ae
.sp
Calls the remote procedure associated with
\f2prognum\fP, \f2versnum\fP, and \f2procnum\fP on the machine
\f2host\fP.  The parameter \f2in\fP
is the address of the procedure's argument(s), and
\f2out\fP is the address where the result(s) are to be placed.
\f2inproc\fP is used to encode the procedure's parameters, and
\f2outproc\fP is used to decode the procedure's results.
If this routine succeeds, it returns zero.  If it fails, the value of
\f2enum clnt_stat\fP cast to an integer is returned.
The routine \f3clnt_perrno()\fP can be used to translate
failure statuses into messages.
.Ns W
When calling remote procedures with this routine,
UDP/IP is used as a transport.  See
\f1clntudp_create()\fP for restrictions.
.Ne
.sp 2
\f3clnt_broadcast()\fP
.sp
.As
enum clnt_stat
clnt_broadcast(prognum, versnum, procnum,
  inproc, in, outproc, out, eachresult)
	u_long prognum, versnum, procnum;
	char *in, *out;
	xdrproc_t inproc, outproc;
	resultproc_t eachresult;
.Ae
.sp
This routine is similar to \f3callrpc()\fP, except the
call message is broadcast to
all locally-connected broadcast nets.
Each time this routine receives a response, it calls
\f3eachresult()\fP, whose form is:
.Ps
	eachresult(out, addr)
		char *out;
		struct sockaddr_in *addr;
.Pe
\f2out\fP is the same as \f2out\fP passed to
\f3clnt_broadcast()\fP,
except that the remote procedure output is decoded there.
\f2addr\fP points to the address of the machine that sent the results.  If
\f3eachresult()\fP returns zero, \f3clnt_broadcast()\fP
waits for more replies.
Otherwise, it returns with appropriate status.
.sp 2
\f3clnt_call()\fP
.sp
.As
enum clnt_stat
clnt_call(clnt, procnum, inproc, in, outproc, out, tout)
	CLIENT *clnt; long procnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
	struct timeval tout;
.Ae
.sp
A macro that calls the remote procedure \f2procnum\fP
associated with the client handle (\f2clnt\fP)
that is obtained with an RPC client-creation routine such as
\f3clntudp_create()\fP.  The parameter \f2in\fP
is the address of the procedure's argument(s), and
\f2out\fP is the address where the result(s) are to be placed.
\f2inproc\fP is used to encode the procedure's parameters, and
\f2outproc\fP is used to decode the procedure's results.
\f2tout\fP is the time allowed for results to come back.
.sp 2
\f3clnt_destroy()\fP
.Ps
clnt_destroy(clnt)
	CLIENT *clnt;
.Pe
A macro that destroys a client's RPC handle.
Destruction usually involves deallocation
of private data structures, including \f2clnt\fP
itself.  
After calling \f3clnt_destroy()\fP,
use of \f2clnt\fP is undefined. 
It is the user's responsibility to close sockets associated with
\f2clnt\fP. 
.sp 2
\f3clnt_freeres()\fP
.sp
.As
clnt_freeres(clnt, outproc, out)
	CLIENT *clnt;
	xdrproc_t outproc;
	char *out;
.Ae
.sp
A macro that frees any data allocated by the RPC/XDR system
when it decoded the results of an RPC call.
The parameter \f2out\fP is the address of the results, and
\f2outproc\fP is the XDR routine describing the results in simple primitives.
This routine returns one if the results were freed successfully, 
and zero otherwise.
.sp 2
.bp
\f3clnt_geterr()\fP
.sp
.As
void
clnt_geterr(clnt, errp)
	CLIENT *clnt;
	struct rpc_err *errp;
.Ae
.sp
A macro that copies the error structure out of the client handle
to the structure at address \f2errp\fP.
.sp 2
\f3clnt_pcreateerror()\fP
.sp
.As
void
clnt_pcreateerror(s)
	char *s;
.Ae
.sp
Prints a message to standard error indicating
why a client RPC handle could not be created.
The message is prepended with string \f2s\fP
and a colon.
Used after a \f3clntraw_create()\fP,
\f3clnttcp_create()\fP, or \f3clntudp_create()\fP call.
.sp 2
\f3clnt_perrno()\fP
.sp
.As
void
clnt_perrno(stat)
	enum clnt_stat stat;
.Ae
.sp
Prints a message to standard error corresponding
to the condition indicated by \f2stat\fP.  Used after
\f3callrpc()\fP.
.sp 2
\f3clnt_perror()\fP
.sp
.As
clnt_perror(clnt, s)
	CLIENT *clnt;
	char *s;
.Ae
.sp
Prints a message to standard error indicating why an RPC call failed.
\f2clnt\fP is the handle used to do the call.
The message is prepended with string
\f2s\fP and a colon.
Used after \f3clnt_call()\fP.
.sp 2
.bp
\f3clntraw_create()\fP
.sp
.As
CLIENT *
clntraw_create(prognum, versnum)
	u_long prognum, versnum;
.Ae
.sp
This routine creates a toy RPC client for the remote program
\f2prognum\fP, version \f2versnum\fP.
Because the transport used to pass messages to the service
is actually a buffer within the address space of the process,
the corresponding RPC server should live in the same address space 
(see \f3svcraw_create()\fP).
This allows simulation of RPC and acquisition of RPC overheads
(such as round trip times) without any kernel interference.
If this routine fails, NULL is returned.
.sp 2
\f3clnttcp_create()\fP
.sp
.As
CLIENT *
clnttcp_create(addr,prognum,versnum,sockp,sendsz,recvsz)
	struct sockaddr_in *addr;
	u_long prognum, versnum;
	int *sockp;
	u_int sendsz, recvsz;
.Ae
.sp
This routine creates an RPC client for the remote program
\f2prognum\fP, version \f2versnum\fP.  The client uses TCP/IP as a transport.
The remote program is located at Internet address \f2*addr\fP.
If \f2addr->sin_port\fP
is zero, then it is set to the actual port where the remote
program is listening (the remote \f3portmap\fP
service is consulted for this information).
The parameter \f2*sockp\fP is a socket.  If the parameter is
\f2RPC_ANYSOCK\fP, then this routine opens a new socket and sets
\f2*sockp\fP.  Since TCP-based RPC uses buffered I/O, you can 
use the parameters \f2sendsz\fP and \f2recvsz\fP to specify
the size of the send and receive buffers. 
Values of zero choose suitable defaults.
If this routine fails, NULL is returned.
.sp 2
\f3clntudp_create()\fP
.sp
.As
CLIENT *
clntudp_create(addr, prognum, versnum, wait, sockp)
	struct sockaddr_in *addr;
	u_long prognum, versnum;
	struct timeval wait;
	int *sockp;
.Ae
.sp
This routine creates an RPC client for the remote program
\f2prognum\fP, version \f2versnum\fP.
The client uses UDP/IP as a transport.
The remote program is located at Internet address
\f2*addr\fP.  If \f2addr->sin_port\fP
is zero, then it is set to the actual port where the remote
program is listening (the remote \f3portmap\fP
service is consulted for this information).
The parameter \f2*sockp\fP is a socket.  If it is
\f2RPC_ANYSOCK\fP, then this routine opens a new socket and sets
\f2*sockp\fP.
The UDP transport resends the call message in intervals of \f2wait\fP
time until a response is received or until the call times out.
The total time for the call to time out is specified by
\f3clnt_call()\fP.
.Ns W
Because UDP-based RPC messages can only hold up to 8 Kbytes
of encoded data, this transport cannot be used for procedures
that take large arguments or return huge results.
.Ne
.sp 2
\f3get_myaddress()\fP
.sp
.As
void
get_myaddress(addr)
	struct sockaddr_in *addr;
.Ae
.sp
This routine stuffs the machine's IP address into \f2*addr\fP
without consulting the library routines that deal with
\f2/etc/hosts\fP.
The port number is always set to \f2htons(PMAPPORT)\fP.
.sp 2
\f3pmap_getmaps()\fP
.sp
.As
struct pmaplist *
pmap_getmaps(addr)
	struct sockaddr_in *addr;
.Ae
.sp
A user interface to the \f3portmap\fP
service, this routine returns a list of the current RPC program-to-port mappings
on the host located at IP address \f2*addr\fP.
This routine can return NULL.
The command \f3rpcinfo -p\fP uses this routine.
.sp 2
.bp
\f3pmap_getport()\fP
.sp
.As
u_short
pmap_getport(addr, prognum, versnum, protocol)
	struct sockaddr_in *addr;
	u_long prognum, versnum, protocol;
.Ae
.sp
A user interface to the \f3portmap\fP
service, this routine returns the port number
for a service that supports program number
\f2prognum\fP, version \f2versnum\fP,
and that speaks the transport protocol associated with \f2protocol\fP.
A return value of zero means that the mapping does not exist or that
the RPC system failed to contact the remote \f3portmap\fP
service.  In the latter case, the global variable
\f2rpc_createerr\fP contains the RPC status.
.sp 2
\f3pmap_rmtcall()\fP
.sp
.As
enum clnt_stat
pmap_rmtcall(addr, prognum, versnum, procnum,
  inproc, in, outproc, out, tout, portp)
	struct sockaddr_in *addr;
	u_long prognum, versnum, procnum;
	char *in, *out;
	xdrproc_t inproc, outproc;
	struct timeval tout;
	u_long *portp;
.Ae
.sp
A user interface to the \f3portmap\fP
service, this routine instructs \f3portmap\fP
on the host at IP address \f2*addr\fP
to make an RPC call on your behalf to a procedure on that host.
If the procedure succeeds, the parameter \f2*portp\fP
will be modified to the program's port number. 
The definitions of other parameters are discussed in
\f3callrpc()\fP and \f3clnt_call()\fP.
This procedure should be used for a \*Qping\*U and nothing else.
Also see \f3clnt_broadcast()\fP.
.sp 2
\f3pmap_set()\fP
.sp
.As
pmap_set(prognum, versnum, protocol, port)
	u_long prognum, versnum, protocol;
	u_short port;
.Ae
.sp
A user interface to the \f3portmap\fP
service, this routine establishes a mapping between the triple
[\f2prognum,versnum,protocol\fP]
and \f2port\fP on the machine's \f3portmap\fP
service.  The value of \f2protocol\fP is most likely
\f2IPPROTO_UDP\fP or \f2IPPROTO_TCP\fP.
This routine returns one if it succeeds, zero otherwise.
It is done automatically by \f3svc_register()\fP.
.sp 2
\f3pmap_unset()\fP
.sp
.As
pmap_unset(prognum, versnum)
	u_long prognum, versnum;
.Ae
.sp
A user interface to the \f3portmap\fP service,
this routine destroys all mappings between the triple
[\f2prognum,versnum,*\fP] and \f2port\fP
on the machine's \f3portmap\fP service.
This routine returns one if it succeeds, zero otherwise.
.sp 2
\f3registerrpc()\fP
.sp
.As
registerrpc(prognum,versnum,procnum,procname,inproc,outproc)
	u_long prognum, versnum, procnum;
	char *(*procname)();
	xdrproc_t inproc, outproc;
.Ae
.sp
Registers procedure \f2procname\fP
with the RPC service package.  If a request arrives for program
\f2prognum\fP, version \f2versnum\fP, and procedure \f2procnum\fP,
\f2procname\fP is called with a pointer to its parameter(s).
\f2progname\fP should return a pointer to its static result(s).
\f2inproc\fP is used to decode the parameters; 
\f2outproc\fP is used to encode the results.
This routine returns zero if the registration succeeded, \-1 otherwise.
.Ns W
Remote procedures registered in this form
are accessed using the UDP/IP transport.  See
\f1svcudp_create()\fP for restrictions.
.Ne
.sp 1
\f3rpc_createerr\fP
.sp
.As
struct rpc_createerr	rpc_createerr;
.Ae
.sp
A global variable whose value is set by any RPC client-creation routine
that does not succeed.  Use the routine \f3clnt_pcreateerror()\fP
to print the explanation.
.sp 2
\f3svc_destroy()\fP
.sp
.As
svc_destroy(xprt)
	SVCXPRT *xprt;
.Ae
.sp
A macro that destroys the RPC service transport handle (\f2xprt\fP).
Destruction usually involves deallocation
of private data structures, including \f2xprt\fP
itself.  After calling this routine, use of \f2xprt\fP is undefined. 
.sp 2
\f3svc_fds\fP
.sp
.As
int	svc_fds;
.Ae
.sp
A global variable reflecting the 
read file descriptor bit mask on the RPC service side.  
It is suitable as a parameter to the
\f3select()\fP system call.  This is of interest only
if a service implementor does not call \f3svc_run()\fP,
but rather does his own asynchronous event processing.
This variable is read-only (do not pass its address to
\f3select())\fP. However, it can change after calls to
\f3svc_getreq()\fP or to any creation routines.
.sp 2
\f3svc_freeargs()\fP
.sp
.As
svc_freeargs(xprt, inproc, in)
	SVCXPRT *xprt;
	xdrproc_t inproc;
	char *in;
.Ae
.sp
A macro that frees any data allocated by the RPC/XDR system
when it used \f3svc_getargs()\fP to decode the arguments to a 
service procedure. 
This routine returns one if the results were successfully freed,
and zero otherwise.
.sp 2
\f3svc_getargs()\fP
.sp
.As
svc_getargs(xprt, inproc, in)
	SVCXPRT *xprt;
	xdrproc_t inproc;
	char *in;
.Ae
.sp
A macro that decodes the arguments of an RPC request
associated with the RPC service transport handle
(\f2xprt\fP).  The parameter \f2in\fP
is the address where the arguments will be placed;
\f2inproc\fP is the XDR routine used to decode the arguments.
This routine returns one if decoding succeeds, and zero otherwise.
.bp 
\f3svc_getcaller()\fP
.sp
.As
struct sockaddr_in
svc_getcaller(xprt)
	SVCXPRT *xprt;
.Ae
.sp
The approved way to obtain the network address of the caller
of a procedure associated with the RPC service transport handle
(\f2xprt\fP).
.sp 2
\f3svc_getreq()\fP
.sp
.As
svc_getreq(rdfds)
	int rdfds;
.Ae
.sp
This routine is of interest only if a service implementor does not call
\f3svc_run()\fP,
but instead implements custom asynchronous event processing.
It is called when the \f3select()\fP
system call has determined that an RPC request
has arrived on some RPC socket(s).
\f2rdfds\fP is the resultant read file descriptor bit mask.
The routine returns when all sockets associated with the value of
\f2rdfds\fP have been serviced.
.sp 2
\f3svc_register()\fP
.sp
.As
svc_register(xprt, prognum, versnum, dispatch, protocol)
	SVCXPRT *xprt;
	u_long prognum, versnum;
	void (*dispatch)();
	u_long protocol;
.Ae
.sp
Associates \f2prognum\fP and \f2versnum\fP
with the service dispatch procedure, \f3dispatch()\fP.
If \f2protocol\fP is zero, the service is not registered with the
\f3portmap\fP service.  If \f2protocol\fP
is non-zero, then a mapping of the triple
[\f2prognum,versnum,protocol\fP] to
\f2xprt->xp_port\fP is established with the local
\f3portmap\fP service (\f2protocol\fP is usually
zero, \f2IPPROTO_UDP\fP or \f2IPPROTO_TCP)\fP.
The procedure \f3dispatch()\fP has the following form:
.Ps
	dispatch(request, xprt)
		struct svc_req *request;
		SVCXPRT *xprt;
.Pe
If the \f3svc_register()\fP routine succeeds, one is returned.  If it
fails, zero is returned.
.sp 2
.bp
\f3svc_run()\fP
.sp
.As
svc_run()
.Ae
.sp
This routine never returns.  It waits for RPC requests to arrive,
and calls the appropriate service procedure (using \f3svc_getreq()\fP)
when one arrives.  This procedure is usually waiting for a
\f3select()\fP system call to return.
.sp 2
\f3svc_sendreply()\fP
.sp
.As
svc_sendreply(xprt, outproc, out)
	SVCXPRT *xprt;
	xdrproc_t outproc;
	char *out;
.Ae
.sp
Called by an RPC service dispatch routine
to send the results of a remote procedure call.
The parameter \f2xprt\fP is the caller's transport handle.
\f2outproc\fP is the XDR routine that is used to encode the results, and
\f2out\fP is the address of the results.
This routine returns one if it succeeds, zero otherwise.
.sp 2
\f3svc_unregister()\fP
.sp
.As
void
svc_unregister(prognum, versnum)
	u_long prognum, versnum;
.Ae
.sp
Removes all mapping of the double [\f2prognum,versnum\fP]
to dispatch routines, and of the triple [\f2prognum,versnum,*\fP]
to port number.
.sp 2
\f3svcerr_auth()\fP
.sp
.As
void
svcerr_auth(xprt, why)
	SVCXPRT *xprt;
	enum auth_stat why;
.Ae
.sp
Called by a service dispatch routine that refuses to perform
a remote procedure call due to an authentication error.
.sp 2
.bp
\f3svcerr_decode()\fP
.sp
.As
void
svcerr_decode(xprt)
	SVCXPRT *xprt;
.Ae
.sp
Called by a service dispatch routine that can't successfully
decode its parameters.  (Also see \f3svc_getargs()\fP.)
.sp 2
\f3svcerr_noproc()\fP
.sp
.As
void
svcerr_noproc(xprt)
	SVCXPRT *xprt;
.Ae
.sp
Called by a service dispatch routine that can't implement
the procedure number the caller requested.
.sp 2
\f3svcerr_noprog()\fP
.sp
.As
void
svcerr_noprog(xprt)
	SVCXPRT *xprt;
.Ae
.sp
Called when the desired program is not registered with the RPC package.
Service implementors usually don't need this routine.
.sp 2
\f3svcerr_progvers()\fP
.sp
.As
void
svcerr_progvers(xprt)
	SVCXPRT *xprt;
.Ae
.sp
Called when the desired version of a program is not registered
with the RPC package.
Service implementors usually don't need this routine.
.sp 2
\f3svcerr_systemerr()\fP
.sp
.As
void
svcerr_systemerr(xprt)
	SVCXPRT *xprt;
.Ae
.sp
Called by a service dispatch routine when it detects a system error
not covered by any particular protocol.
For example, if a service can no longer allocate storage,
it can call this routine.
.sp 2
\f3svcerr_weakauth()\fP
.sp
.As
void
svcerr_weakauth(xprt)
	SVCXPRT *xprt;
.Ae
.sp
Called by a service dispatch routine that refuses to perform
a remote procedure call due to insufficient (but correct)
authentication parameters.  The routine calls
\f3svcerr_auth\f2(xprt,AUTH_TOOWEAK)\f1.
.sp 2
\f3svcraw_create()\fP
.sp
.As
SVCXPRT *
svcraw_create()
.Ae
.sp
This routine creates a toy RPC service transport
to which it returns a pointer.  Because the transport
is really a buffer within the process address space,
the corresponding RPC client should live in the same address space (see
\f3clntraw_create()\fP).
This routine allows simulation of RPC and acquisition of RPC overheads
(such as round trip times), without any kernel interference.
If this routine fails, NULL is returned.
.sp 2
\f3svctcp_create()\fP
.sp
.As
SVCXPRT *
svctcp_create(sock, send_buf_size, recv_buf_size)
	int sock;
	u_int send_buf_size, recv_buf_size;
.Ae
.sp
This routine creates a TCP/IP-based RPC service transport
to which it returns a pointer.
The transport is associated with the socket
\f2sock\fP, which can be \f2RPC_ANYSOCK\fP
(in which case a new socket is created).
If the socket is not bound to a local TCP port, this routine
will bind it to an arbitrary port.  Upon completion of the routine,
\f2xprt->xp_sock\fP is the socket number of the transport, and
\f2xprt->xp_port\fP is the port number of the transport.
If this routine fails, NULL
is returned.  Since TCP-based RPC uses buffered I/O,
users can specify the size of the \f2send\fP
and \f2receive\fP buffers.  Values of zero choose suitable defaults.
.sp 2
\f3svcudp_create()\fP
.sp
.As
SVCXPRT *
svcudp_create(sock)
	int sock;
.Ae
.sp
This routine creates a UDP/IP-based RPC service transport
to which it returns a pointer.
The transport is associated with the socket
\f2sock\fP, which can be \f2RPC_ANYSOCK\fP
(in which case a new socket is created).
If the socket is not bound to a local UDP port, this routine
will bind it to an arbitrary port.  Upon completion of the routine,
\f2xprt->xp_sock\fP is the socket number of the transport, and
\f2xprt->xp_port\fP is the port number of the transport.
If this routine fails, NULL is returned.
.Ns W
Because UDP-based RPC messages can only hold up to 8 Kbytes
of encoded data, this transport cannot be used for procedures
that take large arguments or return huge results.
.Ne
.sp 2
\f3xdr_accepted_reply()\fP
.sp
.As
xdr_accepted_reply(xdrs, ar)
	XDR *xdrs;
	struct accepted_reply *ar;
.Ae
.sp
Used for describing RPC messages externally,
this routine is helpful for users wanting to generate
RPC-style messages without using the RPC package.
.sp 2
\f3xdr_array()\fP
.sp
.As
xdr_array(xdrs, arrp, sizep, maxsize, elsize, elproc)
	XDR *xdrs;
	char **arrp;
	u_int *sizep, maxsize, elsize;
	xdrproc_t elproc;
.Ae
.sp
A filter primitive that translates between arrays
and their corresponding external representations.
The parameter \f2arrp\fP
is the address of the pointer to the array, while
\f2sizep\fP is the address of the element count of the array.
The element count cannot exceed \f2maxsize\fP.
The parameter \f2elsize\fP is the \f3sizeof()\fP
each of the array's elements, and \f2elproc\fP
is an XDR filter that translates between the C form of
the array elements and their external representation.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_authunix_parms()\fP
.sp
.As
xdr_authunix_parms(xdrs, aupp)
	XDR *xdrs;
	struct authunix_parms *aupp;
.Ae
.sp
Used for describing UNIX credentials externally,
this routine is helpful for users wanting to generate
these credentials without using the RPC authentication package.
.sp 2
\f3xdr_bool()\fP
.sp
.As
xdr_bool(xdrs, bp)
	XDR *xdrs;
	bool_t *bp;
.Ae
.sp
A filter primitive that translates between booleans (C integers)
and their external representations.
When encoding data, this filter produces values of either one or zero.
If this routine succeeds, one is returned. If it fails, zero is returned.
.sp 2
\f3xdr_bytes()\fP
.sp
.As
xdr_bytes(xdrs, sp, sizep, maxsize)
	XDR *xdrs;
	char **sp;
	u_int *sizep, maxsize;
.Ae
.sp
A filter primitive that translates between counted byte strings
and their external representations.
The parameter \f2sp\fP is the address of the string pointer.
The length of the string is located at address
\f2sizep\fP; strings cannot be longer than \f2maxsize\fP.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
.bp
\f3xdr_callhdr()\fP
.sp
.As
void
xdr_callhdr(xdrs, chdr)
	XDR *xdrs;
	struct rpc_msg *chdr;
.Ae
.sp
Used for describing RPC messages externally,
this routine is helpful for users wanting to generate
RPC-style messages without using the RPC package.
.sp 2
\f3xdr_callmsg()\fP
.sp
.As
xdr_callmsg(xdrs, cmsg)
	XDR *xdrs;
	struct rpc_msg *cmsg;
.Ae
.sp
Used for describing RPC messages externally,
this routine is helpful for users wanting to generate
RPC-style messages without using the RPC package.
.sp 2
\f3xdr_double()\fP
.sp
.As
xdr_double(xdrs, dp)
	XDR *xdrs;
	double *dp;
.Ae
.sp
A filter primitive that translates between C \f2double\fP
precision numbers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_enum()\fP
.sp
.As
xdr_enum(xdrs, ep)
	XDR *xdrs;
	enum_t *ep;
.Ae
.sp
A filter primitive that translates between C \f2enum s\fP
(actually integers) and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_float()\fP
..sps
.As
xdr_float(xdrs, fp)
	XDR *xdrs;
	float *fp;
.Ae
.sp
A filter primitive that translates between C \f2float s\fP
and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_inline()\fP
.sp
.As
long *
xdr_inline(xdrs, len)
	XDR *xdrs;
	int len;
.Ae
.sp
A macro that invokes the in-line routine associated with the XDR stream
\f2xdrs\fP.  The routine returns a pointer
to a contiguous piece of the stream's buffer.
\f2len\fP is the byte length of the desired buffer.
The pointer is cast to \f2long *\fP.
.Ns W
\f1xdr_inline()\fP might return NULL
(0) if it cannot allocate a contiguous piece of a buffer.
Therefore its behavior might vary among stream instances;
it exists for the sake of efficiency.
.Ne
.sp 2
\f3xdr_int()\fP
.sp
.As
xdr_int(xdrs, ip)
	XDR *xdrs;
	int *ip;
.Ae
.sp
A filter primitive that translates between C integers
and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_long()\fP
.sp
.As
xdr_long(xdrs, lp)
	XDR *xdrs;
	long *lp;
.Ae
.sp
A filter primitive that translates between C \f2long\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_opaque()\fP
.sp
.As
xdr_opaque(xdrs, cp, cnt)
	XDR *xdrs;
	char *cp;
	u_int cnt;
.Ae
.sp
This routine is a filter primitive that 
translates between fixed size opaque data
and its external representation.  The parameter \f2cp\fP
is the address of the opaque object, and \f2cnt\fP
is its size in bytes.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_opaque_auth()\fP
.sp
.As
xdr_opaque_auth(xdrs, ap)
	XDR *xdrs;
	struct opaque_auth *ap;
.Ae
.sp
Used for describing RPC messages externally,
this routine is helpful for users wanting to generate
RPC-style messages without using the RPC package.
.sp 2
\f3xdr_pmap()\fP
.sp
.As
xdr_pmap(xdrs, regs)
	XDR *xdrs;
	struct pmap *regs;
.Ae
.sp
Used for describing parameters externally to various \f3portmap\fP
procedures,
this routine is helpful for users wanting to generate
these parameters without using the \f3pmap\fP interface.
.sp 2
\f3xdr_pmaplist()\fP
.sp
.As
xdr_pmaplist(xdrs, rp)
	XDR *xdrs;
	struct pmaplist **rp;
.Ae
.sp
Used for describing a list of port mappings externally,
this routine is helpful for users wanting to generate
these parameters without using the \f3pmap\fP interface.
.sp 2
\f3xdr_reference()\fP
.sp
.As
xdr_reference(xdrs, pp, size, proc)
	XDR *xdrs;
	char **pp;
	u_int size;
	xdrproc_t proc;
.Ae
.sp
A primitive that provides pointer chasing within structures.
The parameter \f2pp\fP is the address of the pointer,
\f2size\fP is the \f3sizeof()\fP the structure that
\f2*pp\fP points to, and \f2proc\fP is an XDR procedure
that filters the structure
between its C form and its external representation.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_rejected_reply()\fP
.sp
.As
xdr_rejected_reply(xdrs, rr)
	XDR *xdrs;
	struct rejected_reply *rr;
.Ae
.sp
Used for describing RPC messages externally,
this routine is helpful for users wanting to generate
RPC-style messages without using the RPC package.
.sp 2
\f3xdr_replymsg()\fP
.sp
.As
xdr_replymsg(xdrs, rmsg)
	XDR *xdrs;
	struct rpc_msg *rmsg;
.Ae
.sp
Used for describing RPC messages externally,
this routine is helpful for users wanting to generate
RPC style messages without using the RPC package.
.sp 2
\f3xdr_short()\fP
.sp
.As
xdr_short(xdrs, sp)
	XDR *xdrs;
	short *sp;
.Ae
.sp
A filter primitive that translates between C \f2short\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_string()\fP
.sp
.As
xdr_string(xdrs, sp, maxsize)
	XDR *xdrs;
	char **sp;
	u_int maxsize;
.Ae
.sp
A filter primitive that translates between C strings and their
corresponding external representations.
Strings cannot be longer than \f2maxsize\fP.  Note that
\f2sp\fP is the address of the string's pointer.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_u_int()\fP
.sp
.As
xdr_u_int(xdrs, up)
	XDR *xdrs;
	unsigned *up;
.Ae
.sp
A filter primitive that translates between C \f2unsigned\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_u_long()\fP
.sp
.As
xdr_u_long(xdrs, ulp)
	XDR *xdrs;
	unsigned long *ulp;
.Ae
.sp
A filter primitive that translates between C \f2unsigned long\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_u_short()\fP
.sp
.As
xdr_u_short(xdrs, usp)
	XDR *xdrs;
	unsigned short *usp;
.Ae
.sp
A filter primitive that translates between C \f2unsigned short\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_union()\fP
.sp
.As
xdr_union(xdrs, dscmp, unp, choices, dfault)
	XDR *xdrs;
	int *dscmp;
	char *unp;
	struct xdr_discrim *choices;
	xdrproc_t dfault;
.Ae
.sp
A filter primitive that translates between a discriminated C
\f2union\fP and its corresponding external representation.  The parameter
\f2dscmp\fP is the address of the union's discriminant, while
\f2unp\fP in the address of the union.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_void()\fP
.sp
.As
xdr_void()
.Ae
.sp
This routine always returns one.
.sp 2
\f3xdr_wrapstring()\fP
.sp
.As
xdr_wrapstring(xdrs, sp)
	XDR *xdrs;
	char **sp;
.Ae
.sp
A primitive that calls \f3xdr_string\f2(xdrs,sp,MAXUNSIGNED)\f1,
where \f2MAXUNSIGNED\fP
is the maximum value of an unsigned integer.
This is handy because the RPC package passes
only two parameters to XDR routines, but \f3xdr_string()\fP
(one of the most frequently used primitives) requires three parameters.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xprt_register()\fP
.sp
.As
void
xprt_register(xprt)
	SVCXPRT *xprt;
.Ae
.sp
After RPC service transport handles are created,
they should register themselves with the RPC service package.
This routine modifies the global variable \f2svc_fds\fP.
Service implementors usually don't need this routine.
.sp 2
\f3xprt_unregister()\fP
.sp
.As
void
xprt_unregister(xprt)
	SVCXPRT *xprt;
.Ae
.sp
Before an RPC service transport handle is destroyed,
it should unregister itself with the RPC service package.
This routine modifies the global variable \f2svc_fds\fP.
Service implementors usually don't need this routine.
