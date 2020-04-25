/*
 * $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * $Header: msg.h 2.1 1991/06/14 00:27:22 $
 */

/*
 * $Log: msg.h,v $
 *
 */

#ifndef _SYS_MSG_H_

/*
**	IPC Message Facility.
*/

/*
**	Implementation Constants.
*/

#define PMSGM	(PZERO - 4)	/* mutex for messages */
#define	PMSG	(PZERO + 2)	/* message facility sleep priority */
#define SPLMSG	SPLFS

/*
**	Permission Definitions.
*/

#define	MSG_R	0400	/* read permission */
#define	MSG_W	0200	/* write permission */

/*
**	ipc_perm Mode Definitions.
**			NOT USED IN THIS IMPLEMENTATION
*/

#define	MSG_RWAIT	01000	/* a reader is waiting for a message */
#define	MSG_WWAIT	02000	/* a writer is waiting to send */

/*
**	Message Operation Flags.
*/

#define	MSG_NOERROR	010000	/* no error if big message */

/*
**	Structure Definitions.
*/

/*
**	There is one msg queue id data structure for each q in the system.
*/

#ifdef KERNEL
struct Dmsqid_ds {
	struct ipc_perm	Dmsg_perm;	/* operation permission struct */
	struct msg	*Dmsg_first;	/* ptr to first message on q */
	struct msg	*Dmsg_last;	/* ptr to last message on q */
	ushort		Dmsg_cbytes;	/* current # bytes on q */
	ushort		Dmsg_qnum;	/* # of messages on q */
	ushort		Dmsg_qbytes;	/* max # of bytes on q */
	ushort		Dmsg_lspid;	/* pid of last msgsnd */
	ushort		Dmsg_lrpid;	/* pid of last msgrcv */
	time_t		Dmsg_stime;	/* last msgsnd time */
	time_t		Dmsg_rtime;	/* last msgrcv time */
	time_t		Dmsg_ctime;	/* last change time */
};

struct msqid_ds {
	struct Dmsqid_ds Dmsq;
	lock_t		mutex;
	sema_t		msg_readers;	/* block sema for readers */
	sema_t		msg_writers;	/* block sema for writers */
};

#define	msg_perm	Dmsq.Dmsg_perm
#define	msg_first	Dmsq.Dmsg_first
#define	msg_last	Dmsq.Dmsg_last
#define	msg_cbytes	Dmsq.Dmsg_cbytes
#define	msg_qnum	Dmsq.Dmsg_qnum
#define	msg_qbytes	Dmsq.Dmsg_qbytes
#define	msg_lspid	Dmsq.Dmsg_lspid
#define	msg_lrpid	Dmsq.Dmsg_lrpid
#define	msg_stime	Dmsq.Dmsg_stime
#define	msg_rtime	Dmsq.Dmsg_rtime
#define	msg_ctime	Dmsq.Dmsg_ctime

#define MSGGATE		63

#else KERNEL

struct msqid_ds {
	struct ipc_perm	msg_perm;	/* operation permission struct */
	struct msg	*msg_first;	/* ptr to first message on q */
	struct msg	*msg_last;	/* ptr to last message on q */
	ushort		msg_cbytes;	/* current # bytes on q */
	ushort		msg_qnum;	/* # of messages on q */
	ushort		msg_qbytes;	/* max # of bytes on q */
	ushort		msg_lspid;	/* pid of last msgsnd */
	ushort		msg_lrpid;	/* pid of last msgrcv */
	time_t		msg_stime;	/* last msgsnd time */
	time_t		msg_rtime;	/* last msgrcv time */
	time_t		msg_ctime;	/* last change time */
};

#endif KERNEL

/*
**	There is one msg structure for each message that may be in the system.
*/

struct msg {
	struct msg	*msg_next;	/* ptr to next message on q */
	long		msg_type;	/* message type */
	short		msg_ts;		/* message text size */
	short		msg_spot;	/* message text map address */
};

/*
**	User message buffer template for msgsnd and msgrecv system calls.
*/

struct msgbuf {
	long	mtype;		/* message type */
	char	mtext[1];	/* message text */
};

/*
**	Message information structure.
*/

struct msginfo {
	int	msgmap,	/* # of entries in msg map */
		msgmax,	/* max message size */
		msgmnb,	/* max # bytes on queue */
		msgmni,	/* # of message queue identifiers */
		msgssz,	/* msg segment size (should be word size multiple) */
		msgtql;	/* # of system message headers */
	ushort	msgseg;	/* # of msg segments (MUST BE < 32768) */
};
#define _SYS_MSG_H_
#endif  /* _SYS_MSG_H_ */

