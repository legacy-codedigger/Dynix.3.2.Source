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
. \" $Header: 2.1.t 1.3 86/03/06 $
.\" %M% %I% %E%
.sh "Generic operations
.PP
.PP
Many system abstractions support the
operations \f2read\fP, \f2write\fP and \f2ioctl\fP.  We describe
the basics of these common primitives here.
Similarly, the mechanisms whereby normally synchronous operations
may occur in a non-blocking or asynchronous fashion are
common to all system-defined abstractions and are described here.
.NH 3
Read and write
.PP
The \f2read\fP and \f2write\fP system calls can be applied
to communications channels, files, terminals and devices.
They have the form:
.DS
cc = read(fd, buf, nbytes);
result int cc; int fd; result caddr_t buf; int nbytes;

cc = write(fd, buf, nbytes);
result int cc; int fd; caddr_t buf; int nbytes;
.DE
The \f2read\fP call transfers as much data as possible from the
object defined by \f2fd\fP to the buffer at address \f2buf\fP of
size \f2nbytes\fP.  The number of bytes transferred is
returned in \f2cc\fP, which is \-1 if a return occurred before
any data was transferred because of an error or use of non-blocking
operations.
.PP
The \f2write\fP call transfers data from the buffer to the
object defined by \f2fd\fP.  Depending on the type of \f2fd\fP,
it is possible that the \f2write\fP call will accept some portion
of the provided bytes; the user should resubmit the other bytes
in a later request in this case.
Error returns because of interrupted or otherwise incomplete operations
are possible.
.PP
Scattering of data on input or gathering of data for output
is also possible using an array of input/output vector descriptors.
The type for the descriptors is defined in <sys/uio.h> as:
.DS
._f
struct iovec {
	caddr_t	iov_msg;	/* base of a component */
	int	iov_len;	/* length of a component */
};
.DE
The calls using an array of descriptors are:
.DS
cc = readv(fd, iov, iovlen);
result int cc; int fd; struct iovec *iov; int iovlen;

cc = writev(fd, iov, iovlen);
result int cc; int fd; struct iovec *iov; int iovlen;
.DE
Here \f2iovlen\fP is the count of elements in the \f2iov\fP array.
.NH 3
Input/output control
.PP
Control operations on an object are performed by the \f2ioctl\fP
operation:
.DS
ioctl(fd, request, buffer);
int fd, request; caddr_t buffer;
.DE
This operation causes the specified \f2request\fP to be performed
on the object \f2fd\fP.  The \f2request\fP parameter specifies
whether the argument buffer is to be read, written, read and written,
or is not needed, and also the size of the buffer, as well as the
request.
Different descriptor types and subtypes within descriptor types
may use distinct \f2ioctl\fP requests.  For example,
operations on terminals control flushing of input and output
queues and setting of terminal parameters; operations on
disks cause formatting operations to occur; operations on tapes
control tape positioning.
.PP
The names for basic control operations are defined in <sys/ioctl.h>.
.NH 3
Non-blocking and asynchronous operations
.PP
A process that wishes to do non-blocking operations on one of
its descriptors sets the descriptor in non-blocking mode as
described in section 1.5.4.  Thereafter the \f2read\fP call will
return a specific EWOULDBLOCK error indication if there is no data to be
\f2read\fP.  The process may
\f2dselect\fP the associated descriptor to determine when a read is
possible.
.PP
Output attempted when a descriptor can accept less than is requested
will either accept some of the provided data, returning a shorter than normal
length, or return an error indicating that the operation would block.
More output can be performed as soon as a \f2select\fP call indicates
the object is writeable.
.PP
Operations other than data input or output
may be performed on a descriptor in a non-blocking fashion.
These operations will return with a characteristic error indicating
that they are in progress
if they cannot return immediately.  The descriptor
may then be \f2select\fPed for \f2write\fP to find out
when the operation can be retried.  When \f2select\fP indicates
the descriptor is writeable, a respecification of the original
operation will return the result of the operation.
