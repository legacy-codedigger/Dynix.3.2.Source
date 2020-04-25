/* $Copyright:	$
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
static char rcsid[] = "$Header: mount.c 2.16 91/01/23 $";
#endif

#ifndef lint
/* @(#)mount.c	2.2 86/05/16 NFSSRC */ 
static  char sccsid[] = "@(#)mount.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#define	NFS
/*
 * mount
 */
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/mount.h>
#include <sys/dir.h>
#ifdef OFS
#include <sys/ofs.h>
#endif
#include <mntent.h>

int	ro = 0;
int	quota = 0;
int	fake = 0;
int	freq = 1;
int	passno = 2;
int	all = 0;
int	verbose = 0;
int	printed = 0;
int	inmtab = 0;
int	failed = 0;	/* for program return status */


#define	NRETRY	10000	/* number of times to retry a mount request */
#define	BGSLEEP	5	/* initial sleep time for background mount in seconds */
#define MAXSLEEP 120	/* max sleep time for background mount in seconds */

extern int errno;

char	*index(), *rindex();
char	host[MNTMAXSTR];
char	name[MNTMAXSTR];
char	dir[MNTMAXSTR];
char	type[MNTMAXSTR];
char	opts[MNTMAXSTR];
char	*empty;

/*
 * Structure used to build a mount tree.  The tree is traversed to do
 * the mounts and catch dependancies
 */
struct mnttree {
	struct mntent *mt_mnt;
	struct mnttree *mt_sib;
	struct mnttree *mt_kid;
};
struct mnttree *maketree();

main(argc, argv)
	int argc;
	char **argv;
{
	struct mntent mnt;
	struct mntent *mntp;
	FILE *mnttab;
	char *options;
	char *colon;
	struct mnttree *mtree;

	if (argc == 1) {
		mnttab = setmntent(MOUNTED, "r");
		while ((mntp = getmntent(mnttab)) != NULL) {
			if (strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) {
				continue;
			}
			printent(mntp);
		}
		endmntent(mnttab);
		exit(0);
	}

	close(2);
	if (dup2(1, 2) < 0) {
		perror("dup");
		exit(1);
	}

	/*
	 * Set options
	 */
	while (argc > 1 && argv[1][0] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'a':
				all++;
				break;
			case 'f':
				fake++;
				break;
			case 'o':
				if (argc < 3) {
					usage();
				}
				strcpy(opts, argv[2]);
				argv++;
				argc--;
				break;
			case 'p':
				if (argc != 2) {
					usage();
				}
				printmtab(stdout);
				exit(0);
			case 'q':
				quota++;
				break;
			case 'r':
				ro++;
				break;
			case 't':
				if (argc < 3) {
					usage();
				}
				strcpy(type, argv[2]);
				argv++;
				argc--;
				break;
			case 'v':
				verbose++;
				break;
			default:
				fprintf(stderr, "mount: unknown option: %c\n",
				    *options);
				usage();
			}
			options++;
		}
		argc--, argv++;
	}

	if (geteuid() != 0) {
		fprintf(stderr, "Must be root to use mount\n");
		exit(1);
	}

	if (all) {
		if (argc != 1) {
			usage();
		}
		mnttab = setmntent(MNTTAB, "r");
		if (mnttab == NULL) {
			fprintf(stderr, "mount: ");
			perror(MNTTAB);
			exit(1);
		}
		mtree = NULL;
		while ((mntp = getmntent(mnttab)) != NULL) {
			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0) ||
                            hasmntopt(mntp,"hide") ||
			    (strcmp(mntp->mnt_dir, "/") == 0) ) {
				continue;
			}
			if (type[0] != '\0' &&
			    strcmp(mntp->mnt_type, type) != 0) {
				continue;
			}
			mtree = maketree(mtree, mntp);
		}
		endmntent(mnttab);
		mounttree(mtree);
		exit(failed);
	}

	/*
	 * Command looks like: mount <dev>|<dir>
	 * we walk through /etc/fstab til we match either fsname or dir.
	 */
	if (argc == 2) {
		mnttab = setmntent(MNTTAB, "r");
		if (mnttab == NULL) {
			fprintf(stderr, "mount: ");
			perror(MNTTAB);
			exit(1);
		}
		while ((mntp = getmntent(mnttab)) != NULL) {
			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0) ) {
				continue;
			}
			if ((strcmp(mntp->mnt_fsname, argv[1]) == 0) ||
			    (strcmp(mntp->mnt_dir, argv[1]) == 0) ) {
				mounttree(maketree(NULL, mntp));
				exit(failed);
			}
		}
		fprintf(stderr, "mount: %s not found in %s\n", argv[1], MNTTAB);
		exit(1);
	}

	if (argc != 3) {
		usage();
	}
	strcpy(dir, argv[2]);
	strcpy(name, argv[1]);

	/*
	 * Check for file system names of the form
	 *     host:path
	 * make these type nfs
	 */
	colon = index(name, ':');
	if (colon) {
		if (type[0] != '\0' && strcmp(type, "nfs") != 0) {
			fprintf(stderr,"%s: %s; must use type nfs\n",
			    "mount: remote file system", name);
			usage();
		}
		strcpy(type, MNTTYPE_NFS);
	}
	if (type[0] == '\0') {
		strcpy(type, MNTTYPE_42);		/* default type = 4.2 */
	}
	if (dir[0] != '/') {
		fprintf(stderr, "mount: directory path must begin with '/'\n");
		exit(1);
	}

	if (opts[0] == '\0') {
		strcpy(opts, ro ? MNTOPT_RO : MNTOPT_RW);
		if (strcmp(type, MNTTYPE_42) == 0) {
			strcat(opts, ",");
			strcat(opts, quota ? MNTOPT_QUOTA : MNTOPT_NOQUOTA);
		}
	}

	if (!strcmp(type, MNTTYPE_NFS) || !strcmp(type, MNTTYPE_OFS)) {
		passno = 0;
		freq = 0;
	}

	mnt.mnt_fsname = name;
	mnt.mnt_dir = dir;
	mnt.mnt_type = type;
	mnt.mnt_opts = opts;
	mnt.mnt_freq = freq;
	mnt.mnt_passno = passno;
	mounttree(maketree(NULL, &mnt));
	exit(failed);
}

/*
 * attempt to mount file system, return errno or 0
 */
mountfs(print, mnt)
	int print;
	struct mntent *mnt;
{
	int error;
	extern int errno;
	int type = -1;
	int flags = 0;
	static char opts[1024];
	char *optp, *optend;
	union data {
		struct ufs_args	ufs_args;
		struct nfs_args nfs_args;
#ifdef OFS
		struct ofs_args ofs_args;
#else
		char	*ofs_args;
#endif
#ifdef PCFS
		struct pc_args pc_args;
#else
		char	*pc_args;
#endif
	} data;

	if (mounted(mnt)) {
		if (print) {
			fprintf(stderr, "mount: %s already mounted\n",
			    mnt->mnt_fsname);
		}
		return (0);
	}
	if (fake) {
		addtomtab(mnt);
		return (0);
	}
	if (strcmp(mnt->mnt_type, MNTTYPE_42) == 0) {
		type = MOUNT_UFS;
		error = mount_42(mnt, &data.ufs_args);
	} else if (strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0) {
		type = MOUNT_NFS;
		error = mount_nfs(mnt, &data.nfs_args);
	} else if (strcmp(mnt->mnt_type, MNTTYPE_OFS) == 0) {
		type = MOUNT_OFS;
		error = mount_ofs(mnt, &data.ofs_args);
	} else if (strcmp(mnt->mnt_type, MNTTYPE_PC) == 0) {
		type = MOUNT_PC;
		error = mount_pc(mnt, &data.pc_args);
	} else {
		fprintf(stderr,
		    "mount: unknown file system type %s\n",
		    mnt->mnt_type);
		error = EINVAL;
	}

	if (error) {
		return (error);
	}

	flags |= (hasmntopt(mnt, MNTOPT_RO) == NULL) ? 0 : M_RDONLY;
	flags |= (hasmntopt(mnt, MNTOPT_NOSUID) == NULL) ? 0 : M_NOSUID;
	check(mnt->mnt_dir, mnt->mnt_fsname);
	if (mount(type, mnt->mnt_dir, flags, &data) < 0) {
		failed = 1;
		if (print) {
			if (errno == ENOSPC) {
				fprintf(stderr,
					"mount: %s needs to be fsck'ed\n",
					mnt->mnt_fsname);
			} else if (errno == EBUSY) {
				fprintf(stderr,
					"mount: %s Device busy or mount table full\n",
					mnt->mnt_fsname);
			} else if (errno == EINVAL) {
				fprintf(stderr,
					"mount: %s invalid file system\n",
					mnt->mnt_fsname);
			} else {
				fprintf(stderr, "mount: %s on ",
					mnt->mnt_fsname);
				perror(mnt->mnt_dir);
			}
		}
		return (errno);
	}
	if (empty != NULL)
		fprintf(stderr, "mount: %s\n", empty);
	if ((optp = hasmntopt(mnt, MNTOPT_QUOTA)) != NULL) {
		/*
		 * cut out quota option and put in noquota option, for mtab
		 */
		optend = index(optp, ',');
		if (optp != mnt->mnt_opts)
			optp--;			/* back up to ',' */
		if (optend == NULL)
			*optp = '\0';
		else
			while (*optp++ = *optend++)
				;
		sprintf(opts, "%s,%s", mnt->mnt_opts, MNTOPT_NOQUOTA);
		mnt->mnt_opts = opts;
	}
	addtomtab(mnt);
	return (0);
}

#ifdef OFS
mount_ofs(mnt, args)
	struct mntent *mnt;
	struct ofs_args *args;
{
    char *optr;
    static char name[MNTMAXSTR];

    strcpy(name, mnt->mnt_fsname);
    args->device = name;
    args->nodes = nopt(mnt, MNTOPT_FILES);
    if ((optr = hasmntopt(mnt, MNTOPT_DATE)) != NULL)
    {
	if (hasmntopt(mnt, MNTOPT_RW))
	{
	    fprintf(stderr, "mount: date option requires ro filesystem.\n");
	    return(1);
	}
	if (!hasmntopt(mnt, MNTOPT_RO))
	{
	    strcat(mnt->mnt_opts, ",");
	    strcat(mnt->mnt_opts, MNTOPT_RO);
	}
	if (dateopt(optr, &args->tv))
	{
	    fprintf(stderr, "mount: invalid date option.\n");
	    return(1);
	}
    }
    else
	gettimeofday(&args->tv, 0);
    return (0);
}
#else
mount_ofs(mnt, args)
	struct mntent *mnt;
	char	*args;
{
	fprintf(stderr, "mount: %s - Optical File System not supported\n", mnt->mnt_fsname);
	return (EINVAL);
}
#endif

mount_42(mnt, args)
	struct mntent *mnt;
	struct ufs_args *args;
{
	static char name[MNTMAXSTR];

	strcpy(name, mnt->mnt_fsname);
	args->fspec = name;
	return (0);
}

mount_nfs(mnt, args)
	struct mntent *mnt;
	struct nfs_args *args;
{
	static struct sockaddr_in sin;
	struct hostent *hp;
	static struct fhstatus fhs;
	char *cp;
	char *hostp = host;
	char *path;
	int s;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	int rsize, wsize;
	u_short port;

	cp = mnt->mnt_fsname;
	while ((*hostp = *cp) != ':') {
		if (*cp == '\0') {
			fprintf(stderr,
			    "mount: nfs file system; use host:path\n");
			return (1);
		}
		hostp++;
		cp++;
	}
	*hostp = '\0';
	path = ++cp;
	/*
	 * Get server's address
	 */
	if ((hp = gethostbyname(host)) == NULL) {
		/*
		 * XXX
		 * Failure may be due to yellow pages, try again
		 */
		if ((hp = gethostbyname(host)) == NULL) {
			fprintf(stderr,
			    "mount: %s not in hosts database\n", host);
			return (1);
		}
	}

	args->flags = 0;
	if (hasmntopt(mnt, MNTOPT_SOFT) != NULL) {
		args->flags |= NFSMNT_SOFT;
	}
	if (hasmntopt(mnt, MNTOPT_INTR) != NULL) {
		args->flags |= NFSMNT_INT;
	}

	/*
	 * get fhandle of remote path from server's mountd
	 */
	bzero(&sin, sizeof(sin));
	bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
	sin.sin_family = AF_INET;
	timeout.tv_usec = 0;
	timeout.tv_sec = 20;
	s = RPC_ANYSOCK;
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s)) == NULL) {
		if (!printed) {
			fprintf(stderr, "mount: %s server not responding",
			    mnt->mnt_fsname);
			clnt_pcreateerror("");
			printed = 1;
		}
		return (ETIMEDOUT);
	}
	if (! bindresvport(s)) {
		fprintf(stderr,"Warning: mount: cannot do local bind\n");
	}
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 20;
	rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &path,
	    xdr_fhstatus, &fhs, timeout);
	errno = 0;
	if (rpc_stat != RPC_SUCCESS) {
		failed = 1;
		if (!printed) {
			fprintf(stderr, "mount: %s server not responding",
			    mnt->mnt_fsname);
			clnt_perror(client, "");
			printed = 1;
		}
		switch (rpc_stat) {
		case RPC_TIMEDOUT:
		case RPC_PMAPFAILURE:
		case RPC_PROGNOTREGISTERED:
			errno = ETIMEDOUT;
			break;
		case RPC_AUTHERROR:
			errno = EACCES;
			break;
		default:
			errno = 0;
			break;
		}
	}
	close(s);
	clnt_destroy(client);
	if (errno) {
		return(errno);
	}

	if (errno = fhs.fhs_status) {
		if (errno == EACCES) {
			fprintf(stderr, "mount: access denied for %s:%s\n",
			    host, path);
		} else {
			fprintf(stderr, "mount: ");
			perror(mnt->mnt_fsname);
		}
		return (errno);
	}
	if (printed) {
		fprintf(stderr, "mount: %s server ok\n", mnt->mnt_fsname);
		printed = 0;
	}

	/*
	 * set mount args
	 */
	args->fh = &fhs.fhs_fh;
	args->hostname = host;
	args->flags |= NFSMNT_HOSTNAME;
	if (args->rsize = nopt(mnt, "rsize")) {
		args->flags |= NFSMNT_RSIZE;
	}
	if (args->wsize = nopt(mnt, "wsize")) {
		args->flags |= NFSMNT_WSIZE;
	}
	if (args->timeo = nopt(mnt, "timeo")) {
		args->flags |= NFSMNT_TIMEO;
	}
	if (args->retrans = nopt(mnt, "retrans")) {
		args->flags |= NFSMNT_RETRANS;
	}
	if (port = nopt(mnt, "port")) {
		sin.sin_port = htons(port);
	} else {
		sin.sin_port = htons(NFS_PORT);	/* XXX should use portmapper */
	}
	args->addr = &sin;

	/*
	 * should clean up mnt ops to not contain defaults
	 */
	return (0);
}

#ifdef PCFS
mount_pc(mnt, args)
	struct mntent *mnt;
	struct pc_args *args;
{
	args->fspec = mnt->mnt_fsname;
	return (0);
}
#else
mount_pc(mnt, args)
	struct mntent *mnt;
	char	*args;
{
	fprintf(stderr, "mount: %s - PC File System not supported\n", mnt->mnt_fsname);
	return (EINVAL);
}
#endif

printent(mnt)
	struct mntent *mnt;
{
	fprintf(stdout, "%s on %s type %s (%s)\n",
	    mnt->mnt_fsname, mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts);
}

printmtab(outp)
	FILE *outp;
{
	FILE *mnttab;
	struct mntent *mntp;
	int maxfsname = 0;
	int maxdir = 0;
	int maxtype = 0;
	int maxopts = 0;

	/*
	 * first go through and find the max width of each field
	 */
	mnttab = setmntent(MOUNTED, "r");
	while ((mntp = getmntent(mnttab)) != NULL) {
		if (strlen(mntp->mnt_fsname) > maxfsname) {
			maxfsname = strlen(mntp->mnt_fsname);
		}
		if (strlen(mntp->mnt_dir) > maxdir) {
			maxdir = strlen(mntp->mnt_dir);
		}
		if (strlen(mntp->mnt_type) > maxtype) {
			maxtype = strlen(mntp->mnt_type);
		}
		if (strlen(mntp->mnt_opts) > maxopts) {
			maxopts = strlen(mntp->mnt_opts);
		}
	}
	endmntent(mnttab);

	/*
	 * now print them oput in pretty format
	 */
	mnttab = setmntent(MOUNTED, "r");
	while ((mntp = getmntent(mnttab)) != NULL) {
		fprintf(outp, "%-*s", maxfsname+1, mntp->mnt_fsname);
		fprintf(outp, "%-*s", maxdir+1, mntp->mnt_dir);
		fprintf(outp, "%-*s", maxtype+1, mntp->mnt_type);
		fprintf(outp, "%-*s", maxopts+1, mntp->mnt_opts);
		fprintf(outp, " %d %d\n", mntp->mnt_freq, mntp->mnt_passno);
	}
	endmntent(mnttab);
	return (0);
}

/*
 * Check to see if mntck is already mounted.
 * We have to be careful because getmntent modifies its static struct.
 */
mounted(mntck)
	struct mntent *mntck;
{
	int found = 0;
	struct mntent *mnt, mntsave;
	FILE *mnttab;

	mnttab = setmntent(MOUNTED, "r");
	if (mnttab == NULL) {
		fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	mntcp(mntck, &mntsave);
	inmtab = 0;
	while ((mnt = getmntent(mnttab)) != NULL) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) {
			continue;
		}
		if ((strcmp(mntsave.mnt_fsname, mnt->mnt_fsname) == 0) &&
		    (strcmp(mntsave.mnt_dir, mnt->mnt_dir) == 0) ) {
			inmtab = 1;
			if (mountpoint(mnt->mnt_dir))
				found = 1;
			break;
		}
	}
	endmntent(mnttab);
	*mntck = mntsave;
	return (found);
}

/*
 * Return whether "dir" is a 
 * mount point or not.
 */
int
mountpoint(dir)
	char *dir;
{
	struct stat sbuf, pbuf;
	char pname[MNTMAXSTR+8];

	(void) sprintf(pname, "%s/..", dir);
	if (stat(dir, &sbuf) >= 0 
	 && stat(pname, &pbuf) >= 0
	 && sbuf.st_dev != pbuf.st_dev)
		return 1;
	return 0;
}

mntcp(mnt1, mnt2)
	struct mntent *mnt1, *mnt2;
{
	static char fsname[128], dir[128], type[128], opts[128];

	mnt2->mnt_fsname = fsname;
	strcpy(fsname, mnt1->mnt_fsname);
	mnt2->mnt_dir = dir;
	strcpy(dir, mnt1->mnt_dir);
	mnt2->mnt_type = type;
	strcpy(type, mnt1->mnt_type);
	mnt2->mnt_opts = opts;
	strcpy(opts, mnt1->mnt_opts);
	mnt2->mnt_freq = mnt1->mnt_freq;
	mnt2->mnt_passno = mnt1->mnt_passno;
}

/*
 * Return the value of a numeric option of the form foo=x, if
 * option is not found or is malformed, return 0.
 */
nopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	int val = 0;
	char *equal;
	char *str;

	if (str = hasmntopt(mnt, opt)) {
		if (equal = index(str, '=')) {
			val = atoi(&equal[1]);
		} else {
			fprintf(stderr, "mount: bad numeric option '%s'\n",
			    str);
		}
	}
	return (val);
}

/*
 * update /etc/mtab
 */
addtomtab(mnt)
	struct mntent *mnt;
{
	FILE *mnted;

	if (!inmtab) {
		mnted = setmntent(MOUNTED, "r+");
		if (mnted == NULL) {
			fprintf(stderr, "mount: ");
			perror(MOUNTED);
			exit(1);
		}
		if (addmntent(mnted, mnt)) {
			fprintf(stderr, "mount: ");
			perror(MOUNTED);
			exit(1);
		}
		endmntent(mnted);
	}

	if (verbose) {
		fprintf(stdout, "%s mounted on %s\n",
		    mnt->mnt_fsname, mnt->mnt_dir);
	}
}

char *
xmalloc(size)
	int size;
{
	char *ret;
	
	if ((ret = (char *)malloc(size)) == NULL) {
		fprintf(stderr, "umount: ran out of memory!\n");
		exit(1);
	}
	return (ret);
}

struct mntent *
mntdup(mnt)
	struct mntent *mnt;
{
	struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = (char *)xmalloc(strlen(mnt->mnt_fsname) + 1);
	strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);
}

/*
 * Build the mount dependency tree
 */
struct mnttree *
maketree(mt, mnt)
	struct mnttree *mt;
	struct mntent *mnt;
{
	if (mt == NULL) {
		mt = (struct mnttree *)xmalloc(sizeof (struct mnttree));
		mt->mt_mnt = mntdup(mnt);
		mt->mt_sib = NULL;
		mt->mt_kid = NULL;
	} else {
		if (substr(mt->mt_mnt->mnt_dir, mnt->mnt_dir)) {
			mt->mt_kid = maketree(mt->mt_kid, mnt);
		} else {
			mt->mt_sib = maketree(mt->mt_sib, mnt);
		}
	}
	return (mt);
}

printtree(mt)
	struct mnttree *mt;
{
	if (mt) {
		printtree(mt->mt_sib);
		printf("   %s\n", mt->mt_mnt->mnt_dir);
		printtree(mt->mt_kid);
	}
}

mounttree(mt)
	struct mnttree *mt;
{
	int error;
	int slptime;
	int forked;
	int retry;
	int firsttry;

	if (mt) {
		mounttree(mt->mt_sib);
		forked = 0;
		printed = 0;
		firsttry = 1;
		slptime = BGSLEEP;
		retry = nopt(mt->mt_mnt, "retry");
		if (retry == 0) {
			retry = NRETRY;
		}

		do {
			error = mountfs(!forked, mt->mt_mnt);
			if (error != ETIMEDOUT && error != ENETDOWN &&
			    error != ENETUNREACH && error != ENOBUFS &&
			    error != ECONNREFUSED && error != ECONNABORTED) {
				break;
			}
			if (!forked && hasmntopt(mt->mt_mnt, "bg")) {
				fprintf(stderr, "mount: backgrounding\n");
				fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
				printtree(mt->mt_kid);
				/* this should arguably return nonzero 
				 * (failed = 1), since the filesys isn't
				 * mounted, but this way is compatible
				 * with Sun NFS4.0 mount.
				 */
				if (fork()) {
					return;
				} else {
					forked = 1;
				}
			}
			if (!forked && firsttry) {
				fprintf(stderr, "mount: retrying\n");
				fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
				printtree(mt->mt_kid);
				firsttry = 0;
			}
			sleep(slptime);
			slptime = MIN(slptime << 1, MAXSLEEP);
		} while (retry--);

		if (!error) {
			mounttree(mt->mt_kid);
		} else {
			fprintf(stderr, "mount: giving up on:\n");
			fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
			printtree(mt->mt_kid);
			failed = 1;
		}
		if (forked) {
			exit(0);
		}
	}
}

printsp(n)
	int n;
{
	while (n--) {
		printf(" ");
	}
}

/*
 * Returns true if s1 is a pathname substring of s2.
 */
substr(s1, s2)
	char *s1;
	char *s2;
{
	while (*s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '/') {
		return (1);
	}
	return (0);
}

bindresvport(sd)
	int sd;
{
 
	u_short port;
	struct sockaddr_in sin;
	int err = -1;

#	define MAX_PRIV (IPPORT_RESERVED-1)
#	define MIN_PRIV (IPPORT_RESERVED/2)

	get_myaddress(&sin);
	sin.sin_family = AF_INET;
	for (port = MAX_PRIV; err && port >= MIN_PRIV; port--) {
		sin.sin_port = htons(port);
		err = bind(sd,&sin,sizeof(sin));
	}
	return (err == 0);
}
 

usage()
{
	fprintf(stdout,
	    "Usage: mount [-ravpfto [type|option]] ... [fsname] [dir]\n");
	exit(1);
}

check(dir, fsname)
	register char *dir;
{
	register struct direct *dp;
	register int found = 0;
	register DIR *d;
	struct stat stbuf;
	static char message[ 256 ];
	
	empty = NULL;
	if (stat(dir, &stbuf) < 0
	  || (stbuf.st_mode & S_IFMT) != S_IFDIR || (d = opendir(dir)) == NULL)
		return;
	for (dp = readdir(d); dp != NULL; dp = readdir(d)) {
		if (strcmp(dp->d_name, ".") == 0)
			continue;
		if (strcmp(dp->d_name, "..") == 0)
			continue;
		++found;
	}
	(void) closedir(d);
	if (found) {
		(void) sprintf(message,
		    "%s: directory %s is not empty (warning only)",
		    fsname, dir);
		empty = message;
	}
}

#ifdef OFS
/* this code is partly lifted from the date(1) program */

#define is_leap(A) (!((A)%4) && (A)%100)

dateopt(string, tv)
char *string;
struct timeval *tv;
{
    struct timezone tz;
    int i, year;
    int month = 1, day = 1, hour = 0, mins = 0, secs = 0;
    char x;
    char	*ep, *sp;
    static  int     dmsize[12] =
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    /* The string is of the form 'date=yymmddhhmmss' where
       everything except the year defaults. */

    if ((sp = index(string, '=')) == 0)
	return(1);
    ep = ++sp;
    while (*ep != 0 && *ep != ',')
	ep++;
    if (ep == sp || ep - sp & 1)	/*ep-sp cannot be 0 and must be even*/
        return(1);
    if (gp(&sp, &ep, &year))
	return(1);
    if (gp(&sp, &ep, &month))
	return(1);
    if (gp(&sp, &ep, &day))
	return(1);
    if (gp(&sp, &ep, &hour))
	return(1);
    if (gp(&sp, &ep, &mins))
	return(1);
    if (gp(&sp, &ep, &secs))
	return(1);

    if (year < 90)
	year += 2000;
    else
	year += 1900;
    if (is_leap(year))
	dmsize[1] = 29;
    if (month < 1 || month > 12)
	return(1);
    month--;
    if (day < 1 || day > dmsize[month])
	return(1);
    day--;
    if (hour == 24)
    {
	hour = 0;
	day++;
    }
    if (hour < 0 || hour > 23)
	    return (1);
    if (mins < 0 || mins > 59)
	return(1);
    if (secs < 0 || secs > 59)
	return (1);

    gettimeofday(tv, &tz);
    tv->tv_sec = tv->tv_usec = 0;
    for (i = 1970; i < year; i++)
	    tv->tv_sec += (is_leap(i) ? 366 : 365);
    while (month)
	    tv->tv_sec += dmsize[--month];
    tv->tv_sec += day;
    tv->tv_sec = 24 * tv->tv_sec + hour;
    tv->tv_sec = 60 * tv->tv_sec + mins;
    tv->tv_sec = 60 * tv->tv_sec + secs;
    tv->tv_sec += (long)tz.tz_minuteswest * 60;
    if (localtime(&tv->tv_sec)->tm_isdst)
	tv->tv_sec -= 60 * 60;
    return (0);
}


gp(sp, ep, val)
char **sp, **ep;
int *val;
{
    register int c, d;

    if (*sp == *ep)
	return(0);
    c = (*(*sp)++) - '0';
    d = (*(*sp)++) - '0';
    if (c < 0 || c > 9 || d < 0 || d > 9)
	return (-1);
   *val = (d + 10 * c);
   return(0);
}
#endif
