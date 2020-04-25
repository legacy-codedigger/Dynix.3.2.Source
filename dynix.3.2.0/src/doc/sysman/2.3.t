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
. \" $Header: 2.3.t 1.4 86/03/06 $
.\" %M% %I% %E%
.sh "Interprocess communications
.NH 3
Interprocess communication primitives
.NH 4
Communication domains
.PP
The system provides access to an extensible set of 
communication \f2domains\fP.  A communication domain
is identified by a manifest constant defined in the
file <sys/socket.h>.
Important standard domains supported by the system are the ``unix''
domain, AF_UNIX, for communication within the system, and the ``internet''
domain for communication in the DARPA internet, AF_INET.  Other domains can
be added to the system.
.NH 4
Socket types and protocols
.PP
Within a domain, communication takes place between communication endpoints
known as \f2sockets\fP.  Each socket has the potential to exchange
information with other sockets within the domain.
.PP
Each socket has an associated
abstract type, which describes the semantics of communication using that
socket.  Properties such as reliability, ordering, and prevention
of duplication of messages are determined by the type.
The basic set of socket types is defined in <sys/socket.h>:
.DS
/* Standard socket types */
._d
#define	SOCK_DGRAM	1	/* datagram */
#define	SOCK_STREAM	2	/* virtual circuit */
#define	SOCK_RAW	3	/* raw socket */
#define	SOCK_RDM	4	/* reliably-delivered message */
#define	SOCK_SEQPACKET	5	/* sequenced packets */
.DE
The SOCK_DGRAM type models the semantics of datagrams in network communication:
messages may be lost or duplicated and may arrive out-of-order.
The SOCK_RDM type models the semantics of reliable datagrams: messages
arrive unduplicated and in-order, the sender is notified if
messages are lost.
The \f2send\fP and \f2receive\fP operations (described below)
generate reliable/unreliable datagrams.
The SOCK_STREAM type models connection-based virtual circuits: two-way
byte streams with no record boundaries.
The SOCK_SEQPACKET type models a connection-based,
full-duplex, reliable, sequenced packet exchange;
the sender is notified if messages are lost, and messages are never
duplicated or presented out-of-order.
Users of the last two abstractions may use the facilities for
out-of-band transmission to send out-of-band data.
.PP
SOCK_RAW is used for unprocessed access to internal network layers
and interfaces; it has no specific semantics.
.PP
Other socket types can be defined.\(dg
.FS
\(dg 4.2BSD does not support the SOCK_RDM and SOCK_SEQPACKET types.
.FE
.PP
Each socket may have a concrete \f2protocol\fP associated with it.
This protocol is used within the domain to provide the semantics
required by the socket type.
For example, within the ``internet'' domain, the SOCK_DGRAM type may be
implemented by the UDP user datagram protocol, and the SOCK_STREAM
type may be implemented by the TCP transmission control protocol, while
no standard protocols to provide SOCK_RDM or SOCK_SEQPACKET sockets exist.
.NH 4
Socket creation, naming and service establishment
.PP
Sockets may be \f2connected\fP or \f2unconnected\fP.  An unconnected
socket descriptor is obtained by the \f2socket\fP call:
.DS
s = socket(domain, type, protocol);
result int s; int domain, type, protocol;
.DE
.PP
An unconnected socket descriptor may yield a connected socket descriptor
in one of two ways: either by actively connecting to another socket,
or by becoming associated with a name in the communications domain and
\f2accepting\fP a connection from another socket.
.PP
To accept connections, a socket must first have a binding
to a name within the communications domain.  Such a binding
is established by a \f2bind\fP call:
.DS
bind(s, name, namelen);
int s; char *name; int namelen;
.DE
A socket's bound name may be retrieved with a \f2getsockname\fP call:
.DS
getsockname(s, name, namelen);
int s; result caddr_t name; result int *namelen;
.DE
while the peer's name can be retrieved with \f2getpeername\fP:
.DS
getpeername(s, name, namelen);
int s; result caddr_t name; result int *namelen;
.DE
Domains may support sockets with several names.
.NH 4
Accepting connections
.PP
Once a binding is made, it is possible to \f2listen\fP for
connections:
.DS
listen(s, backlog);
int s, backlog;
.DE
The \f2backlog\fP specifies the maximum count of connections
that can be simultaneously queued awaiting acceptance.
.PP
An \f2accept\fP call:
.DS
t = accept(s, name, anamelen);
result int t; int s; result caddr_t name; result int *anamelen;
.DE
returns a descriptor for a new, connected, socket
from the queue of pending connections on \f2s\fP.
.NH 4
Making connections
.PP
An active connection to a named socket is made by the \f2connect\fP call:
.DS
connect(s, name, namelen);
int s; caddr_t name; int namelen;
.DE
.PP
It is also possible to create connected pairs of sockets without
using the domain's name space to rendezvous; this is done with the
\f2socketpair\fP call\(dg:
.FS
\(dg 4.2BSD supports \f2socketpair\fP creation only in the ``unix''
communication domain.
.FE
.DS
socketpair(d, type, protocol, sv);
int d, type, protocol; result int sv[2];
.DE
Here the returned \f2sv\fP descriptors correspond to those obtained with
\f2accept\fP and \f2connect\fP.
.PP
The call
.DS
pipe(pv)
result int pv[2];
.DE
creates a pair of SOCK_STREAM sockets in the UNIX domain,
with pv[0] only writeable and pv[1] only readable.
.NH 4
Sending and receiving data
.PP
Messages may be sent from a socket by:
.DS
cc = sendto(s, buf, len, flags, to, tolen);
result int cc; int s; caddr_t buf; int len, flags; caddr_t to; int tolen;
.DE
if the socket is not connected or:
.DS
cc = send(s, buf, len, flags);
result int cc; int s; caddr_t buf; int len, flags;
.DE
if the socket is connected.
The corresponding receive primitives are:
.DS
msglen = recvfrom(s, buf, len, flags, from, fromlenaddr);
result int msglen; int s; result caddr_t buf; int len, flags;
result caddr_t from; result int *fromlenaddr;
.DE
and
.DS
msglen = recv(s, buf, len, flags);
result int msglen; int s; result caddr_t buf; int len, flags;
.DE
.PP
In the unconnected case,
the parameters \f2to\fP and \f2tolen\fP
specify the destination or source of the message, while
the \f2from\fP parameter stores the source of the message,
and \f2*fromlenaddr\fP initially gives the size of the \f2from\fP
buffer and is updated to reflect the true length of the \f2from\fP
address.
.PP
All calls cause the message to be received in or sent from
the message buffer of length \f2len\fP bytes, starting at address \f2buf\fP.
The \f2flags\fP specify
peeking at a message without reading it or sending or receiving
high-priority out-of-band messages, as follows:
.DS
._d
#define	MSG_PEEK	0x1	/* peek at incoming message */
#define	MSG_OOB	0x2	/* process out-of-band data */
.DE
.NH 4
Scatter/gather and exchanging access rights
.PP
It is possible scatter and gather data and to exchange access rights
with messages.  When either of these operations is involved,
the number of parameters to the call becomes large.
Thus the system defines a message header structure, in <sys/socket.h>,
which can be
used to conveniently contain the parameters to the calls:
.DS
.if t .ta .5i 1.25i 2i 2.7i
.if n ._f
struct msghdr {
	caddr_t	msg_name;		/* optional address */
	int	msg_namelen;	/* size of address */
	struct	iov *msg_iov;	/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_accrights;	/* access rights sent/received */
	int	msg_accrightslen;	/* size of msg_accrights */
};
.DE
Here \f2msg_name\fP and \f2msg_namelen\fP specify the source or destination
address if the socket is unconnected; \f2msg_name\fP may be given as
a null pointer if no names are desired or required.
The \f2msg_iov\fP and \f2msg_iovlen\fP describe the scatter/gather
locations, as described in section 2.1.3.
Access rights to be sent along with the message are specified
in \f2msg_accrights\fP, which has length \f2msg_accrightslen\fP.
In the ``unix'' domain these are an array of integer descriptors,
taken from the sending process and duplicated in the receiver.
.PP
This structure is used in the operations \f2sendmsg\fP and \f2recvmsg\fP:
.DS
sendmsg(s, msg, flags);
int s; struct msghdr *msg; int flags;

msglen = recvmsg(s, msg, flags);
result int msglen; int s; result struct msghdr *msg; int flags;
.DE
.NH 4
Using read and write with sockets
.PP
The normal UNIX \f2read\fP and \f2write\fP calls may be
applied to connected sockets and translated into \f2send\fP and \f2receive\fP
calls from or to a single area of memory and discarding any rights
received.  A process may operate on a virtual circuit socket, a terminal
or a file with blocking or non-blocking input/output
operations without distinguishing the descriptor type.
.NH 4
Shutting down halves of full-duplex connections
.PP
A process that has a full-duplex socket such as a virtual circuit
and no longer wishes to read from or write to this socket can
give the call:
.DS
shutdown(s, direction);
int s, direction;
.DE
where \f2direction\fP is 0 to not read further, 1 to not
write further, or 2 to completely shut the connection down.
.NH 4
Socket and protocol options
.PP
Sockets, and their underlying communication protocols, may
support \f2options\fP.  These options may be used to manipulate
implementation specific or non-standard facilities. 
The \f2getsockopt\fP
and \f2setsockopt\fP calls are used to control options:
.DS
getsockopt(s, level, optname, optval, optlen)
int s, level, optname; result caddr_t optval; result int *optlen;

setsockopt(s, level, optname, optval, optlen)
int s, level, optname; caddr_t optval; int optlen;
.DE
The option \f2optname\fP is interpreted at the indicated
protocol \f2level\fP for socket \f2s\fP.  If a value is specified
with \f2optval\fP and \f2optlen\fP, it is interpreted by
the software operating at the specified \f2level\fP.  The \f2level\fP
SOL_SOCKET is reserved to indicate options maintained
by the socket facilities.  Other \f2level\fP values indicate
a particular protocol which is to act on the option request;
these values are normally interpreted as a ``protocol number''.
.NH 3
UNIX domain
.PP
This section describes briefly the properties of the UNIX communications
domain.
.NH 4
Types of sockets
.PP
In the UNIX domain,
the SOCK_STREAM abstraction provides pipe-like
facilities, while SOCK_DGRAM provides (usually)
reliable message-style communications.
.NH 4
Naming
.PP
Socket names are strings and may appear in the UNIX file
system name space through portals\(dg.
.FS
\(dg The 4.2BSD implementation of the UNIX domain embeds
bound sockets in the UNIX file system name space; this
is a side effect of the implementation.
.FE
.NH 4
Access rights transmission\(dd
.FS
\(dd\
DYNIX does not support access rights transmission in the UNIX domain.
.FE
.PP
The ability to pass UNIX descriptors with messages in this domain
allows migration of service within the system and allows
user processes to be used in building system facilities.
.NH 3
INTERNET domain
.PP
This section describes briefly how the INTERNET domain is
mapped to the model described in this section.  More
information will be found in the document describing the
network implementation in 4.2BSD.
.NH 4
Socket types and protocols
.PP
SOCK_STREAM is supported by the INTERNET TCP protocol;
SOCK_DGRAM by the UDP protocol.  The SOCK_SEQPACKET
has no direct INTERNET family analogue; a protocol
based on one from the XEROX NS family and layered on
top of IP could be implemented to fill this gap.
.NH 4
Socket naming
.PP
Sockets in the INTERNET domain have names composed of the 32 bit
internet address, and a 16 bit port number.
Options may be used to
provide source routing for the address, security options,
or additional address for subnets of INTERNET for which the basic 32 bit
addresses are insufficient.
.NH 4
Access rights transmission
.PP
No access rights transmission facilities are provided in the INTERNET domain.
.NH 4
Raw access
.PP
The INTERNET domain allows the super-user access to the raw facilities
of the various network interfaces and the various internal layers
of the protocol implementation.  This allows administrative and debugging
functions to occur.  These interfaces are modeled as SOCK_RAW sockets.
