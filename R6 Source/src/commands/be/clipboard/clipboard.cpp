// clipboard.cpp
//
// clipboard  | a shell tool for manipulating the BeOS system clipboard
// 2001-05-11 | dan sandler <dsandler@be.com> 
//
// TODO: add a parameter to allow access to any named clipboard

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Clipboard.h>

#define CLIPBOARD_HELP_MESSAGE "\
usage: clibpoard (--set <string> | --get)\n\
    --get : writes text in system clipboard to standard output.\n\
            returns non-zero if no text data in clipboard.\n\
    --set : writes <string> to system clipboard as text/plain.\n\
            returns non-zero on error committing clipboard data.\n\
"

void print_usage_and_exit()
{
	printf(CLIPBOARD_HELP_MESSAGE);
	exit(1);
}

int main(int argc, char *argv[])
{
	const char *val; // points to the string in question
	int retVal = 0;

	if ( argc < 2 ) print_usage_and_exit();

	BClipboard *systemClip = new BClipboard("system");
	if (!systemClip) exit(B_ERROR);

	// TODO: use getopt here or something sensible
	if ( strcmp(argv[1], "--set") == 0 ) {
		if ( argc != 3 ) {
			print_usage_and_exit();
		} else {
			val = argv[2];
			BMessage *clip = (BMessage *)NULL;

			if (systemClip->Lock()) {
				systemClip->Clear();
				if ((clip = systemClip->Data()) != NULL) {
					if ( (retVal = clip->AddData("text/plain", B_MIME_TYPE, val, strlen(val))) == B_OK )
						retVal = systemClip->Commit();
				}
				systemClip->Unlock();
			}
		}
	} else if ( strcmp(argv[1], "--get") == 0 ) {
		int32 textLen;
		BMessage *clip = (BMessage *)NULL;

		if (systemClip->Lock()) {
			if ((clip = systemClip->Data()) != NULL) {
				retVal = clip->FindData("text/plain", B_MIME_TYPE,
				                        (const void **)&val, &textLen);
			} else {
				retVal = ENOENT;
			}
			systemClip->Unlock();
		}

		if (retVal == B_OK) {
			if (val)
				printf("%s\n", val);
			else
				retVal = B_ERROR;
		}
	} else {
		print_usage_and_exit();
	}

	exit(retVal);

}
