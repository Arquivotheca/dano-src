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
		case 0x00:	fColorTable = gTabColor_720x720_00_plain;		break;
		case 0x11:	fColorTable = gTabColor_1440x720_00_archival;	break;
		case 0x21:	fColorTable = gTabColor_1440x720_10_archival;	break;
		case 0x12:	fColorTable = gTabColor_1440x720_00_semigloss;	break;
		case 0x22:	fColorTable = gTabColor_1440x720_10_semigloss;	break;
		case 0x32:	fColorTable = gTabColor_2880x720_10_semigloss;	break;
		case 0x13:	fColorTable = gTabColor_1440x720_00_glossy;		break;
		case 0x23:	fColorTable = gTabColor_1440x720_10_glossy;		break;
		case 0x33:	fColorTable = gTabColor_2880x720_10_glossy;		break;
	}
}

MColorProcess::~MColorProcess(void)
{
}


