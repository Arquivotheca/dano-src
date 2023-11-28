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
	switch ((printer_def->res*16) + printer_def->paper)
	{
		case 0x00:	fColorTable = gTabColor_360x360_12_plain;		break;
		case 0x20:	fColorTable = gTabColor_720x720_11_plain;		break;
		case 0x01:	fColorTable = gTabColor_360x360_12_inkjet;		break;
		case 0x12:	fColorTable = gTabColor_360x720_12_inkjet;		break;
		case 0x22:	fColorTable = gTabColor_720x720_11_inkjet;		break;
		case 0x13:	fColorTable = gTabColor_360x720_12_matte;		break;
		case 0x23:	fColorTable = gTabColor_720x720_11_matte;		break;
		case 0x14:	fColorTable = gTabColor_360x720_12_photo;		break;
		case 0x24:	fColorTable = gTabColor_720x720_11_photo;		break;
		case 0x35:	fColorTable = gTabColor_1440x720_11_film;		break;
		case 0x06:	fColorTable = gTabColor_360x360_12_transparency;	break;
	}
}

MColorProcess::~MColorProcess(void)
{
}
