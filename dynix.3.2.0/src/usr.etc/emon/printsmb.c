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

/* $Header: printsmb.c 1.3 87/04/11 $
 */

/*
 * $Log:	printsmb.c,v $
 */

#ifndef	lint
static	char	rcsid[] = "$Header: printsmb.c 1.3 87/04/11 $";
#endif

#include "emon.h"

printsmb(smbp)
	struct smb_msg *smbp;
{
	u_short	*w;
	u_char	*c;
	int	 i;
	WORD	bcc;
	WORD	err;
	WORD	*ep;

	printf("****** SMB (CORE) Protocol Message ******\n");

	if(smbprintlots){
	printf("\tBYTE	smb_idf[4] %x %x %x %x\n", (u_char)smbp->smb_idf[0],
			(u_char)smbp->smb_idf[1], (u_char)smbp->smb_idf[2],
			(u_char)smbp->smb_idf[3]);
	}

	printf("\tBYTE	smb_com %x -> ", (u_char)smbp->smb_com);

	switch(smbp->smb_com) {

	case	SMBmkdir:	/* 0x00		/* create directory */
		printf("SMBmkdir\n");
		break;
	case	SMBrmdir:	/* 0x01		/* delete directory */
		printf("SMBrmdir\n");
		break;
	case	SMBopen:	/* 0x02		/* open file */
		printf("SMBopen\n");
		break;
	case	SMBcreate:	/* 0x03		/* create file */
		printf("SMBcreate\n");
		break;
	case	SMBclose:	/* 0x04		/* close file */
		printf("SMBclose\n");
		break;
	case	SMBflush:	/* 0x05		/* flush file */
		printf("SMBflush\n");
		break;
	case	SMBunlink:	/* 0x06		/* unlink file */
		printf("SMBunlink\n");
		break;
	case	SMBmv:		/* 0x07		/* rename file */
		printf("SMBmv\n");
		break;
	case	SMBgetatr:	/* 0x08		/* get file attributes */
		printf("SMBgetatr\n");
		break;
	case	SMBsetatr:	/* 0x09		/* set file attributes */
		printf("SMBsetatr\n");
		break;
	case	SMBread:	/* 0x0A		/* read from file */
		printf("SMBread\n");
		break;
	case	SMBwrite:	/* 0x0B		/* write to file */
		printf("SMBwrite\n");
		break;
	case	SMBlock:	/* 0x0C		/* lock byte range */
		printf("SMBlock\n");
		break;
	case	SMBunlock:	/* 0x0D		/* unlock byte range */
		printf("SMBunlock\n");
		break;
	case	SMBctemp:	/* 0x0E		/* create temporary file */
		printf("SMBctemp\n");
		break;
	case	SMBmknew:	/* 0x0F		/* make new file */
		printf("SMBmknew\n");
		break;
	case	SMBchkpth:	/* 0x10		/* check directory path */
		printf("SMBchkpth\n");
		break;
	case	SMBexit:	/* 0x11		/* process exit */
		printf("SMBexit\n");
		break;
	case	SMBlseek:	/* 0x12		/* seek */
		printf("SMBlseek\n");
		break;
	case	SMBtcon:	/* 0x70		/* tree connect */
		printf("SMBtcon\n");
		break;
	case	SMBtdis:	/* 0x71		/* tree disconnect */
		printf("SMBtdis\n");
		break;
	case	SMBnegprot:	/* 0x72		/* negotiate protocol */
		printf("SMBnegprot\n");
		break;
	case	SMBdskattr:	/* 0x80		/* get disk attributes */
		printf("SMBdskattr\n");
		break;
	case	SMBsearch:	/* 0x81		/* search directory */
		printf("SMBsearch\n");
		break;
	case	SMBsplopen:	/* 0xC0		/* open print spool file */
		printf("SMBsplopen\n");
		break;
	case	SMBsplwr:	/* 0xC1		/* write to print spool file */
		printf("SMBsplwr\n");
		break;
	case	SMBsplclose:	/* 0xC2		/* close print spool file */
		printf("SMBsplclose\n");
		break;
	case	SMBsplretq:	/* 0xC3		/* return print queue */
		printf("SMBsplretq\n");
		break;
	default:
		printf("SMB???\n");

	}

	printf("\tBYTE	smb_rcls %x ", (u_char)smbp->smb_rcls);

	ep = (WORD *)&smbp->smb_err[0];
	err = *ep;

	switch(smbp->smb_rcls) {
	case SUCCESS:	/* successful request class */
		printf("SUCCESS: ");
		switch(err) {
		case SUCCESS:	/* successful request */
			printf("\n");
			break;
		case BUFFERED:	/* message has been buffered */
			printf("BUFFERED\n");
			break;
		case LOGGED:	/* message has been logged */
			printf("LOGGED\n");
			break;
		case DISPLAYED:	/* user message displayed */
			printf("DISPLAYED\n");
			break;
		default:
			printf("???\n");
		}
		break;
	case ERRDOS:	/* error generated by operating system */
		printf("ERRDOS: ");
		switch(err) {
		case ERRbadfunc:	/* invalid function, EINVAL */
			printf("ERRbadfunc\n");
			break;
		case ERRbadfile:	/* file not found, ENOENT */
			printf("ERRbadfile\n");
			break;
		case ERRbadpath:	/* directory invalid, ENOENT */
			printf("ERRbadpath\n");
			break;
		case ERRnofids:		/* out of file descripters, EMFILE */
			printf("ERRnofids\n");
			break;
		case ERRnoaccess:	/* access denied, EPERM */
			printf("ERRnoaccess\n");
			break;
		case ERRbadfid:		/* invalid FID, EBADF */
			printf("ERRbadfid\n");
			break;
		case ERRbadmcb:		/* memory control blocks destroyed */
			printf("ERRbadmcb\n");
			break;
		case ERRnomem:		/* out of server memory, ENOMEM */
			printf("ERRnomem\n");
			break;
		case ERRbadmem:		/* invalid memory address, EFAULT */
			printf("ERRbadmem\n");
			break;
		case ERRbadenv:		/* invalid environment */
			printf("ERRbadenv\n");
			break;
		case ERRbadformat:	/* invalid format */
			printf("ERRbadformat\n");
			break;
		case ERRbadaccess:	/* invalid open mode */
			printf("ERRbadaccess\n");
			break;
		case ERRbaddata:	/* invalid data, E2BIG */
			printf("ERRbaddata\n");
			break;
		case ERR:		/* RESERVED??? */
			printf("ERR: RESERVED ???\n");
			break;
		case ERRbaddrive:	/* invalid drive */
			printf("ERRbaddrive\n");
			break;
		case ERRremcd:		/* delete dir on current dir */
			printf("ERRremcd\n");
			break;
		case ERRdiffdevice:	/* not same device, EXDEV */
			printf("ERRdiffdevice\n");
			break;
		case ERRnofiles:	/* file search cmd failed */
			printf("ERRnofiles\n");
			break;
		case ERRbadshare:	/* share mode conflict */
			printf("ERRbadshare\n");
			break;
		case ERRlock:		/* lock request conflict */
			printf("ERRlock\n");
			break;
		case ERRfilexists:	/* file exists, EEXIST */
			printf("ERRfilexists\n");
			break;
		default:
			printf("???\n");
		}
		break;
	case ERRSRV:		/* error generated by server */
		printf("ERRSRV: ");
		switch(err) {
		case ERRerror:		/* non-specific error */
			printf("ERRerror\n");
			break;
		case ERRbadpw:		/* bad name/passwd pair */
			printf("ERRbadpw\n");
			break;
		case ERRbadtype:	/* RESERVED */
			printf("ERRbadtype: RESERVED ???\n");
			break;
		case ERRaccess:		/* access rights denied */
			printf("ERRaccess\n");
			break;
		case ERRinvnid:		/* TID invalid */
			printf("ERRinvnid\n");
			break;
		case ERRinvnetname:	/* invalid net name */
			printf("ERRinvnetname\n");
			break;
		case ERRinvdevice:	/* invalid device */
			printf("ERRinvdevice\n");
			break;
		case ERRqfull:	/* printq full, on open */
			printf("ERRqfull\n");
			break;
		case ERRqtoobig:	/* printq full, no space */
			printf("ERRqtoobig\n");
			break;
		case ERRqeof:		/* EOF on printq dump */
			printf("ERRqeof\n");
			break;
		case ERRinvpfid:	/* invalid print file FID */
			printf("ERRinvpfid\n");
			break;
		case ERRpaused:		/* server is paused */
			printf("ERRpaused\n");
			break;
		case ERRmsgoff:		/* not receiving messages */
			printf("ERRmsgoff\n");
			break;
		case ERRnoroom:		/* no room to buffer message */
			printf("ERRnoroom\n");
			break;
		case ERRrmuns:		/* too many remote user names */
			printf("ERRrmuns\n");
			break;
		case ERRnosupport:	/* function not supported */
			printf("ERRnosupport\n");
			break;
		default:
			printf("???\n");
		}
		break;
	case ERRHRD:
		printf("ERRHRD: error generated by hardware - ");
		switch(err) {
		case ERRnowrite:	/* invalid FID, EBADF */
			printf("ERRnowrite: read-only media, EROFS\n");
			break;
		case ERRbadunit:	/* unknown unit, ENODEV */
			printf("ERRbadunit\n");
			break;
		case ERRnotready:	/* drive not ready */
			printf("ERRnotready\n");
			break;
		case ERRbadcmd:		/* invalid disk drive */
			printf("ERRbadcmd\n");
			break;
		case ERRdata:		/* data error(CRC), EIO */
			printf("ERRdata\n");
			break;
		case ERRbadreq:		/* bad request structure length */
			printf("ERRbadreq\n");
			break;
		case ERRseek:		/* seek error */
			printf("ERRseek\n");
			break;
		case ERRbadmedia:	/* unknown media type */
			printf("ERRbadmedia\n");
			break;
		case ERRbadsector:	/* sector not found */
			printf("ERRbadsector\n");
			break;
		case ERRnopaper:	/* printer out of paper */
			printf("ERRnopaper\n");
			break;
		case ERRwrite:		/* write fault */
			printf("ERRwrite\n");
			break;
		case ERRread:		/* read fault */
			printf("ERRread\n");
			break;
		case ERRgeneral:	/* general failure */
			printf("ERRgeneral\n");
			break;
		case ERRbadshar:	/* compat open conflict */
			printf("ERRbadshar\n");
			break;
		default:
			printf("???\n");
		}
		break;
	case ERRCMD:
		printf("ERRCMD: command not in SMB format\n");
		break;
	default:
		printf("SMB???\n");
	}

	if(smbprintlots) {
	printf("\tBYTE	smb_reh %x\n", (u_char)smbp->smb_reh);

	printf("\tBYTE	smb_err[2] %x %x\n", (u_char)smbp->smb_err[0],
						(u_char)smbp->smb_err[1]);

	printf("\tBYTE	smb_reb %x\n", (u_char)smbp->smb_reb);

	printf("\tWORD	smb_res[0-7] %x %x %x %x %x %x %x (reserved)\n",
					(u_short)smbp->smb_res[0],
					(u_short)smbp->smb_res[1],
					(u_short)smbp->smb_res[2],
					(u_short)smbp->smb_res[3],
					(u_short)smbp->smb_res[4],
					(u_short)smbp->smb_res[5],
					(u_short)smbp->smb_res[6]);
	}

	printf("\tWORD	smb_tid %x ntohs(%x)\n", (u_short)smbp->smb_tid,
						(u_short)ntohs(smbp->smb_tid));
	printf("\tWORD	smb_pid %x ntohs(%x)\n", (u_short)smbp->smb_pid,
						(u_short)ntohs(smbp->smb_pid));
	printf("\tWORD	smb_uid %x ntohs(%x)\n", (u_short)smbp->smb_uid,
						(u_short)ntohs(smbp->smb_uid));
	printf("\tWORD	smb_mid %x ntohs(%x)\n", (u_short)smbp->smb_mid,
						(u_short)ntohs(smbp->smb_mid));

	printf("\tBYTE	smb_wct %x\n", smbp->smb_wct);

	c = (u_char *)&smbp->smb_wct;
	c++;
	w = (u_short *) c;

	if(smbp->smb_wct) printf("\tPARAMETERS:\n");
	for(i = 0; i < smbp->smb_wct; i++) {
		printf("\t[%d] 0x%x [%d]\n", i, *w, *w);
		w++;
	}

	bcc = *w;
	printf("\tWORD	smb_bcc 0x%x [%d]\n", bcc, bcc);
	w++;

	if(smbprintlots) {
	if(bcc){
		printf("smb_buf[] - ");
		c = (u_char *) w;
		for(i = 0; i < bcc; i++) {
			if(i == 20) {
				printf("\n            ");
				translate((u_char *)w, 20);
				printf("\n            ");
				bcc -= 20; w += 10; i = 0; /* w is WORD */
			}
			printf("%x ", *c);
			c++;
		}
		printf("\n            ");
		translate((u_char *)w, (int) bcc);
		printf("\n");
	}
	}	/* end if smbprintlots */
}
