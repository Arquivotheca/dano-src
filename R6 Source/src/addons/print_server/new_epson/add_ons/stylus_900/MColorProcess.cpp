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
			case 0x00:
				if (printer_def->microdot == 1)
					fColorTable = gTabColor_360x360_01_plain;
				else
					fColorTable = gTabColor_360x360_11_plain;
				break;
			case 0x10:	fColorTable = gTabColor_720x720_10_plain;			break;
			case 0x01:	fColorTable = gTabColor_360x360_11_inkjet360;		break;
			case 0x22:	fColorTable = gTabColor_360x720_11_inkjet;			break;
			case 0x12:	fColorTable = gTabColor_720x720_10_inkjet;			break;
			case 0x23:	fColorTable = gTabColor_360x720_11_photo;			break;
			case 0x13:	fColorTable = gTabColor_720x720_10_photo;			break;
			case 0x33:	fColorTable = gTabColor_1440x720_10_photo;			break;
			case 0x34:	fColorTable = gTabColor_1440x720_10_glossy;			break;
			case 0x05:	fColorTable = gTabColor_360x360_01_transparency;	break;
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
