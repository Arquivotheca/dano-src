#ifndef __APP_H__
#define __APP_H__

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <Alert.h>
#include <Button.h>
#include <Debug.h>
#include <CheckBox.h>
#include <ClassInfo.h>
#include <FindDirectory.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Path.h>
#include <PrivateScreen.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <StringView.h>
#include <TextView.h>

#include <interface_misc.h>
#include <screen_private.h>

#include <Application.h>

#include "Wind.h"

class TApp : public BApplication
{
public:
								TApp();
								~TApp();
				
		void 					AboutRequested();
		void 					MessageReceived(BMessage*);
		void 					ReadyToRun();
		
private:
		MainWindow*				win;
};

extern TApp *theApp;


#endif
