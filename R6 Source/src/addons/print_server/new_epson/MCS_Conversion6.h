//*******************************************************************************************
// Project: Driver EPSON Stylus
// File: MCS_Conversion6.h
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

#ifndef SPACE_CONVERSION6_H
#define SPACE_CONVERSION6_H

#include <BeBuild.h>
#include <ByteOrder.h>
#include <SupportDefs.h>

#include "MCS_Conversion.h"

class MCS_Conversion6 : public MCS_Conversion
{
public:
			MCS_Conversion6(const void *tab);
	virtual	~MCS_Conversion6(void);
	
	virtual bool SpaceConversion(uint32 rgb, int16 *cmyk_i);

protected:
	typedef struct
	{
		uint8 c;
		uint8 m;
		uint8 y;
		uint8 k;
		uint8 lc;
		uint8 lm;
	} epson_color_t;
	typedef epson_color_t (*color_t)[8][8];
	color_t fTabColor;
};

#endif
