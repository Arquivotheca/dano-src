#include <stdio.h>
#include <string.h>
#include <Messenger.h>

int
main(int argc, char *argv[])
{
	status_t err;
	if(argc != 2) {
		printf("use: %s mime_sig\n", argv[0]);
		return 1;
	}
	BMessenger msgr(argv[1]);
	if(!msgr.IsValid()) {
		printf("could not find running app with sig %s\n", argv[1]);
		return 1;
	}
	err = msgr.SendMessage(B_QUIT_REQUESTED);
	if(err != B_NO_ERROR) {
		printf("could not send message, %s\n", strerror(err));
		return 1;
	}
	return 0;
}
