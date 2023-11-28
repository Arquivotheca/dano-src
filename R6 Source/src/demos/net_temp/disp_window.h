#ifndef _WINDOW_H
#include <Window.h>
#endif
#include "view1.h"


#define	HS	540
#define	VS	680

class TDispWindow : public BWindow {

public:
									TDispWindow(BRect, const char*);
						void		disp();
virtual					bool		QuitRequested( void );

						BView		*view;
};
