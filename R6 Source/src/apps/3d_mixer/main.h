#ifndef _APPLICATION_H
#include <Application.h>
#endif
#include "wave_window.h"

extern const char *app_signature;

class TSoundApplication : public BApplication {
		TWaveWindow		*ww;

public:
						TSoundApplication();
						~TSoundApplication(void);
virtual	void			MessageReceived(BMessage *msg);
virtual	void			RefsReceived(BMessage *inMessage);
virtual	void			ArgvReceived(int32 argc, char **argv);

private:
};
