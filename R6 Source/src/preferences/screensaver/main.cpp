#include "BlanketApp.h"
#include "Settings.h"

#include <stdlib.h>
#include <time.h>

int
main()
{
	BlanketApp	app;
	srand(system_time());
	app.Run();
	SaveSettings();
	
	return 0;
}
