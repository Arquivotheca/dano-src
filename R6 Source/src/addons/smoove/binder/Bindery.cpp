
#include <File.h>
#include <xml/BXMLBinderNode.h>


extern void AddTestStuff();

extern "C" _EXPORT GHandler *return_handler(GDispatcher * /* dispatcher */)
{
	XMLBinderNode *r = new XMLBinderNode();
	int32 status = r->Mount("/binder");
	r->Acquire();
	BFile file("/boot/home/config/settings/binder/root",O_RDONLY);
	if (!status) r->Load(file);
	return r;
}
