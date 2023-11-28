#ifndef __Kaleidoscope
#define __Kaleidoscope

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#include "KalDraw.h"

extern KalDraw *drawThread;

class Kaleidoscope : public BApplication {

public:
					Kaleidoscope ();
		void		ReadyToRun ();
		bool		QuitRequested ();
};

#endif
