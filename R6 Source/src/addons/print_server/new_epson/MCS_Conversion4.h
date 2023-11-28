//*******************************************************************************************
// Project: Driver EPSON Stylus
// File: MCS_Conversion4.h
//
// Purpose: Conversion RGB->CMYK
//
// Author: M. AGOPIAN
// Last Modifications:
// ___________________________________________________________________________________________
// Author		| Date		| Purpose
// ____________	|__________	|_________________________________________________________________
// M.AG.		| 06/24/99	| Debut du projet
// ____________	|__________	|_________________________________________________________________
//********************************************************************************************

#ifndef SPACE_CONVERSION4_H
#define SPACE_CONVERSION4_H

#include <BeBuild.h>
#include <ByteOrder.h>
#include <SupportDefs.h>

#include "MCS_Conversion.h"

class MCS_Conversion4 : public MCS_Conversion
{
public:
			MCS_Conversion4(const void *tab);
	virtual	~MCS_Conversion4(void);
	
	virtual bool SpaceConversion(uint32 rgb, int16 *cmyk_i);

protected:
	typedef struct
	{
		uint8 c;
		uint8 m;
		uint8 y;
		uint8 k;
	} epson_color_t;
	typedef epson_color_t (*color_t)[8][8];
	color_t fTabColor;

private:
	inline bool SpaceConversion_cpp(uint32 rgb, int16 *cmyk_i);
	inline bool SpaceConversion_mmx(uint32 rgb, int16 *cmyk_i);
};


#endif
