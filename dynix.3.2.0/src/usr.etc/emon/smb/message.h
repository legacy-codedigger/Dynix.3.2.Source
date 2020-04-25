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

/*
 * $Header: message.h 1.1 86/05/20 $
 */

/*
 * $Log:	message.h,v $
 */

/* CORE File Sharing Protocol Message */

struct smb_msg { 		/* BE CAREFUL OF ALIGNMENT */
	struct smb_hdr {		/* start of header */
		BYTE	h_idf[4];	/* FF'SMB' */
		BYTE	h_com;		/* command */
		BYTE	h_rcls;		/* error code class */
		BYTE	h_reh;		/* reserved */
		BYTE	h_err[2];	/* error code (really WORD) */
		BYTE	h_reb;		/* reserved */
		WORD	h_res[7];	/* reserved */
		WORD	h_tid;		/* tree id # */
		WORD	h_pid;		/* process id # */
		WORD	h_uid;		/* user id # */
		WORD	h_mid;		/* multiplex id # */
	} smb_hdr;			/* end of header */
	BYTE	smb_wct;		/* count of parameter words */
	BYTE	smb_bcc[2];		/* data bytes (really WORD) */

	/*
	 * NOTE BENE: smb_bcc is in variable location in message - here only
	 * if smb_wct == 0 - useful for cases when smb_wct is 0.
	 */
};

/*
 *	struct smb_var is used to help some of the BYTE alignment
 *	issues - use BYTEs only, define maximum size message based on
 *	maximum number of parameters including smb_bcc.
 *	(not really meaningful in struct definition).
 */

#define SMBPARAMS	16	/* Max number of params in a command (est) */

struct smb_var {
	BYTE	a_wct;		  	  	/* return parameter count */
	BYTE	a_vwv[sizeof(WORD)*SMBPARAMS];	/* parameters */
};

typedef	struct smb_hdr	HDR;
typedef	struct smb_msg	MSG;

#define	smb_idf		smb_hdr.h_idf
#define	smb_com		smb_hdr.h_com
#define	smb_rcls	smb_hdr.h_rcls
#define	smb_reh		smb_hdr.h_reh
#define	smb_err		smb_hdr.h_err
#define	smb_reb		smb_hdr.h_reb
#define	smb_res		smb_hdr.h_res
#define	smb_tid		smb_hdr.h_tid
#define	smb_pid		smb_hdr.h_pid
#define	smb_uid		smb_hdr.h_uid
#define	smb_mid		smb_hdr.h_mid

struct smbidphdr {
	struct idp smbi_i;
	struct sphdr smbi_spp;
	struct smb_msg smbi_s;
};

/* macros to address bytes in the smb_message */

#define	AW(n)	   *(WORD *)&ap->a_vwv[sizeof(WORD)*(n)]
#define	AA(type,n)  (type *)&ap->a_vwv[sizeof(WORD)*(n)]
