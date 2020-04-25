.V= $Header: rpc.spec.t 1.2 87/07/31 $
.Ct "" "" "RPC Protocol Specification"
.Ah "Introduction"
This document describes the message protocol used in implementing
the Remote Procedure Call (RPC) package.
The message protocol is specified with the
eXternal Data Representation (XDR) language.
Readers should be familiar with both RPC and XDR.
Casual users of RPC do not need to be
familiar with this information. 
.Bh "The RPC Model"
The remote procedure call model is similar to
the local procedure call model.
In the local case, the caller places arguments to a procedure
in a specified location (such as a result register).
It then transfers control to the procedure,
and eventually gains back control.
At that point, the results of the procedure
are extracted from the specified location
and the caller continues execution.
.Pa
The remote procedure call is similar,
except that one thread of control winds through two processes,
the caller and the server.
The caller process sends a call message containing the 
parameters for a procedure
to the server process.  The caller process then 
waits (blocks) for a reply message.
When the reply message is received,
the results of the procedure are extracted,
and the caller can resume execution. 
.Pa
On the server side,
a process is dormant until a call message arrives.
When a message is received, the server process extracts the parameters for
the procedure,
computes the results, sends a reply message,
and then awaits the next call message.
Note that 
only one of the two processes is active at any given time.
The RPC protocol does not explicitly support
multi-threading of caller or server processes.
.Bh "Transports and Semantics"
The RPC protocol is independent of transport protocols.
RPC does not care how a message is passed
from one process to another.
The protocol deals only with
the specification and interpretation of messages.
.Pa
Because of transport independence,
the RPC protocol does not attach specific semantics
to the remote procedures or to their execution.
Some semantics can be inferred from
(but should be explicitly specified by)
the underlying transport protocol.
For example, using UDP/IP for RPC message passing is unreliable.
If a caller retransmits call messages after short time-outs,
the only thing he can infer from the lack of a reply message
is that the remote procedure was executed
zero or more times (and from a reply message, one or more times).
.Pa
On the other hand, using TCP/IP for RPC message passing is reliable.
No reply message means that the remote procedure was executed at most once,
whereas a reply message means that the remote procedure was executed 
exactly once.
.Ns N
RPC is currently implemented
on top of TCP/IP and UDP/IP transports.
.Ne
.Bh "Binding and Rendezvous Independence"
The act of binding a client to a service is
not part of the remote procedure call specification.
This important and necessary function
is left up to some higher-level software.
.Pa
Implementors should think of the RPC protocol as the
jump-subroutine instruction (\*QJSR\*U) of a network.
The loader (binder) makes JSR useful,
and the loader itself uses JSR to accomplish its task.
Similarly, the network makes RPC useful,
and RPC is used to accomplish networking tasks.
.Bh "Message Authentication"
The RPC protocol provides the fields necessary for a client to
identify himself to a service and vice versa.
Security and access control mechanisms
can be built on top of the message authentication.
.bp
.Ah "RPC Protocol Requirements"
The RPC protocol provides the following: 
.Ls 1
.Li
A unique specification of the procedure to be called.
.Li
Provisions for matching response messages to request messages.
.Li
Provisions for authenticating the caller to service and vice versa.
.Le
.Pa
The RPC protocol also supports features that detect the following: 
.Ls 1
.Li
RPC protocol mismatches.
.Li
Remote program protocol version mismatches.
.Li
Protocol errors (such as mis-specification of procedure parameters).
.Li
Reasons why remote authentication failed.
.Li
Any other reasons why the desired procedure was not called.
.Le
.Bh "Remote Programs and Procedures"
The RPC call message has three unsigned fields:
Remote program number,
remote program version number,
and remote procedure number.
These three fields uniquely identify the procedure to be called.
Program numbers are administered by a central authority (such as Sun).
A program number is needed to implement a remote program.
.Pa
The first implementation of a protocol would
probably be version number 1.
Because most new protocols evolve into better,
more stable and mature protocols,
a version field has been included in the call message.  This field 
identifies the version
of the protocol that the caller is using.
Version numbers make it possible to speak old and new protocols
through the same server process.
.Pa
The procedure number identifies the procedure to be called.
These numbers are documented in this manual in
the protocol specification for each program.
For example, a file service protocol specification
might state that procedure number 5 is \f2read\fP
and procedure number 12 is \f2write\fP.
.Pa
Just as remote program protocols can change over several versions,
the actual RPC message protocol can also change.
Therefore, the call message also contains the RPC version number, 
which must be two (2).
.Pa
A reply message contains enough information
to distinguish the following error conditions:
.Ls 1
.Li
The remote implementation of RPC doesn't speak protocol version 2.  The
lowest and highest supported RPC version numbers are returned.
.Li
The remote program is not available on the remote system.
.Li
The remote program does not support the requested version number.
The lowest and highest supported
remote program version numbers are returned.
.Li
The requested procedure number does not exist
(this is usually a caller-side protocol or programming error).
.Li
From the server's point of view, the parameters to the remote procedure
appear to be garbage. 
(Again, this is caused by a disagreement about the
protocol between client and service.)
.Bh "Authentication"
Provisions for authentication of caller to service and vice versa are
provided in the RPC protocol.  The call message
has two authentication fields, credentials and verifier.
The reply message has one authentication field,
the response verifier.
The RPC protocol specification defines all three fields
to be the following opaque type:
.Ps
enum auth_flavor {
	AUTH_NULL	= 0,
	AUTH_UNIX	= 1,
	AUTH_SHORT	= 2
	/* and more to be defined */
};
.sp.5
struct opaque_auth {
	union switch (enum auth_flavor) {
		default: string auth_body<400>;
	};
};
.Pe
An \f3opaque_auth\fP
structure is an \f3auth_flavor\fP enumeration followed by a counted string
whose bytes are opaque to the RPC protocol implementation.
.Pa
The interpretation and semantics of the data contained
within the authentication fields are specified by individual,
independent authentication protocol specifications.
The \*QAuthentication Parameter Specification\*U section of this article 
defines two authentication protocols.
.Pa
If authentication parameters are rejected,
the response message tells why.
.Bh "Program Number Assignment"
Program numbers are assigned in groups of 0x20000000 (536870912)
according to the following list:
.Ps
       0 - 1fffffff	defined by Sun
20000000 - 3fffffff	defined by user
40000000 - 5fffffff	transient
60000000 - 7fffffff	reserved
80000000 - 9fffffff	reserved
a0000000 - bfffffff	reserved
c0000000 - dfffffff	reserved
e0000000 - ffffffff	reserved
.Pe
The first group, which is a range of numbers administered by Sun Microsystems,
should be identical for all Sun customers.  
When a customer develops an application that might be of general
interest, that application should be assigned a number in this range.
The second range
is for applications specific to a particular customer.
This range is intended primarily for debugging new programs.
The third group is for applications that
generate program numbers dynamically.  The final groups
are reserved for future use.
.Bh "Other Uses of the RPC Protocol"
The RPC protocol is intended for calling remote procedures.
That is, each call message is matched with a response message.
However, the protocol itself is a message-passing protocol
that can implement other non-RPC protocols. 
Currently the RPC message protocol is used 
for two non-RPC protocols:
Batching (or pipelining) and broadcast RPC.
.Ch "Batching"
Batching allows a client to send an arbitrarily large sequence
of call messages to a server,
using reliable byte stream protocols such as TCP/IP
for their transport.
The client never waits for a reply
from the server and the server does not send replies to batch requests.
A sequence of batch calls is usually terminated by a legitimate
RPC in order to flush the pipeline (with positive acknowledgement).
.Ch "Broadcast RPC"
In broadcast RPC-based protocols,
the client sends a broadcast packet to
the network and waits for numerous replies.
Broadcast RPC uses unreliable, packet-based protocols such as UDP/IP
as transports.
Servers that support broadcast protocols respond only
when the request is processed successfully.
They are silent in the face of errors.
.bp
.Ah "The RPC Message Protocol"
This section defines the RPC message protocol in the XDR data description
language.  The message is defined in a top-down style.
.Ns N
This is an XDR specification, not C code.
.Ne
.sp
.As
enum msg_type {
	CALL = 0,
	REPLY = 1
};
.sp.5
.NE
/*
 * A reply to a call message can take on two forms:
 * the message was either accepted or rejected.
 */
enum reply_stat {
	MSG_ACCEPTED = 0,
	MSG_DENIED = 1
};
.sp.5
.NE
/*
 * Given that a call message was accepted, the following is
 * the status of an attempt to call a remote procedure.
 */
enum accept_stat {
	SUCCESS = 0,	  /* RPC executed successfully */
	PROG_UNAVAIL = 1, /* remote hasn't exported program */
	PROG_MISMATCH= 2, /* remote can't support version # */
	PROC_UNAVAIL = 3, /* program can't support procedure */
	GARBAGE_ARGS = 4  /* procedure can't decode params */
};
.sp.5
.NE
/*
 * Reasons why a call message was rejected:
 */
enum reject_stat {
	RPC_MISMATCH = 0, /* RPC version number != 2 */
	AUTH_ERROR = 1    /* remote can't authenticate caller */
};
.sp.5
.NE
/*
 * Why authentication failed:
 */
enum auth_stat {
	AUTH_BADCRED = 1,    /* bad credentials (seal broken) */
	AUTH_REJECTEDCRED=2, /* client must begin new session */
	AUTH_BADVERF = 3,    /* bad verifier (seal broken) */
	AUTH_REJECTEDVERF=4, /* verifier expired or replayed */
	AUTH_TOOWEAK = 5,    /* rejected for security reasons */
};
.sp.5
.NE
/*
 * The RPC message:
 * All messages start with a transaction identifier, xid,
 * followed by a two-armed discriminated union.  The union's
 * discriminant is a msg_type which switches to one of the
 * two types of the message.  The xid of a REPLY message
 * always matches that of the initiating CALL message.  NB:
 * The xid field is only used for clients matching reply
 * messages with call messages; the service side cannot
 * treat this id as any type of sequence number.
 */
struct rpc_msg {
	unsigned	xid;
	union switch (enum msg_type) {
		CALL:	struct call_body;
		REPLY:	struct reply_body;
	};
};
.sp.5
.NE
/*
 * Body of an RPC request call:
 * In version 2 of the RPC protocol specification, rpcvers
 * must be equal to 2.  The fields prog, vers, and proc
 * specify the remote program, its version number, and the
 * procedure within the remote program to be called.  After
 * these fields are two  authentication parameters: cred
 * (authentication credentials) and verf (authentication
 * verifier).  The two  authentication parameters are
 * followed by the parameters to the remote procedure,
 * which are specified by the specific program protocol.
 */
struct call_body {
	unsigned rpcvers;	/* must be equal to two (2) */
	unsigned prog;
	unsigned vers;
	unsigned proc;
	struct opaque_auth cred;
	struct opaque_auth verf;
	/* procedure specific parameters start here */
};
.sp.5
.NE
/*
 * Body of a reply to an RPC request.
 * The call message was either accepted or rejected.
 */
struct reply_body {
	union switch (enum reply_stat) {
		MSG_ACCEPTED:	struct accepted_reply;
		MSG_DENIED:	struct rejected_reply;
	};
};
.sp.5
.NE
/*
 * Reply to an RPC request that was accepted by the server.
 * Note: there could be an error even though the request
 * was accepted.  The first field is an authentication
 * verifier that the server generates in order to validate
 * itself to the caller.  It is followed by a union whose
 * discriminant is an enum accept_stat.  The SUCCESS arm
 * of the union is protocol specific.  The PROG_UNAVAIL,
 * PROC_UNAVAIL, and GARBAGE_ARGS arms of the union are
 * void.  The PROG_MISMATCH arm specifies the lowest and
 * highest version numbers of the remote program that are
 * supported by the server.
 */
struct accepted_reply {
	struct opaque_auth	verf;
	union switch (enum accept_stat) {
		SUCCESS: struct {
			/*
			 * procedure-specific results start here
			 */
		};
		PROG_MISMATCH: struct {
			unsigned low;
			unsigned  high;
		};
		default: struct {
			/*
			 * void. Cases include PROG_UNAVAIL,
			 * PROC_UNAVAIL, and GARBAGE_ARGS.
			 */
		};
	};
};
.sp.5
.NE
/*
 * Reply to an RPC request that was rejected by the server.
 * The request can be rejected because of two reasons:
 * either the server is not running a compatible version of
 * the RPC protocol (RPC_MISMATCH), or the server refuses
 * to authenticate the caller (AUTH_ERROR).  In the case of
 * an RPC version mismatch, the server returns the lowest
 * and highest supported RPC version numbers.  In the case
 * of refused authentication, failure status is returned.
 */
struct rejected_reply {
	union switch (enum reject_stat) {
		RPC_MISMATCH: struct {
			unsigned low;
			unsigned high;
		};
		AUTH_ERROR: enum auth_stat;
	};
};
.Pe
.Bh "Authentication Parameter Specification"
Although authentication parameters are opaque,
they are open-ended to the rest of the
RPC protocol.  This section defines some \*Qflavors\*U of authentication
that have been implemented at (and supported by) Sun.
.Ch "Null Authentication"
Often calls must be made where neither the caller or the server needs to know
the identity of the caller.
In this case, the 
credentials, verifier, and response verifier of the RPC message would have
an auth_flavor value (the discriminant of the opaque_auth union) of
AUTH_NULL (0).
The bytes of the auth_body string are undefined.
It is recommended that the string length be zero.
.Ch "UNIX Authentication"
The caller of a remote procedure might want to identify himself as he is
identified on a UNIX system.
In this case, the value of the \f2credential\fP
discriminant of an RPC call message is
AUTH_UNIX (1).  The bytes of the \f2credential\fP
string encode the following (XDR) structure:
.Ps
struct auth_unix {
	unsigned	stamp;
	string		machinename<255>;
	unsigned	uid;
	unsigned	gid;
	unsigned	gids<10>;
};
.Pe
The \f2stamp\fP
is an arbitrary id generated by the caller machine.
The \f2machinename\fP
is the name of the caller's machine (such as \*Qkrypton\*U).
The \f2uid\fP is the caller's effective user id.
The \f2gid\fP is the caller's effective group id.
The \f2gids\fP is a counted array of groups
to which the caller belongs.
The \f2verifier\fP
accompanying the credentials should be 
AUTH_NULL.
.Pa
The value of the discriminate of the \f2response verifier\fP
received in the reply message from the server can be either
AUTH_NULL or AUTH_SHORT.
In the case of AUTH_SHORT,
the bytes of the \f2response verifier\fP
string encode an \f3auth_opaque\fP structure.
This new \f3auth_opaque\fP
structure may now be passed to the server
instead of the original AUTH_UNIX flavor credentials.
The server keeps a cache that maps shorthand
\f3auth_opaque\fP structures (passed back by way of an
AUTH_SHORT style \f2response verifier\fP)
to the original credentials of the caller.
The caller can save network bandwidth and server cpu cycles
by using the new credentials.
.Pa
The server can flush the shorthand \f3auth_opaque\fP
structure at any time.
If this happens, the remote procedure call message
will be rejected due to an authentication error.
The reason for the failure will be AUTH_REJECTEDCRED.
At this point, the caller might want to try the original
AUTH_UNIX style of credentials.
.Bh "Record Marking Standard"
When RPC messages are passed on top of a byte stream protocol
such as TCP/IP, it is necessary or at least desirable
to delimit one message from another in order to detect
and possibly recover from user protocol errors.
This is called record marking (RM).
NFS uses the RM/TCP/IP transport for passing
RPC messages on TCP streams.
One RPC message fits into one RM record.
.Pa
A record is composed of one or more record fragments.
A record fragment is a four-byte header followed by
\f20\fP to \f22\s-2\u31\d\s+2\-1\fP
bytes of fragment data.
The bytes encode an unsigned binary number.
As with XDR integers, the byte order is from highest to lowest.
The number encodes two values: 
A boolean that indicates if the fragment is the last fragment
of the record (bit value 1 implies the fragment is the last fragment)
and a 31-bit unsigned binary value that is the length in bytes of the
fragment's data.
The boolean value is the highest-order bit of the header;
the length is the 31 low-order bits.
.Ns N
This record specification is
\f1not\fP
in XDR standard form.
.Ne
.Ah "Port Mapper Program Protocol"
The port mapper program maps RPC program and version numbers
to UDP/IP or TCP/IP port numbers.
This program makes dynamic binding of remote programs possible.
.Pa
This is desirable because the range of reserved port numbers is very small
and the number of potential remote programs is very large.  By running 
the port mapper only on a reserved port, 
the port numbers of other remote programs
can be ascertained by querying the port mapper.
.Pa
The RPC protocol is specified by the XDR description language:
.Ps
Port Mapper RPC Program Number: 100000
	Version Number: 1
	Supported Transports:
		UDP/IP on port 111
		RM/TCP/IP on port 111
.Pe
.Bh "Transport Protocol Numbers"
.As
#define IPPROTO_TCP	6	/* protocol number for TCP/IP */
#define IPPROTO_UDP	17	/* protocol number for UDP/IP */
.Ae
.Bh "RPC Procedures"
Following is a list of RPC procedures:
.sp
\f4Do Nothing\fP
.Pa
Procedure 0, Version 2.
.Ps
0. PMAPPROC_NULL () returns ()
.Pe
This procedure does no work.
By convention, procedure zero of any protocol
takes no parameters and returns no results.
.sp 2
\f4Set a Mapping\fP
.Pa
Procedure 1, Version 2.
.Ps
1. PMAPPROC_SET (prog,vers,prot,port) returns (resp)
	unsigned prog;
	unsigned vers;
	unsigned prot;
	unsigned port;
	boolean resp;
.Pe
When a program first becomes available on a machine,
it registers itself with the port mapper program on the same machine.
The program passes its program number \f2prog\fP,
version number \f2vers\fP,
transport protocol number \f2prot\fP,
and the port \f2port\fP
on which it awaits a service request.
The procedure returns \f2resp\fP,
whose value is TRUE
if the procedure successfully established the mapping and
FALSE otherwise.  The procedure refuses to establish a mapping
if one already exists for the tuple [\f2prog,vers,prot\fP].
.sp 2
\f4Unset a Mapping\fP
.Pa
Procedure 2, Version 2.
.Ps
2. PMAPPROC_UNSET (prog,vers,dummy1,dummy2) returns (resp)
	unsigned prog;
	unsigned vers;
	unsigned dummy1;  /* value always ignored */
	unsigned dummy2;  /* value always ignored */
	boolean resp;
.Pe
When a program becomes unavailable, it should
unregister itself with the port mapper program on the same machine.
The parameters and results for this procedure are identical to those for
\f2PMAPPROC_SET\fP.
.sp 2
\f4Look Up a Mapping\fP
.Pa
Procedure 3, Version 2.
.sp
.As
3. PMAPPROC_GETPORT (prog,vers,prot,dummy) returns (port)
	unsigned prog;
	unsigned vers;
	unsigned prot;
	unsigned dummy;	/* this value always ignored */
	unsigned port;	/* zero means program not registered */
.Ae
.sp
Given a program number \f2prog\fP,
version number \f2vers\fP,
and transport protocol number \f2prot\fP,
this procedure returns the port number on which
the program is awaiting call requests.
A \f2port\fP value of zero means the program has not been registered.
.bp 
\f4Dumping the Mappings\fP
.Pa
Procedure 4, Version 2.
.Ps
4. PMAPPROC_DUMP () returns (maplist)
	struct maplist {
		union switch (boolean) {
			FALSE: struct { /* void, end of list */ };
			TRUE: struct {
				unsigned prog;
				unsigned vers;
				unsigned prot;
				unsigned port;
				struct maplist the_rest;
			};
		};
	} maplist;
.Pe
This procedure enumerates all entries in the port mapper's database.
The procedure takes no parameters and returns a list of
program, version, protocol, and port values.
.sp 2
\f4Indirect Call Routine\fP
.sp
Procedure 5, Version 2.
.sp
.As
5. PMAPPROC_CALLIT (prog,vers,proc,args) returns (port,res)
	unsigned prog;
	unsigned vers;
	unsigned proc;
	string args<>;
	unsigned port;
	string res<>;
.Ae
.sp
This procedure allows a caller to call another remote procedure
on the same machine without knowing the remote procedure's port number.
It is intended for supporting broadcasts
to arbitrary remote programs via the well-known port mapper's port.
The parameters \f2prog\fP, \f2vers\fP, \f2proc\fP,
and the bytes of \f2args\fP
are the program number, version number, procedure number,
and parameters of the remote procedure.
.bp
.Ns N
This procedure sends a response only if the procedure was
successfully executed.  It is silent (makes no response) otherwise.
.sp 
The port mapper uses only UDP/IP to communicate with the remote program. 
.Ne
The procedure returns the port number of the remote program.
The bytes of results are the results of the remote procedure.
