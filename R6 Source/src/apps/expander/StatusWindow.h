#include <stdio.h>

#include <Application.h>
#include <Box.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Window.h>
#include <View.h>

#include "Expander.h"

#ifndef STATUS_WINDOW_H
#define STATUS_WINDOW_H

#define kStatusWindWidth				300.0
#define kStatusMaxWindWidth			480.0
#define kStatusWindHeight				32.0
#define kStatusWindXLoc 			50.0
#define kStatusWindYLoc 			50.0

class TExpanderStatusWindow : public BWindow {
   	BView		*fBackdrop;
	BStringView *fMsg;
public:
  				TExpanderStatusWindow(void); 
  				~TExpanderStatusWindow(void);
  
		void	MessageReceived(BMessage *msg);
		bool	QuitRequested();
		
		void 	AddParts(void);
		
		void 	SetMsg(char *msg);
		
		bool	DropInMsg(BPoint pt);
};

#endif
