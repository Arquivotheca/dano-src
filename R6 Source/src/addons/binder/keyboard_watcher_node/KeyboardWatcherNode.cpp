/* KeyboardWatcherNode.cpp
**
** A Binder node that indicates whether a USB keyboard is plugged-in.
**
** Has only one property:
**
**		plugged_in
**			: "true" if a USB keyboard is plugged-in, "false" otherwise
*/

#include <string.h>

#include <Debug.h>
#include <Binder.h>
#include <Directory.h>
#include <Locker.h>
#include <experimental/MultiLocker.h>
#include <NodeMonitor.h>
#include <Entry.h>

#define BYLINE "KeyboardWatcherNode"

const char* kPluggedInProp = "plugged_in";

// NOTE: these properties need to stay in alphabetical order.  If a new property
// is inserted in this list, code to handle it should be added to ReadProperty().
const char* kPubProperties[] = {
	kPluggedInProp
};

const int32	kNumPubProperties = (sizeof(kPubProperties)
	/ sizeof(kPubProperties[0]));

const char* kWatchPath = "/dev/input/keyboard/usb/";

class KeyboardWatcherNode : public BinderNode {
public:
	KeyboardWatcherNode(void);
	virtual ~KeyboardWatcherNode(void);
	
	virtual status_t HandleMessage(BMessage* message);

	void PluggedIn(void);
	void Unplugged(void);

protected:
	virtual status_t OpenProperties(void** cookie, void* copyCookie);
	virtual status_t NextProperty(void* cookie, char* nameBuf, int32* len);
	virtual status_t CloseProperties(void* cookie);

	virtual	put_status_t WriteProperty(const char* name, const property& prop);
	virtual	get_status_t ReadProperty(const char* name, property& prop,
		const property_list& args = empty_arg_list);

private:
	int32 fPluggedIn;
	BMultiLocker* fLocker;

	struct store_cookie {
		int32 index;
	};
};

KeyboardWatcherNode::KeyboardWatcherNode(void)
	: fPluggedIn(-1), fLocker(new BMultiLocker(BYLINE " Lock"))
{
	status_t err;
	node_ref watchRef;

	PRINT((BYLINE "::KeyboardWatcherNode()\n"));
	
	BEntry entry(kWatchPath);
	err = entry.InitCheck();
	if (err != B_OK) {
		PRINT((BYLINE ": %s doesn't exist?\n", kWatchPath));
		return;
	}
	
	// protect fPluggedIn while setting up node watching
	fLocker->WriteLock();
	
	entry.GetNodeRef(&watchRef);
	err = watch_node(&watchRef, B_WATCH_DIRECTORY, this);
	PRINT((BYLINE ": watch_node() returned 0x%08lx: %s\n", err,
		strerror(err)));
			
	BDirectory dir(&entry);
	if (dir.InitCheck() == B_OK) {
		// Properly handle case where more than one USB keyboard
		// is plugged in. It's a silly edge case, but it seems
		// easy enough to do it right.
		fPluggedIn = dir.CountEntries();
		PRINT((BYLINE ": fPluggedIn set to %ld\n", fPluggedIn));
		NotifyListeners(B_PROPERTY_CHANGED, kPluggedInProp);
	}
	
	fLocker->WriteUnlock();
}

KeyboardWatcherNode::~KeyboardWatcherNode(void)
{
	PRINT((BYLINE "::~KeyboardWatcherNode()\n"));
	
	stop_watching(this);
	
	delete fLocker;
}

status_t KeyboardWatcherNode::OpenProperties(void **cookie, void *copyCookie)
{
	store_cookie *c = new store_cookie;
	if (copyCookie) {
		*c = *((store_cookie*)copyCookie);
	} else {
		c->index = 0;
	}
	*cookie = c;
	
	return B_OK;
}

status_t KeyboardWatcherNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	status_t err = ENOENT;
	store_cookie *c = (store_cookie*)cookie;

	if (c->index < kNumPubProperties) {	
		const char *name = kPubProperties[c->index];
		strncpy(nameBuf, name, *len);
		*len = strlen(name);
		c->index++;
		err = B_OK;
	}
	
	return err;
}

status_t KeyboardWatcherNode::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;

	return B_OK;
}

put_status_t KeyboardWatcherNode::WriteProperty(const char*, const property&)
{
	return B_ERROR;
}

get_status_t KeyboardWatcherNode::ReadProperty(const char* name, property& prop,
	const property_list&)
{
	if (strcmp(name, kPluggedInProp) == 0) {
		fLocker->ReadLock();
		prop = (fPluggedIn > 0) ? "true" : "false";
		fLocker->ReadUnlock();
		
		return B_OK;
	}
	
	return ENOENT;
}
		
status_t KeyboardWatcherNode::HandleMessage(BMessage* message)
{
#if DEBUG
	message->PrintToStream();
#endif
	switch (message->what) {
		case B_NODE_MONITOR: {
			int32 opcode;
			
			if (message->FindInt32("opcode", &opcode) == B_OK) {
				switch (opcode) {
					case B_ENTRY_CREATED:
						PluggedIn();
						break;
					case B_ENTRY_REMOVED:
						Unplugged();
						break;
				}
			}
			break;
		}
		default:
			PRINT((BYLINE ": unexpected message\n"));
			break;
	}
	
	return B_OK;
}

void KeyboardWatcherNode::PluggedIn(void)
{
	PRINT((BYLINE "::PluggedIn()\n"));

	fLocker->WriteLock();
	fPluggedIn++;
	PRINT((BYLINE ": fPluggedIn set to %ld\n", fPluggedIn));
	
	if (fPluggedIn == 1)
		// the first USB keyboard has been plugged-in
		NotifyListeners(B_PROPERTY_CHANGED, kPluggedInProp);
		
	fLocker->WriteUnlock();
}

void KeyboardWatcherNode::Unplugged(void)
{
	PRINT((BYLINE "::Unplugged()\n"));

	fLocker->WriteLock();
	fPluggedIn--;
	PRINT((BYLINE ": fPluggedIn set to %ld\n", fPluggedIn));
	
	if (fPluggedIn == 0)
		// the last USB keyboard has been unplugged
		NotifyListeners(B_PROPERTY_CHANGED, kPluggedInProp);
		
	fLocker->WriteUnlock();
}

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new KeyboardWatcherNode();
}
