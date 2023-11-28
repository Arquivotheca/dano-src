#ifndef _APPLICATION_H
#include <Application.h>
#endif
#include "edit_window.h"

extern const char *app_signature;

class TEditApplication : public BApplication {
		TEditWindow		*ww;

public:
						TEditApplication();
						~TEditApplication(void);
virtual	void			MessageReceived(BMessage *msg);
virtual	void			RefsReceived(BMessage *inMessage);
virtual	void			ArgvReceived(int32 argc, char **argv);

private:
};
