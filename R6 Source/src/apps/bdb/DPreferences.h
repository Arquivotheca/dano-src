/*	$Id: DPreferences.h,v 1.1 1998/12/03 10:34:48 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 11/24/98 11:06:43
*/

#ifndef DPREFERENCES_H
#define DPREFERENCES_H

class DPreferences : public HDialog
{
  public:
	DPreferences();

	enum { sResID = 130 };

	static DPreferences* Instance();
	
	virtual bool OKClicked();
};

#endif
