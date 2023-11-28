/*	$Id: DShowAddress.h,v 1.2 1998/11/17 12:16:40 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 10/30/98 16:56:54
*/

#ifndef DSHOWADDRESS_H
#define DSHOWADDRESS_H

#include "HDialog.h"

class DTeam;

class DShowAddress : public HDialog
{
  public:
	DShowAddress(BRect frame, const char *name, window_type type, int flags,
		BWindow *owner, BPositionIO& data);
		
	enum { sResID = 128 };
	
	void SetTeam(DTeam *team);
	virtual bool OKClicked();

  private:
	DTeam *fTeam;
};

#endif
