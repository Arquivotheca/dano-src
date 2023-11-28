#ifndef _APPLICATION_H
#include <Application.h>
#endif

extern const char *app_signature;

class TSoundApplication : public BApplication {

public:
						TSoundApplication();
virtual	void			MessageReceived(BMessage *msg);

private:
};
