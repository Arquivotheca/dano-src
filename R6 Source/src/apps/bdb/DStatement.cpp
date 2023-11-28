/*	$Id: DStatement.cpp,v 1.3 1999/05/11 21:31:05 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 16:20:17
*/

#include "bdb.h"
#include "DStatement.h"

DStatement nullStatement;

void DStatement::Reset()
{
	fFile = -1;
	fLine = 0;
	fPC = 0;
	fSize = 0;
	fSourceLevel = false;
} /* DStatement::Reset */
