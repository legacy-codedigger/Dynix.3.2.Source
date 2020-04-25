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

#ifndef	lint
static char rcsid[] = "$Header: gethostent.c 2.6 1991/05/01 17:47:58 $";
#endif

#ifndef lint
static  char sccsid[] = "@(#)gethostent.c 1.1 86/02/03  Copyr 1984 Sun Micro";
/* @(#)gethostent.c	2.1 86/04/11 NFSSRC */
#endif

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <rpcsvc/ypclnt.h>

/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14
#define MAXADDRS	35

static char *h_addr_ptrs[MAXADDRS+1];

static char domain[256];
static int stayopen;
static char *current = NULL;	/* current entry, analogous to hostf */
static int currentlen;
static struct hostent *interpret();
struct hostent *gethostent();
char *inet_ntoa();
static char *any();
static char HOSTDB[] = "/etc/hosts";
static FILE *hostf = NULL;
static struct hostent host;
static char hostbuf[BUFSIZ+1];
static char *host_aliases[MAXALIASES];
static struct in_addr host_addr;
static char hostaddr[MAXADDRS];
static char *host_addrs[2];
static int usingyellow;		/* are yellow pages up? */

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

typedef union {
    HEADER hdr;
    u_char buf[MAXPACKET];
} querybuf;

static union {
    long al;
    char ac;
} align;


int h_errno;
extern errno;

static struct hostent *
getanswer(answer, anslen, iquery)
	querybuf *answer;
	int anslen;
	int iquery;
{
	register HEADER *hp;
	register u_char *cp;
	register int n;
	u_char *eom;
	char *bp, **ap;
	int type, class, buflen, ancount, qdcount;
	int haveanswer, getclass = C_ANY;
	char **hap;

	eom = answer->buf + anslen;
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	bp = hostbuf;
	buflen = sizeof(hostbuf);
	cp = answer->buf + sizeof(HEADER);
	if (qdcount) {
		if (iquery) {
			if ((n = dn_expand((char *)answer->buf, eom,
			     cp, bp, buflen)) < 0) {
				h_errno = NO_RECOVERY;
				return ((struct hostent *) NULL);
			}
			cp += n + QFIXEDSZ;
			host.h_name = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
		} else
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
	} else if (iquery) {
		if (hp->aa)
			h_errno = HOST_NOT_FOUND;
		else
			h_errno = TRY_AGAIN;
		return ((struct hostent *) NULL);
	}
	ap = host_aliases;
	host.h_aliases = host_aliases;
	hap = h_addr_ptrs;
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
	host.h_addr_list = h_addr_ptrs;
#endif
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		if ((n = dn_expand((char *)answer->buf, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		type = _getshort(cp);
 		cp += sizeof(u_short);
		class = _getshort(cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		n = _getshort(cp);
		cp += sizeof(u_short);
		if (type == T_CNAME) {
			cp += n;
			if (ap >= &host_aliases[MAXALIASES-1])
				continue;
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
			continue;
		}
		if (iquery && type == T_PTR) {
			if ((n = dn_expand((char *)answer->buf, eom,
			    cp, bp, buflen)) < 0) {
				cp += n;
				continue;
			}
			cp += n;
			host.h_name = bp;
			return(&host);
		}
		if (iquery || type != T_A)  {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("unexpected answer type %d, size %d\n",
					type, n);
#endif
			cp += n;
			continue;
		}
		if (haveanswer) {
			if (n != host.h_length) {
				cp += n;
				continue;
			}
			if (class != getclass) {
				cp += n;
				continue;
			}
		} else {
			host.h_length = n;
			getclass = class;
			host.h_addrtype = (class == C_IN) ? AF_INET : AF_UNSPEC;
			if (!iquery) {
				host.h_name = bp;
				bp += strlen(bp) + 1;
			}
		}

		bp += sizeof(align) - ((u_long)bp % sizeof(align));

		if (bp + n >= &hostbuf[sizeof(hostbuf)]) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("size (%d) too big\n", n);
#endif
			break;
		}
		bcopy(cp, *hap++ = bp, n);
		bp +=n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		*ap = NULL;
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
		*hap = NULL;
		host.h_addr0 = h_addr_ptrs[0];
#else
		host.h_addr = h_addr_ptrs[0];
#endif
		return (&host);
	} else {
		h_errno = TRY_AGAIN;
		return ((struct hostent *) NULL);
	}
}

struct hostent *
gethostbyaddr(addr, len, type)
	char *addr;
	register int len, type;
{
	register struct hostent *p;
	int reason;
	char *adrstr, *val;
	int vallen;

	int n;
	querybuf buf;
	register struct hostent *hp;
	char qbuf[MAXDNAME];
	
	if (type != AF_INET)
		return ((struct hostent *) NULL);
	(void)sprintf(qbuf, "%d.%d.%d.%d.in-addr.arpa",
		((unsigned)addr[3] & 0xff),
		((unsigned)addr[2] & 0xff),
		((unsigned)addr[1] & 0xff),
		((unsigned)addr[0] & 0xff));
	n = res_query(qbuf, C_IN, T_PTR, (char *)&buf, sizeof(buf));
	if (n < 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("res_query failed\n");
#endif
		if (errno == ECONNREFUSED) {
			_sethtent(0);
			if (!usingyellow) {
				while (p = gethostent()) {
					if (p->h_addrtype != type || p->h_length != len)
						continue;
					if (bcmp(p->h_addr, addr, len) == 0)
						break;
				}
			}
			else {
				adrstr = inet_ntoa(*(int *)addr);
				if (reason = yp_match(domain, "hosts.byaddr",
		    		adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
					fprintf(stderr, "reason yp_first failed is %d\n",
			    		reason);
#endif
					p = NULL;
		   		}
				else {
					p = interpret(val, vallen);
					free(val);
				}
			}
			_endhtent();
			return (p);
		}
		return ((struct hostent *) NULL);
	}
	hp = getanswer(&buf, n, 1);
	if (hp == NULL)
		return ((struct hostent *) NULL);
	hp->h_addrtype = type;
	hp->h_length = len;
	h_addr_ptrs[0] = (char *)&host_addr;
	h_addr_ptrs[1] = (char *)0;
	host_addr = *(struct in_addr *)addr;
	return(hp);
}

struct hostent *
gethostbyname(name)
	register char *name;
{
	register struct hostent *p;
	register char **cp;
	int reason;
	char *val;
	int vallen;
	querybuf buf;
	register char *bp;
	int n;
	struct hostent *hp;

	/*
	 * disallow names consisting only of digits/dots, unless
	 * they end in a dot.
	 */
	if (isdigit(name[0]))
		for (bp = name;; ++bp) {
			if (!*bp) {
				if (*--bp == '.')
					break;
				/*
				 * All-numeric, no dot at the end.
				 * Fake up a hostent as if we'd actually
				 * done a lookup.  What if someone types
				 * 255.255.255.255?  The test below will
				 * succeed spuriously... ???
				 */
				if ((host_addr.s_addr = inet_addr(name)) == -1){
					h_errno = HOST_NOT_FOUND;
					return((struct hostent *) NULL);
				}
				host.h_name = name;
				host.h_aliases = host_aliases;
				host_aliases[0] = NULL;
				host.h_addrtype = AF_INET;
				host.h_length = sizeof(u_long);
				h_addr_ptrs[0] = (char *)&host_addr;
				h_addr_ptrs[1] = (char *)0;
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
				host.h_addr_list = h_addr_ptrs;
#else
				host.h_addr = h_addr_ptrs[0];
#endif
				return (&host);
			}
			if (!isdigit(*bp) && *bp != '.') 
				break;
		}

	if ((n = res_search(name, C_IN, T_A, buf.buf, sizeof(buf))) < 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("res_search failed\n");
#endif
		if (errno == ECONNREFUSED) {

			_sethtent(0);
			if (!usingyellow) {
				while (p = gethostent()) {
					if (strcasecmp(p->h_name, name) == 0)
						break;
					for (cp = p->h_aliases; *cp != 0; cp++)
						if (strcasecmp(*cp, name) == 0)
							goto found;
				}
			}
			else {
				if (reason = yp_match(domain, "hosts.byname",
		    		name, strlen(name), &val, &vallen)) {
#ifdef DEBUG
						fprintf(stderr, "reason yp_first failed is %d\n",
				    		reason);
#endif
						p = NULL;
		    		}
				else {
					p = interpret(val, vallen);
					free(val);
				}
			}
found:
			_endhtent();
			return (p);
		}
		else
			return ((struct hostent *) NULL);
	}
	return (getanswer(&buf, n, 0));
}

_sethtent(f)
	int f;
{
	if (getdomainname(domain, sizeof(domain)) < 0) {
		fprintf(stderr, 
		    "sethostent: getdomainname system call missing\n");
		exit(1);
	}
	if (hostf == NULL)
		hostf = fopen(HOSTDB, "r" );
	else
		rewind(hostf);
	if (current)
		free(current);
	current = NULL;
	stayopen |= f;
	yellowup(1);
}

_endhtent()
{
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
	if (hostf && !stayopen) {
		(void) fclose(hostf);
		hostf = NULL;
	}
}

sethostent(f)
	int f;
{
	if (getdomainname(domain, sizeof(domain)) < 0) {
		fprintf(stderr, 
		    "sethostent: getdomainname system call missing\n");
		exit(1);
	}
	if (hostf == NULL)
		hostf = fopen(HOSTDB, "r");
	else
		rewind(hostf);
	if (current)
		free(current);
	current = NULL;
	stayopen |= f;

	if (stayopen)
		_res.options |= RES_STAYOPEN | RES_USEVC;

	yellowup(1);	/* recompute whether yellow pages are up */
}

endhostent()
{
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
	if (hostf && !stayopen) {
		fclose(hostf);
		hostf = NULL;
	}
	_res.options &= ~(RES_STAYOPEN | RES_USEVC);
	_res_close();
}

struct hostent *
gethostent()
{
	struct hostent *hp;
	int reason;
	char *key, *val;
	int keylen, vallen;
	static char line1[BUFSIZ+1];

	yellowup(0);
	if (!usingyellow) {
		if (hostf == NULL && (hostf = fopen(HOSTDB, "r")) == NULL)
			return (NULL);
	        if (fgets(line1, BUFSIZ, hostf) == NULL)
			return (NULL);
		return interpret(line1, strlen(line1));
	}
	if (current == NULL) {
		if (reason =  yp_first(domain, "hosts.byaddr",
		    &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n",
			    reason);
#endif
			return NULL;
		    }
	}
	else {
		if (reason = yp_next(domain, "hosts.byaddr",
		    current, currentlen, &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_next failed is %d\n",
			    reason);
#endif
			return NULL;
		}
	}
	if (current)
		free(current);
	current = key;
	currentlen = keylen;
	hp = interpret(val, vallen);
	free(val);
	return (hp);
}

static struct hostent *
interpret(val, len)
char *val;
int len;
{
	static char line[BUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		return (gethostent());
	cp = any(p, "#\n");
	if (cp == NULL)
		return (gethostent());
	*cp = '\0';
	cp = any(p, " \t");
	if (cp == NULL)
		return (gethostent());
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
#if BSD >= 43 || defined(h_addr)	/* new style hostent structure */
	host.h_addr_list = host_addrs;
#endif
	host.h_addr = hostaddr;
	*((u_long *)host.h_addr) = inet_addr(p);
	host.h_addr0 = host.h_addr_list[0];
	host.h_length = sizeof (u_long);
	host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	host.h_name = cp;
	q = host.h_aliases = host_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &host_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&host);
}

static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

/* 
 * check to see if yellow pages are up, and store that fact in usingyellow.
 * The check is performed once at startup and thereafter if flag is set
 */
static
yellowup(flag)
{
	static int firsttime = 1;
	char *key, *val;
	int keylen, vallen;

	if (firsttime || flag) {
		firsttime = 0;
		if (domain[0] == 0) {
			if (getdomainname(domain, sizeof(domain)) < 0) {
				fprintf(stderr, 
			    "gethostent/sethostent: getdomainname system call missing\n");
				exit(1);
			}
		}
		usingyellow = !yp_bind(domain);
	}	
}
