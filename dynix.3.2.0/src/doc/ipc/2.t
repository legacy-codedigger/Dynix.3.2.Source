.\" $Copyright:	$
.\" Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
.\" Sequent Computer Systems, Inc.   All rights reserved.
.\"  
.\" This software is furnished under a license and may be used
.\" only in accordance with the terms of that license and with the
.\" inclusion of the above copyright notice.   This software may not
.\" be provided or otherwise made available to, or used by, any
.\" other person.  No title to or ownership of the software is
.\" hereby transferred.
...
. \" $Header: 2.t 1.5 86/03/06 $
.ds RH "Basics
.bp
.nr H1 2
.nr H2 0
.bp
.LG
.B
.ce
2. BASICS
.sp 2
.R
.NL
.PP
The basic building block for communication is the \f2socket\fP.
A socket is an endpoint of communication to which a name may
be \f2bound\fP.  Each socket in use has a \f2type\fP
and one or more associated processes.  Sockets exist within
\f2communication domains\fP.  
A communication domain is an
abstraction introduced to bundle common properties of
processes communicating through sockets.
One such property is the scheme used to name sockets.  For
example, in the UNIX communication domain sockets are
named with UNIX path names; e.g. a
socket may be named \*(lq/dev/foo\*(rq.  Sockets normally
exchange data only with
sockets in the same domain (it may be possible to cross domain
boundaries, but only if some translation process is
performed).  The
4.2BSD ipc supports two separate communication domains:
the UNIX domain, and the Internet domain is used by
processes which communicate
using the DARPA standard communication protocols. 
The underlying communication
facilities provided by these domains have a significant influence
on the internal system implementation as well as the interface to
socket facilities available to a user.  An example of the
latter is that a socket \*(lqoperating\*(rq in the UNIX domain
sees a subset of the possible error conditions which are possible
when operating in the Internet domain.
.NH 2
Socket types
.PP
Sockets are
typed according to the communication properties visible to a
user. 
Processes are presumed to communicate only between sockets of
the same type, although there is
nothing that prevents communication between sockets of different
types should the underlying communication
protocols support this.
.PP
Three types of sockets currently are available to a user.
A \f2stream\fP socket provides for the bidirectional, reliable,
sequenced, and unduplicated flow of data without record boundaries.
Aside from the bidirectionality of data flow, a pair of connected
stream sockets provides an interface nearly identical to that of pipes*.
.FS
* In the UNIX domain, in fact, the semantics are identical and,
as one might expect, pipes have been implemented internally
as simply a pair of connected stream sockets.
.FE
.PP
A \f2datagram\fP socket supports bidirectional flow of data which
is not promised to be sequenced, reliable, or unduplicated. 
That is, a process
receiving messages on a datagram socket may find messages duplicated, 
and, possibly,
in an order different from the order in which it was sent. 
An important characteristic of a datagram
socket is that record boundaries in data are preserved.  Datagram
sockets closely model the facilities found in many contemporary
packet switched networks such as the Ethernet.
.PP
A \f2raw\fP socket provides users access to
the underlying communication
protocols which support socket abstractions.
These sockets are normally datagram oriented, though their
exact characteristics are dependent on the interface provided by
the protocol.  Raw sockets are not intended for the general user; they
have been provided mainly for those interested in developing new 
communication protocols, or for gaining access to some of the more
esoteric facilities of an existing protocol.  The use of raw sockets
is considered in section 5.
.PP
Two potential socket types which have interesting properties are
the \f2sequenced packet\fP socket and the \f2reliably delivered
message\fP socket.  A sequenced packet socket is identical to
a stream socket
with the exception that record boundaries are preserved.  This interface
is very similar to that provided by the Xerox NS Sequenced Packet protocol.
The reliably delivered message socket has
similar properties to a datagram socket, but with
reliable delivery.  While these two socket types have been loosely defined,
they are currently unimplemented in 4.2BSD.  As such, in this
document we will concern ourselves
only with the three socket types for which support exists.
.NH 2
Socket creation
.PP
To create a socket the \f2socket\fP system call is used:
.DS
s = socket(domain, type, protocol);
.DE
This call requests that the system create a socket in the specified
\f2domain\fP and of the specified \f2type\fP.  A particular protocol may
also be requested.  If the protocol is left unspecified (a value
of 0), the system will select an appropriate protocol from those
protocols which comprise the communication domain and which
may be used to support the requested socket type.  The user is
returned a descriptor (a small integer number) which may be used
in later system calls which operate on sockets.  The domain is specified as
one of the manifest constants defined in the file <\f2sys/socket.h\fP>.
For the UNIX domain the constant is AF_UNIX*;  for the Internet
.FS
* The manifest constants are named AF_whatever as they indicate
the ``address format'' to use in interpreting names.
.FE
domain AF_INET.  The socket types are also defined in this file
and one of SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW must be specified.
To create a stream socket in the Internet domain the following
call might be used:
.DS
s = socket(AF_INET, SOCK_STREAM, 0);
.DE
This call would result in a stream socket being created with the TCP
protocol providing the underlying communication support.  To
create a datagram socket for on-machine use a sample call might
be:
.DS
s = socket(AF_UNIX, SOCK_DGRAM, 0);
.DE
.PP
To obtain a particular protocol one selects the protocol number,
as defined within the communication domain.  For the Internet
domain the available protocols are defined in <\f2netinet/in.h\fP>
or, better yet, one may use one of the library routines
discussed in section 3, such as  \f2getprotobyname\fP:
.DS
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
 ...
pp = getprotobyname("tcp");
s = socket(AF_INET, SOCK_STREAM, pp->p_proto);
.DE
.PP
There are several reasons a socket call may fail.  Aside from
the rare occurrence of lack of memory (ENOBUFS), a socket
request may fail due to a request for an unknown protocol
(EPROTONOSUPPORT), or a request for a type of socket for
which there is no supporting protocol (EPROTOTYPE). 
.NH 2
Binding names
.PP
A socket is created without a name.  Until a name is bound
to a socket, processes have no way to reference it and, consequently,
no messages may be received on it.  The \f2bind\fP call is used to
assign a name to a socket:
.DS
bind(s, name, namelen);
.DE
The bound name is a variable length byte string which is interpreted
by the supporting protocol(s).  Its interpretation may vary from
communication domain to communication domain (this is one of
the properties which comprise the \*(lqdomain\*(rq).  In the
UNIX domain names are path names while in the Internet domain
names contain an Internet address and port number.
If one wanted to bind the name \*(lq/dev/foo\*(rq to
a UNIX domain socket, the following would be used:
.DS
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

struct sockaddr_un seq;
 ...
seq.sun_family = AF_UNIX;
strcpy(seq.sun_path,"/dev/foo");
 ...
bind(s, &seq, sizeof (seq) \- 1);
.DE
(Note how the null byte in the name is not counted as part of
the name.)  In binding an Internet address things become more
complicated.  The actual call is simple,
.DS
#include <sys/types.h>
#include <netinet/in.h>
 ...
struct sockaddr_in sin;
 ...
bind(s, &sin, sizeof (sin));
.DE
but the selection of what to place in the address \f2sin\fP
requires some discussion.  We will come back to the problem
of formulating Internet addresses in section 3 when 
the library routines used in name resolution are discussed.
.NH 2
Connection establishment
.PP
With a bound socket it is possible to rendezvous with
an unrelated process.  This operation is usually asymmetric
with one process a \*(lqclient\*(rq and the other a \*(lqserver\*(rq.
The client requests services from the server by initiating a
\*(lqconnection\*(rq to the server's socket.  The server, when
willing to offer its advertised services, passively \*(lqlistens\*(rq
on its socket.  On the client side the \f2connect\fP call is
used to initiate a connection.  Using the UNIX domain, this
might appear as,
.DS
connect(s, "server-name", sizeof ("server-name"));
.DE
while in the Internet domain,
.DS
struct sockaddr_in server;
connect(s, &server, sizeof (server));
.DE
If the client process's socket is unbound at the time of
the connect call,
the system will automatically select and bind a name to
the socket; c.f. section 5.4.
An error is returned when the connection was unsuccessful
(any name automatically bound by the system, however, remains).
Otherwise, the socket is associated with the server and
data transfer may begin.
.PP
Many errors can be returned when a connection attempt
fails.  The most common are:
.IP ETIMEDOUT
.br
After failing to establish a connection for a period of time,
the system decided there was no point in retrying the
connection attempt any more.  This usually occurs because
the destination host is down, or because problems in
the network resulted in transmissions being lost.
.IP ECONNREFUSED
.br
The host refused service for some reason.  When connecting
to a host running 4.2BSD this is usually
due to a server process
not being present at the requested name.
.IP "ENETDOWN or EHOSTDOWN"
.br
These operational errors are 
returned based on status information delivered to
the client host by the underlying communication services.
.IP "ENETUNREACH or EHOSTUNREACH"
.br
These operational errors can occur either because the network
or host is unknown (no route to the network or host is present),
or because of status information returned by intermediate
gateways or switching nodes.  Many times the status returned
is not sufficient to distinguish a network being down from a
host being down.  In these cases the system is conservative and
indicates the entire network is unreachable.
.PP
For the server to receive a client's connection it must perform
two steps after binding its socket.
The first is to indicate a willingness to listen for
incoming connection requests:
.DS
listen(s, 5);
.DE
The second parameter to the \f2listen\fP call specifies the maximum
number of outstanding connections which may be queued awaiting 
acceptance by the server process.  Should a connection be
requested while the queue is full, the connection will not be
refused, but rather the individual messages which comprise the
request will be ignored.  This gives a harried server time to 
make room in its pending connection queue while the client
retries the connection request.  Had the connection been returned
with the ECONNREFUSED error, the client would be unable to tell
if the server was up or not.  As it is now it is still possible
to get the ETIMEDOUT error back, though this is unlikely.  The
backlog figure supplied with the listen call is limited
by the system to a maximum of 5 pending connections on any
one queue.  This avoids the problem of processes hogging system
resources by setting an infinite backlog, then ignoring
all connection requests.
.PP
With a socket marked as listening, a server may \f2accept\fP
a connection:
.DS
fromlen = sizeof (from);
snew = accept(s, &from, &fromlen);
.DE
A new descriptor is returned on receipt of a connection (along with
a new socket).  If the server wishes to find out who its client is,
it may supply a buffer for the client socket's name.  The value-result
parameter \f2fromlen\fP is initialized by the server to indicate how
much space is associated with \f2from\fP, then modified on return
to reflect the true size of the name.  If the client's name is not
of interest, the second parameter may be zero.
.PP
Accept normally blocks.  That is, the call to accept
will not return until a connection is available or the system call
is interrupted by a signal to the process.  Further, there is no
way for a process to indicate it will accept connections from only
a specific individual, or individuals.  It is up to the user process
to consider who the connection is from and close down the connection
if it does not wish to speak to the process.  If the server process
wants to accept connections on more than one socket, or not block
on the accept call there are alternatives;  they will be considered
in section 5.
.NH 2
Data transfer
.PP
With a connection established, data may begin to flow.  To send
and receive data there are a number of possible calls.
With the peer entity at each end of a connection
anchored, a user can send or receive a message without specifying
the peer.  As one might expect, in this case, then
the normal \f2read\fP and \f2write\fP system calls are useable,
.DS
write(s, buf, sizeof (buf));
read(s, buf, sizeof (buf));
.DE
In addition to \f2read\fP and \f2write\fP,
the new calls \f2send\fP and \f2recv\fP
may be used:
.DS
send(s, buf, sizeof (buf), flags);
recv(s, buf, sizeof (buf), flags);
.DE
While \f2send\fP and \f2recv\fP are virtually identical to
\f2read\fP and \f2write\fP,
the extra \f2flags\fP argument is important.  The flags may be
specified as a non-zero value if one or more
of the following is required:
.DS
.TS
l l.
SOF_OOB	send/receive out of band data
SOF_PREVIEW	look at data without reading
SOF_DONTROUTE	send data without routing packets
.TE
.DE
Out of band data is a notion specific to stream sockets, and one
which we will not immediately consider.  The option to have data
sent without routing applied to the outgoing packets is currently 
used only by the routing table management process, and is
unlikely to be of interest to the casual user.  The ability
to preview data is, however, of interest.  When SOF_PREVIEW
is specified with a \f2recv\fP call, any data present is returned
to the user, but treated as still \*(lqunread\*(rq.  That
is, the next \f2read\fP or \f2recv\fP call applied to the socket will
return the data previously previewed.
.NH 2
Discarding sockets
.PP
Once a socket is no longer of interest, it may be discarded
by applying a \f2close\fP to the descriptor,
.DS
close(s);
.DE
If data is associated with a socket which promises reliable delivery
(e.g. a stream socket) when a close takes place, the system will
continue to attempt to transfer the data. 
However, after a fairly long period of
time, if the data is still undelivered, it will be discarded.
Should a user have no use for any pending data, it may 
perform a \f2shutdown\fP on the socket prior to closing it.
This call is of the form:
.DS
shutdown(s, how);
.DE
where \f2how\fP is 0 if the user is no longer interested in reading
data, 1 if no more data will be sent, or 2 if no data is to
be sent or received.  Applying shutdown to a socket causes
any data queued to be immediately discarded.
.NH 2
Connectionless sockets
.PP
To this point we have been concerned mostly with sockets which
follow a connection oriented model.  However, there is also
support for connectionless interactions typical of the datagram
facilities found in contemporary packet switched networks.
A datagram socket provides a symmetric interface to data
exchange.  While processes are still likely to be client
and server, there is no requirement for connection establishment.
Instead, each message includes the destination address.
.PP
Datagram sockets are created as before, and each should
have a name bound to it in order that the recipient of
a message may identify the sender.  To send data,
the \f2sendto\fP primitive is used,
.DS
sendto(s, buf, buflen, flags, &to, tolen);
.DE
The \f2s\fP, \f2buf\fP, \f2buflen\fP, and \f2flags\fP
parameters are used as before. 
The \f2to\fP and \f2tolen\fP
values are used to indicate the intended recipient of the
message.  When using an unreliable datagram interface, it is
unlikely any errors will be reported to the sender.  Where
information is present locally to recognize a message which may
never be delivered (for instance when a network is unreachable),
the call will return \-1 and the global value \f2errno\fP will
contain an error number. 
.PP
To receive messages on an unconnected datagram socket, the
\f2recvfrom\fP primitive is provided:
.DS
recvfrom(s, buf, buflen, flags, &from, &fromlen);
.DE
Once again, the \f2fromlen\fP parameter is handled in
a value-result fashion, initially containing the size of
the \f2from\fP buffer.
.PP
In addition to the two calls mentioned above, datagram
sockets may also use the \f2connect\fP call to associate
a socket with a specific address.  In this case, any
data sent on the socket will automatically be addressed
to the connected peer, and only data received from that
peer will be delivered to the user.  Only one connected
address is permitted for each socket (i.e. no multi-casting).
Connect requests on datagram sockets return immediately,
as this simply results in the system recording
the peer's address (as compared to a stream socket where a
connect request initiates establishment of an end to end
connection).
Other of the less
important details of datagram sockets are described
in section 5.
.NH 2
Input/Output multiplexing
.PP
One last facility often used in developing applications
is the ability to multiplex i/o requests among multiple
sockets and/or files.  This is done using the \f2select\fP
call:
.DS
select(nfds, &readfds, &writefds, &execptfds, &timeout);
.DE
\f2Select\fP takes as arguments three bit masks, one for
the set of file descriptors for which the caller wishes to
be able to read data on, one for those descriptors to which
data is to be written, and one for which exceptional conditions
are pending.  
Bit masks are created
by or-ing bits of the form \*(lq1 << fd\*(rq.  That is,
a descriptor \f2fd\fP is selected if a 1 is present in
the \f2fd\fP'th bit of the mask.
The parameter \f2nfds\fP specifies the range
of file descriptors  (i.e. one plus the value of the largest
descriptor) specified in a mask. 
.PP
A timeout value may be specified if the selection
is not to last more than a predetermined period of time.  If
\f2timeout\fP is set to 0, the selection takes the form of a
\f2poll\fP, returning immediately.  If the last parameter is
a null pointer, the selection will block indefinitely*.
.FS
* To be more specific, a return takes place only when a
descriptor is selectable, or when a signal is received by
the caller, interrupting the system call.
.FE
\f2Select\fP normally returns the number of file descriptors selected.
If the \f2select\fP call returns due to the timeout expiring, then
a value of \-1 is returned along with the error number EINTR.
.PP
\f2Select\fP provides a synchronous multiplexing scheme.
Asynchronous notification of output completion, input availability,
and exceptional conditions is possible through use of the
SIGIO and SIGURG signals described in section 5.
