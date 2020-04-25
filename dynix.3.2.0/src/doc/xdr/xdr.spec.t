.V= $Header: xdr.spec.t 1.1 87/06/23 $
.EQ
delim $$
.EN
.Ct "" ""  "XDR Protocol Specification"
.Ah "Introduction"
This article discusses library routines that allow C programmers to 
describe arbitrary data structures in a machine-independent fashion.
A part of the Remote Procedure Call (RPC) package, the eXternal Data
Representation (XDR) standard is used to transmit
data for remote procedure calls.
XDR library routines should be used to transmit data
that is accessed (read or written) by more than one type of machine.
.Pa
This article includes a description of XDR library routines,
a guide to accessing currently available XDR streams,
information on defining new streams and data types,
and a formal definition of the XDR standard.
XDR was designed to work across different languages,
operating systems, and machine architectures.
Most users (particularly RPC users)
need only the information in this section and the section entitled \*QXDR
Library Primitives.\*U 
Programmers wanting to implement RPC and XDR on new machines
will need the information in the sections \*QXDR Stream Access,\*U 
\*QXDR Stream Implementation,\*U and \*QXDR Standard.\*U
Advanced topics, which are not necessary for all implementations,
are also discussed in this article.
.Pa
To use XDR routines, C programs
must include the file \f2<rpc/rpc.h>\fP,
which contains all of the necessary interfaces to the XDR system.
Because the C library \f2libc.a\fP
contains all of the XDR routines,
you can compile as normal:
.Ps
% \ \c
.UL "cc \f2program\fP.c"
.Pe
.Bh "Justification"
Consider the following two programs, \f2writer\fP:
.sp
.As
#include <stdio.h>
.sp.5
main()			/* writer.c */
{
	long i;
.sp.5
	for (i = 0; i < 8; i++) {
		if (fwrite((char *)&i, sizeof(i), 1, stdout) != 1) {
			fprintf(stderr, "failed!\en");
			exit(1);
		}
	}
}
.Ae
.sp
and \f2reader\fP:
.sp
.As
#include <stdio.h>
.sp.5
main()			/* reader.c */
{
	long i, j;
.sp.5
	for (j = 0; j < 8; j++) {
		if (fread((char *)&i, sizeof (i), 1, stdin) != 1) {
			fprintf(stderr, "failed!\en");
			exit(1);
		}
		printf("%ld ", i);
	}
	printf("\en");
}
.Ae
.sp
The two programs appear to be portable, because
they pass \f3lint\fP checking and
they exhibit the same behavior when executed
on two different hardware architectures, a Sun and a VAX. 
.Pa
Piping the output of the \f2writer\fP
program to the \f2reader\fP
program gives identical results on a Sun or a VAX:
.Ps
sun% \ \c
.UL "writer | reader"
0 1 2 3 4 5 6 7
sun%
.Pe
.Ps
vax% \ \c
.UL "writer | reader"
0 1 2 3 4 5 6 7
vax%
.Pe
The advent of local area networks and 4.2 BSD
UNIX brought about the concept of \*Qnetwork pipes,\*U in which
a process produces data on one machine,
and a second process consumes data on another machine.
A network pipe can be constructed with \f2writer\fP
and \f2reader\fP.
Here are the results if the first process produces data on a Sun
and the second process consumes data on a VAX:
.Ps
sun% \ \c
.UL "writer | rsh vax reader"
0 16777216 33554432 50331648 67108864 83886080 100663296
117440512
sun%
.Pe
The same result is obtained if
\f2writer\fP is executed on the VAX and \f2reader\fP is executed on the Sun.
These results occur because the byte ordering
of long integers differs between the VAX and the Sun,
even though word size is the same.
Note that \f516777216\fP is $2 sup 24$. 
When four bytes are reversed, the 1 winds up in the 24th bit.
.Pa
Whenever data is shared by two or more machine types,
there is a need for portable data.
Programs can be made data-portable by replacing the
\f3read()\fP and \f3write()\fP calls with calls to the XDR library routine
\f3xdr_long()\fP, a filter that knows the standard representation
of a long integer in its external form.
Here are the revised versions of \f2writer\fP:
.Ps
#include <stdio.h>
#include <rpc/rpc.h>	/* xdr is a sub-library of rpc */
.sp.5
main()		/* writer.c */
{
	XDR xdrs;
	long i;
.sp.5
	xdrstdio_create(&xdrs, stdout, XDR_ENCODE);
	for (i = 0; i < 8; i++) {
		if (!xdr_long(&xdrs, &i)) {
			fprintf(stderr, "failed!\en");
			exit(1);
		}
	}
}
.Pe
and \f2reader\fP:
.Ps
#include <stdio.h>
#include <rpc/rpc.h>	/* xdr is a sub-library of rpc */
.sp.5
main()		/* reader.c */
{
	XDR xdrs;
	long i, j;
.sp.5
	xdrstdio_create(&xdrs, stdin, XDR_DECODE);
	for (j = 0; j < 8; j++) {
		if (!xdr_long(&xdrs, &i)) {
			fprintf(stderr, "failed!\en");
			exit(1);
		}
		printf("%ld ", i);
	}
	printf("\en");
}
.Pe
The new programs were executed on a Sun,
on a VAX, and from a Sun to a VAX.
The results are shown below:
.Ps
sun%\ \c
.UL "writer | reader"
0 1 2 3 4 5 6 7
sun%
.Pe
.Ps
vax%\ \c
.UL "writer | reader"
0 1 2 3 4 5 6 7
vax%
.Pe
.Ps
sun%\ \c
.UL "writer | rsh vax reader"
0 1 2 3 4 5 6 7
sun%
.Pe
Dealing with integers is just the tip of the portable-data iceberg.
Arbitrary data structures present portability problems,
particularly with respect to alignment and pointers.
Alignment on word boundaries can cause the
size of a structure to vary from machine to machine.
Pointers are convenient to use,
but have no meaning outside the machine where they are defined.
.Bh "The XDR Library"
The XDR library solves data-portability problems.
It allows you to write and read arbitrary C constructs
in a consistent, specified, and well-documented manner.
It makes sense to use the library even when the data
is not shared among machines on a network.
.Pa
The XDR library has filter routines for
strings (null-terminated arrays of bytes),
structures, unions, and arrays, to name a few.
Using more primitive routines,
you can write your own specific XDR routines
to describe arbitrary data structures,
including elements of arrays, arms of unions,
or objects pointed at from other structures.
The structures themselves can contain arrays of arbitrary elements
or pointers to other structures.
.Pa
Let's examine the \f2writer\fP and \f2reader\fP programs more closely.
There is a family of XDR stream creation routines
in which each member treats the stream of bits differently.
In our example, standard I/O routines are used to manipulate data,
so we use \f3xdrstdio_create()\fP.
The parameters to XDR stream creation routines
vary according to their function.
In our example, \f3xdrstdio_create()\fP
takes a pointer to an XDR structure that it initializes,
a pointer to the \f2FILE\fP where
the input or output is performed, and an operation.
The operation can be either \f2XDR_ENCODE\fP
for serializing in the \f2writer\fP program, or
\f2XDR_DECODE\fP for deserializing in the \f2reader\fP program.
.Pa
.Ns N
RPC clients don't need to create XDR streams.
The RPC system itself creates these streams,
which are then passed to the clients.
.Ne
The \f3xdr_long()\fP
primitive is characteristic of most XDR library 
primitives and of all client XDR routines.
First, the routine returns FALSE (0) if it fails, and
TRUE (1) if it succeeds.
Second, for each data type \f2xxx\fP,
there is an associated XDR routine of the form:
.Ps
xdr_xxx(xdrs, fp)
	XDR *xdrs;
	xxx *fp;
{
}
.Pe
In our case, \f2xxx\fP is long and the corresponding XDR routine is
the primitive \f3xdr_long\fP. 
The client could also define an arbitrary structure
\f2xxx\fP.  In this case, the client would also supply the routine
\f2xdr_xxx\fP, describing each field by calling XDR routines
of the appropriate type.
In all cases, the first parameter, \f2xdrs\fP,
can be treated as an opaque handle
and passed to the primitive routines.
.Pa
XDR routines are direction independent;
that is, the same routines are called to serialize or to deserialize data.
This feature is critical for software engineering of portable data.
Calling the same routine for either operation 
almost guarantees that serialized data can also be deserialized.
One routine is used by both the producer and the consumer of networked data.
This is implemented by always passing the address
of an object rather than the object itself. 
The object is modified only in the case of deserialization.
This feature is not shown in our trivial example,
but its value becomes obvious when non-trivial data structures
are passed among machines.
If needed, you can obtain the direction of the XDR operation.
(See the section entitled \*QXDR Operation Directions\*U for details.)
.Pa
Let's look at a slightly more complicated example.
Assume that a person's gross assets and liabilities
are to be exchanged among processes.
Also assume that these values are important enough
to warrant their own data type:
.Ps
struct gnumbers {
	long g_assets;
	long g_liabilities;
};
.Pe
The corresponding XDR routine describing this structure would be:
.Ps
bool_t  		/* TRUE is success, FALSE is failure */
xdr_gnumbers(xdrs, gp)
	XDR *xdrs;
	struct gnumbers *gp;
{
	if (xdr_long(xdrs, &gp->g_assets) &&
	    xdr_long(xdrs, &gp->g_liabilities))
		return(TRUE);
	return(FALSE);
}
.Pe
Note that the parameter \f2xdrs\fP is never inspected or modified;
it is simply passed on to the subcomponent routines.
It is imperative to inspect the return value of each XDR routine call
and, if the subroutine fails, to give up immediately and return FALSE.
.Pa
This example also shows that the type \f2bool_t\fP
is declared as an integer having the values TRUE
(1) and FALSE (0).  This article uses the following definitions:
.Ps
#define bool_t	int
#define TRUE	1
#define FALSE	0
.sp.5
#define enum_t int	/* enum_t used for generic enums */
.Pe
Keeping these conventions in mind, \f3xdr_gnumbers()\fP
can be rewritten as follows:
.Ps
xdr_gnumbers(xdrs, gp)
	XDR *xdrs;
	struct gnumbers *gp;
{
	return(xdr_long(xdrs, &gp->g_assets) &&
		xdr_long(xdrs, &gp->g_liabilities));
}
.Pe
This document uses both coding styles.
.bp
.Ah "XDR Library Primitives"
This section gives a synopsis of each XDR primitive,
beginning with basic data types and moving on to constructed data types.
XDR utilities are also discussed.
The interface to these primitives and utilities is defined in the include file
\f2<rpc/xdr.h>\fP, which is automatically included by \f2<rpc/rpc.h>\fP.
.Bh "Number Filters"
The XDR library provides primitives to translate between numbers
and their corresponding external representations.
Primitives cover the set of numbers in:
.EQ
[signed, unsigned] * [short, int, long]
.EN
Specifically, the six primitives are:
.Ps
bool_t xdr_int(xdrs, ip)
	XDR *xdrs;
	int *ip;
.sp.5
bool_t xdr_u_int(xdrs, up)
	XDR *xdrs;
	unsigned *up;
.sp.5
bool_t xdr_long(xdrs, lip)
	XDR *xdrs;
	long *lip;
.sp.5
bool_t xdr_u_long(xdrs, lup)
	XDR *xdrs;
	u_long *lup;
.sp.5
bool_t xdr_short(xdrs, sip)
	XDR *xdrs;
	short *sip;
.sp.5
bool_t xdr_u_short(xdrs, sup)
	XDR *xdrs;
	u_short *sup;
.Pe
The first parameter, \f2xdrs\fP, is an XDR stream handle.
The second parameter is the address of the number
that provides data to the stream or that receives data from it.
All routines return TRUE if they complete successfully, and
FALSE otherwise.
.Bh "Floating Point Filters"
The XDR library also provides primitive routines
for the C floating point types:
.Ps
bool_t xdr_float(xdrs, fp)
	XDR *xdrs;
	float *fp;
.sp.5
bool_t xdr_double(xdrs, dp)
	XDR *xdrs;
	double *dp;
.Pe
The first parameter, \f2xdrs\fP, is an XDR stream handle.
The second parameter is the address
of the floating point number that provides data to the stream
or that receives data from it.  All routines return TRUE
if they complete successfully, and FALSE otherwise.
.Pa
.Ns N
Because the numbers are represented in IEEE floating point,
routines can fail when decoding a valid IEEE representation
into a machine-specific representation, or vice-versa.
.Ne
.Bh "Enumeration Filters"
The XDR library provides a primitive for generic enumerations.
The primitive assumes that a C \f2enum\fP
has the same representation inside the machine as a C integer.
The boolean type is an important instance of the \f2enum\fP.
The external representation of a boolean is always one
(TRUE) or zero (FALSE).
.Ps
#define bool_t	int
#define FALSE	0
#define TRUE	1
.sp.5
#define enum_t int
.sp.5
bool_t xdr_enum(xdrs, ep)
	XDR *xdrs;
	enum_t *ep;
.sp.5
bool_t xdr_bool(xdrs, bp)
	XDR *xdrs;
	bool_t *bp;
.Pe
The second parameters (\f2ep\fP and \f2bp\fP)
are addresses of the associated type
that provides data to or receives data from the stream
\f2xdrs\fP.  The routines return TRUE
if they complete successfully, and FALSE otherwise.
.Bh "No Data"
Occasionally an XDR routine must be supplied to the RPC system,
even when no data is passed or required.
The library provides such a routine:
.Ps
bool_t xdr_void();  /* always returns TRUE */
.Pe
.Bh "Constructed Data Type Filters"
Constructed or compound data type primitives
require more parameters and perform more complicated functions
then the primitives discussed earlier.
This section includes primitives for
strings, arrays, unions, and pointers to structures.
.Pa
Constructed data type primitives can use memory management.
In many cases, memory is allocated when deserializing data with
\f2XDR_DECODE\fP.
Therefore, the XDR package must provide means to deallocate memory.
This is done by the XDR operation \f2XDR_FREE\fP.
To review, the three XDR directional operations are
\f2XDR_ENCODE\fP, \f2XDR_DECODE\fP, and \f2XDR_FREE\fP.
.Ch "Strings"
In C, a string is defined as a sequence of bytes
terminated by a null byte,
which is not considered when calculating string length.
However, when a string is passed or manipulated,
a pointer to it is employed.
Therefore, the XDR library defines a string to be a
\f2char *\fP, rather than a sequence of characters.
The external representation of a string is drastically different
from its internal representation.
Externally, strings are represented as
sequences of ASCII characters,
while internally, they are represented with character pointers.
Conversion between the two representations
is accomplished with the routine \f3xdr_string()\fP:
.Ps
bool_t xdr_string(xdrs, sp, maxlength)
	XDR *xdrs;
	char **sp;
	u_int maxlength;
.Pe
The first parameter, \f2xdrs\fP, is the XDR stream handle.
The second parameter, \f2sp\fP, is a pointer to a string (type
\f2char **\fP).  The third parameter, \f2maxlength\fP,
specifies the maximum number of bytes allowed during encoding or decoding.
Its value is usually specified by a protocol.
For example, a protocol specification might say
that a file name can be no longer than 255 characters.
If the number of characters exceeds \f2maxlength\fP, 
the routine returns FALSE.
If \f2maxlength\fP is not exceeded, TRUE is returned.
.Pa
The behavior of \f3xdr_string()\fP
is similar to the behavior of other routines
discussed in this section.  The direction
\f2XDR_ENCODE\fP is easiest to understand.  The parameter
\f2sp\fP points to a string of a certain length.
If the string does not exceed \f2maxlength\fP,
the bytes are serialized.
.Pa
The effect of deserializing a string is subtle.
First the length of the incoming string is determined;
it must not exceed \f2maxlength\fP.
Next \f2sp\fP is dereferenced.  If the value is
NULL, then a string of the appropriate length is allocated and
\f2*sp\fP is set to this string.
If the original value of \f2*sp\fP
is non-null, then the XDR package assumes
that a target area has been allocated.
(The target area can hold strings no longer than
\f2maxlength\fP.)
In either case, the string is decoded into the target area.
The routine then appends a null character to the string.
.Pa
In the \f2XDR_FREE\fP
operation, the string is obtained by dereferencing
\f2sp\fP.  If the string is not NULL,
it is freed and \f2*sp\fP is set to NULL.
In this operation, \f3xdr_string\fP
ignores the \f2maxlength\fP parameter.
.Ch "Byte Arrays"
Frequently variable-length arrays of bytes are preferable to strings.
Byte arrays differ from strings in the following ways: 
.Ls 1
.Li
The length of the array (the byte count) is explicitly
located in an unsigned integer.
.Li
The byte sequence is not terminated by a null character. 
.Li
The external representation of the bytes is the same as the
internal representation.
.Le
The primitive \f3xdr_bytes()\fP is used to
convert between the internal and external
representations of byte arrays:
.Ps
bool_t xdr_bytes(xdrs, bpp, lp, maxlength)
	XDR *xdrs;
	char **bpp;
	u_int *lp;
	u_int maxlength;
.Pe
The first, second and fourth parameters
are identical to the first, second and third parameters of
\f3xdr_string()\fP, respectively.
The length of the byte area is obtained by dereferencing
\f2lp\fP when serializing.  \f2*lp\fP
is set to the byte length when deserializing.
.Ch "Arrays"
The XDR library package provides a primitive
for handling arrays of arbitrary elements.
The \f3xdr_bytes()\fP
routine treats a subset of generic arrays
in which the size of array elements is known to be 1
and the external description of each element is built-in.
The generic array primitive, \f3xdr_array()\fP,
requires parameters identical to those of
\f3xdr_bytes()\fP plus two additional parameters.
These parameters are the size of the array elements
and an XDR routine to handle each of the elements.
This routine is called to encode or decode
each element of the array:
.sp
.As
bool_t
xdr_array(xdrs, ap, lp, maxlength, elementsiz, xdr_element)
	XDR *xdrs;
	char **ap;
	u_int *lp;
	u_int maxlength;
	u_int elementsiz;
	bool_t (*xdr_element)();
.Ae
.sp
The parameter \f2ap\fP
is the address of the pointer to the array.
If \f2*ap\fP is NULL
when the array is being deserialized,
XDR allocates an array of the appropriate size and sets
\f2*ap\fP to that array.
The element count of the array is obtained from
\f2*lp\fP when the array is serialized.
\f2*lp\fP is set to the array length when the array is deserialized. 
The parameter \f2maxlength\fP
is the maximum number of elements that the array is allowed to have.
\f2elementsiz\fP is the byte size of each element of the array.
(The C function \f3sizeof()\fP
can be used to obtain this value.)
The routine \f3xdr_element\fP
is called to serialize, deserialize, or free
each element of the array.
.Ch "Examples"
Before defining more constructed data types,
let's look at three examples.
.Pa
\f4Example A:\fP
.Pa
A user on a networked machine can be identified by: 
.Ls a
.Li
The machine name, such as \f2machine_a\fP
(see \f2gethostname\fP(3)). 
.Li
The user's UID (see \f2geteuid\fP(2)).
.Li
The group numbers to which the user belongs (see
\f2getgroups\fP(2)).
.Le
.Pa
A structure containing this information and its associated XDR routine
could be coded like this:
.sp
.As
struct netuser {
	char	*nu_machinename;
	int 	nu_uid;
	u_int	nu_glen;
	int 	*nu_gids;
};
#define NLEN 255	/* machine names < 256 chars */
#define NGRPS 20	/* user can't be in > 20 groups */
.sp.5
bool_t
xdr_netuser(xdrs, nup)
	XDR *xdrs;
	struct netuser *nup;
{
	return(xdr_string(xdrs, &nup->nu_machinename, NLEN) &&
	    xdr_int(xdrs, &nup->nu_uid) &&
	    xdr_array(xdrs, &nup->nu_gids, &nup->nu_glen, NGRPS,
		sizeof (int), xdr_int));
}
.Ae
.sp
\f4Example B:\fP
.Pa
A party of network users could be implemented
as an array of \f2netuser\fP structure.
The declaration and its associated XDR routines
are: 
.sp
.As
struct party {
	u_int p_len;
	struct netuser *p_nusers;
};
#define PLEN 500 /* max number of users in a party */
.sp.5
bool_t
xdr_party(xdrs, pp)
	XDR *xdrs;
	struct party *pp;
{
	return(xdr_array(xdrs, &pp->p_nusers, &pp->p_len, PLEN,
	    sizeof (struct netuser), xdr_netuser));
}
.Ae
.sp
.Pa
\f4Example C:\fP
.Pa
The well-known parameters to \f3main()\fP, \f3argc\fP,
and \f3argv\fP can be combined into a structure.
An array of these structures can make up a history of commands.
The declarations and XDR routines might look like this:
.sp
.As 
struct cmd {
	u_int c_argc;
	char **c_argv;
};
#define ALEN 1000	/* args cannot be > 1000 chars */
#define NARGC 100	/* commands cannot have > 100 args */
.sp.5
struct history {
	u_int h_len;
	struct cmd *h_cmds;
};
#define NCMDS 75  /* history is no more than 75 commands */
.sp.5
bool_t
xdr_wrap_string(xdrs, sp)
	XDR *xdrs;
	char **sp;
{
	return(xdr_string(xdrs, sp, ALEN));
}
.sp.5
bool_t
xdr_cmd(xdrs, cp)
	XDR *xdrs;
	struct cmd *cp;
{
	return(xdr_array(xdrs, &cp->c_argv, &cp->c_argc, NARGC,
	    sizeof (char *), xdr_wrap_string));
}
.sp.5
bool_t
xdr_history(xdrs, hp)
	XDR *xdrs;
	struct history *hp;
{
	return(xdr_array(xdrs, &hp->h_cmds, &hp->h_len, NCMDS,
	    sizeof (struct cmd), xdr_cmd));
}
.Ae
.sp
The most confusing part of this example is that the routine
\f3xdr_wrap_string()\fP is needed to package the
\f3xdr_string()\fP routine.  Because the implementation of
\f3xdr_array()\fP passes only two parameters to the array
element description routine,
\f3xdr_wrap_string()\fP is used to supply the third parameter. 
.Pa
By now the recursive nature of the XDR library should be obvious.
Let's continue with more constructed data types.
.Ch "Opaque Data"
In some protocols, handles are passed from a server to a client, and
the client passes the handle back to the server at a later time.
Handles are never inspected by clients;
they are obtained and submitted.
In other words, handles are opaque.
The primitive \f3xdr_opaque()\fP
is used for describing fixed-size, opaque bytes:
.Ps
bool_t xdr_opaque(xdrs, p, len)
	XDR *xdrs;
	char *p;
	u_int len;
.Pe
The parameter \f2p\fP is the location of the bytes;
\f2len\fP is the number of bytes in the opaque object.
By definition, the actual data
contained in the opaque object are not machine-portable.
.Ch "Fixed-Size Arrays"
The XDR library does not provide a primitive for fixed-length arrays
(the primitive \f3xdr_array()\fP is for varying-length arrays).
To use fixed-size arrays with Example A, it could be rewritten 
as follows:
.sp
.As 
#define NLEN 255	/* machine names must be < 256 chars */
#define NGRPS 20	/* user can't belong to > 20 groups */
.sp.5
struct netuser {
	char *nu_machinename;
	int nu_uid;
	int nu_gids[NGRPS];
};
.sp.5
bool_t
xdr_netuser(xdrs, nup)
	XDR *xdrs;
	struct netuser *nup;
{
	int i;
.sp.5
	if (!xdr_string(xdrs, &nup->nu_machinename, NLEN))
		return(FALSE);
	if (!xdr_int(xdrs, &nup->nu_uid))
		return(FALSE);
	for (i = 0; i < NGRPS; i++) {
		if (!xdr_int(xdrs, &nup->nu_gids[i]))
			return(FALSE);
	}
	return(TRUE);
}
.Ae
.sp
\f4Exercise:\fP  \ 
Rewrite Example A so that it uses varying-length arrays and so that the
\f2netuser\fP structure contains the actual \f2nu_gids\fP
array body as shown in the previous example.
.Ch "Discriminated Unions"
The XDR library supports discriminated unions.
A discriminated union is a C union and an \f2enum_t\fP
value that selects an \f2arm\fP of the union:
.Ps
struct xdr_discrim {
	enum_t value;
	bool_t (*proc)();
};
.sp.5
bool_t xdr_union(xdrs, dscmp, unp, arms, defaultarm)
	XDR *xdrs;
	enum_t *dscmp;
	char *unp;
	struct xdr_discrim *arms;
	bool_t (*defaultarm)();  /* may equal NULL */
.Pe
First, the routine translates the discriminant of the union located at 
\f2*dscmp\fP.  (The discriminant is always an \f2enum_t\fP.)
Next, the union located at \f2*unp\fP is translated.
The parameter \f2arms\fP is a pointer to an array of
\f2xdr_discrim\fP structures. 
Each structure contains an order pair of
[\f2value,proc\fP].
If the union's discriminant is equal to the associated
\f2value\fP, then the \f2proc\fP
is called to translate the union.
The end of the \f2xdr_discrim\fP
structure array is denoted by a routine having the value
NULL (0).  If the discriminant is not found in the
\f2arms\fP array and it is non-null, the \f2defaultarm\fP
procedure is called;
otherwise the routine returns FALSE.
.Pa
\f4Example D:\fP
.Pa
Suppose the type of a union can be integer,
character pointer (a string), or a \f2gnumbers\fP
structure.
Also, assume the union and its current type
are declared in a structure.
The declaration is:
.Ps
enum utype { INTEGER=1, STRING=2, GNUMBERS=3 };
.sp.5
struct u_tag {
	enum utype utype;	/* the union's discriminant */
	union {
		int ival;
		char *pval;
		struct gnumbers gn;
	} uval;
};
.Pe
The following constructs and XDR procedure (de)serialize
the discriminated union:
.Ps
struct xdr_discrim u_tag_arms[4] = {
	{ INTEGER, xdr_int },
	{ GNUMBERS, xdr_gnumbers }
	{ STRING, xdr_wrap_string },
	{ __dontcare__, NULL }
	/* always terminate arms with a NULL xdr_proc */
}
.sp.5
bool_t
xdr_u_tag(xdrs, utp)
	XDR *xdrs;
	struct u_tag *utp;
{
	return(xdr_union(xdrs, &utp->utype, &utp->uval,
		u_tag_arms, NULL));
}
.Pe
The routine \f3xdr_gnumbers()\fP
was presented in the section \*QThe XDR Library.\*U
\f3xdr_wrap_string()\fP was presented in Example C.
The default arm parameter to \f3xdr_union()\fP
(the last parameter) is NULL
in this example.  Therefore, the value of the union's discriminant
can legally take on only the values listed in the \f2u_tag_arms\fP
array.  This example also demonstrates that
the elements of the arm's array do not need to be sorted.
.Pa
It is worth pointing out that the values of the discriminant
may be sparse, although in this example they are not.
It is always good
practice to assign integer values explicitly to each element of the
discriminant's type.
This practice documents the external
representation of the discriminant and guarantees that different
C compilers emit identical discriminant values.
.Pa
\f4Exercise:\fP  Using the other primitives in this section, 
implement \f3xdr_union()\fP.
.Ch "Pointers"
In C it is often convenient to place pointers
to another structure within a structure.
The primitive \f3xdr_reference()\fP
makes it easy to serialize, deserialize, and free
these referenced structures:
.Ps
bool_t xdr_reference(xdrs, pp, size, proc)
	XDR *xdrs;
	char **pp;
	u_int ssize;
	bool_t (*proc)();
.Pe
Parameter \f2pp\fP is the address of
the pointer to the structure,
parameter \f2ssize\fP is the size in bytes of the structure
(use the C function \f3sizeof()\fP
to obtain this value), and
\f2proc\fP is the XDR routine that describes the structure.
When decoding data, storage is allocated if
\f2*pp\fP is NULL.
.Pa
Because pointers are always sufficient, there is no need 
for a primitive \f3xdr_struct()\fP
to describe structures within structures.
.Pa
\f4Exercise:\fP  Using \f3xdr_array()\fP, implement
\f3xdr_reference()\fP.
.Ns W
\f1xdr_reference()\fP and \f1xdr_array()\fP
are NOT interchangeable external representations of data.
.Ne
\f4Example E:\fP
.Pa
Suppose there is a structure that contains a person's name
and a pointer to a \f2gnumbers\fP
structure containing the person's gross assets and liabilities.
The construct is:
.Ps
struct pgn {
	char *name;
	struct gnumbers *gnp;
};
.Pe
The corresponding XDR routine for this structure is:
.Ps
bool_t
xdr_pgn(xdrs, pp)
	XDR *xdrs;
	struct pgn *pp;
{
	if (xdr_string(xdrs, &pp->name, NLEN) &&
	  xdr_reference(xdrs, &pp->gnp,
	  sizeof(struct gnumbers), xdr_gnumbers))
		return(TRUE);
	return(FALSE);
}
.Pe
.Ch "Pointer Semantics and XDR"
In many applications,
C programmers attach double meaning to the values of a pointer.
Typically, the value NULL
(or zero) means data is not needed,
yet some application-specific interpretation applies.
In essence, the C programmer is encoding
a discriminated union efficiently
by overloading the interpretation of the value of a pointer.
For instance, in Example E a NULL
pointer value for \f2gnp\fP could indicate that
the person's assets and liabilities are unknown.
That is, the pointer value encodes two things:
Whether or not the data is known,
and if it is known, where it is located in memory.
Linked lists are an extreme example of the use
of application-specific pointer interpretation.
.Pa
During serialization, the primitive \f3xdr_reference()\fP
cannot and does not attach any special
meaning to a null-value pointer. 
When serializing data, passing the address of a pointer whose value is
NULL to \f3xdr_reference()\fP
will most likely cause a memory fault and, on
UNIX, a core dump for debugging.
.Pa
It is the explicit responsibility of the programmer
to expand non-referenceable pointers into their specific semantics.
This usually involves describing data with a two-armed discriminated union.
One arm is used when the pointer is valid,
the other is used when the pointer is invalid (NULL).  
The \*QAdvanced Topics\*U section 
has an example (linked lists encoding) that deals
with invalid pointer interpretation.
.Pa
\f4Exercise:\fP \  
After reading the \*QAdvanced Topics\*U section, 
extend Example E so that
it can deal with null pointer values correctly.
.Pa
\f4Exercise:\fP  Using the \f3xdr_union()\fP,
\f3xdr_reference()\fP, and \f3xdr_void()\fP
primitives, implement a generic pointer-handling primitive
that implicitly deals with NULL
pointers.  The XDR library does not provide such a primitive
because it does not want to give the illusion
that pointers have meaning in the external world.
.Bh "Non-filter Primitives"
XDR streams can be manipulated with
the primitives discussed in this section:
.Ps
u_int xdr_getpos(xdrs)
	XDR *xdrs;
.sp.5
bool_t xdr_setpos(xdrs, pos)
	XDR *xdrs;
	u_int pos;
.sp.5
xdr_destroy(xdrs)
	XDR *xdrs;
.Pe
The routine \f3xdr_getpos()\fP returns an unsigned integer
that describes the current position in the data stream.
.Ns W
In some XDR streams, the returned value of \f1xdr_getpos()\fP
is meaningless.  The routine returns a \-1 in this case
(although \-1 should be a legitimate value).
.Ne
The routine \f3xdr_setpos()\fP sets a stream position to
\f2pos\fP.
.Ns W
In some XDR streams, setting a position is impossible.
In such cases, \f1xdr_setpos()\fP will return FALSE.
.Ne
This routine will also fail if the requested position is out-of-bounds.
The definition of bounds varies from stream to stream.
.Pa
The \f3xdr_destroy()\fP
primitive destroys the XDR stream.
After calling this routine, usage of the stream
is undefined.
.Bh "XDR Operation Directions"
At times you might want to optimize XDR routines by taking
advantage of the direction of the operation:
\f2XDR_ENCODE\fP, \f2XDR_DECODE\fP, or \f2XDR_FREE\fP.
The value \f2xdrs->x_op\fP always contains the
direction of the XDR operation.
Because programmers are not encouraged to take advantage of this information,
an example is not presented here.
However, an example in the \*QAdvanced Topics\*U section
demonstrates the usefulness of the \f2xdrs->x_op\fP field.
.Ah "XDR Stream Access"
An XDR stream is obtained by calling the appropriate creation routine.
These creation routines take arguments that are tailored to the
specific properties of the stream.
.Pa
Streams currently exist for (de)serialization of data to or from
standard I/O \f2FILE\fP
streams, TCP/IP connections and
UNIX files, and memory.
The section \*QXDR Standard\*U, found later in this document,
documents the XDR object and tells how to make
new XDR streams as required.
.Bh "Standard I/O Streams"
XDR streams can be interfaced to standard I/O by using the
\f3xdrstdio_create()\fP routine:
.Ps
#include <stdio.h>
#include <rpc/rpc.h>	/* xdr streams part of rpc */
.sp.5
void
xdrstdio_create(xdrs, fp, x_op)
	XDR *xdrs;
	FILE *fp;
	enum xdr_op x_op;
.Pe
The routine \f3xdrstdio_create()\fP
initializes the XDR stream pointed to by \f2xdrs\fP.
The XDR stream interfaces to the standard I/O library.
Parameter \f2fp\fP is an open file, and
\f2x_op\fP is an XDR direction.
.Bh "Memory Streams"
Memory streams allow the streaming of data into or out of
a specified area of memory:
.Ps
#include <rpc/rpc.h>
.sp.5
void
xdrmem_create(xdrs, addr, len, x_op)
	XDR *xdrs;
	char *addr;
	u_int len;
	enum xdr_op x_op;
.Pe
The routine \f3xdrmem_create()\fP
initializes an XDR stream in local memory.
The memory is pointed to by parameter \f2addr\fP;
parameter \f2len\fP is the length in bytes of the memory.
The parameters \f2xdrs\fP and \f2x_op\fP
are identical to the corresponding parameters of
\f3xdrstdio_create()\fP.
Currently, the UDP/IP implementation of RPC uses
\f3xdrmem_create()\fP.
Complete call or result messages are built in memory before calling the
\f3sendto()\fP system routine.
.Bh "Record (TCP/IP) Streams"
A record stream is an XDR stream built on top of
a record marking standard that is built on top of the
UNIX file or 4.2 BSD connection interface.
.Ps
#include <rpc/rpc.h>	/* xdr streams part of rpc */
.sp.5
xdrrec_create(xdrs,
  sendsize, recvsize, iohandle, readproc, writeproc)
	XDR *xdrs;
	u_int sendsize, recvsize;
	char *iohandle;
	int (*readproc)(), (*writeproc)();
.Pe
The routine \f3xdrrec_create()\fP
provides an XDR stream interface that allows for a bidirectional,
arbitrarily long sequence of records.
The contents of the records are meant to be data in XDR form.
The primary use of the stream is to interface RPC to TCP connections.
However, it can be used to stream data into or out of normal
UNIX files.
.Pa
The parameter \f2xdrs\fP
is identical to the corresponding parameter of \f3xdrstdio_create\fP.
The stream does its own data buffering, similar to the buffering done in
standard I/O.
The parameters \f2sendsize\fP and \f2recvsize\fP
determine the size in bytes of the output and input buffers respectively.
If their values are zero (0), then predetermined defaults are used.
When a buffer needs to be filled or flushed, either the
\f3readproc\fP or \f3writeproc\fP routine is called. 
The usage and behavior of these
routines are similar to the UNIX system calls \f3read()\fP
and \f3write()\fP.  However,
the first parameter to each of these routines is the opaque parameter
\f2iohandle\fP.  The other two parameters (\f2buf\fP and
\f2nbytes\fP) and the results
(byte count) are identical to the system routines.
If \f2xxx\fP is either \f3readproc\fP or \f3writeproc\fP,
it has the following form:
.Ps
/*
 * returns the actual number of bytes transferred.
 * -1 is an error
 */
int
xxx(iohandle, buf, len)
	char *iohandle;
	char *buf;
	int nbytes;
.Pe
The XDR stream provides a means for delimiting records in the byte stream.
(The implementation details of delimiting records in a stream
are discussed in the \*QSynopsis of XDR Routines\*U section of this document.)
The primitives that are specific to record streams are as follows:
.Ps
bool_t
xdrrec_endofrecord(xdrs, flushnow)
	XDR *xdrs;
	bool_t flushnow;
.sp.5
bool_t
xdrrec_skiprecord(xdrs)
	XDR *xdrs;
.sp.5
bool_t
xdrrec_eof(xdrs)
	XDR *xdrs;
.Pe
The routine \f3xdrrec_endofrecord()\fP
causes the current outgoing data to be marked as a record.
If the parameter \f2flushnow\fP is TRUE,
then the stream's \f3writeproc()\fP
will be called.  Otherwise,
\f3writeproc()\fP will be called when the output buffer has been filled.
.Pa
The routine \f3xdrrec_skiprecord()\fP
causes the position of an input stream to be moved past
the current record boundary and onto the
beginning of the next record in the stream.
.Pa
If there is no more data in the stream's input buffer,
then the routine \f3xdrrec_eof()\fP returns TRUE.
There may still be more data
in the underlying file descriptor.
.Ah "XDR Stream Implementation"
This section provides the abstract data types needed
to implement new instances of XDR streams.
.Bh "The XDR Object"
The following structure defines the interface to an XDR stream:
.sp
.As
enum xdr_op { XDR_ENCODE=0, XDR_DECODE=1, XDR_FREE=2 };
.sp.5
typedef struct {
	enum xdr_op x_op;	/* operation; fast added param */
	struct xdr_ops {
		bool_t  (*x_getlong)();  /* get long from stream */
		bool_t  (*x_putlong)();  /* put long to stream */
		bool_t  (*x_getbytes)(); /* get bytes from stream */
		bool_t  (*x_putbytes)(); /* put bytes to stream */
		u_int   (*x_getpostn)(); /* return stream offset */
		bool_t  (*x_setpostn)(); /* reposition offset */
		caddr_t (*x_inline)();   /* ptr to buffered data */
		VOID    (*x_destroy)();  /* free private area */
	} *x_ops;
	caddr_t	x_public;	/* users' data */
	caddr_t	x_private;	/* pointer to private data */
	caddr_t	x_base;		/* private for position info */
	int		x_handy;	/* extra private word */
} XDR;
.Ae
.sp
The \f2x_op\fP
field is the current operation being performed on the stream.
Although this field is important to the XDR primitives,
it should not affect a stream's implementation.
That is, a stream's implementation should not depend
on this value.
The fields \f2x_private\fP, \f2x_base\fP, and
\f2x_handy\fP are private to the particular
stream's implementation.
The field \f2x_public\fP 
is for the XDR client and should never be used by
the XDR stream implementations or the XDR primitives.
.Pa
Macros for accessing the operations \f3x_getpostn()\fP,
\f3x_setpostn()\fP, and \f3x_destroy()\fP
were defined under \*QNon-filter Primitives\*U earlier in this article. 
The operation \f3x_inline()\fP takes two parameters:
An XDR * and an unsigned integer, which is a byte count.
The routine returns a pointer to a piece of
the stream's internal buffer.
The caller can then use the buffer segment for any purpose.
From the stream's point of view, the bytes in the
buffer segment have been consumed or put.
The routine may return NULL
if it cannot return a buffer segment of the requested size.
(The \f3x_inline\fP routine is for cycle squeezers.
Because the resulting buffer is not data-portable,
we recommend that users do not use this feature.) 
.Pa
The operations \f3x_getbytes()\fP and
\f3x_putbytes()\fP blindly get and put sequences of bytes
from or to the underlying stream.  
They return TRUE
if they are successful, and
FALSE otherwise.  The routines have identical parameters (replace
\f2xxx\fP):
.Ps
bool_t
xxxbytes(xdrs, buf, bytecount)
	XDR *xdrs;
	char *buf;
	u_int bytecount;
.Pe
The operations \f3x_getlong()\fP and \f3x_putlong()\fP
receive and put long numbers from and to the data stream.
It is the responsibility of these routines
to translate the numbers between the machine representation
and the (standard) external representation.
The UNIX primitives \f3htonl()\fP
and \f3ntohl()\fP can be helpful in accomplishing this.
The next section, \*QXDR Standard,\*U 
defines the standard representation of numbers.
The higher-level XDR implementation assumes that
signed and unsigned long integers contain the same number of bits
and that non-negative integers
have the same bit representations as unsigned integers.
The routines return TRUE if they succeed, and FALSE
otherwise.  They have identical parameters:
.Ps
bool_t
xxxlong(xdrs, lp)
	XDR *xdrs;
	long *lp;
.Pe
Implementors of new XDR streams must use a create routine to 
make an XDR structure
(with new operation routines) available to clients.
.Ah "XDR Standard"
This section defines the external data representation standard.
The standard is independent of languages,
operating systems and hardware architectures.
When data is shared among machines, it should not matter where the data
is produced or consumed. 
Similarly the choice of operating systems should have no influence
on how the data is represented externally.
For programming languages, data produced by a C program should be
readable by a Fortran or Pascal program.
.Pa
The external data representation standard depends on the assumption that
bytes (or octets) are portable.
A byte is defined to be eight bits of data.
It is assumed that hardware that encodes bytes onto various media
will preserve the meanings of the bytes across hardware boundaries.
For example, the Ethernet standard suggests that bytes be
encoded \*Qlittle endian\*U style.
The Sequent implementation adheres to this standard.
.Pa
The XDR standard also suggests a language used to describe data.
The language is a variation of C;
it is a data description language, not a programming language.
(The Xerox Courier Standard uses a variation of Mesa
as its data description language.)
.Bh "Basic Block Size"
The representation of all items requires
a multiple of four bytes (or 32 bits) of data.
The bytes are numbered
0 through \f2n\fP\-1, where (\f2n\fP mod 4) = 0.
The bytes are read or written to a byte stream
such that byte \f2m\fP always precedes byte \f2m\fP+1.
.Bh "Integer"
An XDR signed integer is a 32-bit datum
that encodes an integer in the range
\f5[-2147483648,2147483647]\fP. 
The integer is represented in two's complement notation. 
The most and least significant bytes are 0 and 3, respectively.
The data description of integers is \f2integer\fP.
.Bh "Unsigned Integer"
An XDR unsigned integer is a 32-bit datum
that encodes a nonnegative integer in the range
\f5[0,4294967295]\fP.
It is represented by an unsigned binary number whose most
and least significant bytes are 0 and 3, respectively.
The data description of unsigned integers is \f2unsigned\fP.
.Bh "Enumerations"
Enumerations have the same representation as integers.
Enumerations are handy for describing subsets of the integers.
The data description of enumerated data is as follows:
.Ps
typedef enum { name = value, .... } type-name;
.Pe
For example the colors red, yellow and blue
could be described by an enumerated type:
.Ps
typedef enum { RED = 2, YELLOW = 3, BLUE = 5 } colors;
.Pe
.Bh "Booleans"
Booleans are important enough and occur frequently enough
to warrant their own explicit type in the standard.
Boolean is an enumeration with the following form:
.Ps
typedef enum { FALSE = 0, TRUE = 1 } boolean;
.Pe
.Bh "Hyper Integer and Hyper Unsigned"
The standard also defines 64-bit (8-byte) numbers called 
\f2hyper integer\fP and \f2hyper unsigned\fP.
Their representations are the obvious extensions of 
the integer and unsigned integer defined earlier.
The most and least significant bytes are 0 and 7, respectively.
.Bh "Floating Point and Double Precision"
The standard defines the encoding for the floating point data types
\f2float\fP (32 bits or 4 bytes) and \f2double\fP
(64 bits or 8 bytes).
The encoding used is the IEEE standard for normalized
single- and double-precision floating point numbers.
(See the IEEE floating point standard for more information.)
The standard encodes the following three fields
that describe the floating point number: 
.IP \f3S\fP
The sign of the number.
Values 0 and 1 represent positive and negative, respectively.
.IP \f3E\fP
The exponent of the number, base 2.
Floats devote 8 bits to this field,
while doubles devote 11 bits.
The exponents for float and double are
biased by 127 and 1023, respectively.
.IP \f3F\fP
The fractional part of the number's mantissa, base 2.
Floats devote 23 bits to this field,
while doubles devote 52 bits.
.Pa
Therefore, the floating point number is described by:
.EQ
(-1) sup S * 2 sup { E - Bias } * 1.F
.EN
.Pa
Just as the most and least significant bytes of a number are 0 and 3,
the most and least significant bits of
a single-precision floating point number are 0 and 31.
The beginning bit (and most significant bit) offsets
of \f2S\fP, \f2E\fP, and \f2F\fP are 0, 1, and 9, respectively.
.Pa
Doubles have the analogous extensions.
The beginning bit (and
most significant bit) offsets of \f2S\fP, \f2E\fP, and \f2F\fP
are 0, 1, and 12, respectively.
.Pa
The IEEE specification should be consulted concerning the encoding for
signed zero, signed infinity (overflow), and denormalized numbers (underflow).
Under IEEE specifications, \*QNaN\*U (not a number)
is system dependent and should not be used.
.Bh "Opaque Data"
At times, fixed-size, uninterpreted data
needs to be passed among machines.
This data is called \f2opaque\fP and is described as:
.Ps
typedef opaque type-name[n];
opaque name[n];
.Pe
\f2n\fP
is the (static) number of bytes necessary to contain the opaque data.
If \f2n\fP is not a multiple of four, then the \f2n\fP
bytes are followed by enough zero-valued bytes (up to 3)
to make the total byte count of the opaque object a multiple of four.
.Bh "Counted Byte Strings"
The standard defines a string of \f2n\fP bytes (numbered 0 through \f2n\fP\-1)
to be the number \f2n\fP, encoded as \f2unsigned\fP,
and followed by the \f2n\fP bytes of the string.
If \f2n\fP is not a multiple of four, then the \f2n\fP bytes are followed by
enough zero-valued bytes (up to 3)
to make the total byte count a multiple of four.
The data description of strings is:
.Ps
typedef string type-name<N>;
typedef string type-name<>;
string name<N>;
string name<>;
.Pe
Note that the data description language uses angle brackets (< and >)
to denote anything that is of varying length
(as opposed to square brackets to denote fixed-length sequences of data).
.Pa
The constant \f2N\fP
denotes an upper bound of the number of bytes that a
string may contain.  If \f2N\fP
is not specified, it is assumed to be $2 sup 32 - 1$,
the maximum length.
The constant \f2N\fP
would normally be found in a protocol specification.
For example, a filing protocol might state
that a file name can be no longer than 255 bytes, such as:
.Ps
string filename<255>;
.Pe
The XDR specification does not say what the
individual bytes of a string represent;
this information is left to higher-level specifications.
A reasonable default is to assume
that the bytes encode ASCII characters.
.Bh "Fixed Arrays"
The data description for fixed-size arrays of
homogeneous elements is as follows:
.Ps
typedef elementtype type-name[n];
elementtype name[n];
.Pe
Fixed-size arrays of elements (numbered 0 through \f2n\fP-1)
are encoded by individually encoding the elements of the array
in their natural order, 0 through \f2n\fP-1.
.Bh "Counted Arrays"
Counted arrays provide the ability to encode variable-length arrays
of homogeneous elements.  The array is encoded as
the element count \f2n\fP (an unsigned integer)
followed by the encoding of each of the array's elements,
starting with element 0 and progressing through element \f2n\fP\-1.
The data description for counted arrays
is similar to that of counted strings:
.Ps
typedef elementtype type-name<N>;
typedef elementtype type-name<>;
elementtype name<N>;
elementtype name<>;
.Pe
Again, the constant N
specifies the maximum acceptable
element count of an array.  If N
is not specified, it is assumed to be $2 sup 32 - 1$.
.Bh "Structures"
The data description for structures is very similar to
that of standard C:
.Ps
typedef struct {
	component-type component-name;
	...
} type-name;
.Pe
The components of the structure are encoded 
in the order of their declaration in the structure.
.Bh "Discriminated Unions"
A discriminated union is a type composed of a discriminant followed by a type
selected from a set of prearranged types according to the value of the
discriminant.
The type of the discriminant is always an enumeration.
The component types are called \*Qarms\*U of the union.
The discriminated union is encoded as its discriminant followed by
the encoding of the implied arm.
The data description for discriminated unions is as follows:
.Ps
typedef union switch (discriminant-type) {
	discriminant-value: arm-type;
	...
	default: default-arm-type;
} type-name;
.Pe
The default arm is optional.  If it is not specified, then a valid
encoding of the union cannot take on unspecified discriminant values.
Most specifications neither need nor use default arms.
.Bh "Missing Specifications"
Because the standard is based on bytes, it lacks representations 
for bit fields and bitmaps.
This is not to say that no specification should be attempted.
.bp
.Bh "Library Primitive/XDR Standard Cross Reference"
The following table describes the association between
the C library primitives and the standard data types. 
.Pa
.Ts 1-1 "Primitives and Data Types"
.TS
box, center;
cb | cb
r | c. 
.sp.1
C Primitive	XDR Type	
_
xdr_int
xdr_long	integer	
xdr_short
_
xdr_u_int
xdr_u_long	unsigned
xdr_u_short
_
-	hyper integer
	hyper unsigned
_
xdr_float	float	
_
xdr_double	double	
_
xdr_enum	enum_t	
_
xdr_bool	bool_t	
_
xdr_string	string	
xdr_bytes		
_
xdr_array	(varying arrays)	
_
-	(fixed arrays)	
_
xdr_opaque	opaque	
_
xdr_union	union	
_
xdr_reference	-	
_
-	struct	
.TE
.Te 
.bp
.Ah "Advanced Topics"
This section describes techniques for passing data structures
that are not covered in the preceding sections.
Such structures include linked lists of arbitrary lengths.
Unlike the simpler examples covered in the earlier sections,
the following examples are written using both
the XDR C library routines and the XDR data description language.
(The XDR data definition language is described in \*QThe XDR Standard\*U 
earlier in this document.)
.Bh "Linked Lists"
The last example in the section entitled \*QThe XDR Library\*U 
presented a C data structure and its
associated XDR routines for a person's gross assets and liabilities.
The example is duplicated below:
.Ps
struct gnumbers {
	long g_assets;
	long g_liabilities;
};
.sp.5
bool_t
xdr_gnumbers(xdrs, gp)
	XDR *xdrs;
	struct gnumbers *gp;
{
	if (xdr_long(xdrs, &(gp->g_assets)))
		return(xdr_long(xdrs, &(gp->g_liabilities)));
	return(FALSE);
}
.Pe
Now assume that we want to implement a linked list of such information.
A data structure could be constructed as follows:
.Ps
typedef struct gnnode {
	struct gnumbers gn_numbers;
	struct gnnode *nxt;
};
.sp.5
typedef struct gnnode *gnumbers_list;
.Pe
The head of the linked list can be thought of as the data object.
That is, the head is not just a convenient shorthand for a structure.
Similarly the \f2nxt\fP
field is used to indicate whether or not the object has terminated.
Unfortunately, if the object continues, the \f2nxt\fP
field is also the address of where it continues.
The link addresses carry no useful information when
the object is serialized.
.Pa
.bp
The XDR data description of this linked list is described by the
recursive type declaration of \f3gnumbers_list\fP:
.Ps
struct gnumbers {
	unsigned g_assets;
	unsigned g_liabilities;
};
.Pe
.Ps
typedef union switch (boolean) {
	case TRUE: struct {
		struct gnumbers current_element;
		gnumbers_list rest_of_list;
	};
	case FALSE: struct {};
} gnumbers_list;
.Pe
In this description,
the boolean indicates if more data follows. 
If the boolean is FALSE,
then it is the last data field of the structure.
If the boolean is TRUE, then it is followed by a \f3gnumbers\fP
structure and (recursively) by a \f3gnumbers_list\fP
(the rest of the object).
Note that the C declaration does not declare a boolean explicitly
(though the \f2nxt\fP
field implicitly carries the information), while
the XDR data description does not declare a pointer explicitly.
.Pa
Hints for writing a set of XDR routines to (de)serialize
a linked list of entries can be taken
from the XDR description of the pointer-less data.
The set consists of the mutually recursive routines
\f3xdr_gnumbers_list\fP, \f3xdr_wrap_list\fP, and
\f3xdr_gnnode\fP:
.Ps
bool_t
xdr_gnnode(xdrs, gp)
	XDR *xdrs;
	struct gnnode *gp;
{
	return(xdr_gnumbers(xdrs, &(gp->gn_numbers)) &&
		xdr_gnumbers_list(xdrs, &(gp->nxt)) );
}
.Pe
.Ps
bool_t
xdr_wrap_list(xdrs, glp)
	XDR *xdrs;
	gnumbers_list *glp;
{
	return(xdr_reference(xdrs, glp, sizeof(struct gnnode),
	    xdr_gnnode));
}
.Pe
.Ps
struct xdr_discrim choices[2] = {
	/*
	 * called if another node needs (de)serializing
	 */
	{ TRUE, xdr_wrap_list },
	/*
	 * called when no more nodes need (de)serializing
	 */
	{ FALSE, xdr_void }
}
.sp.5
bool_t
xdr_gnumbers_list(xdrs, glp)
	XDR *xdrs;
	gnumbers_list *glp;
{
	bool_t more_data;
.sp.5
	more_data = (*glp != (gnumbers_list)NULL);
	return(xdr_union(xdrs, &more_data, glp, choices, NULL);
}
.Pe
The entry routine is \f3xdr_gnumbers_list()\fP.
It translates between the boolean value
\f2more_data\fP and the list pointer values.
If there is no more data, the \f3xdr_union()\fP
primitive calls \f3xdr_void()\fP
and the recursion is terminated.
Otherwise, \f3xdr_union()\fP calls \f3xdr_wrap_list()\fP,
which dereferences the list pointers.
The \f3xdr_gnnode()\fP routine actually (de)serializes data
of the current node of the linked list, and recursively calls
\f3xdr_gnumbers_list()\fP to handle the remainder of the list.
.Pa
You should convince yourself that these routines
function correctly in all three directions
\f2(XDR_ENCODE\fP, \f2XDR_DECODE\fP, and \f2XDR_FREE)\fP
for linked lists of any length (including zero).
Note that the boolean \f2more_data\fP is always initialized, but in the
\f2XDR_DECODE\fP case it is overwritten by an externally generated value.
Also note that the value of \f2bool_t\fP
is lost in the stack.
The essence of the value is reflected in the list's pointers.
.Pa
The unfortunate side effect of (de)serializing a list
with these routines is that the C stack grows linearly
with respect to the number of nodes in the list.
This is due to the recursion.  The routines are also hard to 
code (and understand) due to the number and nature of primitives involved
(such as \f3xdr_reference\fP, \f3xdr_union\fP, and \f3xdr_void\fP).
.Pa
The following routine collapses the recursive routines.
It also has other optimizations that are discussed after the example.
.sp
.As
bool_t
xdr_gnumbers_list(xdrs, glp)
	XDR *xdrs;
	gnumbers_list *glp;
{
	bool_t more_data;
.sp.5
	while (TRUE) {
		more_data = (*glp != (gnumbers_list)NULL);
		if (!xdr_bool(xdrs, &more_data))
			return(FALSE);
		if (!more_data)
			return(TRUE);  /* we are done */
		if (!xdr_reference(xdrs, glp, sizeof(struct gnnode),
		    xdr_gnumbers))
			return(FALSE);
		glp = &((*glp)->nxt); 
	}
}
.Ae
.sp
The claim is that this routine is easier to code and understand than the
three recursive routines shown earlier.
(It is also buggy, as discussed below.)
The parameter \f2glp\fP is treated as the address of the pointer 
to the head of the remainder of the list to be (de)serialized.
Therefore, at the end of the \f2while\fP loop, \f2glp\fP is set to the 
address of the current node's \f2nxt\fP field. 
The discriminated union is implemented in-line.  The variable
\f2more_data\fP has the same use in this routine as in the routines shown
earlier.
Its value is recomputed and re-(de)serialized in each iteration of the loop.
Since \f2*glp\fP is a pointer to a node, the pointer is dereferenced using 
\f3xdr_reference()\fP.
Note that the third parameter is truly the size of a node
(data values plus \f2nxt\fP pointer), while \f3xdr_gnumbers()\fP
only (de)serializes the data values.
We can get away with this tricky optimization only because the
\f2nxt\fP data comes after all legitimate external data.
.Pa
The routine is buggy in the \f2XDR_FREE\fP
case.  The bug is that \f3xdr_reference()\fP
will free the node \f2*glp\fP.
Upon return, the assignment \f5glp = &((*glp)->nxt)\fP
cannot be guaranteed to work, as \f2*glp\fP
is no longer a legitimate node.
The following is a rewrite that works in all cases.
The hard part is to avoid dereferencing a pointer
that has not been initialized or that has been freed.
.sp
.As
bool_t
xdr_gnumbers_list(xdrs, glp)
	XDR *xdrs;
	gnumbers_list *glp;
{
	bool_t more_data;
	bool_t freeing;
	gnumbers_list *next;  /* the next value of glp */
.sp.5
	freeing = (xdrs->x_op == XDR_FREE);
	while (TRUE) {
		more_data = (*glp != (gnumbers_list)NULL);
		if (!xdr_bool(xdrs, &more_data))
			return(FALSE);
		if (!more_data)
			return(TRUE);  /* we are done */
		if (freeing)
			next = &((*glp)->nxt);
		if (!xdr_reference(xdrs, glp, sizeof(struct gnnode),
		    xdr_gnumbers))
			return(FALSE);
		glp = (freeing) ? next : &((*glp)->nxt);
	}
}
.Ae
.sp
Note that this is the first example in this article
that actually inspects the direction of the operation
\f2xdrs->x_op\fP.  
The claim is that the correct iterative implementation is still 
easier to understand or code than the recursive implementation.
It is certainly more efficient with respect to C stack requirements.
.Bh "The Record Marking Standard"
A record is composed of one or more record fragments.
A record fragment is a four-byte header followed by
$ 0 ~ "\f1to\fP" ~ {2 sup 31} - 1$ bytes of fragment data.
The bytes encode an unsigned binary number.
As with XDR integers, the byte order is from highest to lowest.
The number encodes two values: 
A boolean that indicates whether the fragment is the last fragment
of the record (bit value 1 implies the fragment is the last fragment),
and a 31-bit unsigned binary value
that is the length in bytes of the fragment's data.
The boolean value is the high-order bit of the
header; the length is the 31 low-order bits.
.Pa
(Note that this record specification is \f2not\fP
in XDR standard form
and cannot be implemented using XDR primitives!)
.bp
.Ah "Synopsis of XDR Routines"
\f3xdr_array()\fP
.Ps
xdr_array(xdrs, arrp, sizep, maxsize, elsize, elproc)
	XDR *xdrs;
	char **arrp;
	u_int *sizep, maxsize, elsize;
	xdrproc_t elproc;
.Pe
This routine is a filter primitive that translates between arrays
and their corresponding external representations.
The parameter \f2arrp\fP
is the address of the pointer to the array, while
\f2sizep\fP is the address of the element count of the array
(the element count cannot exceed \f2maxsize\fP).
The parameter \f2elsize\fP is the \f3sizeof()\fP
each of the array elements, and \f2elproc\fP
is an XDR filter that translates between
the C form of the array elements and their external representation.  
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_bool()\fP
.Ps
xdr_bool(xdrs, bp)
	XDR *xdrs;
	bool_t *bp;
.Pe
This routine is a filter primitive that translates between booleans (C integers)
and their external representations.
When encoding data, this filter produces values of either one or zero.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_bytes()\fP
.Ps
xdr_bytes(xdrs, sp, sizep, maxsize)
	XDR *xdrs;
	char **sp;
	u_int *sizep, maxsize;
.Pe
This routine is a filter primitive that translates between counted byte strings
and their external representations.
The parameter \f2sp\fP is the address of the string pointer.
The length of the string is located at address
\f2sizep\fP (strings cannot be longer than \f2maxsize\fP).
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_destroy()\fP
.Ps
void
xdr_destroy(xdrs)
	XDR *xdrs;
.Pe
This routine is a macro that invokes the destroy routine
associated with the XDR stream \f2xdrs\fP.
Destruction usually involves freeing private data structures
associated with the stream.  
After invoking \f3xdr_destroy()\fP, the use of \f2xdrs\fP is undefined.
.sp 2
\f3xdr_double()\fP
.Ps
xdr_double(xdrs, dp)
	XDR *xdrs;
	double *dp;
.Pe
This routine is a filter primitive that translates between C \f2double\fP
precision numbers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_enum()\fP
.Ps
xdr_enum(xdrs, ep)
	XDR *xdrs;
	enum_t *ep;
.Pe
This routine is a filter primitive that translates between C \f2enum s\fP
(actually integers) and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_float()\fP
.Ps
xdr_float(xdrs, fp)
	XDR *xdrs;
	float *fp;
.Pe
This routine is a filter primitive that translates between C \f2float\fPs
and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_getpos()\fP
.Ps
u_int
xdr_getpos(xdrs)
	XDR *xdrs;
.Pe
This routine is a macro that invokes the get-position routine
associated with the XDR stream \f2xdrs\fP.
The routine returns an unsigned integer
that indicates the position of the XDR byte stream.
A desirable feature of XDR streams
is that simple arithmetic works with this number,
although the XDR stream instances need not guarantee this.
.sp 2
\f3xdr_inline()\fP
.Ps
long *
xdr_inline(xdrs, len)
	XDR *xdrs;
	int len;
.Pe
This routine is a macro that invokes the in-line routine 
associated with the XDR stream
\f2xdrs\fP.  The routine returns a pointer
to a contiguous piece of the stream's buffer;
\f2len\fP is the byte length of the desired buffer.
Note that the pointer is cast to \f2long *\fP.
.Ns W
\f1xdr_inline()\fP may return \f2NULL\fP
if it cannot allocate a contiguous piece of a buffer.
Therefore, its behavior may vary among stream instances.
It exists for the sake of efficiency.
.Ne
.sp 2
\f3xdr_int()\fP
.Ps
xdr_int(xdrs, ip)
	XDR *xdrs;
	int *ip;
.Pe
This routine is a filter primitive that translates between C integers
and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_long()\fP
.Ps
xdr_long(xdrs, lp)
	XDR *xdrs;
	long *lp;
.Pe
This routine is a filter primitive that translates between C \f2long\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
.bp	
\f3xdr_opaque()\fP
.Ps
xdr_opaque(xdrs, cp, cnt)
	XDR *xdrs;
	char *cp;
	u_int cnt;
.Pe
This routine is a filter primitive that translates 
between fixed-size opaque data
and its external representation.  The parameter \f2cp\fP
is the address of the opaque object, and \f2cnt\fP
is its size in bytes.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_reference()\fP
.Ps
xdr_reference(xdrs, pp, size, proc)
	XDR *xdrs;
	char **pp;
	u_int size;
	xdrproc_t proc;
.Pe
This routine is a primitive that provides pointer chasing within structures.
The parameter \f2pp\fP is the address of the pointer,
\f2size\fP is the \f3sizeof()\fP the structure that
\f2*pp\fP points to, and \f2proc\fP is an XDR procedure
that filters the structure
between its C form and its external representation.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_setpos()\fP
.Ps
xdr_setpos(xdrs, pos)
	XDR *xdrs;
	u_int pos;
.Pe
This routine is a macro that invokes the set position routine 
associated with the XDR stream
\f2xdrs\fP.  The parameter \f2pos\fP is a position value obtained from
\f3xdr_getpos()\fP.
If the XDR stream could be repositioned, one is returned.  If it could
not be repositioned, zero is returned.
.Ns W
Because it is difficult to reposition some types of XDR streams,
this routine may fail with one type of stream and succeed with another. 
.Ne
.sp 2
.bp
\f3xdr_short()\fP
.Ps
xdr_short(xdrs, sp)
	XDR *xdrs;
	short *sp;
.Pe
This routine is a filter primitive that translates between C \f2short\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_string()\fP
.Ps
xdr_string(xdrs, sp, maxsize)
	XDR *xdrs;
	char **sp;
	u_int maxsize;
.Pe
This routine is a filter primitive that translates between C strings and their
corresponding external representations.
Strings cannot cannot be longer than \f2maxsize\fP.
Note that \f2sp\fP is the address of the string's pointer.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_u_int()\fP
.Ps
xdr_u_int(xdrs, up)
	XDR *xdrs;
	unsigned *up;
.Pe
This routine is a filter primitive that translates between C \f2unsigned\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_u_long()\fP
.Ps
xdr_u_long(xdrs, ulp)
	XDR *xdrs;
	unsigned long *ulp;
.Pe
This routine is a filter primitive that translates between C \f2unsigned long\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
.bp
\f3xdr_u_short()\fP
.Ps
xdr_u_short(xdrs, usp)
	XDR *xdrs;
	unsigned short *usp;
.Pe
This routine is a filter primitive that translates 
between C \f2unsigned short\fP
integers and their external representations.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_union()\fP
.Ps
xdr_union(xdrs, dscmp, unp, choices, dfault)
	XDR *xdrs;
	int *dscmp;
	char *unp;
	struct xdr_discrim *choices;
	xdrproc_t dfault;
.Pe
This routine is a filter primitive that translates between a discriminated C
\f2union\fP and its corresponding external representation.  The parameter 
\f2dscmp\fP is the address of the union's discriminant, while
\f2unp\fP is the address of the union.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdr_void()\fP
.Ps
xdr_void()
.Pe
This routine always returns one.
It can be passed to RPC routines that require a function parameter
even though nothing is to be done.
.sp 2
\f3xdr_wrapstring()\fP
.Ps
xdr_wrapstring(xdrs, sp)
	XDR *xdrs;
	char **sp;
.Pe
This routine is a primitive that calls \f3xdr_string\f2(xdrs,sp,MAXUNSIGNED)\f1.
\f2MAXUNSIGNED\fP is the maximum value of an unsigned integer.
This is useful because the RPC package passes
only two parameters to XDR routines, however \f3xdr_string()\fP
(one of the most frequently used primitives) requires three parameters.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
.bp
\f3xdrmem_create()\fP
.Ps
void
xdrmem_create(xdrs, addr, size, op)
	XDR *xdrs;
	char *addr;
	u_int size;
	enum xdr_op op;
.Pe
This routine initializes the XDR stream object pointed to by
\f2xdrs\fP.  The stream's data is written to or read from
a chunk of memory at location \f2addr\fP
whose length is no more than \f2size\fP bytes long.  The
\f2op\fP determines the direction of the XDR stream
(either \f2XDR_ENCODE\fP, \f2XDR_DECODE\fP, or
\f2XDR_FREE\fP).
.sp 2
\f3xdrrec_create()\fP
.Ps
void
xdrrec_create(xdrs,
  sendsize, recvsize, handle, readit, writeit)
	XDR *xdrs;
	u_int sendsize, recvsize;
	char *handle;
	int (*readit)(), (*writeit)();
.Pe
This routine initializes the XDR stream object pointed to by
\f2xdrs\fP.
The stream's data is written to a buffer of size \f2sendsize\fP.
A value of zero indicates the system should use a suitable default.
The stream's data is read from a buffer of size \f2recvsize\fP.
It too can be set to a suitable default by passing a zero value.
When a stream's output buffer is full, \f3writeit()\fP
is called.  Similarly, when a stream's input buffer is empty,
\f3readit()\fP is called.  The behavior of these two routines
is similar to the UNIX system calls \f3read\fP and \f3write\fP,
except that \f2handle\fP is passed to the former routines as the
first parameter.  Note that the XDR stream's \f2op\fP
field must be set by the caller.
.Ns W
This XDR stream implements an intermediate record stream.
Therefore there are additional bytes in the stream
to provide record boundary information.
.Ne
.bp
\f3xdrrec_endofrecord()\fP
.Ps
xdrrec_endofrecord(xdrs, sendnow)
	XDR *xdrs;
	int sendnow;
.Pe
This routine can be invoked only on streams created by
\f3xdrrec_create()\fP.
The data in the output buffer is marked as a completed record,
and the output buffer is optionally written out if \f2sendnow\fP
is non-zero.  
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdrrec_eof()\fP
.Ps
xdrrec_eof(xdrs)
	XDR *xdrs;
	int empty;
.Pe
This routine can be invoked only on streams created by
\f3xdrrec_create()\fP.
After consuming the rest of the current record in the stream,
this routine returns one if the stream has no more input.  If 
there is more input, zero is returned.
.sp 2
\f3xdrrec_skiprecord()\fP
.Ps
xdrrec_skiprecord(xdrs)
	XDR *xdrs;
.Pe
This routine can be invoked only on streams created by
\f3xdrrec_create()\fP.
It tells the XDR implementation that the rest of the current record
in the stream's input buffer should be discarded.
If this routine succeeds, one is returned.  If it fails, zero is returned.
.sp 2
\f3xdrstdio_create()\fP
.Ps
void
xdrstdio_create(xdrs, file, op)
	XDR *xdrs;
	FILE *file;
	enum xdr_op op;
.Pe
This routine initializes the XDR stream object pointed to by
\f2xdrs\fP.
The XDR stream data is written to or read from the Standard I/O stream
\f2file\fP.  The parameter \f2op\fP
determines the direction of the XDR stream (either
\f2XDR_ENCODE\fP, \f2XDR_DECODE\fP, or \f2XDR_FREE\fP).
.Ns W
The destroy routine associated with such XDR streams calls
\f1fflush()\fP on the \f1file\fP stream, but never \f1fclose()\fP.
