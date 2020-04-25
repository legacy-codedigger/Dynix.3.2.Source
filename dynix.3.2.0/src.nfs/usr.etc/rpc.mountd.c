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

#ifndef	lint
static char rcsid[] = "$Header: rpc.mountd.c 1.7 90/01/03 $";
#endif

#ifndef lint
/* @(#)rpc.mountd.c	2.1 86/04/17 NFSSRC */ 
static char sccsid[] = "@(#)rpc.mountd.c 1.1 86/02/05 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

/* NFS server */

#include <sys/param.h>
#include <ufs/fs.h>
#include <rpc/rpc.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <netdb.h>

#define	EXPORTS	"/etc/exports"
#define RMTAB	"/etc/rmtab"
#define	MAXLINE	2048

extern int errno;

int mnt();
char *exmalloc();
int catch();
char *inet_ntoa();
struct groups  *newgroup();
struct exports *newexport();
void log_cant_reply();

static struct mountlist *mountlist;
char myname[256];
char mydomain[256];
struct sockaddr_in myaddr;
char *exportfile = EXPORTS;
struct exports *exports = NULL;
int nfs_portmon = 0;

main(argc, argv)
char	*argv[];
{
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	SVCXPRT *transp;

#ifdef DEBUG
	{
		int s;
		struct sockaddr_in addr;
		int len = sizeof(struct sockaddr_in);

		if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			perror("inet: socket");
			return - 1;
		}
		if (bind(s, &addr, sizeof(addr)) < 0) {
			perror("bind");
			return - 1;
		}
		if (getsockname(s, &addr, &len) != 0) {
			perror("inet: getsockname");
			(void)close(s);
			return - 1;
		}
		pmap_unset(MOUNTPROG, MOUNTVERS);
		pmap_set(MOUNTPROG, MOUNTVERS, IPPROTO_UDP,
		    ntohs(addr.sin_port));
		if (dup2(s, 0) < 0) {
			perror("dup2");
			exit(1);
		}
	}
#endif	

	if (getsockname(0, &addr, &len) != 0) {
		perror("mountd: getsockname");
		exit(1);
	}
	if ((transp = svcudp_create(0)) == NULL) {
		fprintf(stdout, "couldn't create udp transport\n");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, 0)) {
		fprintf(stdout, "couldn't register MOUNTPROG");
		exit(1);
	}

	/*
	 * Initalize the world
	 */
	readnfsoptions();
	gethostname(myname, sizeof(myname));
	getdomainname(mydomain, sizeof(mydomain));
	get_myaddress(&myaddr);
	readfromfile();
	set_exports();

	/*
	 * Start serving
	 */
	svc_run();
	syslog(LOG_ERR, "Error: svc_run shouldn't have returned\n");
	abort();
}

/*
 * Server procedure switch routine
 */
mnt(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{

	switch(rqstp->rq_proc) {
		case NULLPROC:
			if (!svc_sendreply(transp, xdr_void, 0)) {
			    	fprintf(stdout,
				     "couldn't reply to rpc call\n");
				log_cant_reply(transp);
			}
			return;
		case MOUNTPROC_MNT:
#ifdef DEBUG
			fprintf(stdout, "about to do a mount\n");
#endif
			if (imposter(rqstp,transp)) {
				svcerr_weakauth(transp);
				return;
			}
			set_exports();
			mount(rqstp, transp);
			return;
		case MOUNTPROC_DUMP:
#ifdef DEBUG
			fprintf(stdout, "about to do a dump\n");
#endif
			if (!svc_sendreply(transp,xdr_mountlist,&mountlist)) {
				log_cant_reply(transp);
			}
			return;
		case MOUNTPROC_UMNT:
#ifdef DEBUG
			fprintf(stdout, "about to do an unmount\n");
#endif
			if (imposter(rqstp,transp)) {
				svcerr_weakauth(transp);
				return;
			}
			umount(rqstp, transp);
			return;
		case MOUNTPROC_UMNTALL:
#ifdef DEBUG
			fprintf(stdout, "about to do an unmountall\n");
#endif
			if (imposter(rqstp,transp)) {
				svcerr_weakauth(transp);
				return;
			}
			umountall(rqstp, transp);
			return;
		case MOUNTPROC_EXPORT:
		case MOUNTPROC_EXPORTALL:
#ifdef DEBUG
			fprintf(stdout, "about to do a export\n");
#endif
			set_exports();
			export(rqstp, transp);
			return;
		default:
			svcerr_noproc(transp);
			return;
	}
}

imposter(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct sockaddr_in actual;

	if (rqstp->rq_cred.oa_flavor != AUTH_UNIX) {
		svcerr_weakauth(transp);
		return(1);
	}
	if (nfs_portmon) {
		actual = *svc_getcaller(transp);
		if (ntohs(actual.sin_port) >= IPPORT_RESERVED) {
			return(1);
		}
	}
	return(0);
}

char *
getclientsname(transp)
	SVCXPRT *transp;
{
	struct hostent *hp;
	struct sockaddr_in actual;

	actual = *svc_getcaller(transp);
	/*
	 * Don't use the unix credentials to get the machine name, instead use
	 * the source IP address. 
	 */
	hp = gethostbyaddr((char *)&actual.sin_addr, sizeof(actual.sin_addr), 
			   AF_INET);
	if (hp == (struct hostent *)NULL)
		return ((char *)NULL);
	return (hp->h_name);
}

void
log_cant_reply(transp)
        SVCXPRT *transp;
{
	int saverrno;
	struct sockaddr_in actual;
	struct hostent *hp;
	char *name;

	saverrno = errno;	/* save error code */
	actual = *svc_getcaller(transp);
	/*
	 * Don't use the unix credentials to get the machine name, instead use
	 * the source IP address.
	 */
	if ((hp = gethostbyaddr(&actual.sin_addr, sizeof(actual.sin_addr),
		AF_INET)) != NULL) {
		name = hp->h_name;
	} else {
		name = inet_ntoa(actual.sin_addr);
	}

	errno = saverrno;
	if (errno == 0) {
		syslog(LOG_ERR, "couldn't send reply to %s", name);
	} else {
		syslog(LOG_ERR, "couldn't send reply to %s: %m", name);
	}
}

/*
 * Check mount requests, add to mounted list if ok
 */
mount(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	fhandle_t fh;
	struct fhstatus fhs;
	char *path, *machine;
	int fd;
	struct mountlist *ml;
	struct stat statbuf;
	struct exports *ex;
	struct groups *gl;

	path = NULL;
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}
#ifdef DEBUG
	fprintf(stdout, "path is %s\n", path);
#endif
	machine = getclientsname(transp);
	if (machine == (char *)NULL) {
		fhs.fhs_status = EACCES;
		goto fail;
	}
	if ((fd = open(path, O_RDONLY, 0)) < 0) {
		fhs.fhs_status = errno;
		perror("mountd: open");
		goto fail;
	}
	if (getfh(fd, &fh) < 0) {
		fhs.fhs_status = errno;
		perror("mountd: getfh");
		close(fd);
		goto fail;
	}
	else
		fhs.fhs_status = 0;
	if (fstat(fd, &statbuf) < 0) {
		fhs.fhs_status = errno;
		perror("mountd: stat");
		close(fd);
		goto fail;
	}
	close(fd);
	for(ex = exports; ex != NULL; ex = ex->ex_next) {
#ifdef DEBUG
		fprintf(stdout, "checking %s %o for %o\n", ex->ex_name, ex->ex_dev, statbuf.st_dev);
#endif
		if (ex->ex_dev != statbuf.st_dev)
			continue;
		if (ex->ex_groups == NULL) {
			goto hit;
		}
		for (gl = ex->ex_groups; gl != NULL; gl = gl->g_next) {
#ifdef DEBUG
			fprintf(stdout, "checking %s for %s\n", gl->g_name, machine);
#endif
			if (innetgr(gl->g_name, machine, NULL, mydomain))
				goto hit;
			if (strcmp(gl->g_name, machine) == 0) {
				goto hit;
			}
		}
	}
	fhs.fhs_status = EACCES;
	goto fail;
  hit:
	fhs.fhs_fh = fh;
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_path, path) == 0 &&
		    strcmp(ml->ml_name, machine) == 0)
			break;
	}
	if (ml == NULL) {
		ml = (struct mountlist *)exmalloc(sizeof(struct mountlist));
		ml->ml_path = (char *)exmalloc(strlen(path) + 1);
		strcpy(ml->ml_path, path);
		ml->ml_name = (char *)exmalloc(strlen(machine) + 1);
		strcpy(ml->ml_name, machine);
		ml->ml_nxt = mountlist;
		mountlist = ml;
	}
fail:
	if (!svc_sendreply(transp, xdr_fhstatus, &fhs)) {
		log_cant_reply(transp);
	}
	dumptofile();
	svc_freeargs(transp, xdr_path, &path);
}

/*
 * Remove an entry from mounted list
 */
umount(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char *path, *machine;
	struct mountlist *ml, *oldml;

	path = NULL;
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}
	if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
		machine = getclientsname(transp);
		if (machine == (char *)NULL) {
			goto done;
		}
	}
	else
		return;
#ifdef DEBUG
	fprintf(stdout, "name %s path %s\n", machine, path);
#endif
	oldml = mountlist;
	for (ml = mountlist; ml != NULL;
	    oldml = ml, ml = ml->ml_nxt) {
		if (strcmp(ml->ml_path, path) == 0 &&
		    strcmp(ml->ml_name, machine) == 0) {
			if (ml == mountlist)
				mountlist = ml->ml_nxt;
			else
				oldml->ml_nxt = ml->ml_nxt;
#ifdef DEBUG
	fprintf(stdout, "freeing %s\n", path);
#endif
			free(ml->ml_path);
			free(ml->ml_name);
			free(ml);
			break;
		    }
	}
done:
	if (!svc_sendreply(transp,xdr_void, NULL)) {
		log_cant_reply(transp);
	}
	dumptofile();
	svc_freeargs(transp, xdr_path, &path);
}

/*
 * Remove all entries for one machine from mounted list
 */
umountall(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char *machine;
	struct mountlist *ml, *oldml;

	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}
	/*
	 * We assume that this call is asynchronous and made via the 
	 * portmapper callit routine.  Therefore return control immediately.
	 * The error causes the portmapper to remain silent, as apposed to
	 * every machine on the net blasting the requester with a response.
	 */
	svcerr_systemerr(transp);
	if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
		machine = getclientsname(transp);
		if (machine == (char *)NULL) {
			return;
		}
	}
	else
		return;
	oldml = mountlist;
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_name, machine) == 0) {
#ifdef DEBUG
			fprintf(stdout, "got a hit\n");
#endif
			if (ml == mountlist) {
				mountlist = ml->ml_nxt;
				oldml = mountlist;
			}
			else
				oldml->ml_nxt = ml->ml_nxt;
			free(ml->ml_path);
			free(ml->ml_name);
			free(ml);
		}
		else
			oldml = ml;
	}
	dumptofile();
	svc_freeargs(transp, xdr_void, NULL);
}

/*
 * send current export list
 */
export(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct exports *ex;

	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}

	ex = exports;
	if (!svc_sendreply(transp, xdr_exports, &ex)) {
		log_cant_reply(transp);
	}
}

/*
 * Save current mount state info so we
 * can attempt to recover in case of a crash.
 */
dumptofile()
{
	static char *t1 = "/etc/zzXXXXXX";
	static char *t2 = "/etc/zzXXXXXX";
	FILE *fp;
	struct mountlist *ml;
	char *mktemp();
	int mf;
	
	strcpy(t2, t1);
	t2 = mktemp(t2);
	if ((mf = creat(t2, 0644)) < 0)
		perror("mountd: creat");
	if ((fp = fdopen(mf, "w")) == NULL)
		fprintf(stdout, "mountd: fdopen");
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt)
		fprintf(fp, "%s:%s\n", ml->ml_name, ml->ml_path);
	if (rename(t2, RMTAB) < 0)
		perror("mountd: link");
	fclose(fp);
}

/*
 * Restore saved mount state
 */
readfromfile()
{
	FILE *fp;
	struct mountlist *ml;
	char name[BUFSIZ];
	char *path, *index(), *rindex();
	
	fp = fopen(RMTAB, "r");
	if (fp == NULL)
		return;
	while (1) {
		if (fgets(name, sizeof(name), fp) == NULL)
			break;
		path = rindex(name, '\n');
		if (path == NULL)
			break;
		*path = 0;
		path = index(name, ':');
		if (path == NULL)
			break;
		*path++ = NULL;
		ml = (struct mountlist *) exmalloc(sizeof(struct mountlist));
		ml->ml_path = (char *)exmalloc(strlen(path) + 1);
		strcpy(ml->ml_path, path);
		ml->ml_name = (char *)exmalloc(strlen(name) + 1);
		strcpy(ml->ml_name, name);
		ml->ml_nxt = mountlist;
		mountlist = ml;
	}
	fclose(fp);
}

struct groups *
newgroup(name, next)
	char *name;
	struct groups *next;
{
	struct groups *new;
	char *newname;

	new = (struct groups *)exmalloc(sizeof(*new));
	newname = (char *)exmalloc(strlen(name) + 1);
	strcpy(newname, name);

	new->g_name = newname;
	new->g_next = next;
	return (new);
}

struct exports *
newex(name, dev, groups, next)
	char *name;
	dev_t dev;
	struct groups *groups;
	struct exports *next;
{
	struct exports *new;
	char *newname;

	new = (struct exports *)exmalloc(sizeof(*new));
	newname = (char *)exmalloc(strlen(name) + 1);
	strcpy(newname, name);

	new->ex_name = newname;
	new->ex_dev = dev;
	new->ex_groups = groups;
	new->ex_next = next;
	return (new);
}


struct stat exportstat;
int exportdone = 0;
/*
 * Parse exports file
 * If this is the first call or the file exportfile (set in main) has
 * changed exportfile is opened and parsed to create an exports list.
 * file should look like:
 * ^dir names*
 *   or
 * #anything
 * where: dir is the name of a mount point for a local file system
 *        names is a netgroup or host name or a list of white seperated names
 *        A '#' anywhere in the line marks a comment to the end of that line
 * NOTE: a non-white character in column 1 indicates a new export specification.
 */
set_exports()
{
	int bol;	/* begining of line */
	int eof;	/* end of file */
	int opt;	/* beginning of option */
	struct exports *ex;
	char ch;
	char *str;
	char *l;
	char line[MAXLINE];	/* current line */
	struct stat statb;
	FILE *fp;

	if (stat(exportfile, &statb) < 0) {
		fprintf(stdout, "mountd: stat failed ");
		perror(exportfile);
		freeex(exports);
		exports = NULL;
		return;
	}
	if (exportdone++) {
		if (exportstat.st_mtime == statb.st_mtime) {
			return;
		}
	}
	exportstat = statb;

	if ((fp = fopen(exportfile, "r")) == NULL) {
		perror(exportfile);
		freeex(exports);
		exports = NULL;
		return;
	}

	freeex(exports);
	eof = 0;
	ex = NULL;
	l = line;
	*l = '\0';
	while (!eof) {
		switch (*l) {
		case '\0':
		case '\n':
			/*
			 * End of line, read next line and set state vars
			 */
			if (fgets(line, MAXLINE, fp) == NULL) {
				eof = 1;
			} else {
				bol = 1;
				opt = 0;
				l = line;
			}
			break;
		case ' ':
		case '	':
			/*
			 * White space, skip to first non-white
			 */
			while (*l == ' ' || *l == '	') {
				l++;
			}
			bol = 0;
			break;
		case '#':
			/*
			 * Comment, skip to end of line.
			 */
			*l = '\0';
			break;
		case '-':
			/*
			 * option of the form: -option=value
			 */
			if (bol) {
				fprintf(stdout,
				    "mountd: can't parse '%s'\n", l);
				*l = '\0';
				break;
			}
			opt = 1;
			l++;
			break;
		default:
			/*
			 * normal character: if col one get dir else name or opt
			 */
			str = l;
			while (*l != ' ' && *l != '	' &&
			     *l != '\0' && *l != '\n') {
				l++;
			}
			ch = *l;
			*l = '\0';
			if (bol) {
				if (stat(str, &statb) < 0) {
					fprintf(stdout, "mountd: can't stat ");
					perror(str);
					break;
				}
				if (statb.st_ino != ROOTINO ||
				    (major(statb.st_dev) == 0377)) {
					fprintf(stdout,
					    "mountd: %s bad file system root\n",
					    str);
					break;
				} else {
					ex = newex(str, statb.st_dev, NULL, ex);
				}
			} else {
				if (opt) {
					opt = 0;
					setopt(str, ex);
				} else {
					ex->ex_groups =
					    newgroup(str, ex->ex_groups);
				}
			}
			*l = ch;
			bol = 0;
			break;
		}
	}
	fclose(fp);
	exports = ex;
	return;
}

setopt(str, ex)
	char *str;
	struct exports *ex;
{
}


freeex(ex)
	struct exports *ex;
{
	struct groups *groups, *tmpgroups;
	struct exports *tmpex;

	while (ex) {
		groups = ex->ex_groups;
		while (groups) {
			tmpgroups = groups->g_next;
			free(groups->g_name);
			free(groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free(ex->ex_name);
		free(ex);
		ex = tmpex;
	}
}

char *
exmalloc(size)
	int size;
{
	char *ret;

	if ((ret = (char *)malloc(size)) == 0) {
		fprintf(stdout, "Out of memory\n");
		exit(1);
	}
	return (ret);
}

catch()
{
}

struct	options {
	char *o_name;
	int *o_addr;
} options[] = {
	{ "nfs_portmon", &nfs_portmon },
	{ (char *)NULL, (int *)NULL },
};

#define	MYNAME	"rpc.mountd"
#define	NFSOPTS	"/etc/NFS.OPTIONS"

/*
 * Set options from global file.
 * NB: this routine may wish to 
 *     move to a library.
 */
readnfsoptions()
{
	register FILE *	f;
	register char *	p;
	register int	n;
	register struct options *op;
	char buf[128];

	f = fopen(NFSOPTS, "r");
	if (f == NULL)
		return;
	while (fgets(buf, sizeof(buf)-1, f) != NULL) {
		if (buf[0] == '#')
			continue;
		p = buf;
		n = strlen(MYNAME);
		if (strncmp(p, MYNAME, n))
			continue;
		for (p += n; *p && (*p == ' ' || *p == '\t'); p++)
			/* void */;
		if (!*p)
			continue;
		for (op = options; op->o_name != NULL; op++) {
			n = strlen(op->o_name);
			if (strncmp(op->o_name, p, n))
				continue;
#ifdef	DEBUG
			fprintf(stderr, "%s: %d -> %d\n", 
				op->o_name,*op->o_addr,atoi(p+n));
#endif
			*op->o_addr = atoi(p+n);
		}
	}
	(void) fclose(f);
}
