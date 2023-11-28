#ifndef __LEGAL_MESSAGE__
#define __LEGAL_MESSAGE__

#include <Window.h>

class LegalMessage : public BWindow {
public:
	LegalMessage();
	virtual ~LegalMessage();
	
	virtual void MessageReceived(BMessage *msg);
};

#endif
