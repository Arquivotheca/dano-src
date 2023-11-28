#ifndef _APPLICATION_H
#include <Application.h>
#endif

extern const char *app_signature;

class TSearchApplication : public BApplication {

public:
						TSearchApplication(char *search_string);
						~TSearchApplication();
virtual	void			MessageReceived(BMessage *msg);

private:
};
