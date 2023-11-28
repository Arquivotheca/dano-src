
#include <stdio.h>
#include "SmooveD.h"

BMessenger call_flunky(const char *flunkyName)
{
	BMessenger r,mess(SmooveDSignature);
	BMessage msg(bmsgFetchFlunky),reply;
	msg.AddString("flunky_name",flunkyName);
	mess.SendMessage(&msg,&reply);
	reply.FindMessenger("flunky_messenger",&r);
	return r;
}
