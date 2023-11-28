/*	$Id: HHelpWindow.h,v 1.1 1998/11/14 14:20:54 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 1/8/98
*/

#ifndef HHELPWINDOW_H
#define HHELPWINDOW_H

#include <View.h>
#include <Window.h>

class HHelpView : public BView {
public:
			HHelpView(BRect r, const char *helptext);
			~HHelpView();
			
virtual	void Draw(BRect update);
			
private:
			char *fHelpText;
};

class HHelpWindow : public BWindow {
public:
			HHelpWindow(BRect r, const char *msg);
};

#endif
