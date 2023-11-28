#if DEBUG_DISPLAY
#include <stdio.h>
#endif

#include "fxconfig.h"
#include "fxresman.h"
#include "fxparman.h"
#include "fxpgmman.h"
#include "fxpatman.h"
#include "fxcbkman.h"

int fxAllocedChips = 0;
ULONG fx8210Mutex;

void	
fxSystemInitialize()
{
	fxRsrcInitialize( );
	fxPatchInitialize( );
	fxCallbackInitialize( );
	fxPgmInitialize( );
	fxParamInitialize( );
	OS_CREATEMUTEX( &fx8210Mutex );
}

FXSTATUS
fxSystemDiscoverChip( char *zChipRev, ULONG ulRevReg, ULONG ulBaseAddr, ULONG ulXTRAM, 
					  ULONG ulChipHandle, FXID *chipID )
{
	FXSTATUS status;
	ULONG	 ulTemp=0x2000;
	ULONG	 ulSize=0;

	/* Round up XTRAM size to nearest multiple of 16Kb (4Kw) */
	for( ; ulSize<7 && ulTemp<ulXTRAM; ulSize++, ulTemp<<=1 );

	status = fxRsrcInitChip( chipID, zChipRev, ulRevReg, ulChipHandle, 
							 ulXTRAM );
	if( status != FXERROR_NO_ERROR ) return status;

	OS_WRITEBASEADDRREG( ulChipHandle, ulBaseAddr );
	OS_WRITEXTRAMSIZE( ulChipHandle, ulSize );

	status = fxCallbackInitChip( ulChipHandle );
	if( status != FXERROR_NO_ERROR ) return status;

	status = fxParamInitChip( );
	if( status != FXERROR_NO_ERROR ) return status;

	status = fxPatchInitChip( );
	if( status != FXERROR_NO_ERROR ) return status;

	status = fxPgmInitChip( *chipID, ulChipHandle );

	/* Start the FX engine */
	OS_STOPSINGLESTEP( ulChipHandle );

	/* Start the XTRAM engine */
	OS_STARTXTRAM( ulChipHandle );

	fxAllocedChips++;

	return status;
}

FXSTATUS
fxSystemUndiscoverChip( FXID chipID, ULONG ulChipHandle )
{
	FXSTATUS status;

	if( !fxRsrcValidChipID(chipID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	/* Stop the FX engine */
	OS_STARTSINGLESTEP( ulChipHandle );

	/* Stop the XTRAM engine */
	OS_STOPXTRAM( ulChipHandle );

	status = fxPgmFreeChip(chipID);
	if( status == FXERROR_NO_ERROR ) {
		status = fxCallbackFreeChip(ulChipHandle);
	}
	if( status == FXERROR_NO_ERROR ) {
		status = fxRsrcDestroy(chipID);
	}
	if( status == FXERROR_NO_ERROR ) {
		status = fxRsrcFreeChip(chipID);
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return status;
}

/************************* DEBUG DEBUG DEBUG *************************/
#if DEBUG_DISPLAY
static char *op[] = { "MAC", "MACM", "MAD", "MADM", "MACINT", "MADINT",
			   "ACC3", "MACMV", "ANDXOR", "TSTNEG", "LOLIMIT",
			   "HILIMIT", "LOG", "EXP", "INTERP", "SKIP" };

static char *conreg[] = {
	"ZERO", "x1", "x2", "x3", "x4", "x8", "x16", "x32", "x256", "x64K",
	"1LSTRAM", "1/8", "1/4", "1/2", "-1.0", "POSFULLSCALE", "-1LSB",
	"-2LSB", "NEGHALFSCALE", "OPTALLPASS", "-3dB", "Const0x55",

	"ACC", "CCReg", "DITHER1", "DITHER2", "INTERRUPT", "DELAYBASE",
	"TABLEBASE", "Reg0x5d", "Reg0x5e", "Reg0x5f" };

#define MOPCODE(i)    (((ulUCH)>>20)&0xf)
#define MOPERAND(i,j) ((j==0)?((ulUCH)&0x3ff)       : \
					   (j==1)?(((ulUCL)>>10)&0x3ff) : \
					   (j==2)?((ulUCL)&0x3ff)	    : \
						 	  (((ulUCH)>>10)&0x3ff))

static void disasm_line(int, ULONG);

/******************************* disasm **************************************
* 
* @doc
* @func (return-type) | disasm | 
* 
* (description)
* 
* @parm (type) | (name) | (description)
* 
* @rdesc (description of return codes)
* 
******************************************************************************/
void EMUAPIEXPORT
fxDisasm( ULONG ulChipHandle )
{
	int i,j;

	for( i=0; i < 0x200; i++ ) {

		disasm_line(i, ulChipHandle );
	}

	printf( "\n\nGPR Space:\n" );
	for( i = 0x100; i< 0x400; i+=4 ) {
		printf( "%03x: ", i );
		for( j=0; j<4; j++ ) {
			printf( "%08lx  ", OS_READGPR(ulChipHandle, i+j ) );
		}
		printf( "\n" );
	}

	printf( "\n\nFlags:\n" );
	for( i = 0x300; i< 0x3a0; i+=8 ) {
		printf( "%03x: ", i );
		for( j=0; j<8; j++ ) {
			printf( "%02x   ", OS_READFLAGS(ulChipHandle, i+j ) );
		}
		printf( "\n" );
	}

}

static void
disasm_line( int i, ULONG ulChipHandle )
{
	int j;

	ULONG ulUCL, ulUCH;

	OS_READINSTRUCTION( ulChipHandle, i, &ulUCL, &ulUCH );

	printf( "%03x\t", i );
	if( MOPCODE(i) == 0x0f ) {
		/* Is it a NOP? */
		if( MOPERAND(i,0) == MOPERAND(i,3) &&
			MOPERAND(i,2) == 0x40 ) {
			printf( "NOP\n" );
			return;
		}
	}

	printf( "%s\t", op[MOPCODE(i)] );

	for( j=0; j<4; j++ ) {
		if( MOPERAND(i,j) < 0x20 ) {
			printf( "INPUT%d", MOPERAND(i,j) );
		} else if( MOPERAND(i,j) < 0x40 ) {
			printf( "OUTPUT%d", MOPERAND(i,j)-0x20 );
		} else if( MOPERAND(i,j) < 0x60 ) {
			printf( "%s", conreg[MOPERAND(i,j)-0x40] );
		} else {
			printf( "GPR0x%03x", MOPERAND(i,j) );
		}
		if( j<2 ) printf( ", " );
		else if( j==2 ) printf( " -> " );
		else printf( "\n" );
	}
}
#else
/* if not debugging: this function that does nothing */
void EMUAPIEXPORT
fxDisasm( ULONG ulChipHandle )
{;}
#endif



