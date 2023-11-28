#ifndef ICON_WINDOW_H
#define ICON_WINDOW_H

#include <stdio.h>
#include <Window.h>

#include "IconEditor.h"

class IconEditView;
class BBitmap;
class BMessage;

class TIconWindow : public BWindow {
public:
						TIconWindow(IconEditView *parent, const char* iconName, BBitmap *largeIcon, BBitmap *miniIcon);
						~TIconWindow();
	
		void 			MessageReceived(BMessage *);
		bool 			QuitRequested();
		
		void			GetPrefs();
		void			SetPrefs();
	
		void 			MenusBeginning();

		bool 			Save();
		bool 			Dirty() const;
private:
		TIconEditor*	fIconEditor;
		IconEditView*	fParent;
		BMenuBar*		fMenuBar;
};

#endif
