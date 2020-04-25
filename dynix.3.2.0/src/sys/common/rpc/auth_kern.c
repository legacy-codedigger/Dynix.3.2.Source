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

#ifdef	NFS

#ifndef	lint
static	char	rcsid[] = "$Header: auth_kern.c 1.2 87/05/05 $";
#endif	lint

/*
 * auth_kern.c
 *	Implements UNIX style authentication parameters in the kernel. 
 *  
 * Copyright (C) 1984, Sun Microsystems, Inc. 
 *
 * Interfaces with svc_auth_unix on the server. See auth_unix.c for the user
 * level implementation of unix auth.
 */

/* $Log:	auth_kern.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/auth_unix.h"
#include "../netinet/in.h"

/*
 * Unix authenticator operations vector
 */
static void	authkern_nextverf();
static bool_t	authkern_marshal();
static bool_t	authkern_validate();
static bool_t	authkern_refresh();
static void	authkern_destroy();

static struct auth_ops auth_kern_ops = {
	authkern_nextverf,
	authkern_marshal,
	authkern_validate,
	authkern_refresh,
	authkern_destroy
};


/*
 * Create a kernel unix style authenticator.
 * Returns an auth handle.
 */
AUTH *
authkern_create()
{
	register AUTH *auth;

	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)kmem_alloc((u_int)sizeof(*auth));
	auth->ah_ops = &auth_kern_ops;
	auth->ah_verf = _null_auth;
	return (auth);
}

/*
 * authkern operations
 */
/*ARGSUSED*/
static void
authkern_nextverf(auth)
	AUTH *auth;
{

	/* no action necessary */
}

static bool_t
authkern_marshal(auth, xdrs)
	AUTH *auth;
	XDR *xdrs;
{
	register int *gp, *gpend;
	register long *ptr;
	register int gidlen, credsize;
	char	*sercred;
	struct	opaque_auth *cred;
	bool_t	ret = FALSE;
	XDR	xdrm;

	/*
	 * First we try a fast path to get through
	 * this very common operation.
	 */
	gp = u.u_groups;
	gpend = &u.u_groups[NGRPS];
	while (gpend > u.u_groups && gpend[-1] < 0)
		gpend--;
	gidlen = gpend - gp;
	credsize = sizeof(long) + sizeof(long)
		   + roundup(hostnamelen, sizeof(long)) + sizeof(long)
		   + sizeof(long) + sizeof(long) + gidlen * sizeof(long);
	ptr = XDR_INLINE(xdrs, sizeof(long) + sizeof(long) + credsize +
				sizeof(long) + sizeof(long));
	if (ptr) {
		/*
		 * We can do the fast path.
		 */
		IXDR_PUT_LONG(ptr, AUTH_UNIX);	/* cred flavor */
		IXDR_PUT_LONG(ptr, credsize);	/* cred len */
		IXDR_PUT_LONG(ptr, time.tv_sec);
		IXDR_PUT_LONG(ptr, hostnamelen);
		bcopy(hostname, (caddr_t)ptr, (u_int)hostnamelen);
		ptr += roundup(hostnamelen, sizeof(long)) / sizeof(long);
		IXDR_PUT_LONG(ptr, u.u_uid);
		IXDR_PUT_LONG(ptr, u.u_gid);
		IXDR_PUT_LONG(ptr, gidlen);
		while (gp < gpend) {
			IXDR_PUT_LONG(ptr, *gp++);
		}
		IXDR_PUT_LONG(ptr, AUTH_NULL);	/* verf flavor */
		IXDR_PUT_LONG(ptr, 0);	/* verf len */
		return (TRUE);
	}
	sercred = (char *)kmem_alloc((u_int)MAX_AUTH_BYTES);
	/*
	 * serialize u struct stuff into sercred
	 */
	xdrmem_create(&xdrm, sercred, MAX_AUTH_BYTES, XDR_ENCODE);
	if (! xdr_authkern(&xdrm)) {
		printf("authkern_marshal: xdr_authkern failed\n");
		ret = FALSE;
		goto done;
	}

	/*
	 * Make opaque auth credentials that point at serialized u struct.
	 */
	cred = &(auth->ah_cred);
	cred->oa_length = XDR_GETPOS(&xdrm);
	cred->oa_flavor = AUTH_UNIX;
	cred->oa_base = sercred;

	/*
	 * serialize credentials and verifiers (null)
	 */
	if ((xdr_opaque_auth(xdrs, &(auth->ah_cred)))
	    && (xdr_opaque_auth(xdrs, &(auth->ah_verf)))) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}
done:
	kmem_free((caddr_t)sercred, (u_int)MAX_AUTH_BYTES);
	return (ret);
}

/*ARGSUSED*/
static bool_t
authkern_validate(auth, verf)
	AUTH *auth;
	struct opaque_auth verf;
{

	return (TRUE);
}

/*ARGSUSED*/
static bool_t
authkern_refresh(auth)
	AUTH *auth;
{
}

static void
authkern_destroy(auth)
	register AUTH *auth;
{

	kmem_free((caddr_t)auth, (u_int)sizeof(*auth));
}
#endif	NFS
