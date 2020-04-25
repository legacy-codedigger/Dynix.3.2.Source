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

/* $Header: getname.c 1.1 89/03/12 $ */
#include "h00vars.h"
#include "libpc.h"

/*
 * GETNAME - activate a file
 *
 * takes a name, name length, element size, and variable
 * level and returns a pointer to a file structure.
 *
 * a new file structure is initialized if necessary.
 * temporary names are generated, and given
 * names are blank trimmed.
 */

static char *tmpname = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

IOREC *
GETNAME(filep, name, namlim, datasize)
register IOREC *filep;
char *name;
long namlim;
long datasize;
{
	register int cnt;
	register int maxnamlen = namlim;
	IOREC *prev, *next, locvar;

	if (filep->fblk < MAXFILES && _actfile[filep->fblk] == filep) {
		/* 
		 * Close and immediately reactivate the file.
		 */
		PFCLOSE(filep, name != NULL);
		_actfile[filep->fblk] = filep;
		filep->funit &= (TEMP | FTEXT);
	} else {
		/*
		 * Initialize a new file record.
		 */
		filep->funit = 0;
		if (datasize == 0) {
			filep->funit |= FTEXT;
			datasize = 1;
		}
		filep->fsize = datasize;
		filep->fbuf = 0;
		filep->lcount = 0;
		filep->llimit = 0x7fffffff;
		filep->fileptr = &filep->window[0];
		/*
		 * Check to see if file is global, or allocated in
		 * the stack by checking its address against the
		 * address of one of our routine's local variables.
		 */
		if (filep < &locvar)
			filep->flev = GLVL;
		else
			filep->flev = filep;
		for (_filefre++; _filefre < MAXFILES; _filefre++)
			if (_actfile[_filefre] == FILNIL)
				goto gotone;
		for (_filefre = PREDEF + 1; _filefre < MAXFILES; _filefre++)
			if (_actfile[_filefre] == FILNIL)
				goto gotone;
		ERROR("File table overflow\n");
		/*NOTREACHED*/
		return;
gotone:
		filep->fblk = _filefre;
		_actfile[_filefre] = filep;
		/*
		 * Link the new record into the file chain.
		 */
		prev = (struct iorec *)&_fchain;
		next = _fchain.fchain;
		while (filep->flev > next->flev) {
			prev = next;
			next = next->fchain;
		}
		if (filep->flev == GLVL)
			/*
			 * Must order global files so that all dynamic files
			 * within a record are grouped together.
			 */
			while ((next != FILNIL) &&
			       (next->flev == GLVL) &&
			       ((struct iorec *)filep > next)) {
				prev = next;
				next = next->fchain;
			}
		filep->fchain = next;
		prev->fchain = filep;
	}
	/*
	 * Get the filename associated with the buffer.
	 */
	if (NULL == name) {
		if (*filep->fname != NULL) {
			return filep;
		}
		/*
		 * No name given and no previous name, so generate
		 * a new one of the form #tmp.xxxxxx.
		 */
		filep->funit |= TEMP;
		sprintf(filep->fname, "#tmp.%c%d", tmpname[filep->fblk],
		    getpid());
		filep->pfname = &filep->fname[0];
		return filep;
	}
	/*
	 * Trim trailing blanks, and insure that the name 
	 * will fit into the file structure.
	 */
	for (cnt = 0; cnt < maxnamlen; cnt++)
		if (name[cnt] == '\0' || name[cnt] == ' ')
			break;
	if (cnt >= NAMSIZ) {
		ERROR("%s: File name too long\n", name);
		/*NOTREACHED*/
		return;
	}
	maxnamlen = cnt;
	filep->funit &= ~TEMP;
	/*
	 * Put the new name into the structure.
	 */
	for (cnt = 0; cnt < maxnamlen; cnt++)
		filep->fname[cnt] = name[cnt];
	filep->fname[cnt] = '\0';
	filep->pfname = &filep->fname[0];
	return filep;
}
