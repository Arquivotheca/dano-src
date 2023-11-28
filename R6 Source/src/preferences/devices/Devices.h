#include <Application.h>

#include "DeviceListWindow.h"

class TApp : public BApplication {
public:
							TApp();
							~TApp();
		
		void				MessageReceived(BMessage*);		
		void				AboutRequested();

private:
		TDeviceListWindow*	fDeviceList;
};
