/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*  ADA support used by permission of Verdix Corporation
 *  Given Sept 25, 1985 by Steve Zeigler
 *  Author: Ben Priest
 */

#ifndef lint
static char *rcsid = "$Header: a.vi.iskey.c 2.1 86/04/15 $";
#endif

#include <ctype.h>
#include "iskey.h"

iskey(s)
register char *s;
{
	switch(*s++) {
	case 'a':
		if(*s++ != 'c') return(-1);
		if(*s++ != 'c') return(-1);
		if(*s++ != 'e') return(-1);
		if(*s++ != 'p') return(-1);
		if(*s++ != 't') return(-1);
		if(*s == '\0') return(ACCEPT);
		return(-1);
	case 'b':
		if(*s++ != 'o') return(-1);
		if(*s++ != 'd') return(-1);
		if(*s++ != 'y') return(-1);
		if(*s == '\0') return(BODY);
		return(-1);
	case 'c':
		if(*s++ != 'a') return(-1);
		if(*s++ != 's') return(-1);
		if(*s++ != 'e') return(-1);
		if(*s == '\0') return(CASE);
		return(-1);
	case 'd':
		if(*s++ != 'e') return(-1);
		if(*s++ != 'c') return(-1);
		if(*s++ != 'l') return(-1);
		if(*s++ != 'a') return(-1);
		if(*s++ != 'r') return(-1);
		if(*s++ != 'e') return(-1);
		if(*s == '\0') return(DECLARE);
		return(-1);
	case 'f':
		switch(*s++) {
		case 'o':
			if(*s++ != 'r') return(-1);
			if(*s == '\0') return(FOR);
			return(-1);
		case 'u':
			if(*s++ != 'n') return(-1);
			if(*s++ != 'c') return(-1);
			if(*s++ != 't') return(-1);
			if(*s++ != 'i') return(-1);
			if(*s++ != 'o') return(-1);
			if(*s++ != 'n') return(-1);
			if(*s == '\0') return(FUNCTION);
		default:
			return(-1);
		}
	case 'i':
		switch(*s++) {
		case 'f':
			if(*s == '\0') return(IF);
			return(-1);
		case 's':
			if(*s == '\0') return(IS);
			return(-1);
		default:
			return(-1);
		}
	case 'l':
		if(*s++ != 'o') return(-1);
		if(*s++ != 'o') return(-1);
		if(*s++ != 'p') return(-1);
		if(*s == '\0') return(LOOP);
		return(-1);
	case 'p':
		switch(*s++) {
		case 'a':
			if(*s++ != 'c') return(-1);
			if(*s++ != 'k') return(-1);
			if(*s++ != 'a') return(-1);
			if(*s++ != 'g') return(-1);
			if(*s++ != 'e') return(-1);
			if(*s == '\0') return(PACKAGE);
			return(-1);
		case 'r':
			if(*s++ != 'o') return(-1);
			if(*s++ != 'c') return(-1);
			if(*s++ != 'e') return(-1);
			if(*s++ != 'd') return(-1);
			if(*s++ != 'u') return(-1);
			if(*s++ != 'r') return(-1);
			if(*s++ != 'e') return(-1);
			if(*s == '\0') return(PROCEDURE);
		default:
			return(-1);
		}
	case 'r':
		if(*s++ != 'e') return(-1);
		if(*s++ != 'c') return(-1);
		if(*s++ != 'o') return(-1);
		if(*s++ != 'r') return(-1);
		if(*s++ != 'd') return(-1);
		if(*s == '\0') return(RECORD);
		return(-1);
	case 's':
		if(*s++ != 'e') return(-1);
		if(*s++ != 'l') return(-1);
		if(*s++ != 'e') return(-1);
		if(*s++ != 'c') return(-1);
		if(*s++ != 't') return(-1);
		if(*s == '\0') return(SELECT);
		return(-1);
	case 't':
		if(*s++ != 'a') return(-1);
		if(*s++ != 's') return(-1);
		if(*s++ != 'k') return(-1);
		if(*s == '\0') return(TASK);
		return(-1);
	case 'w':
		if(*s++ != 'h') return(-1);
		if(*s++ != 'i') return(-1);
		if(*s++ != 'l') return(-1);
		if(*s++ != 'e') return(-1);
		if(*s == '\0') return(WHILE);
	default:
		return(-1);
	}
}
