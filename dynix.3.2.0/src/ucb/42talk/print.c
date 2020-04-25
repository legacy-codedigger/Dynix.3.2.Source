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

/* $Header: print.c 1.1 89/10/09 $ */

/* debug print routines */

#include <stdio.h>
#include "ctl.h"

print_request(request)
CTL_MSG *request;
{
    
    printf("type is %d, l_user %s, r_user %s, r_tty %s\n",
	    request->type, request->l_name, request->r_name,
	    request->r_tty);
    printf("        id = %d\n", request->id_num);
    fflush(stdout);
}

print_response(response)
CTL_RESPONSE *response;
{
    printf("type is %d, answer is %d, id = %d\n\n", response->type, 
	    response->answer, response->id_num);
    fflush(stdout);
}
