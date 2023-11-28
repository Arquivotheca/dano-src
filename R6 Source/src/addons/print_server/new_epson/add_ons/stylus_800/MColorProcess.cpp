//*******************************************************************************************
// Project: Driver EPSON Stylus
// File: MColorProcess.cpp
//
// Purpose: Partie dependante de l'imprimante
//
// Author: M. AGOPIAN
// Last Modifications:
// ___________________________________________________________________________________________
// Author		| Date		| Purpose
// ____________	|__________	|_________________________________________________________________
// M.AG.		| 01/11/97	| Debut du projet
// ____________	|__________	|_________________________________________________________________
//********************************************************************************************

#include "MColorProcess.h"
#include "DriverTypes.h"
#include "MDefinePrinter.h"
#include "ColorType.h"

// -----------------------------------------------------
// class name: MColorProcess
// purpose: Color processing
// -----------------------------------------------------

MColorProcess::MColorProcess(tPrinterDef *printer_def)
{
	if (printer_def->color_mode == 4)
	{
		switch ((printer_def->res*16) + printer_def->paper)
		{
			case 0x00:	fColorTable = gTabColor_360x360_regular_color;			break;	// OK
			case 0x10:	fColorTable = gTabColor_360x360_regular_color;			break;	// OK
			case 0x20:	fColorTable = gTabColor_720x720_regular_color;			break;	// OK
			case 0x31:	fColorTable = gTabColor_720x360_coated_color;			break;	// OK
			case 0x22:	fColorTable = gTabColor_720x720_coated_color;			break;	// OK
			case 0x42:	fColorTable = gTabColor_1440x720_coated_color;			break;	// OK
			case 0x23:	fColorTable = gTabColor_720x720_photo_color;			break;	// OK
			case 0x43:	fColorTable = gTabColor_1440x720_photo_color;			break;	// OK
			case 0x24:	fColorTable = gTabColor_720x720_film_color;				break;	// OK
			case 0x44:	fColorTable = gTabColor_1440x720_film_color;			break;	// OK
			case 0x15:	fColorTable = gTabColor_360x360_transparency_color;		break;	// OK
			default:
				fColorTable = gTabColor_720x720_coated_black;
		}
	}
	else
	{
		fColorTable = gTabColor_720x720_coated_black;
	}
}

MColorProcess::~MColorProcess(void)
{
}
