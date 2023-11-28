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
			case 0x00:	fColorTable = gTabColor_360x360_2_plain;		break;
			case 0x20:	fColorTable = gTabColor_360x720_2_plain;		break;
			case 0x30:	fColorTable = gTabColor_720x720_1_plain;		break;
			case 0x01:	fColorTable = gTabColor_360x360_2_inkjet;		break;
			case 0x22:	fColorTable = gTabColor_360x720_2_inkjet;		break;
			case 0x32:	fColorTable = gTabColor_720x720_1_inkjet;		break;
			case 0x23:	fColorTable = gTabColor_360x720_2_photo;		break;
			case 0x33:	fColorTable = gTabColor_720x720_1_photo;		break;
			case 0x44:	fColorTable = gTabColor_1440x720_0_glossy;		break;
			case 0x05:	fColorTable = gTabColor_360x360_2_transparency;		break;
			default:
				fColorTable = gTabColor_360x360_2_plain;
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
