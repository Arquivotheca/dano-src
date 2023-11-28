
#include "microphone.h"

#include <Application.h>

int
main()
{
	BApplication a("application/x-vnd.be.mictest");
	MicrophoneTest mt;
	fprintf(stderr, "calling Begin()\n");
	mt.Begin(new BMessenger(&a), new BMessage(B_QUIT_REQUESTED));
	fprintf(stderr, "starting appliaction\n");
	a.Run();
	fprintf(stderr, "test done\n");
	return 0;
}
