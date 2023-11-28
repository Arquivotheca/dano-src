/*	$Id: DSettingsDialog.h,v 1.1 1999/02/11 15:52:40 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 02/04/99 13:36:20
*/

#ifndef DSETTINGSDIALOG_H
#define DSETTINGSDIALOG_H

#include "HDialog.h"

class DSettingsDialog : public HDialog
{
	static DSettingsDialog *sInstance;

  public:

	DSettingsDialog(BRect frame, const char *name, window_type type, int flags,
				BWindow *owner, BPositionIO& data);
	~DSettingsDialog();

	static void Display();
	
	enum { sResID = 130 };
	
	virtual bool OKClicked();
	virtual bool CancelClicked();
	virtual void UpdateFields();

  protected:
	BMenu *fFont;
	font_family fFontFamily;
	font_style fFontStyle;
	float fFontSize;
};

#endif
