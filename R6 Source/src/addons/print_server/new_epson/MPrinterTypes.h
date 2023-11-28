//*******************************************************************************************
// Project: Driver EPSON Stylus
// File: MPrinterTypes.h
//
// Purpose: Contient les defines dependant de l'imrpimante
//
// Author: M. AGOPIAN
// Last Modifications:
// ___________________________________________________________________________________________
// Author		| Date		| Purpose
// ____________	|__________	|_________________________________________________________________
// M.AG.		| 01/11/97	| Debut du projet
// ____________	|__________	|_________________________________________________________________
//********************************************************************************************

#ifndef MPRINTER_TYPES_H
#define MPRINTER_TYPES_H


// Defined classes
class MPrinter;

// Size of a normal dot, used as unit for the tables (inch)
#define DOT_SIZE_UNIT				(1.0/360.0)

// t_printer_color: couleur du raster
typedef enum
{
	M_BLACK_COLOR			=	0,
	M_MAGENTA_COLOR			=	1,
	M_CYAN_COLOR			=	2,
	M_YELLOW_COLOR			=	4,
	M_LIGHT_MAGENTA_COLOR	=	0x101,
	M_LIGHT_CYAN_COLOR		=	0x102
}t_printer_color;

// Definit le numero de version minimum des fichiers d'imprimantes
// VERSION_MIN: pas d'importance (changement de la valeur des parametres de l'imprimante)
// VERSION_MID: celui de l'imprimante doit etre == a celui-ci
// VERSION_MAJ: celui de l'imprimante doit etre == a celui-ci

#define	VERSION_MAJ		5
#define VERSION_MID		8
#define VERSION_MIN		0

#endif
