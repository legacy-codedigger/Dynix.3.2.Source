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

#ifndef lint
static char rcsid[] = "$Header: /usr/src/dynix.3.2.0/src/usr.bin/RCS/file.c,v 1.2 92/11/11 20:08:34 bruce Exp $";
#endif

/*
 * file - determine type of file
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <a.out.h>
int	errno;
char	*sys_errlist[];
int in;
int i  = 0;
char buf[BUFSIZ];
char *troff[] = {	/* new troff intermediate lang */
	"x","T","res","init","font","202","V0","p1",0};
char *fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","data","double",0};
char *asc[] = {
#ifdef	vax
	"enter","movd","br","addr","movqd",0};
#endif
#ifdef	ns32000
	"enter","br","bsr","movqd","movd","addr",0};
#endif
#ifdef	i386
	"call", "popl", "pushl", "movl",0};
#endif
char *c[] = {
	"int","short","char","float","double","struct","extern","long",
	"unsigned", "static","void", "typedef",0};
char *as[] = {
	"globl","byte","align","text","data","comm",0};
int	ifile;

main(argc, argv)
char **argv;
{
	FILE *fl;
	register char *p;
	char ap[128];
	extern char _sobuf[];

	if (argc>1 && argv[1][0]=='-' && argv[1][1]=='f') {
		if ((fl = fopen(argv[2], "r")) == NULL) {
			perror(argv[2]);
			exit(2);
		}
		while ((p = fgets(ap, 128, fl)) != NULL) {
			int l = strlen(p);
			if (l>0)
				p[l-1] = '\0';
			printf("%s:	", p);
			type(p);
			if (ifile>=0)
				close(ifile);
		}
		exit(1);
	}
	while(argc > 1) {
		printf("%s:	", argv[1]);
		type(argv[1]);
		fflush(stdout);
		argc--;
		argv++;
		if (ifile >= 0)
			close(ifile);
	}
}

type(file)
char *file;
{
	int j,nl;
	char ch;
	struct stat mbuf;

	ifile = -1;
	if (lstat(file, &mbuf) < 0) {
		printf("%s\n", sys_errlist[errno]);
		return;
	}
	switch (mbuf.st_mode & S_IFMT) {

	case S_IFCHR:
		printf("character");
		goto spcl;

	case S_IFLNK:
		printf("symbolic link\n");
		return;

	case S_IFDIR:
		printf("directory\n");
		return;

	case S_IFIFO:
		printf("fifo\n");
		return;

	case S_IFBLK:
		printf("block");

spcl:
		printf(" special (%d/%d)\n", major(mbuf.st_rdev), minor(mbuf.st_rdev));
		return;
	}

	ifile = open(file, 0);
	if(ifile < 0) {
		printf("cannot open\n");
		return;
	}
	in = read(ifile, buf, BUFSIZ);
	if(in == 0){
		printf("empty\n");
		return;
	}
	buf[in == BUFSIZ ? BUFSIZ - 1 : in] = '\000';
	switch(*(int *)buf) {

	case 0x12eb	/* i386 OMAGIC */:
		printf("SYMMETRY i386 .o");
		goto isstripped;
	case 0x22eb	/* i386 ZiAGIC */:
		printf("SYMMETRY i386 executable (0 @ 0)");
		goto isstripped;
	case 0x32eb	/* i386 XMAGIC */:
		printf("SYMMETRY i386 executable (invalid @ 0)");
		goto isstripped;
	case 0x42eb	/* i386 SMAGIC */:
		printf("SYMMETRY i386 stand alone executable");
		goto isstripped;

	case 0x00ea	/* ns32000 OMAGIC */:
		printf("BALANCE ns32000 .o");
		goto isstripped;
	case 0x10ea	/* ns32000 ZMAGIC */:
		printf("BALANCE ns32000 executable (0 @ 0)");
		goto isstripped;
	case 0x20ea	/* ns32000 XMAGIC */:
		printf("BALANCE ns32000 executable (invalid @ 0)");
		goto isstripped;
	case 0x30ea	/* ns32000 SMAGIC */:
		printf("BALANCE ns32000 stand alone executable");
		goto isstripped;

isstripped:
		if(((int *)buf)[4] != 0)
			printf(" not stripped");
		if (((struct exec *)buf)->a_version != 0)
			printf(" version %d", ((struct exec *)buf)->a_version);
		printf("\n");
		goto out;

	case 0413:
		printf("demand paged VAX or old BALANCE ns32000 executable\n");
		goto out;

	case 0410:
		printf("pure ");
	case 0407:
		printf("VAX unix executable or old BALANCE ns32000 .o\n");
		goto out;

	case 0411:
		printf("jfr or pdp-11 unix 411 executable\n");
		return;

	/*
	 * Most-common Sun executable and object files.
	 * Magic numbers are in Sun <sys/exec.h> (but are byte-swapped on i386).
	 * We don't attempt to determined stripped status.
	 */
	case 0x0b010381:
		printf("Sun sparc demand paged dynamically linked executable\n");
		goto out;
	case 0x0b010301:
		printf("Sun sparc demand paged executable\n");
		goto out;
	case 0x07010301:
		printf("Sun sparc .o\n");
		goto out;
	case 0x0b010280:
		printf("Sun mc68020 demand paged dynamically linked executable\n");
		goto out;
	case 0x0b010200:
		printf("Sun mc68020 demand paged executable\n");
		goto out;
	case 0x07010200:
		printf("Sun mc68020 .o\n");
		goto out;

	case 0177555:
		printf("very old archive\n");
		goto out;

	case 0177545:
		printf("old archive\n");
		goto out;

	case 070707:
		printf("cpio data\n");
		goto out;
	case 0xdeadbabe:
		printf("symmetry/balance memory dump\n");
		goto out;
	case 0xbeadface:
		printf("symmetry/balance compressed memory dump\n");
		goto out;
	}

	if(strncmp(buf, "!<arch>\n__.SYMDEF", 17) == 0 ) {
		printf("archive random library\n");
		goto out;
	}
	if (strncmp(buf, "!<arch>\n", 8)==0) {
		printf("archive\n");
		goto out;
	}
#ifdef NOTYET
	if (mbuf.st_size % 512 == 0) {	/* it may be a PRESS file */
		lseek(ifile, -512L, 2);	/* last block */
		if (read(ifile, buf, BUFSIZ) > 0
		 && *(short int *)buf == 12138) {
			printf("PRESS file\n");
			goto out;
		}
	}
#endif
	i = 0;
	if(ccom() == 0)goto notc;
	while(buf[i] == '#'){
		j = i;
		while(buf[i++] != '\n'){
			if(i - j > 255){
				printf("data\n"); 
				goto out;
			}
			if(i >= in)goto notc;
		}
		if(ccom() == 0)goto notc;
	}
check:
	if(lookup(c) == 1){
		while((ch = buf[i++]) != ';' && ch != '{')if(i >= in)goto notc;
		printf("c program text");
		goto outa;
	}
	nl = 0;
	while(buf[i] != '('){
		if(buf[i] <= 0)
			goto notas;
		if(buf[i] == ';'){
			i++; 
			goto check; 
		}
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	while(buf[i] != ')'){
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	while(buf[i] != '{'){
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	printf("c program text");
	goto outa;
notc:
	i = 0;
	while(buf[i] == 'c' || buf[i] == '#'){
		while(buf[i++] != '\n')if(i >= in)goto notfort;
	}
	if(lookup(fort) == 1){
		printf("fortran program text");
		goto outa;
	}
notfort:
	i=0;
	if(ascom() == 0)goto notas;
	j = i-1;
	if(buf[i] == '.'){
		i++;
		if(lookup(as) == 1){
			printf("assembler program text"); 
			goto outa;
		}
		else if(buf[j] == '\n' && isalpha(buf[j+2])){
			printf("roff, nroff, or eqn input text");
			goto outa;
		}
	}
	while(lookup(asc) == 0){
		if(ascom() == 0)goto notas;
		while(buf[i] != '\n' && buf[i++] != ':')
			if(i >= in)goto notas;
		while(buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t')if(i++ >= in)goto notas;
		j = i-1;
		if(buf[i] == '.'){
			i++;
			if(lookup(as) == 1){
				printf("assembler program text"); 
				goto outa; 
			}
			else if(buf[j] == '\n' && isalpha(buf[j+2])){
				printf("roff, nroff, or eqn input text");
				goto outa;
			}
		}
	}
	printf("assembler program text");
	goto outa;
notas:
	for(i=0; i < in; i++)if(buf[i]&0200){
		if (buf[0]=='\100' && buf[1]=='\357') {
			printf("troff (CAT) output\n");
			goto out;
		} else {
			union {
                                struct user us;
                                char pad[UPAGES][NBPG];
                        } ku;

                        lseek(ifile, 0L, 0);
                        if(read(ifile, &ku, sizeof ku) == sizeof ku) {
                                int p = UPAGES + ku.us.u_dsize - ku.us.u_tsize + ku.us.u_ssize;
                                if( p * NBPG == mbuf.st_size && p > UPAGES) {
                                        printf("core from %s\n", ku.us.u_comm);
                                        return;
                                }
                        }
                        printf("data\n");
		}
		goto out; 
	}
	if (mbuf.st_mode&((S_IEXEC)|(S_IEXEC>>3)|(S_IEXEC>>6)))
		printf("commands text");
	else if (troffint(buf, in))
		printf("troff intermediate output text");
	else if (english(buf, in))
		printf("English text");
	else
		printf("ascii text");
outa:
	while(i < in)
		if((buf[i++]&0377) > 127){
			printf(" with garbage\n");
			goto out;
		}
	/* if next few lines in then read whole file looking for nulls ...
		while((in = read(ifile,buf,BUFSIZ)) > 0)
			for(i = 0; i < in; i++)
				if((buf[i]&0377) > 127){
					printf(" with garbage\n");
					goto out;
				}
		/*.... */
	printf("\n");
out:;
}



troffint(bp, n)
char *bp;
int n;
{
	int k;

	i = 0;
	for (k = 0; k < 6; k++) {
		if (lookup(troff) == 0)
			return(0);
		if (lookup(troff) == 0)
			return(0);
		while (i < n && buf[i] != '\n')
			i++;
		if (i++ >= n)
			return(0);
	}
	return(1);
}
lookup(tab)
char *tab[];
{
	char r;
	int k,j,l;
	while(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n')i++;
	for(j=0; tab[j] != 0; j++){
		l=0;
		for(k=i; ((r=tab[j][l++]) == buf[k] && r != '\0');k++);
		if(r == '\0')
			if(buf[k] == ' ' || buf[k] == '\n' || buf[k] == '\t'
			    || buf[k] == '{' || buf[k] == '/'){
				i=k;
				return(1);
			}
	}
	return(0);
}
ccom(){
	char cc;
	while((cc = buf[i]) == ' ' || cc == '\t' || cc == '\n')if(i++ >= in)return(0);
	if(buf[i] == '/' && buf[i+1] == '*'){
		i += 2;
		while(buf[i] != '*' || buf[i+1] != '/'){
			if(buf[i] == '\\')i += 2;
			else i++;
			if(i >= in)return(0);
		}
		if((i += 2) >= in)return(0);
	}
	if(buf[i] == '\n')if(ccom() == 0)return(0);
	return(1);
}
ascom(){
	while(buf[i] == '/'){
		i++;
		while(buf[i++] != '\n')if(i >= in)return(0);
		while(buf[i] == '\n')if(i++ >= in)return(0);
	}
	return(1);
}

english (bp, n)
char *bp;
{
# define NASC 128
	int ct[NASC], j, vow, freq, rare;
	int badpun = 0, punct = 0;
	if (n<50) return(0); /* no point in statistics on squibs */
	for(j=0; j<NASC; j++)
		ct[j]=0;
	for(j=0; j<n; j++)
	{
		if (bp[j]<NASC)
			ct[bp[j]|040]++;
		switch (bp[j])
		{
		case '.': 
		case ',': 
		case ')': 
		case '%':
		case ';': 
		case ':': 
		case '?':
			punct++;
			if ( j < n-1 &&
			    bp[j+1] != ' ' &&
			    bp[j+1] != '\n')
				badpun++;
		}
	}
	if (badpun*5 > punct)
		return(0);
	vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
	freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
	rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];
	if (2*ct[';'] > ct['e']) return(0);
	if ( (ct['>']+ct['<']+ct['/'])>ct['e']) return(0); /* shell file test */
	return (vow*5 >= n-ct[' '] && freq >= 10*rare);
}
