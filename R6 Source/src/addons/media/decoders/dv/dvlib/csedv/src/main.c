//==============================================================================
//  JPEG file Decoder for PowerView
//
//  Copyright(C) 1991-1997 CANOPUS Co.,Ltd.	<<<<< Don't change copyright!
//  Tom created	 1991.10.20			<<<<< Don't change originality!
//  Tom revised	 1996. 9.30	for PowerView95
//  Tom revised	 1996.12.19	support progressive JPEG and CMYK
//
//  <!!! WARNING !!!>
//  Don't change original comments. It's the morality for programmer.
//  Don't modify a lot. You can modify minimum.
//
//==============================================================================
#include <windows.h>
#include "dv.h"

//------------------------------------------------------------------------------
// Externals
//------------------------------------------------------------------------------

extern	void	InitDecParam( void );
extern	void	InitEncParam( void );

//------------------------------------------------------------------------------
//  DllMain (hModule,cbHeap,lpchCmdLine) Called when the libary is loaded.
//------------------------------------------------------------------------------

BOOL APIENTRY DllMain( PVOID hModule, ULONG ulReason, PCONTEXT pctx )
{
//	hInstance = hModule;					// need!
	if( ulReason == DLL_PROCESS_ATTACH )
	{
		InitDecParam();
		InitEncParam();
	}
	else if( ulReason == DLL_PROCESS_DETACH )
	{
	}
	return TRUE;
}

