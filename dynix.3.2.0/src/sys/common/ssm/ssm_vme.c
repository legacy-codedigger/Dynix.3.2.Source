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
static char rcsid[]="$Header: ssm_vme.c 1.6 90/11/08 $";
#endif

/* $Log:	ssm_vme.c,v $
 */

#include "../h/types.h"
#include "../h/cmn_err.h"
#include "../machine/hwparam.h"
#include "../ssm/ioconf.h"
#include "../ssm/ssm_vme.h"
#include "../ssm/ssm.h"

#ifdef notyet
/*
 * ssm_vme_map_setup takes a sequent bus address and a length in bytes for a
 * dma transfer as well as some bus mapping information, and returns as a long
 * a vme bus address suitable for the dma transfer, or -1.  Note that because
 * the SSM has a 1Mbyte window from VME to Sequent bus, a -1 address is only
 * valid in A32 when the window is in the highest possible location.  We
 * disallow this for software ease.  The window is determined by the addresses
 * to which the SSM's I/O PIC is programmed to respond, and the comparator
 * value previously programmed.  The comparator value is read out of the SSM,
 * and the variable ssm has the base address of the SSM (or any address to
 * which the SSM's I/O PIC will respond.  From that, we can calculate the start
 * of the 1Mbyte window.
 *
 * Space is the 6 bit code that the VMEbus uses to designate an address space.
 * First_map is an index into the ssm's mapping registers of the first map that
 * can be used for this dma setup.  N_maps is the number of maps that can be
 * used.  Both first_map and n_maps are updated to reflect what was used, so
 * that successive calls to ssm_vme_map_setup may use the same variables until
 * the maps run out.    Also, length is updated--in case there are not enough
 * maps to map the entire transfer, at least you can still transfer SOMETHING!
 * Note that the index is per address space--there are 32 maps for A16, 512 for
 * A24, and 1024 for A32. However, the A32 space is divided into two 512 index
 * chunks.  These chunks are NOT contiguous unless the 1 Mbyte window has been
 * placed at VME address 7ff00000. Unless it becomes a problem, it will be
 * treated like two 512 index chunks.
 *
 */
long
ssm_vme_map_setup( addr, p_length, space, p_first_map, p_n_maps, ssm)
	caddr_t addr, ssm;
	long *p_length;
	int space, *p_first_map, *p_n_maps;
{
	int map_set = 0, map_limit = 0, maps_needed = 0, index, i;
	long  addr_mask, window_base, map_base;
	unsigned long msb = 0;
    
	ssm = (caddr_t )((long)ssm & SSM_VME_PIC_MASK);

	/* window base is read out of the comparator */
	window_base = *(unsigned long *)((long)ssm) & SSM_VME_COMP_MASK;
    
	if ( *p_first_map < 0  || *p_n_maps <= 0  || *p_length <= 0) return -1;
    
	switch( space)
	{
	case 0x9:
	case 0xa:
	case 0xb:
	case 0xd:
	case 0xe:
	case 0xf:
		/* extended address (A32) */
		if ( *p_first_map < SSM_VME_NMAPS ) 
		{
			map_set = 2;
			map_limit = SSM_VME_NMAPS;
		} else
		{
			map_set = 3;
			map_limit = 2 * SSM_VME_NMAPS;
			msb = 1<<31;
		}
		addr_mask = 0xffffffff;
		break;
	case 0x29:
	case 0x2d:
		/* short addresses (A16) */
		map_set = 0;
		map_limit = SSM_VME_A16_MAPPINGS;
		addr_mask = 0x0000ffff;
		break;
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3d:
	case 0x3e:
	case 0x3f:
		/* standard addresses (A24) */
		map_set = 1;
		map_limit = SSM_VME_NMAPS;
		addr_mask = 0x00ffffff;
		break;
	default:
		/* reserved */
		return -1;
		/*NOTREACHED*/
		/* break; */
	}

	if ( *p_first_map >= map_limit ) return -1;
	if ( *p_first_map + *p_n_maps > map_limit )
		*p_n_maps = map_limit - *p_first_map;
	
	maps_needed = ((long)addr + *p_length -((long)addr
						& SSM_VME_MAP_ADDR_MASK)
		       + SSM_VME_BYTES_PER_MAP - 1)/SSM_VME_BYTES_PER_MAP;
	if ( maps_needed > *p_n_maps ) 
	{
		*p_length -= (maps_needed - *p_n_maps) * SSM_VME_BYTES_PER_MAP;
		maps_needed = *p_n_maps;
	}
    
	*p_n_maps -= maps_needed;
	map_base = *p_first_map << 11;	  /* Get this before its incremented */
	for ( index = 0; index < maps_needed; index++)
	{
		/*
		 * A16 needs special treatment.  For each mapping, there are 16
		 * registers that need to have the same value in them.  The
		 * first will be done below, the rest are done here. 
		 */
		if ( map_set == 0 )
			for( i = 1; i < 16; i++)
				*(long *)((long)ssm | 1<<16 | map_set<<14
					  | i<<7  | *p_first_map<<2)
					= ((long)addr & SSM_VME_MAP_ADDR_MASK)
						+ (index<<11)
					   | 0xf<<7 | SSM_MAP_HIT;

		*(long *)((long)ssm | 1<<16 | map_set<<14
			  | (*p_first_map)++<<2)
			= ((long)addr & SSM_VME_MAP_ADDR_MASK) + (index<<11)
				| 0xf<<7 | SSM_MAP_HIT;
	}
	return ( msb | window_base | map_base
		| (long)addr & SSM_VME_MAP_SIZE-1) & addr_mask;
}
#endif notyet

/*
 * ssm_set_pic_flush_address( ssm )
 *  Given a ptr to the ssm structure
 *  setup for the memory method of flushing the PIC on the SSM2
 *  use the s2v and v2s current map values, allocate a map from
 *  each and initialize the maps for loop-back accessing.
 * 
 *  In this manner, reading/writting the sequent address will cause
 *  the VME to respond and then read/write sequent memory. In so 
 *  doing the PIC will flush its prior values and respond to the current
 *  requests - thus the PIC will be flushed
 */
ssm_set_pic_flush_address(ssm)
struct ssm_desc *ssm;
{
	int ssm_index;
	long sqnt_mem;
	int v2s_map, i, s2v_map;
	ulong phy_ssm;
	caddr_t calloc();

	ssm_index = ssm - SSM_desc;
	phy_ssm = PA_SSMVME( ssm_index );

	s2v_map = ssm->ssm_s2v_map++;
	
	ssm->pic_flush_memory = 
		(long*) ssm_s2v_map(	ssm_index, 
					s2v_map,
					(u_long) 0x0,	/* VME address */
					0x29);		/* A16 space */
		
	callocrnd(2048);		/* needs to be on 2k boundary */
	sqnt_mem = (long) calloc( 64 );	/* just 64 bytes : phy == virt still */

	/*
 	 * now get next v2s A16 maps, need 16 to cover the floating lines
	 */
	v2s_map = ssm->ssm_v2s_map[ SSM_VME_A16_MAPS ];
	ssm->ssm_v2s_map[ SSM_VME_A16_MAPS ] += 16;	

	for( i=0; i < 16; i++ ) {
		*(ulong *)(phy_ssm | 1<<16 | i<<7 | v2s_map<<2 )
			= (sqnt_mem | 0xF<<7 | SSM_MAP_HIT);
	}
}

/*
 * ssm_allocate_maps()
 * Given a pointer to an ssm_dev, allocate the dma maps it needs from its ssm.
 * In addition to the fields filled out by the config utility, we expect that
 * sdv_desc and sdv_ssm_idx are accurate.  Returns 1 on success, 0 on failure.
 * Since failures are serious (but not fatal) conditions, this routine prints
 * out an appropriate error message.
 */
long
ssm_allocate_maps( dev )
	struct ssm_dev *dev;
{
	int mapset;
	int nmaps = dev->sdv_maps_req;
	struct ssm_desc *ssm = dev->sdv_desc;
	

	switch( dev->sdv_map_space & 0x30 ) {
		/* Look at the addr modifier to determine which maps to use */
	case 0x0:
		/* A32.  Try both spaces, MSB=1 first */
		mapset = 3;
		if ( nmaps + ssm->ssm_v2s_map[mapset]  > SSM_VME_NMAPS )
			mapset = 2;
		break;
	case 0x10:
		/* User defined.  We don't define any, so can't get any maps */
		if ( nmaps > 0 ) {
			printf ( "ssm%d: Bad VME address modifier = %x.\n",
				dev->sdv_ssm_idx, dev->sdv_map_space);
			return 0;
		}
		break;
	case 0x20:
		/*
		 * A16.  Need 16 maps per mapping. Note that everywhere else we
		 * are counting in terms of mappings, so we only use nmaps in
		 * the limit testing.  We use dev->sdv_maps_req for the number
		 * of mappings everywhere else
		 */
		nmaps *= 16;
		mapset = 0;
		break;
	case 0x30:
		/* A24 */
		mapset = 1;
		break;
	}

	if ( nmaps + ssm->ssm_v2s_map[mapset]  > SSM_VME_NMAPS ) {
		CPRINTF("ssm%d: out of A%d DMA maps.\n", dev->sdv_ssm_idx,
		       16 + 8*mapset);
		return(0);
	} else {
		dev->sdv_maps_avail = dev->sdv_maps_req;

		/*
		 * The address of the first map is formed with the I/O address
		 * of the ssm, 1<<16 gives access to the vme to sequent map
		 * RAMs , mapset<<14 gives access to the right map set
		 * (0 is A16, 1 is A24, 2 is A32 with the MSB of the address
		 * off, and 3 is A32 MSB on.) and the number of the map is
		 * shifted for long word access.
		 */
		dev->sdv_maps = (caddr_t) (PA_SSMVME( dev->sdv_ssm_idx)
					   | 1<<16  | mapset<<14
					   | ssm->ssm_v2s_map[mapset]<<2);
		ssm->ssm_v2s_map[mapset] += dev->sdv_maps_avail;
		return(1);
	}
}

/*
 * ssm_s2v_map
 * Given the I/O base of an SSM, the number of the map to use, the vme address
 * and the vme address modifier, sets up the map and returns the sequent
 * address to use to access the vme address.  Assumes ssm_d, map and address
 * are valid, masks the admod to six bits.
 */
long
ssm_s2v_map( ssm_index, map, address, admod)
	int ssm_index, map;
	u_long address;
	u_char admod;
{
	long sqnt_addr;
	long ssm;

	ssm = PA_SSMVME( ssm_index );
	admod &= 0x3f;
	
	/*
	 * The expression for the map uses the I/O address of the ssm, 2<<16
	 * which addresses the sequent to vme bus map rams and the number of
	 * the map, shifted over for each map takes a long word.
	 *
	 * The value programmed is the upper portion of the VME address, the
	 * VME address modifier (6 bit) shifted over, 0xf<<4 which need to be
	 * ones in order to parity protect the maps and the hit bit (1), which
	 * says this map is valid.
	 */
	
	*(long *)(ssm | 2<<16 | map<<2)
		= address & SSM_S2V_MAP_MASK    | admod<<8 | 0xf<<4 | 1;
	
	/*
	 * Calculate the sequent address to access the vme address just mapped.
	 * Note that the address bits from "address" that aren't in the map are
	 * in this address.  The sequent address is the SSM's base I/O address
	 * with the map number shifted over and the leftover bits from the VME
	 * address.
	 */
	sqnt_addr = ssm | map<<14 |    address & ~SSM_S2V_MAP_MASK;

	return(sqnt_addr);
}

#ifdef notyet
/*
 * ssm_vme_pic_flush()
 * Flush the PIC buffers given the 7 MSB of the SSM address. No need to lock.
 */
void
ssm_vme_pic_flush(ssm)
	caddr_t ssm;
{
	int i;
	static unsigned long reminder = 0;		  /* debug */
	
	PIC_REGISTER( ssm, PIC_BCR) = PIC_BCR_FLUSH;

	for ( i = 1; ; i++ ) {
		if ( !(PIC_REGISTER( ssm, PIC_BCR) & PIC_BCR_FLUSH) )
			return;		  /* flushed! */


		if ( i%PIC_FLUSH_LOOPS == 0 ) {
			if ( ! reminder++ ) /* debug */
			printf( "SSM VME PIC at 0x%x won't flush!\n", /*debug*/
			       (long) ssm); /* debug */
/* not yet
			panic( "SSM VME PIC unflushable");
not yet */
			break;		  /* in the mean while */
		}
	}
}
#endif notyet
