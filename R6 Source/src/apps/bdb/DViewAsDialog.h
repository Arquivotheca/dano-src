/*	$Id: DViewAsDialog.h,v 1.1 1999/03/05 14:25:20 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 02/18/99 11:38:38
*/

#ifndef DVIEWASDIALOG_H
#define DVIEWASDIALOG_H

#include "HDialog.h"

class BPositionIO;

class DViewAsDialog : public HDialog
{
  public:
	DViewAsDialog(BRect frame, const char *name, window_type type, int flags,
		BWindow *owner, BPositionIO& data);
		
	enum { sResID = 132 };

  protected:
	virtual bool OKClicked();
};

#endif
