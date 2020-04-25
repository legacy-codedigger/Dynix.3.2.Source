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

/*  ADA support used by permission of Verdix Corporation
 *  Given Sept 25, 1985 by Steve Zeigler
 *  Author: Ben Priest
 */

#ifndef lint
static char *rcsid = "$Header: ex_vops3.c 2.1 86/04/15 $";
#endif

/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"
#include "iskey.h"

/*
 * Routines to handle structure.
 * Operations supported are:
 *	( ) { } [ ]
 *
 * These cover:		LISP		TEXT
 *	( )		s-exprs		sentences
 *	{ }		list at same	paragraphs
 *	[ ]		defuns		sections
 *
 * { and } for C used to attempt to do something with matching {}'s, but
 * I couldn't find definitions which worked intuitively very well, so I
 * scrapped this.
 *
 * The code here is very hard to understand.
 */
line	*llimit;
int	(*lf)();

#ifdef LISPCODE
int	lindent();
#endif

bool	wasend;

#define nc_match(unk1, unk2) ((unk1 == unk2 ) || ((unk1 + 32) == unk2) || (unk1 == (unk2 + 32)))

/*
 * Find over structure, repeated count times.
 * Don't go past line limit.  F is the operation to
 * be performed eventually.  If pastatom then the user said {}
 * rather than (), implying past atoms in a list (or a paragraph
 * rather than a sentence.
 */
lfind(pastatom, cnt, f, limit)
	bool pastatom;
	int cnt, (*f)();
	line *limit;
{
	register int c;
	register int rc = 0;
	char save[LBSIZE];

	/*
	 * Initialize, saving the current line buffer state
	 * and computing the limit; a 0 argument means
	 * directional end of file.
	 */
	wasend = 0;
	lf = f;
	strcpy(save, linebuf);
	if (limit == 0)
		limit = dir < 0 ? one : dol;
	llimit = limit;
	wdot = dot;
	wcursor = cursor;

	if (pastatom >= 2) {
		while (cnt > 0 && word(f, cnt))
			cnt--;
		if (pastatom == 3)
			eend(f);
		if (dot == wdot) {
			wdot = 0;
			if (cursor == wcursor)
				rc = -1;
		}
	}
#ifdef LISPCODE
	else if (!value(LISP)) {
#else
	else {
#endif
		char *icurs;
		line *idot;

		if (linebuf[0] == 0) {
			do
				if (!lnext())
					goto ret;
			while (linebuf[0] == 0);
			if (dir > 0) {
				wdot--;
				linebuf[0] = 0;
				wcursor = linebuf;
				/*
				 * If looking for sentence, next line
				 * starts one.
				 */
				if (!pastatom) {
					icurs = wcursor;
					idot = wdot;
					goto begin;
				}
			}
		}
		icurs = wcursor;
		idot = wdot;

		/*
		 * Advance so as to not find same thing again.
		 */
		if (dir > 0) {
			if (!lnext()) {
				rc = -1;
				goto ret;
			}
		} else
			ignore(lskipa1(""));

		/*
		 * Count times find end of sentence/paragraph.
		 */
begin:
		for (;;) {
			while (!endsent(pastatom))
				if (!lnext())
					goto ret;
			if (!pastatom || wcursor == linebuf && endPS())
				if (--cnt <= 0)
					break;
			if (linebuf[0] == 0) {
				do
					if (!lnext())
						goto ret;
				while (linebuf[0] == 0);
			} else
				if (!lnext())
					goto ret;
		}

		/*
		 * If going backwards, and didn't hit the end of the buffer,
		 * then reverse direction.
		 */
		if (dir < 0 && (wdot != llimit || wcursor != linebuf)) {
			dir = 1;
			llimit = dot;
			/*
			 * Empty line needs special treatement.
			 * If moved to it from other than begining of next line,
			 * then a sentence starts on next line.
			 */
			if (linebuf[0] == 0 && !pastatom && 
			   (wdot != dot - 1 || cursor != linebuf)) {
				lnext();
				goto ret;
			}
		}

		/*
		 * If we are not at a section/paragraph division,
		 * advance to next.
		 */
		if (wcursor == icurs && wdot == idot || wcursor != linebuf || !endPS())
			ignore(lskipa1(""));
	}
#ifdef LISPCODE
	else {
		c = *wcursor;
		/*
		 * Startup by skipping if at a ( going left or a ) going
		 * right to keep from getting stuck immediately.
		 */
		if (dir < 0 && c == '(' || dir > 0 && c == ')') {
			if (!lnext()) {
				rc = -1;
				goto ret;
			}
		}
		/*
		 * Now chew up repitition count.  Each time around
		 * if at the beginning of an s-exp (going forwards)
		 * or the end of an s-exp (going backwards)
		 * skip the s-exp.  If not at beg/end resp, then stop
		 * if we hit a higher level paren, else skip an atom,
		 * counting it unless pastatom.
		 */
		while (cnt > 0) {
			c = *wcursor;
			if (dir < 0 && c == ')' || dir > 0 && c == '(') {
				if (!lskipbal("()"))
					goto ret;
				/*
 				 * Unless this is the last time going
				 * backwards, skip past the matching paren
				 * so we don't think it is a higher level paren.
				 */
				if (dir < 0 && cnt == 1)
					goto ret;
				if (!lnext() || !ltosolid())
					goto ret;
				--cnt;
			} else if (dir < 0 && c == '(' || dir > 0 && c == ')')
				/* Found a higher level paren */
				goto ret;
			else {
				if (!lskipatom())
					goto ret;
				if (!pastatom)
					--cnt;
			}
		}
	}
#endif
ret:
	strcLIN(save);
	return (rc);
}

/*
 * Is this the end of a sentence?
 */
endsent(pastatom)
	bool pastatom;
{
	register char *cp = wcursor;
	register int c, d;

	/*
	 * If this is the beginning of a line, then
	 * check for the end of a paragraph or section.
	 */
	if (cp == linebuf)
		return (endPS());

	/*
	 * Sentences end with . ! ? not at the beginning
	 * of the line, and must be either at the end of the line,
	 * or followed by 2 spaces.  Any number of intervening ) ] ' "
	 * characters are allowed.
	 */
	if (!any(c = *cp, ".!?"))
		goto tryps;
	do
		if ((d = *++cp) == 0)
			return (1);
	while (any(d, ")]'"));
	if (*cp == 0 || *cp++ == ' ' && *cp == ' ')
		return (1);
tryps:
	if (cp[1] == 0)
		return (endPS());
	return (0);
}

/*
 * End of paragraphs/sections are respective
 * macros as well as blank lines and form feeds.
 */
endPS()
{

	return (linebuf[0] == 0 ||
		isa(svalue(PARAGRAPHS)) || isa(svalue(SECTIONS)));
	    
}

#ifdef LISPCODE
lindent(addr)
	line *addr;
{
	register int i;
	char *swcurs = wcursor;
	line *swdot = wdot;

again:
	if (addr > one) {
		register char *cp;
		register int cnt = 0;

		addr--;
		getline(*addr);
		for (cp = linebuf; *cp; cp++)
			if (*cp == '(')
				cnt++;
			else if (*cp == ')')
				cnt--;
		cp = vpastwh(linebuf);
		if (*cp == 0)
			goto again;
		if (cnt == 0)
			return (whitecnt(linebuf));
		addr++;
	}
	wcursor = linebuf;
	linebuf[0] = 0;
	wdot = addr;
	dir = -1;
	llimit = one;
	lf = lindent;
	if (!lskipbal("()"))
		i = 0;
	else if (wcursor == linebuf)
		i = 2;
	else {
		register char *wp = wcursor;

		dir = 1;
		llimit = wdot;
		if (!lnext() || !ltosolid() || !lskipatom()) {
			wcursor = wp;
			i = 1;
		} else
			i = 0;
		i += column(wcursor) - 1;
		if (!inopen)
			i--;
	}
	wdot = swdot;
	wcursor = swcurs;
	return (i);
}
#endif

/*
** Retrieves an id for a proc, fn, or pkg so I
** know what to look for at the end of an end (brp)
*/
char
*get_id(ptr, pos)
register char *ptr;
register char *pos;
{
	char *sv_ptr;

	sv_ptr = ptr;

	while (iswhite(*pos) && (*pos != 0)) pos++;
	if (*pos == 0) return(0);
	
	if (*pos == '"') {
		*ptr++ = *pos++;
		while ((*pos != '"') && (*pos != 0))

			*ptr++ = *pos++;
		if (*pos == '"') *ptr++ = '"';
	
	} else 
		while ((isalpha(*pos) || (*pos == '_') || isdigit(*pos)
							  || (*pos == '.')) && (*pos != 0))

			*ptr++ = *pos++;

	if((*pos == 0) && (ptr == sv_ptr)) return(0);
	*ptr = '\0';
	return(pos);
}

/*
** Get an id for declare for syntactic closure and
** for the % operator. Check the contents of ptr to find
** out if an id was returned.(brp)
*/
int
get_decl_id(ptr, pos, addr, save_addr)
register char *ptr;		/* ptr to place to store the id */
register char *pos;		/* pos in the edited file */
register line *addr;	/* starting line of the buffer to search */
register line *save_addr; /* end line of search backwards */
{
	*ptr = '\0';
	while (1) {
		if (*pos == ':') break;
		pos--;
		if (pos < linebuf) {
			if (addr > save_addr) {
				addr--;
				getline(*addr);
				pos = linebuf + strlen(linebuf) - 1;
			} else
				return;
		}
	}
	while (iswhite(*pos) || (!*pos) || (*pos == ':')) { 
		pos--;
		if (pos < linebuf) {
			if (addr > save_addr) {
				addr--;
				getline(*addr);
				pos = linebuf + strlen(linebuf) - 1;
			} else
				return;
		}
	}
	while ((isalpha(*pos) || (*pos == '_') || 
			isdigit(*pos)) && (pos > linebuf)) {
		pos--;
	}

	while (iswhite(*pos)) pos++;

	while ((isalpha(*pos) || (*pos == '_') || isdigit(*pos)) && (*pos))
	{
		*ptr++ = *pos++;
	}

	*ptr = '\0';
	return;
}

/*
** checks for and skips over strings when travelling in either direction (brp)
*/
skip_string()
{
	do {
		if (!lnext()) {
			wdot = NOLINE;
			return(0);
		}
		if (*wcursor == '"') {
			if (dir == -1) { /* bwds */
				if (*(wcursor-1) == '"') 
					wcursor -= 1;	
				else
					break;
			} else { /* fwds */
				if (*(wcursor+1) == '"')
					wcursor += 1;	
				else
					break;
			}
		} else if (*wcursor == 0) break;
	} while (1) ;

	if (!lnext()) {
		wdot = NOLINE;
		return(0);
	}
	return(1);
}

/*
** checks for and skips over comments in the backward direction 
** when searching for keywords (brp)
*/
ck_for_comment()
{
	char *save;

	save = wcursor;
	while (wcursor >= linebuf) {   /* linebuf has current line */

		if ((*wcursor == '-') && (*(wcursor-1) == '-')) 
			return(1);
		else 
			wcursor--;
	}
	wcursor = save;
	return(0);
}

/*
** test to see if a word is surrounded by "white space" (brp)
*/
is_token(pos, ln)
char *pos;
int ln;
{
	char *tcursor;

	if(pos == linebuf)
		tcursor = pos;
	else
		tcursor = pos - 1;

	if ((iswhite(*tcursor) || (tcursor == linebuf) || (*tcursor == ':')) && 
	   (iswhite(*(pos+ln)) || (!*(pos+ln)))) 
		return(1);
	else
		return(0);
}

/*
** finds the end of procs, functions, pkgs, pkg bodies,
** tasks, task bodies, task types (brp)
*/
find_end(wd, len)
register char *wd;
register int len;
{
	register char *temp;
	char spare[128];

	do {
		if (!lnext()) {
			wdot = NOLINE;
			return (0);
		}
		if (*wcursor == '"') {
			skip_string();
		} else if ((*wcursor == '-') && (*(wcursor+1) == '-') && (dir == 1)) {
			while (*wcursor != 0) wcursor++;
			continue;
		}
		if (nc_match(*wcursor, 'e') && (!nc_strncmp(wcursor, "end", 3))) {
			if ((temp = get_id(spare, wcursor + 3)) > 0) 
				if (!nc_strncmp(spare, wd, len)) return(1); 
		} 
	} while (1);
	return (1);
} 

/*
** Finds the start of procedures, functions, packages, 
** tasks, task types, task bodies, and declare blocks.
** Looks for the id used after the "end" (brp)
*/
find_start(wd)
register char *wd;		/* name for the structure */
{
	int len;
	char *temp;
	char spare[120]; 

	len = strlen(wd);
	do {
		if (!lnext()) {
			wdot = NOLINE;
			return (0);
		}
		switch (*wcursor) {

		case '"':
				skip_string();
				break;
		case 'D':
		case 'd':
				if (!nc_strncmp(wcursor, "declare", 7)) {
					/* only looking 3 lines max for the id */
					get_decl_id(spare, 
							wcursor-1, wdot, ((wdot-3) > one) ? (wdot-3) : one);
					if (spare[0] != '\0') {
						if (!nc_strncmp(wd, spare, len)) 
							if (!ck_for_comment()) return(1);
					}
				} 
				break;
		case 'F':
		case 'f':
				if (!nc_strncmp(wcursor, "function", 8)) {
					if ((temp = get_id(spare, wcursor + 8)) > 0) 
						if (!nc_strncmp(wd, spare, len)) 
							if (!ck_for_comment()) return(1);
				} 
				break;
		case 'P':
		case 'p':
				if (nc_match(*(wcursor+1), 'r')) {	/* procedure */
					if (!nc_strncmp(wcursor, "procedure", 9)) {
						if ((temp = get_id(spare, wcursor + 9)) > 0) 
							if (!nc_strncmp(wd, spare, len)) 
								if (!ck_for_comment()) return(1);
					} 
					break;
				} else {						/* package */
					if (!nc_strncmp(wcursor, "package", 7)) {
						if ((temp = get_id(spare, wcursor + 7)) > 0) 
							if (!nc_strncmp(spare, "body", 4)) {
								if ((temp = get_id(spare, temp)) > 0) 
									if (!nc_strncmp(wd, spare, len))
										if (!ck_for_comment()) return(1);
							} else {
								if (!nc_strncmp(wd, spare, len))
									if (!ck_for_comment()) return(1);
							}
					}
				}
				break;
		case 'T':			/* tasks */
		case 't':
				if (!nc_strncmp(wcursor, "task", 4)) {
					if ((temp = get_id(spare, wcursor + 4)) > 0) 
						if ((!nc_strncmp(spare, "body", 4)) ||
						    (!nc_strncmp(spare, "type", 4)))
						{
							if ((temp = get_id(spare, temp)) > 0) 
								if (!nc_strncmp(wd, spare, len))
									if (!ck_for_comment()) return(1);
						} else {
							if (!nc_strncmp(wd, spare, len))
								if (!ck_for_comment()) return(1);
						}
				}
				break;
		}
	} while (1);
	return (1);
} 

/*
** finds the keyword loop from a "while"
** or "for" so the search can start there (brp)
*/
find_loop()
{
	do {
		if (!lnext()) {
			wdot = NOLINE;
			return(0);
		}
		if ((*wcursor == 'l') && (!nc_strncmp(wcursor, "loop", 4))) 
			return(1);
	} while (1);
}

lmatchp_c(addr)
	line *addr;
{
	register int i;
	register char *parens, *cp;

	for (cp = cursor; !any(*cp, "({[)}]");)
		if (*cp++ == 0)
			return (0);
	lf = 0;
	parens = any(*cp, "()") ? "()" : any(*cp, "[]") ? "[]" : "{}";
	if (*cp == parens[1]) {
		dir = -1;
		llimit = one;
	} else {
		dir = 1;
		llimit = dol;
	}
	if (addr)
		llimit = addr;
	if (splitw)
		llimit = dot;
	wcursor = cp;
	wdot = dot;
	i = lskipbal(parens);
	return (i);
}

/*
** This is the big daddy procedure which drives the whole "%"
** operator.  It checks to see if the current char is interesting.
** If so, it makes further checks to see what the token is and 
** then it decides what actions to take. (brp)
*/
lmatchp(addr)
line *addr;
{
	register int i;
	register char *parens, *cp;
	register int len;
	char spare[120]; 
	register char *temp;

	if (value(ADA) == 0) return(lmatchp_c(addr));

	for (cp = cursor; !any(*cp, "{(cdefilprstwCDEFILPRSTW)");)
		if (*cp++ == 0)
			return (0);
	lf = 0;
	if ((*cp == '(') || (*cp == ')') || (*cp == '{') || (*cp == '}')) {
		return(lmatchp_c(addr));
	}
	if (nc_match(*cp, 'e')) {			/* go backward */
		dir = -1;
		llimit = one;
	} else {							/* go forward */
		dir = 1;
		llimit = dol;
	}
	if (addr)
		llimit = addr;
	if (splitw)
		llimit = dot;
	wcursor = cp;
	wdot = dot;
	switch (*cp) {
		case 'C':
		case 'c':
				i = lskip_key_wd("case", "end case");
				break;
		case 'D':
		case 'd':
				if (nc_match(*(cp+1), 'e')) {	/* declare */
					/* only looking 3 lines max for the name */
					get_decl_id(spare, wcursor-1, wdot, 
											((wdot-3) > one) ? (wdot-3) : one);
					if (spare[0] == '\0') break;

					len = strlen(spare);
					i = find_end(spare,len);
				} 
				break;
		case 'F':
		case 'f':
				if (nc_match(*(cp+1), 'u')) {	/* function */
					if ((wcursor = get_id(spare, wcursor + 8)) == 0) {
						i = 0;
						break;
					}
					len = strlen(spare);
					i = find_end(spare,len);
				} else {
					if (nc_match(*(cp+1), 'o')) {		/* for */
						if (find_loop()) 
							i = lskip_key_wd("loop", "end loop");
					}
				}
				break;
		case 'I':
		case 'i':
				i = lskip_key_wd("if", "end if");
				break;
		case 'L':
		case 'l':
				i = lskip_key_wd("loop", "end loop");
				break;
		case 'P':
		case 'p':
				if (nc_match(*(cp+1), 'r')) {	/* procedure */
					if ((wcursor = get_id(spare, wcursor + 9)) == 0) {
						i = 0;
						break;
					}
				} else {						/* package */
					if ((wcursor = get_id(spare, wcursor + 7)) == 0) {
						i = 0;
						break;
					}
					if (!nc_strncmp(spare, "body", 4)) {
						if ((wcursor = get_id(spare, wcursor)) == 0) {
							i = 0;
							break;
						}
					}
				}
				len = strlen(spare);
				i = find_end(spare, len);
				break;
		case 'T':
		case 't':
				if ((wcursor = get_id(spare, wcursor + 4)) == 0) {
					i = 0;
					break;
				}
				if ((!nc_strncmp(spare, "body", 4)) ||
					 (!nc_strncmp(spare, "type", 4))) {
					if ((wcursor = get_id(spare, wcursor)) == 0) {
						i = 0;
						break;
					}
				}
				len = strlen(spare);
				i = find_end(spare, len);
				break;
		case 'R':
		case 'r':
				i = lskip_key_wd("record", "end record");
				break;
		case 'S':
		case 's':
				i = lskip_key_wd("select", "end select");
				break;
		case 'W':
		case 'w':
				/* handle while loop */
				if (find_loop()) 
					i = lskip_key_wd("loop", "end loop");
				break;
		case 'E':
		case 'e':
				if (!nc_strncmp("if", cp+4, 2))
					i = lskip_key_wd("if", "end if");
				else if (!nc_strncmp("loop", cp+4, 4))
					i = lskip_key_wd("loop", "end loop");
				else if (!nc_strncmp("case", cp+4, 4))
					i = lskip_key_wd("case", "end case");
				else if (!nc_strncmp("select", cp+4, 4))
					i = lskip_key_wd("select", "end select");
				else if (!nc_strncmp("record", cp+4, 4))
					i = lskip_key_wd("record", "end record");
				else {
					if ((wcursor = get_id(spare, wcursor + 3)) == 0) {
						i = 0;
						break;
					}
					i = find_start(spare);
				}
				break;
		default:
				;
	}
	return (i);
}

/*
** This function does the searches for the keywords passed to it.
** pass the matching pairs and it will find the mate of the one it is
** at.  Figures out the nesting and everything.  Used for everything
** except procs, fns, and pkgs. (things with ids) (brp)
*/
lskip_key_wd(bwd, ewd)
register char *ewd, *bwd;
{
	register int level = dir;
	register int elen;
	register int blen;

	elen = strlen(ewd);
	blen = strlen(bwd);
	do {
		if (!lnext()) {
			wdot = NOLINE;
			return(0);
		}
		if (*wcursor == '"') {
			skip_string();
		} else if ((*wcursor == '-') && (*(wcursor+1) == '-') && (dir == 1)) {
			while (*wcursor != 0) wcursor++;
			continue;
		}

		if (nc_match(*wcursor, *ewd) && (!nc_strncmp(wcursor, ewd, elen))) {
			level--;
			if ((level != 0) && (dir == 1)) wcursor += elen;
		} else if(nc_match(*wcursor, *bwd) && (!nc_strncmp(wcursor,bwd,blen))) {

			if (dir == 1) {					/* forwards */
				if (is_token(wcursor, blen)) {
					wcursor += blen;
					level++;
				}
			} else {						/* backwards */
				if (nc_strncmp(wcursor-4, ewd, elen)) {
					if (!ck_for_comment() && is_token(wcursor, blen)) {
						level++;
					}
				} else {								/* matched an end xx */
					if (!ck_for_comment()) {
						wcursor -= 5;
						level--;
					}
				}
			}
		}
	} while (level);

	/* ck for a "while" or "for" on same line as "loop" */
	if (!strcmp(bwd, "loop")) {
		register char *save;

		save = wcursor;
		while (wcursor >= linebuf) {   /* linebuf has current line */

			if ((nc_match(*wcursor, 'w') && (!nc_strncmp(wcursor,"while",5))) ||
				(nc_match(*wcursor, 'f') && (!nc_strncmp(wcursor, "for", 3)))) 
				return(1);
			else 
				wcursor--;
		}
		wcursor = save;
	}
	return (1);
} 

/*
** This function does the analysis for ada syntactic closure. (brp)
** It is simple-minded but hopefully good enough to ignore strings
** and comments.  Operates only in the forward direction.
** Pass back the id, if any, and the template number.
*/
lsyn_analysis(addr, cnt, id, ret_idx)
line *addr;
int  *cnt;
char *id;
int  *ret_idx;
{
	register char *idx;
	register char *s;
	register int i;
	int 	save_idx;
	char	buf[80];
	char 	*temp;
	line 	*save_addr;

	int 	keyres = 0;
	int 	bodyflag = 0;
	int 	declareflag = 0;
	int 	specflag = 0;
	int 	count = *cnt;

	save_addr = addr;
	*cnt = 0;
	addr -= 1;

	for (i = 1; i <= count; i++) {	/* look through cnt lines */
		(*cnt)++;
		addr += 1;
		if(addr > dol) {
			getline(*save_addr);
			break;
		}
		getline(*addr);

		/* look through entire line */
		for (idx = linebuf; *idx != 0; idx++) {

			if ((*idx == '-') && (*(idx+1) == '-'))
				break;
			else if (*idx == '"') {
				do {
					idx++;
					if (*idx == '"') {
						if (*(idx+1) == '"')
							idx += 1;	
						else
							break;
					}
				} while (*idx != 0);
			} else if (iswhite(*idx) || (!isalpha(*idx)))
				continue;
				
			if ((!bodyflag) && (!specflag)) {
				if (isupper(*idx)) {
					if (isupper(*(idx+1)))
						rw_case = UC;
					else
						rw_case = UL;
				} else
					rw_case = LC;
			}
			s = buf;
			while ((*idx) && (!iswhite(*idx)) && (*idx != ':')) {
				if (isupper(*idx))
					*s = tolower(*idx);
				else {
					*s = *idx;
				}
				s++;
				idx++;
			}
			*s = '\0';
			
			if (s >= buf + 2)
				keyres = iskey(buf);
			 else continue;

			switch (keyres) {

				case ACCEPT:
						temp = get_id(id, idx);
						if (temp != 0) {
							idx = temp;
							*ret_idx = whitecnt(linebuf);
							return(ACCEPT);
						}
						break;
				case DECLARE:	/* look for declare id before the keyword! */ 
						*ret_idx = whitecnt(linebuf);
						get_decl_id(id, idx-8, addr, save_addr);
						return(DECLARE);
						break;
				case IS:
						if (bodyflag) {
							*ret_idx = save_idx;
							return(BODY);
						}
						if (specflag) {
							*ret_idx = save_idx;
							return(SPEC);
						}
						break;
				case TASK:
						save_idx = whitecnt(linebuf);
						temp = get_id(id, idx);
						if ((temp != 0) && ((!nc_strncmp(id, "body", 4)) || 
											(!nc_strncmp(id, "type", 4)))) {
							bodyflag = 1;
							idx = temp;
							temp = get_id(id, idx);
						}
						if (temp != 0) {
							specflag = 1;
							idx = temp;
						}
						break;
				case PACKAGE:
						save_idx = whitecnt(linebuf);
						temp = get_id(id, idx);
						if((temp != 0) && (!nc_strncmp(id, "body", 4))) {
							bodyflag = 1;
							idx = temp;
							temp = get_id(id, idx);
						}
						if(temp != 0) {
							specflag = 1;
							idx = temp;
						}
						break;
				case FUNCTION:
				case PROCEDURE:
						save_idx = whitecnt(linebuf);
						temp = get_id(id, idx);
						if(temp != 0) {
							bodyflag = 1;
							idx = temp;
						}
						break;
				case BODY:
						bodyflag = 1;
						break;
				case CASE:
				case IF:
				case RECORD:
				case SELECT:
						*ret_idx = whitecnt(linebuf);
						return(keyres);
						break;
				case FOR:
				case LOOP:
				case WHILE:
						*ret_idx = whitecnt(linebuf);
						return(LOOP);
						break;
			}
			if (!(*idx)) break;	
		}
	}
	getline(*save_addr);
	return(0);
} 

lsmatch(cp)
	char *cp;
{
	char save[LBSIZE];
	register char *sp = save;
	register char *scurs = cursor;

	wcursor = cp;
	strcpy(sp, linebuf);
	*wcursor = 0;
	strcpy(cursor, genbuf);
	cursor = strend(linebuf) - 1;
	if (lmatchp(dot - vcline)) {
		register int i = insmode;
		register int c = outcol;
		register int l = outline;

		if (!MI)
			endim();
		vgoto(splitw ? WECHO : LINE(wdot - llimit), column(wcursor) - 1);
		flush();
		sleep(1);
		vgoto(l, c);
		if (i)
			goim();
	}
	else {
		strcLIN(sp);
		strcpy(scurs, genbuf);
		if (!lmatchp((line *) 0))
			beep();
	}
	strcLIN(sp);
	wdot = 0;
	wcursor = 0;
	cursor = scurs;
}

ltosolid()
{

	return (ltosol1("()"));
}

ltosol1(parens)
	register char *parens;
{
	register char *cp;

	if (*parens && !*wcursor && !lnext())
		return (0);
	while (isspace(*wcursor) || (*wcursor == 0 && *parens))
		if (!lnext())
			return (0);
	if (any(*wcursor, parens) || dir > 0)
		return (1);
	for (cp = wcursor; cp > linebuf; cp--)
		if (isspace(cp[-1]) || any(cp[-1], parens))
			break;
	wcursor = cp;
	return (1);
}

lskipbal(parens)
	register char *parens;
{
	register int level = dir;
	register int c;

	do {
		if (!lnext()) {
			wdot = NOLINE;
			return (0);
		}
		c = *wcursor;
		if (c == parens[1])
			level--;
		else if (c == parens[0])
			level++;
	} while (level);
	return (1);
}

lskipatom()
{
	return (lskipa1("()"));
}

lskipa1(parens)
	register char *parens;
{
	register int c;

	for (;;) {
		if (dir < 0 && wcursor == linebuf) {
			if (!lnext())
				return (0);
			break;
		}
		c = *wcursor;
		if (c && (isspace(c) || any(c, parens)))
			break;
		if (!lnext())
			return (0);
		if (dir > 0 && wcursor == linebuf)
			break;
	}
	return (ltosol1(parens));
}

lnext()
{

	if (dir > 0) {
		if (*wcursor)
			wcursor++;
		if (*wcursor)
			return (1);
		if (wdot >= llimit) {
			if (lf == vmove && wcursor > linebuf)
				wcursor--;
			return (0);
		}
		wdot++;
		getline(*wdot);
		wcursor = linebuf;
		return (1);
	} else {
		--wcursor;
		if (wcursor >= linebuf)
			return (1);
#ifdef LISPCODE
		if (lf == lindent && linebuf[0] == '(')
			llimit = wdot;
#endif
		if (wdot <= llimit) {
			wcursor = linebuf;
			return (0);
		}
		wdot--;
		getline(*wdot);
		wcursor = linebuf[0] == 0 ? linebuf : strend(linebuf) - 1;
		return (1);
	}
}

lbrack_c(c, f)
	register int c;
	int (*f)();
{
	register line *addr;

	addr = dot;
	for (;;) {
		addr += dir;
		if (addr < one || addr > dol) {
			addr -= dir;
			break;
		}
		getline(*addr);
		if (linebuf[0] == '{' ||
#ifdef LISPCODE
		    value(LISP) && linebuf[0] == '(' ||
#endif
		    isa(svalue(SECTIONS))) {
			if (c == ']' && f != vmove) {
				addr--;
				getline(*addr);
			}
			break;
		}
		if (c == ']' && f != vmove && linebuf[0] == '}')
			break;
	}
	if (addr == dot)
		return (0);
	if (f != vmove)
		wcursor = c == ']' ? strend(linebuf) : linebuf;
	else
		wcursor = 0;
	wdot = addr;
	vmoving = 0;
	return (1);
}

/*
** Does the "[[" and "]]" commands (brp)
*/
lbrack(c, f)
register int c;
int (*f)();
{
	register line *addr;
	register char *idx;

	if (value(ADA) == 0) return(lbrack_c(c, f));

	addr = dot;
	for (;;) {
		addr += dir;
		if (addr < one || addr > dol) {
			addr -= dir;
			break;
		}
		getline(*addr);
		/* look through entire line */
		for (idx = linebuf; *idx != 0; idx++) {

			if ((*idx == '-') && (*(idx+1) == '-'))
				break;
			else if (*idx == '"') {
				do {
					idx++;
					if (*idx == '"') {
						if (*(idx+1) == '"')
							idx += 1;	
						else
							break;
					}
				} while (*idx != 0) ;
			}
			switch(*idx) {
				case 'p':
				case 'P':
			   		if((nc_strncmp("procedure", idx, 9) || !is_token(idx, 9)) &&
				       (nc_strncmp("package", idx, 7) || !is_token(idx, 7)))
						continue;
					break;
				case 'f':
				case 'F':
					if(nc_strncmp("function", idx, 8) || !is_token(idx, 8))
						continue;
					break;
				case 'd':
				case 'D':
					if(nc_strncmp("declare", idx, 7) || !is_token(idx, 7))
						continue;
					break;
				case 't':
				case 'T':
					if(nc_strncmp("task", idx, 4) || !is_token(idx, 4))
						continue;
					break;
				default:
					if(!isa(svalue(SECTIONS)))
						continue;
			}
			if (c == ']' && f != vmove) {
				addr--;
				getline(*addr);
			}
			goto zip;
		}
	    if(c == ']' && f != vmove) {
			for (idx = linebuf; *idx != 0; idx++) {
				if(nc_match(*idx, 'e')) {
					if(!nc_strncmp(idx,"end",3) && is_token(idx, 3))
						goto zip;
				}
			}
		}
	}
zip: if(addr == dot)
		return (0);
	if(f != vmove)
		wcursor = c == ']' ? strend(linebuf) : linebuf;
	else
		wcursor = 0;
	wdot = addr;
	vmoving = 0;
	return (1);
}

isa(cp)
	register char *cp;
{

	if (linebuf[0] != '.')
		return (0);
	for (; cp[0] && cp[1]; cp += 2)
		if (linebuf[1] == cp[0]) {
			if (linebuf[2] == cp[1])
				return (1);
			if (linebuf[2] == 0 && cp[1] == ' ')
				return (1);
		}
	return (0);
}

/*
** Does string compares with n chars and dosen't care about case (brp)
*/
nc_strncmp(s1, s2, n)
register char *s1, *s2;
register n;
{
	while (--n >= 0) {

		if(!nc_match(*s1, *s2)) {
			break;
		}
		s2++;
		if(*s1++ == '\0')
			return(0);
	}

	return(n<0 ? 0 : *s1 - *s2);
}
