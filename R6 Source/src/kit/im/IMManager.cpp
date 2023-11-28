/*
	IMManager.cpp
*/
#include "IMManager.h"
#include "IMTransportHandle.h"
#include <stdio.h>

// Class member statics...
IMManager *IMManager::fIMManagerInstance = NULL;
BLocker IMManager::fInstanceLock;

IMManager *IMManager::Manager()
{
	fInstanceLock.Lock();
	if (fIMManagerInstance == NULL) {
		fIMManagerInstance = new IMManager();
		fIMManagerInstance->AddEnvVar("ADDON_PATH", "instant_messaging");
		fIMManagerInstance->Scan();
	}

	fInstanceLock.Unlock();
	return fIMManagerInstance;
}

void IMManager::Teardown()
{
	fInstanceLock.Lock();
	if (fIMManagerInstance != NULL) {
		fIMManagerInstance->Unstack();
	}
	fInstanceLock.Unlock();
}

IMManager::IMManager()
	:	BAddOnManager("Instant Message AddOn Manager"),
		BinderNode()
{
	printf("IM: IMManager created\n");
	// Stack onto the IM node
	BinderNode::property node = BinderNode::Root()["service"]["chat"];
	StackOnto(node.Object());
}


IMManager::~IMManager()
{
	printf("IM: IMManager deleted\n");
}

status_t IMManager::HandleMessage(BMessage *message)
{
	status_t result = B_OK;
	
	switch (message->what) {
		default: {
			result = GHandler::HandleMessage(message);
			break;
		}
	}
	return result;
}

BAddOnHandle *IMManager::InstantiateHandle(const entry_ref *entry, const node_ref *node)
{
	printf("Trying to create Handle for '%s'\n", entry->name);
	IMTransportHandle* handle = new IMTransportHandle(entry, node);
	if (handle != NULL) {
		fTransportList.AddItem(handle);
	}
	
	return handle;
}

put_status_t IMManager::WriteProperty(const char *name, const property &prop)
{
	return BinderNode::WriteProperty(name, prop);
}

get_status_t IMManager::ReadProperty(const char *name, property &prop, const property_list &args)
{
	printf("IMManager::ReadProperty with name = '%s'\n", name);
	return BinderNode::ReadProperty(name, prop, args);
}

// End of IMManager.cpp
