/*	$Id: Dx86Nub.h,v 1.2 1999/02/03 08:30:06 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 14:09:54
*/

#ifndef DX86NUB_H
#define DX86NUB_H

#include "DNub.h"

class Dx86Nub : virtual public DNub
{
  public:
  	Dx86Nub();

	virtual void GetStackCrawl(DCpuState& frame, DStackCrawl& outCrawl);
};

#endif
