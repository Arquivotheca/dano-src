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
	if (printer_def->color_mode == 6)
	{
		switch ((printer_def->res*16) + printer_def->paper)
		{
			case 0x10:	fColorTable = gTabColor_720x360_03_plain;		break;
			case 0x30:	fColorTable = gTabColor_720x720_10_plain;		break;
			case 0x40:	fColorTable = gTabColor_1440x720_10_plain;		break;

			case 0x21:	fColorTable = gTabColor_720x720_03_inkjet;		break;
			case 0x31:	fColorTable = gTabColor_720x720_10_inkjet;		break;
			case 0x41:	fColorTable = gTabColor_1440x720_10_inkjet;		break;

			case 0x22:	fColorTable = gTabColor_720x720_03_photo;			break;
			case 0x32:	fColorTable = gTabColor_720x720_10_photo;			break;
			case 0x42:	fColorTable = gTabColor_1440x720_10_photo;			break;
			
			case 0x33:	fColorTable = gTabColor_720x720_10_glossy;			break;
			case 0x43:	fColorTable = gTabColor_1440x720_10_film;			break;
			
			case 0x15:	fColorTable = gTabColor_720x360_03_inkjet360;		break;
			case 0x04:	fColorTable = gTabColor_360x360_04_transparency;	break;
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
