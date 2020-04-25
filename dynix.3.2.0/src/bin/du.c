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

#ifndef lint
static char rcsid[] = "$Header: du.c 2.2 87/05/04 $";
#endif

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>

#ifdef SCGACCT
#include <ctype.h>
#include <pwd.h>
#include <local/scgacct.h>
#include <curses.h>
#include <sys/fs.h>
#include <fstab.h>
#include <sys/file.h>
#endif

char	path[BUFSIZ], name[BUFSIZ];
int	aflg;
int	sflg;
char	*dot = ".";

#define ML	1000
struct {
	int	dev;
	ino_t	ino;
} ml[ML];
int	mlx;

#ifdef SCGACCT
struct account {                /* chained list of accounts, per user */
        char    a_acct;
        long    a_nblks;
        struct account *a_next;
};

struct user {                   /* chained list of users */
        int             u_uid;  
        char            *u_name;
        char            u_alist[N_SCGACCT+1];
        struct account  *u_ap;
        struct acctdisk *u_dap;
        struct user     *u_unext;
};
struct user *Up;        /* Head of  user chain */

int     Noarg,  /* invoked with no args */
        Dflag,  /* write 'diskuse' file (MUST BE ROOT) */
        Qflag,  /* quiet mode */
        Cflag,  /* cost estimate */
        Pflag;  /* print accounting id's */

FILE    *Dirfp,   /* for diracct file */
        *Diskfp;  /* for diskacct file */

long    tblks;

char            *nextfs();
char            *getdiracct();
struct user     *userinit();

#endif

long	descend();
char	*index(), *rindex(), *strcpy(), *sprintf();

#define	kb(n)	(howmany(dbtob(n), 1024))
#ifdef SCGACCT
#define chgblk(n)       (kb(n)*(1024/512))
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	long blocks = 0;
	register char *np;
	int pid;

#ifdef SCGACCT
	register char   *p, *s, *fs;
        char account, getaccount();
        struct stat lstatb;
        
        while(--argc > 0 && (*++argv)[0] == '-') {
                for(s = argv[0]+1; *s != '\0'; s++)
                        switch(*s) {
                        case 's':
				++sflg;
				break;
                        case 'a':
				++aflg;
				break;
                        case 'p':
                                ++Pflag;
                                break;
                        case 'd':
                                if(geteuid()) {
                                        fprintf(stderr, "ABORT, must be superuser for\
 d option\n");
                                        exit(1);
                                }
                                ++Dflag;
                                break;
                        case 'q':
                                ++Qflag;
                                break;
                        case 'c':
                                ++Cflag;
                                break;
                        default:
                                fprintf(stderr, "illegal option %c\n", *s);
                                usage();
                                break;
                        }
        }

        if(argc == 0)
                ++Noarg;

	if(sflg && aflg)
                usage();

        if(Dflag) {
                if((Diskfp = fopen("diskacct", "a+")) == NULL) {
                        fprintf(stderr, "ABORT, cannot open diskacct\n");
                        exit(1);
                }
                userinit();   
        }       

        if(Qflag)
		sflg = Pflag = aflg = Cflag = 0;

#else   /* original 4.2BSD */
	argc--, argv++;
again:
	if (argc && !strcmp(*argv, "-s")) {
		sflg++;
		argc--, argv++;
		goto again;
	}
	if (argc && !strcmp(*argv, "-a")) {
		aflg++;
		argc--, argv++;
		goto again;
	}
	if (argc == 0) {
		argv = &dot;
		argc = 1;
	}
#endif

#ifndef SCGACCT  
/* I don't know why we have to "fork()" but it wrecks acctg
 * none of the data gets accumulated since files are reinitialized
 */
	do {
		if (argc > 1) {
			pid = fork();
			if (pid == -1) {
				fprintf(stderr, "No more processes.\n");
				exit(1);
			}
			if (pid != 0)
				wait((int *)0);
		}
		if (argc == 1 || pid == 0) {
#endif
#ifdef SCGACCT
	while (1) {
			/**
			 **	If "du -d" with no args, 
			 **     read file systems from fstab
			 **/
			if (Dflag && Noarg) {
				if ((fs = nextfs()) == NULL)
					break;
				(void)strcpy(path, fs);
			}
			else
				(void)strcpy(path, Noarg ? "." : *argv);
			(void)strcpy(name, path);
#else   /* original 4.2BSD */
			(void) strcpy(path, *argv);
			(void) strcpy(name, *argv);
#endif
			if (np = rindex(name, '/')) {
				*np++ = '\0';
				if (chdir(*name ? name : "/") < 0) {
					perror(*name ? name : "/");
#ifdef SCGACCT
                                if (Dflag) {
                                        fprintf(stderr, "SKIPPING acctg of <%s>\n", path);
					goto restart;
		                }
                                else
#endif
					exit(1);
				}
			} else
				np = path;
#ifdef SCGACCT
                if(Dflag || Pflag || Cflag) {
                        if((p = getdiracct(*np?np:".")) != NULL) { 
                            if((Dirfp = fopen(p, "r")) == NULL) {
                                fprintf(stderr, "cannot open dir <%s>\n", p);
				if (Dflag) goto restart;
                                else
                                    exit(1);
                            }
                        }
                        else {
				fprintf(stderr, "ABORT, cannot find diracct\n");
                                exit(1);
                        }
                }
                account = '\0';
                if(stat(path,&lstatb)<0) {
                        fprintf(stderr, "cannot stat < %s >\n", path);
                }
                if (Dflag) {
                        account = getaccount(lstatb.st_ino);
		} 
                blocks = descend(path, *np? np: ".", 
					(int)lstatb.st_uid, account);  /* FIX ?? */
#else
			blocks = descend(path, *np ? np : ".");
#endif
			if (sflg)
#ifdef SCGACCT
			{
#endif
				printf("%ld\t%s\n", kb(blocks), path);

#ifdef SCGACCT
				if (Cflag)
					tblks = blocks;
			}
			if (Dirfp || Cflag)
				fclose(Dirfp);
restart:
#endif
#ifndef SCGACCT
			if (argc > 1)
				exit(1);
		}
#else
	    if (Dflag && Noarg) 
		    continue;
#endif
		argc--, argv++;
#ifdef SCGACCT
		if (argc <= 0)
			break;
	};
#else
	} while (argc > 0);
#endif

#ifdef SCGACCT
        if (Cflag)
                costacct();
        if (Dflag)
                wrdiskacct();
#endif
	exit(0);
}

DIR	*dirp = NULL;

#ifdef SCGACCT
long
descend(base, name, defuid, defacct)
	char *base, *name;
int defuid;
char defacct;

#else   /* original 4.2BSD */
long
descend(base, name)
	char *base, *name;
#endif
{
	char *ebase0, *ebase;
	struct stat stb;
	int i;
	long blocks = 0;
	long curoff = NULL;
	register struct direct *dp;
#ifdef SCGACCT
        int uid;
        char account, getaccount();
#endif

	ebase0 = ebase = index(base, 0);
	if (ebase > base && ebase[-1] == '/')
		ebase--;
	if (lstat(name, &stb) < 0) {
		perror(base);
		*ebase0 = 0;
		return (0);
	}
	if (stb.st_nlink > 1 && (stb.st_mode&S_IFMT) != S_IFDIR) {
#ifdef SCGACCT
            if (!Dflag) {
#endif
		for (i = 0; i <= mlx; i++)
			if (ml[i].ino == stb.st_ino && ml[i].dev == stb.st_dev)
				return (0);
		if (mlx < ML) {
			ml[mlx].dev = stb.st_dev;
			ml[mlx].ino = stb.st_ino;
			mlx++;
		}
#ifdef SCGACCT
            }
#endif
	}
	blocks = stb.st_blocks;
	if ((stb.st_mode&S_IFMT) != S_IFDIR) {
		if (aflg)
			printf("%ld\t%s\n", kb(blocks), base);
#ifdef SCGACCT
                if(Dflag)       /* record no. of blocks in file */
		    if ( install(defuid, defacct, chgblk(blocks)) != OK ){
                        fprintf(stderr, "WARNING, check %s (%d blocks)\n",
					base, blocks );
                    }
#endif
		return (blocks);
	}

#ifdef SCGACCT
        if(Dflag || Pflag || Cflag) {
		uid = (int)stb.st_uid;
		account = getaccount(stb.st_ino);
                if(Dflag)
		    if ( install(uid, account, chgblk(blocks)) != OK){
                        fprintf(stderr, "WARNING, check %s (%d blocks)\n",
					base, chgblk(blocks) );
                    }
        }
#endif

	if (dirp != NULL)
		closedir(dirp);
	dirp = opendir(name);
	if (dirp == NULL) {
		perror(base);
		*ebase0 = 0;
		return (0);
	}
	if (chdir(name) < 0) {
		perror(base);
		*ebase0 = 0;
		closedir(dirp);
		dirp = NULL;
		return (0);
	}
	while (dp = readdir(dirp)) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		(void) sprintf(ebase, "/%s", dp->d_name);
		curoff = telldir(dirp);
#ifdef SCGACCT
			blocks += descend(base, ebase+1, uid, account);
#else
		blocks += descend(base, ebase+1);
#endif
		*ebase = 0;
		if (dirp == NULL) {
			dirp = opendir(".");
			if (dirp == NULL) {
				perror(".");
				return (0);
			}
			seekdir(dirp, curoff);
		}
	}
	closedir(dirp);
	dirp = NULL;
#ifdef SCGACCT
	if(!sflg)
                if(Pflag || Cflag) {
                        register struct scgacct *ap;
			if((ap = getacuid(uid)) == NULL) {
				fprintf(stderr, "no accounts for uid %d\n",
				    uid);
                                account = '?';
			}
			else {
                                register int i;
                                for(i = 0; i < N_SCGACCT; i++)
                                        if(account == ap->a_id[i])
                                                break;
                                if(i == N_SCGACCT)
                                        account = '?';
                        }
                        if(account == '?')
				printf("%-7.7s %-8.ld ", "INVALID", chgblk(blocks));
                        else 
                                printf("%-7.7c %-8.ld ", 
					account, chgblk(blocks));
                        if(Cflag) {
                                tblks += blocks;
				printf("$%-10.2f ",(double)chgblk(blocks)*COSTPERBLK);
                        }
			printf("%s\n", base);
                }
                else if(!Qflag)
		    if (Dflag)
			printf("%ld\t%s\n", chgblk(blocks), base);
		    else
#else
	if (sflg == 0)
#endif
		printf("%ld\t%s\n", kb(blocks), base);
	if (chdir("..") < 0) {
		(void) sprintf(index(base, 0), "/..");
		perror(base);
#ifdef SCGACCT
                    if (Dflag)
			fprintf(stderr, "ABORT, cannot chdir <%s>\n", base);
#endif
		exit(1);
	}
	*ebase0 = 0;
	return (blocks);
}

#ifdef SCGACCT
char *calloc();

struct user *userinit()
{
        register struct user    *up;
        register struct passwd  *pwd;
        register struct scgacct *ap;
        register char           *p;
        register int            i;

        Up = (struct user *)calloc(sizeof(struct user), 1);
        up = Up;
        while((pwd = getpwent()) != NULL) {
                up->u_uid = pwd->pw_uid;
                up->u_name = calloc(strlen(pwd->pw_name)+1, 1);
                strcpy(up->u_name, pwd->pw_name);
                up->u_unext = (struct user *)calloc(sizeof(struct user), 1);    
		if((ap = getacuid(up->u_uid)) == NULL) {
			fprintf(stderr, "warning: no accounts for uid %d\n",
				up->u_uid);
                        up->u_alist[0] = '?'; /* stupid, but effective */
		}
                else {
                        i = 0; 
                        p = up->u_alist; 
                        while(ap->a_id[i])
                                *p++ = ap->a_id[i++];
                        *p = '\0';
                }
                up = up->u_unext;
        }
        up->u_unext = NULL;
        return(Up);
}

/* 
 * walk up the directory tree,
 * and snap up the diracct account file
 * in the root directory, if there is one.
 */
char *getdiracct(s)
        char *s;
{
        static char buf[512];
        struct stat root;

        strcpy(buf,s);
        stat(buf, &root);

        while (root.st_ino != ROOTINO) {
                strcat(buf,"/..");
                if (stat(buf,&root) < 0) {
                        fprintf(stderr, "%s: can't move up tree\n", s);
                        return(NULL);
                }
        }
        strcat(buf,"/diracct");
        return(buf);
}

/*
 * get account id
 * associated with inode n.
 * if the account is invalid, return
 * a '?' (uniquely invalid).
 */
char getaccount(n)
        ino_t n;
{
        char account = '\0';

        fseek(Dirfp, (long)n, 0);
        fread(&account, sizeof(account), 1, Dirfp);

        if(isdigit(account) || isalpha(account) )
                return(account);
        return('?');
}

/*
 * cough up usage message and die.
 */
usage() 
{
        fprintf(stderr, "Usage: du [-s | -a] [-c] [-q] [-d] [-p] [name...]\n");
        fprintf(stderr, "-s and -a options are mutually exclusive\n");
        exit(1);
}

/*
 * walk chain of users,
 * installing an account if needed, 
 * else bump block count of account found.
 */
install(uid, account, blocks)
        int     uid;
        char    account;
        long    blocks;
{
        register struct user    *up;
        register struct account *ap, *np;

        for(up = Up; up != NULL; up = up->u_unext) {
                if(up->u_uid == uid) {
                        for(ap = up->u_ap; ap != NULL; ap = ap->a_next) {
                                if(account == ap->a_acct) {
                                        ap->a_nblks += blocks;
                                        return(OK);
                                }
                        }
                        /* 
                         * add a new account to chain 
                         */
                        np = (struct account *)
                                calloc(sizeof(struct account),1);
                        np->a_nblks = blocks;
                        np->a_acct = account;
                        np->a_next = NULL;
                        if(up->u_ap == NULL)
                                up->u_ap = np;
                        else {
                                ap = up->u_ap; 
                                while(ap->a_next != NULL)
                                        ap = ap->a_next;
                                ap->a_next = np;
                        }
                        return(OK);
                }
        }
        if(up == NULL) {
                fprintf(stderr, "BOTCH: no uid %d\n", uid);
                return(~OK);
        }
}

/*
 * write out records to 'diskacct' file.
 * for each account a user has, determine
 * if it is a legal account, and then write the
 * record. If the account id was invalid, default
 * back to the first valid account.
 * He better, by gum, have a valid account, or things
 * will be botched.
 */

wrdiskacct()
{
        register struct user            *up;
        register struct account         *ap;
        struct acctdisk                 dbuf;
        register struct acctdisk        *dp = &dbuf;

        for(up = Up; up != NULL; up = up->u_unext) {
                if(up->u_ap) {
                        for(ap = up->u_ap; ap != NULL; ap = ap->a_next) {       
                                if(ap->a_acct == '?' 
                                   || invalid(up, ap->a_acct)) {
                                        ap->a_acct = up->u_alist[0];
                                }
                                dp->da_uid = up->u_uid;
                                time(&dp->da_time);
                                dp->da_acct = ap->a_acct;
                                dp->da_nblks = ap->a_nblks;
                                fwrite(dp, sizeof(struct acctdisk), 1, Diskfp);
                        }
                }
        }
}

invalid(up, id)
        struct user *up;
        char id;
{
        register char *p;

        for(p = up->u_alist; *p; p++)
                if(*p == id)
                        return(0);
        return(1);
}

/*
 * estimate the cost
 * should get the COSTPERBLK from
 * the outside world, rather than hardcoded...
 */
costacct()
{
        double tcost;

        tcost = (double)(tblks)*COSTPERBLK;
        printf("total blocks %ld\n", tblks);
        printf("total cost  $%-7.2f ($%6.4f/Kbyte/day)\n", tcost,COSTPERBLK);
}

#ifdef CHECK
#define DIRECT 10       /* Number of direct blocks */
#define INDIR  256      /* Number of pointers in an indirect block */
#define INSHFT 8        /* Number of right shifts to divide by INDIR */
long nblock(size)
long size;
{
        long blocks, tot;

        /* compute no. of blocks occupied on disk */
        blocks = tot = ((size + BSIZE-1) >> BSHIFT) ;

        /* add no. of indirect blocks */
        if (blocks > DIRECT)
                tot += ((blocks - DIRECT - 1) >> INSHFT) + 1;

        /* add no. of double indirect blocks */
        if (blocks > DIRECT + INDIR)
                tot += ((blocks - DIRECT - INDIR - 1) >> (INSHFT * 2)) + 1;

        /* add no. of triple indirect blocks */
        if (blocks > DIRECT + INDIR + INDIR*INDIR)
                tot++;
        return(tot);
}
#endif
#endif

#ifdef SCGACCT
/**
 **		routine to find next file system in fstab with diracct file
 **/
char *
nextfs()
{
	char dir[MAXPATHLEN+1];
	char diracct[MAXPATHLEN+1];
	struct fstab *ff;

	while((ff = getfsent()) != NULL) {
		if (strcmp(ff->fs_type, FSTAB_RO) &&
		    strcmp(ff->fs_type, FSTAB_RW) &&
		    strcmp(ff->fs_type, FSTAB_RQ))
		    continue;

		strcpy(dir, ff->fs_file);
		sprintf(diracct, "%s/diracct", dir);
		if (access(diracct, F_OK|R_OK) == 0) {
			fprintf(stderr, "*** Process %s...\n", dir);
			fflush(stderr);
			return(dir);
		}
	}
	return(NULL);
}
#endif
