/* $Copyright: $
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
 * $Header: llist.h 1.2 1991/05/16 22:31:45 $
 *
 * llist.h
 *	Linked list (double and single) structures and macros.
 *
 * Test for empty, append to tail or insert at head, and dequeue from
 * head supported for both.
 */

/* $Log: llist.h,v $
 *
 */

#ifndef	LLIST_H
#define	LLIST_H

/*
 * Doubly-linked list.  Obvious semantics.
 *
 * DL_APPEND appends at tail of list.  DL_INSERT inserts at head.
 * DL_DEQUEUE dequeues head of list (assumes non-empty list).
 */

typedef	struct	dlink	{		/* double-linked list "link" */
	struct	dlink	*dl_next;
	struct	dlink	*dl_prev;
} dlink_t;

#define	DL_INIT(dl)		{ (dl)->dl_next = (dl)->dl_prev = (dl); }
#define	DL_EMPTY(dl)		( (dl)->dl_next == (dl) )
#define	DL_APPEND(dl,ent)	insque((ent), (dl)->dl_prev)
#define	DL_INSERT(dl,ent)	insque((ent), (dl))
#define	DL_DEQUEUE(dl,ent,type)	{ (ent) = (type) (dl)->dl_next; remque(ent); }

/*
 * Singly-linked list.  Maintains list as circular queue with pointer to
 * tail element.  SL_APPEND appends at tail of list.  SL_INSERT inserts at
 * head.  SL_DEQUEUE dequeues head of list (assumes non-empty list).
 */

typedef	struct	slink	{		/* singly-linked list "link" */
	struct	slink	*sl_next;
} slink_t;

#define	SL_INIT(sl)		{ (sl)->sl_next = NULL; }
#define	SL_EMPTY(sl)		( (sl)->sl_next == NULL )
#define	SL_APPEND(sl, ent) { \
	if (SL_EMPTY(sl)) {\
		((slink_t*)(ent))->sl_next = (sl)->sl_next = (slink_t*)(ent); \
	} else {\
		((slink_t*)(ent))->sl_next = (sl)->sl_next->sl_next; \
		(sl)->sl_next = (sl)->sl_next->sl_next = (slink_t*)(ent); }\
}
#define	SL_INSERT(sl, ent) { \
	if (SL_EMPTY(sl)) { \
		((slink_t*)(ent))->sl_next = (sl)->sl_next = (slink_t*)(ent); \
	} else {\
		((slink_t*)(ent))->sl_next = (sl)->sl_next->sl_next; \
		(sl)->sl_next->sl_next = (slink_t*)(ent); }\
}
#define	SL_DEQUEUE(sl, elt, type) { \
	slink_t *sl_head = (sl)->sl_next->sl_next; \
	if (sl_head == (sl)->sl_next) \
		(sl)->sl_next = NULL; \
	else \
		(sl)->sl_next->sl_next = sl_head->sl_next; \
	(elt) = (type) sl_head; \
}

#endif	LLIST_H
