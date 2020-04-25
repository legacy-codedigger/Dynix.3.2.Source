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

#ifdef RCS
static char rcsid[]= "$Header: zdceeld.c 1.6 88/06/01 $";
#endif RCS

/*
 * zdceeld - load the ZDC firmware. 
 *
 * This program allows the user to specify multiple slic addresses
 * of ZDC controllers and the file where a new EEPROM image resides
 * and loads the new image onto the specified controllers.  Before
 * downloading, it does some checking to ensure that the slic address
 * is a valid ZDC address, and it also must check whether the controller 
 * has an EEPROM or an SRAM and modify the download procedure 
 * accordingly.
 * This program also asks the user for hardware and software revision
 * numbers when the specified controller is uninitialized, and for
 * initialized boards, gives the user the option of changing the
 * revisions.
 * Two special cases this program looks for are:
 *	current HW rev of 1 and FW rev of 9 :
 *			indicates chip is an EPROM and can't be upgraded
 *	current HW rev of 1 and FW rev greater than 9:
 *			change HW rev to 2, since this is the version
 *			where the EPROM was changed to an EEPROM
 *
 */

/* $Log:	zdceeld.c,v $
 */

#include <sys/types.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>
#include <machine/hwparam.h>
#include <machine/slic.h>

#define DOT	1023

/*
 * Board types and Chip types
 */
#define NOTFOUND	0
#define ZDC		1
#define UNKN		2

#define EPROM		0
#define EEPROM		1
#define	SRAM		2

/*
 * checksum modes
 */
#define ONES		0	/* one's complement xor sum */
#define TWOS		1	/* two's complement byte sum */

#define	UWORDSZ		8		/* uword is 8 bytes */
#define UBUFSIZE	(16*2048)	/* EEPROM size */
#define MAXSLICS	8		/* S81 can have max of 8 ZDC's */

/*
 * Xicor EEPROM Software Data Protection sequence.
 */
#define EEDATA0	0xAA
#define EEADDR0	0x5555
#define EEDATA1	0x55
#define EEADDR1	0x2AAA
#define EEDATA2	0xA0
#define EEADDR2	0x5555

#define CFGCHK_SIZE	0x1f	/* size of data to be checksummed for config */
#define CHKSUM_ADDR 	0x23	/* based on eetoc structure */	

static	int	find_zdc();
static	u_char	read_zdcmem();
static	char	*input;
static	u_char	ucodebuf[UBUFSIZE];		/* Buffer for ucode */

struct revs {
	int r_hw;
	int r_sw;
	int r_type;
} revs[] = {			/* table of legal revision combinations */
	1, 0, EPROM,
	2, 1, EEPROM,
	3, 2, SRAM
};
#define MAXHWREVS	3	/* number of entries in revs[] table */

char	*prompt();

main() 
{
	int slicaddr[MAXSLICS];
	int hw[MAXSLICS];
	int sw[MAXSLICS];
	int memtype[MAXSLICS];
	int cc, ask;
	int type, btype;
	u_char fw;
	u_char chksum;
	int numslics = 0;
	int i;

	/*
	 * Read up to MAXSLICS slic addresses or until
	 * user enters 'q' or just a return.
	 */
	while (numslics < MAXSLICS) {
		input = prompt("ZDC slic address? ");
		if (*input == '\0' || *input == 'q')
			break;
		slicaddr[numslics] = atoi(input);
		btype = find_zdc(slicaddr[numslics]);
		switch (btype) {
			 
		case NOTFOUND:
			continue;

		case ZDC:
			/*
			 * Board is an initialized ZDC board, so save
			 * old HW and SW revision numbers to patch in
			 * to new firmware.
			 */
			hw[numslics] = read_zdcmem(slicaddr[numslics], SL_G_HGENERATION);
			sw[numslics] = read_zdcmem(slicaddr[numslics], SL_G_SGENERATION);
			fw = read_zdcmem(slicaddr[numslics], SL_Z_VERSION);

			/*
			 * Check for following special cases:
			 *   -- HWrev = 1, FWrev = 9
			 *	   indicates an EPROM so can't download
			 *   -- HWrev = 1, FWrev > 9
			 *	   this is to correct an error; HWrev should
			 *	   be 2 because of switch from EPROM to EEPROM 
			 */

			if (hw[numslics] == 1 && fw == 9) {
				printf("EPROM at slic address %d -- cannot upgrade\n",
						slicaddr[numslics]);
				continue;
			}
			if (hw[numslics] == 1 && fw > 9) {
				hw[numslics] = 2;
				sw[numslics] = 1;
			}
			memtype[numslics] = zdcmem_type(slicaddr[numslics]);
			ask = 1;
			for (i = 0; i < MAXHWREVS; i++) {
				if (revs[i].r_hw == hw[numslics]) {
					if (revs[i].r_type == 
							memtype[numslics]) {
						sw[numslics] = revs[i].r_sw;
						ask = 0;
					}
					break;
				}
			}
			printf("HW Rev = %d  SW Rev = %d -- ", 
					hw[numslics], sw[numslics]);
			if (ask) {
				printf ("current revisions incorrect\n");
				getrevisions(&hw[numslics], &sw[numslics],
							memtype[numslics]);
			} else {
				/*
			 	* Allow user to change current rev numbers.
			 	*/
				input = prompt("OK? (default 'y') ");
				if (*input != 'y' && *input != '\0')
					getrevisions(&hw[numslics], &sw[numslics],
							memtype[numslics]);
			}
			break;

		case UNKN:
			/*
			 * Uninitialized config area, so must get
			 * revision numbers from user.
			 */
			memtype[numslics] = zdcmem_type(slicaddr[numslics]);
			getrevisions(&hw[numslics], &sw[numslics], 
					memtype[numslics]);
			break;
		}
		numslics++;
	}  /* end while */

	/*
	 * Get filename containing new microcode.
	 */
	for (;;) {
		input = prompt("File? ");
		cc = getfile();
		if (cc < 0)
			continue;
		break;
	}

	/*
	 * Now stuff zdc's EEPROM
	 */
	for (i = 0; i < numslics; i++) {
		if (memtype[i] == EEPROM)
			printf("Loading EEPROM at slic address %d\n",
						slicaddr[i]);
		else
			printf("Loading SRAM at slic address %d\n",
						slicaddr[i]);
		/*
		 * Update hardware and software revision numbers and 
		 * associated checksums.
		 */
		ucodebuf[SL_G_HGENERATION] = hw[i];
		ucodebuf[SL_G_SGENERATION] = sw[i];
		/* calculate config prom checksum */
		chksum = checksum(ucodebuf, CFGCHK_SIZE, TWOS);
		ucodebuf[SL_G_CHKSUM] = chksum;
		ucodebuf[CHKSUM_ADDR] = 0;
		/* calculate total checksum */
		chksum = checksum(ucodebuf, UBUFSIZE, ONES); 
		ucodebuf[CHKSUM_ADDR] = chksum;
		/*
		 * Now load chip.
		 */
		eeload((u_char)slicaddr[i], ucodebuf, cc, memtype[i]);
	}

	if (numslics)
		printf("FIRMWARE loaded - Power Cycle System\n");
	else
		printf("No FIRMWARE loaded - Reset System\n");
	exit(0);
}

/*
 * find_zdc
 *	Find board at specified slic address and determine if it is a ZDC
 * Return
 *	ZDC if initialized ZDC board found
 *	UNKN if uninitialized ZDC board found 
 *	NOTFOUND if no board or some other board type found 
 */
static	int
find_zdc(slicaddr)
	int	slicaddr;	/* Slic address */
{
	register struct config_desc *cd = CD_LOC;
	register struct ctlr_desc *cp;
	register int i, n;

	/*
	 * Check ZDCs
	 */
	cp = &cd->c_ctlrs[cd->c_toc[SLB_ZDCBOARD].ct_start];
	n = cd->c_toc[SLB_ZDCBOARD].ct_count;
	for (i = 0; i < n; i++,cp++) {
		if (cp->cd_slic == slicaddr) {
			if (cp->cd_diag_flag & CFG_DECONF) {
				printf("ZDC at slic address %d bad? flags = 0x%x\n",
					slicaddr, cp->cd_diag_flag);
				/*
				 *  This may be an uninitialized SRAM, so
				 *  ask whether to continue.
				 */
				input = prompt("continue load? (default 'y') ");
				if (*input == 'y' || *input == '\0')
					return (UNKN);
				return (NOTFOUND);
			}
			/*
			 * Board found is an initialized ZDC
			 */
			return (ZDC);
		}
	}

	/*
	 * Now check other boards. 
	 */
	for (cp = cd->c_ctlrs; cp != cd->c_end_ctlrs; cp++) {
		if (cp->cd_slic == slicaddr) {
			if (cp->cd_ct->ct_flags & CTF_ISOEM) {
				if (cp->cd_diag_flag & CFG_DECONF) {
					printf("UNKN at slic address %d bad? flags = 0x%x\n",
						slicaddr, cp->cd_diag_flag);
					input = prompt("continue load? (default 'y') ");
					if (*input == 'y' || *input ==  '\0')
						return (UNKN);
					return (NOTFOUND);
				}
				/*
				 *  Assume OEM's are uninitialized EEPROMS
				 */  
				return (UNKN);
			} else if ( !(cp->cd_diag_flag & CFG_DECONF)) {
				/*
				 * If board type is NOT OEM and passed its
				 * tests, it is some other valid board so
				 * don't download on it!
				 */
				printf("slic address %d is not a ZDC\n",
						slicaddr);
				return (NOTFOUND);
			} else {
				/*
				 * This may be an uninitialized ZDC board or it
				 * may be some other board type which had an
				 * error. Make sure before downloading.
				 */
				printf("slic address %d may not be ZDC\n",
						slicaddr);
				input = prompt("continue load? (default 'y') ");
				if (*input == 'y' || *input == '\0')
					return (UNKN);
				return (NOTFOUND);
			}
		}
	}
	/*
	 * No board at all at slic address 
	 */
	printf("Could not find ZDC or UNKN at slic address %d\n", slicaddr);
	return (NOTFOUND);
}

/*
 * getrevisions
 * 	Prompt for hardware revision number and check
 *	for legal inputs.
 * Parameters:
 *	u_char *hw	- set to the new hardware rev number by getrevisions
 *	u_char *sw	- set to the new software rev number by getrevisions
 *	int type	- SRAM or EEPROM
 */

static
getrevisions(hw, sw, type)
	u_char *hw, *sw;
	int type;
{
	int i;

	for (;;) {
		input = prompt("Hardware Revision Number? ");
		*hw = atoi(input);
		for (i = 0; i < MAXHWREVS; i++) {
			if (revs[i].r_hw == *hw) {
				if (revs[i].r_type == type)
					*sw = revs[i].r_sw;
				else {
					printf ("Hardware Rev. does not match");
					printf (" board memory type\n");
					i = MAXHWREVS;
				}
				break;
			}
		}
		if (i == MAXHWREVS) {
			printf ("Invalid hardware revision number\n");
			continue;
		}
		printf("HW Rev = %d  SW Rev = %d -- ", *hw, *sw);
		input = prompt("OK? ");
		if (*input == 'y' || *input == '\0')
			break;
	}
}

/*
 * Read a file into the ucodebuf.
 * input string is passed via global "input" pointer.
 */
static int
getfile()
{
	register char	*sp;
	register int cc;
	int fd;

	/*
	 * Scan past white space.
	 */
	sp = input;
	for (; *sp && *sp == ' '; sp++)
		continue;
	fd = open(sp, 0);

	if (fd < 0) {
		printf("Open of %s failed\n", sp);
		return (-1);
	}
	/*
	 * read entire file into buffer.
	 */
	cc = read(fd, ucodebuf, UBUFSIZE);
	close(fd);

	if (cc != UBUFSIZE) {
		printf("File bad size = %d\n", cc);
		return (-1);
	}
	return (cc);
}

/*
 * load and verify the ucode into the ZDC's EEPROM or SRAM
 *
 * To write:
 *	For EEPROM, write the unlock sequence for Software Data Protection:
 *		Write Data 0xAA to Address 0x5555
 *		Write Data 0x55 to Address 0x2AAA
 *		Write Data 0xA0 to address 0x5555
 *	For EEPROM and SRAM:
 *	Write address bits 7-14 in SL_Z_EEBANK.
 *	Write data in SL_Z_EEWINDOW + address bits (6:0).
 *	Read from SL_Z_EEWINDOW + address bits (6:0) until matches data written.
 * To Verify:
 *	Write address bits 7-14 in SL_Z_EEBANK.
 *	read data in SL_Z_EEWINDOW + address bits (6:0).
 */
static
eeload(slic, buf, cc, type)
	u_char	slic;		/* ZDC slic address */
	u_char	*buf;		/* ucode */
	register int cc;	/* no. of bytes to load */
	int type;		/* SRAM or EEPROM */
{
	register int i;
	u_char	readval;

	/*
	 * Load the firmware.
	 */
	for (i = 0; i < cc; i++) {
		write_zdcmem(slic, &buf[i], i, type);
		while (rdslave(slic, SL_Z_EEWINDOW + (i & 0x7F)) != buf[i]) {
			continue;
		}
		/*
		 * Let 'em know something is happening...
		 */
		if (i != 0 && ((i & DOT) == 0)) {
			printf(".");
		}
	}
	printf(".\n");

	/*
	 * Verify the load.
	 */
	printf("Verifying load.\n");
	for (i = 0; i < cc; i++) {
		wrslave(slic, SL_Z_EEBANK, i >> 7);
		readval = rdslave(slic, SL_Z_EEWINDOW + (i & 0x7F));
		if (readval != buf[i]) {
			printf("Verify Failed - byte %d: ", i);
			printf("Expected 0x%x, Received 0x%x\n", buf[i],
				readval);
			exit(1);
		}
	}
}

/*
 * ZDWRSLAVE - macro used by write_zdcmem to write data to the EEPROM
 *	or SRAM of a ZDC controller
 */
#define ZDWRSLAVE(s,adr,data) { \
	(s)->sl_smessage = SL_Z_EEBANK; \
	(s)->sl_cmd_stat = SL_WRADDR; \
	while ((s)->sl_cmd_stat & SL_BUSY) \
		continue; \
	(s)->sl_smessage = (adr) >> 7; \
	(s)->sl_cmd_stat = SL_WRDATA; \
	while ((s)->sl_cmd_stat & SL_BUSY) \
		continue; \
	(s)->sl_smessage = SL_Z_EEWINDOW + ((adr) & 0x7f); \
	(s)->sl_cmd_stat = SL_WRADDR; \
	while ((s)->sl_cmd_stat & SL_BUSY) \
		continue; \
	(s)->sl_smessage = (data); \
	(s)->sl_cmd_stat = SL_WRDATA; \
	while ((s)->sl_cmd_stat & SL_BUSY) \
		continue; \
}

/*
 * write_zdcmem()
 *	write a byte of data to the ZDC EEPROM or SRAM
 * 
 * To write EEPROM:
 *	Write unlock sequence for Software Data Protection
 *		Write Data 0xAA to Address 0x5555
 *		Write Data 0x55 to Address 0x2AAA
 *		Write Data 0xA0 to address 0x5555
 * To write EEPROM or SRAM:
 *	Write address bits 7-14 in SL_Z_EEBANK.
 *	Write data in SL_Z_EEWINDOW + address bits (6:0).
 *
 * This routine inlines wrslave/wrAddr/wrData for speed.
 * Note: Error checking is NOT done.
 */
static
write_zdcmem(slic, data, addr, type)
	u_char	slic;		/* ZDC slic address */
	register u_char *data;	/* address of data byte */
	register int	addr;	/* EEPROM address */
	int type;		/* SRAM or EEPROM */
{
	register struct cpuslic *sl = (struct cpuslic *)LOAD_CPUSLICADDR;

	sl->sl_dest = slic;		/* Only need for 1st message */
	if (type == EEPROM) {
		/*
		 * Go through Byte Write Sequence with Software
		 * Data Protection Active.
		 *
 		 * Write Data 0xAA to Address 0x5555
		 */
		ZDWRSLAVE(sl, EEADDR0, EEDATA0);
	
		/*
		 * Write Data 0x55 to Address 0x2aaa
		 */
		ZDWRSLAVE(sl, EEADDR1, EEDATA1);

		/*
		 * Write Data 0xa0 to address 0x5555
		 */
	
		ZDWRSLAVE(sl, EEADDR2, EEDATA2);

		/*
		 * Now unlocked!
		 */
	}

	/*
	 * Write data
	 */
	ZDWRSLAVE(sl, addr, *data);
}

/*
 * read_zdcmem
 * 	read from specified address
 */
static u_char 
read_zdcmem(slic, addr)
	u_char slic;		/* ZDC slic address */
	register int addr;	/* EEPROM or SRAM memory address */
{
	wrslave(slic, SL_Z_EEBANK, addr >> 7);
	return (rdslave(slic, SL_Z_EEWINDOW + (addr & 0x7f)));
}

/*
 * zdcmem_type
 *	Do a test to determine whether this board has an EEPROM 
 *      or SRAM
 * Return Value
 *	EEPROM	- board has an EEPROM
 *	SRAM	- board has an SRAM
 */
zdcmem_type(slic)
	u_char slic;		/* ZDC slic address */
{
	u_char data = 0x89;	/* arbitrary test data */
	u_char save;
	u_char ee0, ee1, ee2;
	int type;
	int i;

	/*
	 * EEPROM must be initialized with lock sequence, so write
	 * data as if chip is EEPROM
	 */
	save = read_zdcmem(slic, 1);
	ee0 = read_zdcmem(slic, EEADDR0);
	ee1 = read_zdcmem(slic, EEADDR1);
	ee2 = read_zdcmem(slic, EEADDR2);

	write_zdcmem(slic, &data, 1, EEPROM);
	while (rdslave(slic, SL_Z_EEWINDOW + (1 & 0x7f)) != data)
		continue;
	data = ~data;
	/*
	 * Now write without lock sequence as though chip is SRAM.
	 * If data doesn't get there, this must be EEPROM.
	 */
	write_zdcmem(slic, &data, 1, SRAM);
	type = EEPROM;
	for (i = 0; i < 500; i++) { 
		if (rdslave(slic, SL_Z_EEWINDOW + (1 & 0x7f)) == data) 
			type = SRAM;
	}
	write_zdcmem(slic, &save, 1, type);
	while (rdslave(slic, SL_Z_EEWINDOW + (1 & 0x7f)) != save)
		continue;
	if (type == SRAM) {
		write_zdcmem(slic, &ee0, EEADDR0, type);
		while (rdslave(slic, SL_Z_EEWINDOW + (EEADDR0 & 0x7f)) != ee0)
			continue;
		write_zdcmem(slic, &ee1, EEADDR1, type);
		while (rdslave(slic, SL_Z_EEWINDOW + (EEADDR1 & 0x7f)) != ee1)
			continue;
		write_zdcmem(slic, &ee2, EEADDR2, type);
		while (rdslave(slic, SL_Z_EEWINDOW + (EEADDR2 & 0x7f)) != ee2)
			continue;
	}
	return (type);
}

/*
 * checksum
 *	calculate checksum according to the specified mode
 *	possible modes:
 *		1	- two's complement
 *		0	- one's complement Xor mode
 *
 * Return:
 *	checksum value
 */
checksum(start, sz, mode)
	u_char	*start;		/* pointer to start of data to checksum */
	int sz;			/* size in bytes of data to checksum */
	int mode;		/* checksum mode */
{
	register u_char sum = 0;
	register int	b;

	if (mode) {
		for (b = 0; b < sz; b++)
			sum += *start++;
		return (~sum + 1);	/* 2's compliment */
	} else {
		for (b = 0; b < sz; b++)
			sum ^= *start++;
		return (sum);		/* 1's Xor mode */
	}
}

