/*
	MailCacheNode.cpp
*/
#include <String.h>
#include "CacheContainer.h"
#include "MailDebug.h"
#include "MailCacheNode.h"

MailCacheNode::MailCacheNode()
	:	XMLBinderNode(),
		fDirty(false),
		fMaxContainerSize(kDefaultMaxContainerSize)
{
	BuildContainerList();
	PostDelayedMessage(new BMessage('save'),  1000000 * 10);
}


MailCacheNode::~MailCacheNode()
{
	SaveContainers();
}

status_t MailCacheNode::HandleMessage(BMessage *message)
{
	status_t result = B_OK;
	switch (message->what) {
		case 'save': {
			SaveContainers();
			PostDelayedMessage(new BMessage('save'),  1000000 * 10);
			break;
		default:
			result = XMLBinderNode::HandleMessage(message);
			break;
		}
	}
	return result;
}

put_status_t MailCacheNode::WriteProperty(const char *, const property &)
{
	return B_ERROR;
}

get_status_t MailCacheNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	// printf("MailCacheNode::ReadProperty: name = '%s'\n", name);
	// Stupid binder hack, prop cannot be undefined.
	prop = "StupidBinderHack";
	
	if (strcmp(name, "AddEntry") == 0) {
		return HandleCacheUpdate(args, kAddEntryType);
	} else if (strcmp(name, "TouchEntry") == 0) {
		return HandleCacheUpdate(args, kTouchEntryType);
	} else if (strcmp(name, "ContainsEntry") == 0) {
		return HandleCacheUpdate(args, kContainsEntryType);
	} else if (strcmp(name, "RemoveEntry") == 0) {
		return HandleCacheUpdate(args, kRemoveEntryType);
	} else if (strcmp(name, "ResizeEntry") == 0) {
		return HandleCacheUpdate(args, kResizeEntryType);
	} else if (strcmp(name, "RemoveMessages") == 0) {
		return HandleCacheUpdate(args, kRemoveMessagesType);
	} else if (strcmp(name, "SetContainerMaximumSize") == 0) {
		if (args.Count() > 0)
			SetContainerMaximumSize(int32(args[0].Number()));
		return B_OK;
	} else if (strcmp(name, "currentCacheSize") == 0) {
		prop = CalculateCurrentCacheSize();
		return B_OK;
	} else if (strcmp(name, "Flush") == 0) {
		SaveContainers();
		return B_OK;
	} else if (strcmp(name, "PrintToStream") == 0) {
		prop = name;
		if (args.CountItems() > 0)
			PrintToStream(args[0].String().String());
		else
			PrintToStream();
		return B_OK;
	}
	return XMLBinderNode::ReadProperty(name, prop, args);
}

status_t MailCacheNode::Parse(B::XML:: BParser **stream)
{
	// XXX: Not yet supported
	(void) stream;
#if 0
	*stream = new MailCacheXMLParser(this);
#endif
	return B_OK;
}

status_t MailCacheNode::HandleCacheUpdate(const property_list &args, update_type type)
{
	status_t result = B_OK;

	// Try and find this entry	
	CacheContainer *container = FindContainer(args[0].String().String());
	if (container == NULL) {
		printf("Container was null for user = '%s'\n", args[0].String().String());
		return B_ERROR;
	}
	// Found the container for this user, now tell it to update this entry...
	BString mailbox = args[1].String();
	BString uid = args[2].String();
	BString part = args.Count() > 3 ? args[3].String() : B_EMPTY_STRING;
	
	off_t size = args.Count() > 4 ? (off_t)(args[4].Number()) : -1;
	
	switch (type) {
		case kAddEntryType:
			result = container->AddEntry(mailbox.String(), uid.String(), part.String(), size);
			break;
		case kTouchEntryType:
			result = container->TouchEntry(mailbox.String(), uid.String(), part.String());
			break;
		case kContainsEntryType:
			result = container->ContainsEntry(mailbox.String(), uid.String(), part.String(), size);
			break;
		case kRemoveEntryType:
			result = container->RemoveEntry(mailbox.String(), uid.String(), part.String());
			break;
		case kResizeEntryType:
			result = container->ResizeEntry(mailbox.String(), uid.String(), part.String(), size);
			break;
		case kRemoveMessagesType:
			result = container->RemoveMessages(mailbox.String(), uid.String());
			break;
		default:
			TRESPASS();
			break;
	}
	if ((result == B_OK) && (type != kContainsEntryType))
		fDirty = true;
		
	return result;
}

status_t MailCacheNode::SaveContainers()
{
	status_t result = B_OK;
	int32 count = fContainers.CountItems();
	CacheContainer *container = NULL;
	for (int32 i = 0; i < count; i++) {
		container = static_cast<CacheContainer *>(fContainers.ItemAt(i));
		ASSERT(container != NULL);
		container->Save();
	}

	return result;
}

void MailCacheNode::BuildContainerList()
{
	BinderNode::property rootUser = BinderNode::Root()["user"];
	BinderNode::iterator i = rootUser->Properties();
	BString user;
	while ((user = i.Next()) != "") {
		if ((user != "~") && (user != "system"))
			AddContainer(user.String());
	}
}

void MailCacheNode::AddContainer(const char *username)
{
	// Make sure that it's not already in the list...
	CacheContainer *container = FindContainer(username);
	if (container == NULL)
		fContainers.AddItem(new CacheContainer(username));
}

void MailCacheNode::RemoveContainer(const char *username)
{
	CacheContainer *container = FindContainer(username);
	if (container != NULL) {
		if (fContainers.RemoveItem(container))
			delete container;
	}
}

CacheContainer *MailCacheNode::FindContainer(const char *username)
{
	int32 count = fContainers.CountItems();
	bool found = false;
	CacheContainer *container = NULL;
	for (int32 i = 0; i < count; i++) {
		container = static_cast<CacheContainer *>(fContainers.ItemAt(i));
		ASSERT(container != NULL);
		if (strcmp(container->Username(), username) == 0) {
			found = true;
			break;
		}
	}
	return (found ? container : NULL);
}

off_t MailCacheNode::CalculateCurrentCacheSize()
{
	off_t cacheSize = 0;
	int32 count = fContainers.CountItems();
	CacheContainer *container = NULL;
	
	for (int32 i = 0; i < count; i++) {
		container = static_cast<CacheContainer *>(fContainers.ItemAt(i));
		ASSERT(container != NULL);
		cacheSize += container->Size();	
	}
	return cacheSize;
}

void MailCacheNode::SetContainerMaximumSize(int32 size)
{
	int32 count = fContainers.CountItems();
	CacheContainer *container = NULL;
	
	for (int32 i = 0; i < count; i++) {
		container = static_cast<CacheContainer *>(fContainers.ItemAt(i));
		ASSERT(container != NULL);
		container->SetMaximumSize(size);
	}
}

void MailCacheNode::PrintToStream(const char *username)
{
	if (username != NULL) {
		CacheContainer *container = FindContainer(username);
		if (container != NULL)
			container->PrintToStream();
	}
}

// ----------------------------------------------------------------
//						MailCacheXMLParser
// ----------------------------------------------------------------
MailCacheXMLParser::MailCacheXMLParser(MailCacheNode *node)
	:	BParser(),
		fNode(node)
{
	// empty constructor
}

status_t MailCacheXMLParser::StartTag(BString &name, BStringMap &attributes, BParser **newParser)
{
	fLastTagName = B_EMPTY_STRING;
	attributes.Find("name", fLastTagName);
	return BParser::StartTag(name, attributes, newParser);
}

status_t MailCacheXMLParser::EndTag(BString &name)
{
	fLastTagName = B_EMPTY_STRING;
	return BParser::EndTag(name);
}

status_t MailCacheXMLParser::TextData(const char *data, int32 size)
{
	BString value;
	char *buf = value.LockBuffer(size);
	memcpy(buf, data, size);
	value.UnlockBuffer(size);

	// XXX: Not yet implemented
#if 0
	if (fLastTagName == "userCacheSize") {
		fNode->SetContainerMaximumSize(atoi(value.String()));
	}
#endif
	
	return BParser::TextData(data, size);
}

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new MailCacheNode();
}
