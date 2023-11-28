/*
 * ORION.c
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila, Robert Eggers, Mike Dewsnap
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#include "syshead.h"

#include "config.h"
#include "dtypes.h"
#include "tsimem.h"
#include "t2kstrm.h"
#include "truetype.h"

#include "scoder.h"
#include "orion.h"

#ifdef ENABLE_ORION
/*
 * The standard OrionModel constructor.
 */
OrionModelClass *New_OrionModelClassFromStream( tsiMemObject *mem, InputStream *in )
{
	int i, j;
	unsigned char major, minor;
	
	OrionModelClass *t	= (OrionModelClass *) tsi_AllocMem( mem, sizeof( OrionModelClass ) );
	t->mem				= mem;

	major			= ReadUnsignedByteMacro( in );
	minor			= ReadUnsignedByteMacro( in );
	t->num_eb1		= ReadUnsignedByteMacro( in );
	t->num_e		= ReadUnsignedByteMacro( in );
	
	t->num_eb2		= t->num_e - t->num_eb1 - 1;
	
	assert( major 		== ORION_MAJOR_VERSION );
	assert( minor 		== ORION_MINOR_VERSION );
	/*
	assert( t->num_eb1	== NUM_ECOPYX );
	assert( t->num_e		== NUM_EX );
	*/
	
	t->copy 	= (SCODER **)tsi_AllocMem( mem, t->num_eb1 * sizeof( SCODER * ) );
	t->literal	= (SCODER **)tsi_AllocMem( mem, t->num_e * sizeof( SCODER * ) );
	
	t->control	= New_SCODER_FromStream( mem, in );
	t->ep 		= New_SCODER_FromStream( mem, in );
	for ( i = 0; i < t->num_eb1; i++ ) {
		 t->copy[i] = New_SCODER_FromStream( mem, in );
	}
	for ( i = 0; i < t->num_e; i++ ) {
		t->literal[i] = New_SCODER_FromStream( mem, in );
	}
	t->dx	    = (short *)tsi_AllocMem( mem, t->num_eb1 * 256 * sizeof(short) ); 
	t->dy	    = (short *)tsi_AllocMem( mem, t->num_eb1 * 256 * sizeof(short) ); 
	t->onCurve	= (char *) tsi_AllocMem( mem, t->num_eb1 * 256 * sizeof(char)  ); 
	
	
	j = t->num_eb1 * 256;
	for ( i = 0; i < j; i++ ) {
		t->onCurve[i] = (char)ReadDeltaXYValue( in, &(t->dx[i]), &(t->dy[i]) );
	}
	
	return t; /*****/
}



/*
 * Updates the orion state.
 */
void Set_OrionState( OrionModelClass *t, int dx, int dy, char onCurve )
{
	int adx, ady, OrionState;
	
	adx = dx; ady = dy;
	if ( adx < 0 ) adx = -adx;
	if ( ady < 0 ) ady = -ady;
	if ( adx > ady ) {
		OrionState = dx >= 0 ? 0 : 4;
	} else {
		OrionState = dy >= 0 ? 2 : 6;
	}
	if ( !onCurve ) OrionState |= 1;
	OrionState = OrionState % t->num_eb1;
	assert( OrionState < t->num_eb1 );
	t->OrionState = OrionState;
}

/*
 * The Destructor.
 */
void Delete_OrionModelClass( OrionModelClass *t )
{
	if ( t != NULL ) {
		int i;
		
		for ( i = 0; i < t->num_e; i++ ) {
			Delete_SCODER( t->literal[i] );
		}
		
		for ( i = 0; i < t->num_eb1; i++ ) {
			Delete_SCODER( t->copy[i] );
		}
		tsi_DeAllocMem( t->mem, t->literal );
		tsi_DeAllocMem( t->mem, t->copy );
		Delete_SCODER( t->control );
		Delete_SCODER( t->ep );
		tsi_DeAllocMem( t->mem, t->dx );
		tsi_DeAllocMem( t->mem, t->dy );
		tsi_DeAllocMem( t->mem, t->onCurve );
		tsi_DeAllocMem( t->mem, t );
	}
}

#endif /*  ENABLE_ORION from top of the file */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/orion.c 1.4 1999/10/18 17:00:18 jfatal release $
 *                                                                           *
 *     $Log: orion.c $
 *     Revision 1.4  1999/10/18 17:00:18  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:11:32  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:57:07  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

