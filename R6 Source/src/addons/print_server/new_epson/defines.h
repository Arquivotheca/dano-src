//*******************************************************************************************
// Project: Driver EPSON Stylus
// File: defines.h
//
// Purpose: Contient les definitions communes a tous les modules
//
// Author: M. AGOPIAN
// Last Modifications:
// ___________________________________________________________________________________________
// Author		| Date		| Purpose
// ____________	|__________	|_________________________________________________________________
// M.AG.		| 01/11/97	| Debut du projet
// ____________	|__________	|_________________________________________________________________
//********************************************************************************************

#ifndef DEFINES_H
#define DEFINES_H

// variables definies dans "Driver.cpp"
extern const char *M_XPRINTER_SIGNATURE;
extern const char *M_XPRINTER_ADDON_SIGNATURE;
extern const char *M_APPLICATION_SIGNATURE;
extern const char *M_PRINT_SERVER_SIGNATURE;
extern const char *M_PRINT_ADD_PRINTER_SIGNATURE;
extern const char *M_PRINT_SEL_PRINTER_SIGNATURE;
extern const char *M_TRACKER_SIGNATURE;
extern const char *M_DESKBAR_SIGNATURE;

enum
{
	M_MSG_OK		=	'GOOD',
	M_MSG_CANCEL	=	'CNCL'
};

extern const char *app_signature;
void GetRsrcRef(entry_ref *entry);

#endif
