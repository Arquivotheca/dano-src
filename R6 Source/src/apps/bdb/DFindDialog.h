
/*	$Id: DFindDialog.h,v 1.2 1999/01/05 22:09:17 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 11/22/98 13:33:00
*/

#ifndef DFINDDIALOG_H
#define DFINDDIALOG_H

#include "HDialog.h"

class BWindow;

class DFindDialog : public HDialog
{
  public:
  	enum { sResID = 129 };
  	
	DFindDialog(BRect frame, const char *name, window_type type, int flags,
				BWindow *owner, BPositionIO& data);
	
	virtual bool OKClicked();
	void SetTarget(const char *what, BHandler *h);
	
  private:
	BHandler *fHandler;
};

#endif
