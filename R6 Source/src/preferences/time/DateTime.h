#ifndef _MAIN_H_
#define _MAIN_H_

#include <Application.h>

class TApp : public BApplication {
public:
			TApp();
			~TApp();
			
	void 	ReadyToRun();
	void 	AboutRequested();
	void 	MessageReceived(BMessage *msg);
};

#endif /*  _MAIN_H_ */
