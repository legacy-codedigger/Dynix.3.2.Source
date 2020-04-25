#include <X11/copyright.h>

/* $Header: EvHand.c 1.1 89/09/20 $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

/*
 * XMenu:	MIT Project Athena, X Window system menu package
 *
 * 	XMenuEventHandler - Set the XMenu asynchronous event handler.
 *
 *	Author:		Tony Della Fera, DEC
 *			December 19, 1985
 *
 */

#include "XMenuInt.h"

XMenuEventHandler(handler)
    int (*handler)();
{
    /*
     * Set the global event handler variable.
     */
    _XMEventHandler = handler;
}

