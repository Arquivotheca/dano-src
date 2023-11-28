/*	$Id: DStatement.h,v 1.2 1999/05/11 21:31:06 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/04/98 21:44:04
*/

#ifndef DSTATEMENT_H
#define DSTATEMENT_H

#include "DSourceFileTable.h"

struct DStatement
{
	DFileNr fFile;
	int fLine;
	ptr_t fPC;
	int fSize;
	bool fSourceLevel;
	DStatement()				{ Reset(); };
	void Reset();
};

extern DStatement nullStatement;

inline int operator!=(DStatement& a, DStatement& b)
{
	return (a.fPC != b.fPC); // || (a.fSize != b.fSize) || (a.fLine != b.fLine) || (a.fFile != b.fFile);
} /* operator != */

#endif
