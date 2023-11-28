
#include <storage2/AddOnManager.h>
#include <storage2/Directory.h>
#include <storage2/Entry.h>
#include <storage2/FindDirectory.h>
#include <storage2/Node.h>
#include <storage2/Path.h>
#include <storage2/Volume.h>
#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/Handler.h>
#include <support2/MemoryStore.h>
#include <support2/PositionIO.h>
#include <support2/String.h>

#include <fs_attr.h>

#include <string.h>
#include <time.h>
#include <stdlib.h>

#define DEBUG_IMAGE_OPEN DEBUG
#define DEBUG_IMAGE_LOAD DEBUG
#define DEBUG_NODE_MONITOR DEBUG

// --------------------------- BAddOnHandle ---------------------------

using namespace B::Storage2;

enum {
    // Returned by ReadIdentifiers() to indicate that the attributes
    // specify this is not an addon for us.
    IMAGE_NOT_ADDON		= 1
};

BAddOnHandle::BAddOnHandle(const entry_ref* entry, const node_ref* node)
		: fNode(node ? *node : node_ref()),
		fEntry(entry ? *entry : entry_ref()),
		fImage(B_ERROR), fImageErr(B_OK),
		fUsers(0), fLastUsed(0),
		fHasIdentifiers(false), fStaticImage(false)
{
	PRINT(("Creating BAddOnHandle %p (%s)\n", this, ref.name));
}

BAddOnHandle::~BAddOnHandle()
{
#if DEBUG_IMAGE_OPEN
	printf("Deleting: %p (%s), image=%ld, users=%ld\n",
	        this, fEntry.name, fImage, fUsers);
#endif
	// XXX don't force an unload, for now -- Java dies
	// whenever it gets unloaded.
	//do_flush(B_FLUSH_KEEP_LOADED|B_FLUSH_DYNAMIC);
	do_flush(B_FLUSH_DYNAMIC);
}

node_ref BAddOnHandle::NodeRef() const
{
	return fNode;
}

entry_ref BAddOnHandle::EntryRef() const
{
	return fEntry;
}

status_t BAddOnHandle::SetEntryRef(const entry_ref* ref)
{
	BAutolock lock(fAccess.Lock());
	fEntry = *ref;
	return B_OK;
}

image_id BAddOnHandle::Open()
{
	BAutolock lock(fAccess.Lock());
	image_id ret = do_open();
	return ret;
}

image_id BAddOnHandle::do_open()
{
#if DEBUG_IMAGE_OPEN
	printf("Open(): %x (%s)\n", this, fEntry.name);
#endif

	// Update the time this add-on was last opened.
	fLastUsed = (time_t)(system_time()/1000000);

	if (fImage >= B_OK)
	{
		atomic_add(&fUsers, 1);
		Acquire();
#if DEBUG_IMAGE_OPEN
		printf("Already loaded, user count is now %ld\n", fUsers);
#endif
		return fImage;
	}

	if (fImageErr < B_OK) return fImageErr;

	BPath path;

	if (fEntry != entry_ref())
	{
		// Load image, if there is an entry_ref for it.
		BEntry entry(&fEntry);
		if ((fImageErr=entry.InitCheck()) != B_OK)
		{
#if DEBUG_IMAGE_LOAD
			printf("\"%s\" can't make entry: %s\n", fEntry.name, strerror(fImageErr));
#endif
			return fImageErr;
		}

		if ((fImageErr=path.SetTo(&entry)) != B_OK)
		{
#if DEBUG_IMAGE_LOAD
			printf("\"%s\" can't make path: %s\n", fEntry.name, strerror(fImageErr));
#endif
			return fImageErr;
		}

		fImage = load_add_on(path.Path());
		fStaticImage = false;
#if DEBUG_IMAGE_LOAD
		printf("Loaded image %ld (%s) (addon %p), err=%s\n",
		        fImage, fEntry.name, this, strerror(fImage < B_OK ? fImage : B_OK));
#endif

	}
	else
	{
		// Mark this as a static add-on.
		fImage = B_OK;
		fStaticImage = true;
	}

	if (fImage < B_OK)
	{
		printf("\"%s\" was not an image: %s\n", path.Path(), strerror(fImage));
		fImageErr = fImage;
	}
	else
	{
		atomic_add(&fUsers, 1);
		Acquire();
		ImageLoaded(fImage);
	}

#if DEBUG_IMAGE_OPEN
	printf("Loaded, user count is now %ld\n", fUsers);
#endif

	return fImage;
}

void BAddOnHandle::Close()
{
	BAutolock lock(fAccess.Lock());
	if (fImage >= B_OK)
		do_close();
}

void BAddOnHandle::do_close()
{
	atomic_add(&fUsers, -1);
#if DEBUG_IMAGE_OPEN
	printf("Close(): %x (%s), users now=%ld\n", this, fEntry.name, fUsers);
#endif
}

bool BAddOnHandle::Flush(uint32 flags)
{
	if (KeepLoaded())
	{
		TRESPASS();
		PRINT(("*** Add-on wants to stay loaded.\n"));
		return false;
	}

	BAutolock lock(fAccess.Lock());
	bool res = do_flush(flags);
	return res;
}

bool BAddOnHandle::do_flush(uint32 flags)
{
#if DEBUG_IMAGE_OPEN
	printf("Flush(): %x (%s), image=%ld, users=%ld\n",
	        this, fEntry.name, fImage, fUsers);
#endif

	if (((flags&B_FLUSH_KEEP_LOADED) == 0 && KeepLoaded()) ||
	        ((flags&B_FLUSH_DYNAMIC) == 0 && IsDynamic()))
	{
		PRINT(("*** Add-on wants to stay loaded.\n"));
		return false;
	}

	if (((flags&B_FLUSH_REFERENCED) != 0 || fUsers <= 0) && fImage >= B_OK)
	{
		ImageUnloading(fImage);
		if (!fStaticImage)
		{
#if DEBUG_IMAGE_LOAD
			printf("Unloading image %ld (%s) (addon %p)\n",
			        fImage, fEntry.name, this);
#endif
			unload_add_on(fImage);
		}
		fImage = B_ERROR;
		fStaticImage = false;
		return true;
	}

	return false;
}

bool BAddOnHandle::IsLoaded() const
{
	return fImage >= B_OK ? true : false;
}

bool BAddOnHandle::IsStatic() const
{
	return fStaticImage;
}

bool BAddOnHandle::KeepLoaded() const
{
	return fStaticImage;
}

bool BAddOnHandle::IsDynamic() const
{
	return false;
}

size_t BAddOnHandle::GetMemoryUsage() const
{
	if (fImage < B_OK || fStaticImage) return 0;

	image_info ii;
	if (get_image_info(fImage, &ii) != B_OK) return 0;

	// NOTE: This doesn't take into account any shared libraries
	// the image may have dragged along with it.
	return ii.text_size + ii.data_size;
}

status_t BAddOnHandle::GetIdentifiers(BValue* into, bool quick) const
{
	BAutolock lock(fAccess.Lock());
	status_t ret = const_cast<BAddOnHandle*>(this)->do_get_identifiers(into, quick);
	return ret;
}

status_t BAddOnHandle::do_get_identifiers(BValue *into, bool quick)
{
	if (fHasIdentifiers)
	{
		if (into) *into = fIdentifiers;
		return B_OK;
	}

	if (fImageErr != B_OK) return fImageErr;

	status_t err = B_OK;
	BNode node;

	// If this handle has an entry_ref, first try to retrieve its
	// identifiers from its attributes.
	if (fEntry != entry_ref())
	{

		PRINT(("Retrieving identifers for %s\n", fRef.name));

		err = node.SetTo(&fEntry);

		if (err == B_OK)
		{
			PRINT(("Trying to read attributes...\n"));
			err = do_read_identifiers(&fIdentifiers, &node);
			if (err == B_OK)
			{
#if DEBUG
				PRINT(("Found :"));
				fIdentifiers.PrintToStream();
#endif
				fHasIdentifiers = true;
				if (into) *into = fIdentifiers;
				return B_OK;
			}
			else if (err == IMAGE_NOT_ADDON)
			{
				PRINT(("*** This is a bad add-on.\n"));
				fImageErr = B_BAD_IMAGE_ID;
				return fImageErr;
			}
		}

	}

	// If quick is requested and the add-on lives in a file,
	// then bail here rather than loading it in.
	if (quick && fEntry != entry_ref()) return err;

	// Finally try to load in add-on image and retrieve
	// attributes from it.  If this add-on does not have an
	// entry_ref, it will just call LoadIdentifiers() without
	// actually opening an image.
	PRINT(("Loading attributes from image.\n"));
	image_id image = do_open();
	if (image < B_OK) return image;

	err = LoadIdentifiers(&fIdentifiers, image);
	do_close();

	// And now, if this add-on lives in a file and is not
	// dynamic, write the newly found identifiers into its
	// attribute.
	if (!IsDynamic())
	{
		if (err == B_OK)
		{
			fHasIdentifiers = true;
			if (into) *into = fIdentifiers;
			if (!fIdentifiers.IsDefined() && fEntry != entry_ref())
			{
				do_write_identifiers(&node, &fIdentifiers);
			}
		}
		else if (fEntry != entry_ref())
		{
			do_write_identifiers(&node, 0);
		}
	}

	return err;
}

bool BAddOnHandle::MatchIdentifier(const char* name, const char* value, bool quick) const
{
	BAutolock lock(fAccess.Lock());

	if (const_cast<BAddOnHandle*>(this)->do_get_identifiers(0, quick) != B_OK)
		return false;

#warning "This will not work the same way. fIdentifiers needs to store an array"
//	const char* cur;
//	for (int32 i=0; fIdentifiers.d(name, i, &cur) == B_OK; i++)
//	{
//		if (strcasecmp(cur, value) == 0)
//		{
//			return true;
//		}
//	}

	BValue val;
	if ((val = fIdentifiers[BValue::String(name)]))
	{
		if (strcasecmp(val.AsString(), value) == 0)
			return true;
	}

	return false;
}

time_t BAddOnHandle::SecondsSinceOpen() const
{
	BAutolock lock(fAccess.Lock());

	time_t now = (time_t)(system_time()/1000000);
	time_t ret = now - fLastUsed;

	return ret;
}

void BAddOnHandle::ImageLoaded(image_id image)
{
	(void)image;
}

status_t BAddOnHandle::LoadIdentifiers(BValue* into, image_id from)
{
	(void)into;
	(void)from;
	return B_OK;
}

void BAddOnHandle::ImageUnloading(image_id image)
{
	(void)image;
}

const char* BAddOnHandle::AttrBaseName() const
{
	return "be:addon";
}

const BValue* BAddOnHandle::LockIdentifiers(bool quick) const
{
	fAccess.Lock();
	status_t ret = const_cast<BAddOnHandle*>(this)->do_get_identifiers(NULL, quick);
	if (ret != B_OK)
	{
		fAccess.Unlock();
		return NULL;
	}

	return &fIdentifiers;
}

void BAddOnHandle::UnlockIdentifiers(const BValue* ident) const
{
	if (!ident) return;
	if (ident != &fIdentifiers)
	{
		debugger("Bad identifier BValue returned");
		return;
	}

	fAccess.Unlock();
}

void BAddOnHandle::Lock() const
{
	fAccess.Lock();
}

void BAddOnHandle::Unlock() const
{
	fAccess.Unlock();
}

status_t BAddOnHandle::do_read_identifiers(BValue* into, BNode* from)
{
	const char* name = AttrBaseName();

	void* data = 0;
	size_t size = (size_t)-1;

	ssize_t err = B_OK;

	attr_info ai;
	ai.size = 0;
	while( err >= B_OK && ai.size != size)
	{
		err = from->GetAttrInfo(name, &ai);

		if (err >= B_OK && ai.type != B_RAW_TYPE)
		{
			err = B_BAD_VALUE;
			break;
		}

		if (err >= B_OK)
		{
			size = ai.size;
			if (!data) data = malloc(size);
			else data = realloc(data, size);

			if (!data) err = B_NO_MEMORY;
		}

		if (err >= B_OK) err = from->ReadAttr(name, B_RAW_TYPE, 0, data, size);
		ai.size = err;

		PRINT(("Read attribute %s, size=%ld (err=%s)\n", name, err, strerror(err)));

		if (err >= B_OK && err == (ssize_t)size)
		{
			err = from->GetAttrInfo(name, &ai);
		}

		PRINT(("Initial attr size was %ld, new size is %ld\n", size, ai.size));
	}

	if (err < B_OK)
	{
		free(data);
		return err;
	}

	IStorage::ptr store(new BMemoryStore(data, size));
	BPositionIO::ptr io(new BPositionIO(store));
	
	time_t when;
	err = io->Read((void *)&when, sizeof(when));

	// Determine if the cached identifier is out of date.  The logic is:
	// * If the cached "last modification time" is 0, then the identifier
	//   is never out of date.
	// * If the volume of this add-on is read-only, then the identifier is
	//   never out of date.
	// * Otherwise, compare the file's current modification time with the
	//   cached modification time to determine if it is out of date.
	if (err >= B_OK && when != 0)
	{
		struct stat st;
		err = from->GetStat(&st);

		// By default, assume that the cached time is valid.
		time_t cwhen = when;

		// If volume of this add-on isn't read-only, use the file's
		// modification time.
		if (err >= B_OK)
		{
			BVolume volume(st.st_dev);
			if (volume.InitCheck() == B_OK)
			{
				if (!volume.IsReadOnly())
				{
					cwhen = st.st_mtime;
				}
			}
		}

		// No error if cached identifier is not out of date; otherwise,
		// return error code so that the add-on will be loaded.
		if (cwhen <= when)
		{
			err = B_OK;
		}
		else
		{
			PRINT(("Add-on attributes are out of date: %ld is now %ld\n", when, cwhen));
			err = B_BAD_VALUE;
		}
	}

	PRINT(("Ready to unflatten message; position=%ld, size=%ld\n", (int32)io->Position(), size));

	if (err >= B_OK)
	{
		// If nothing else in the attribute, it is not a valid add-on.
		if (io->Position() >= size)
		{
			PRINT(("*** This is not a valid add-on\n"));
			err = IMAGE_NOT_ADDON;
		}
		else
		{
			err = B_OK;
		}
	}

	if (err == B_OK) err = into->Unflatten(io);
	free(data);

	PRINT(("Result from reading identifier from attribute: %s\n", strerror(err)));

	return err;
}

status_t BAddOnHandle::do_write_identifiers(BNode* into, const BValue* from)
{
	const char* name = AttrBaseName();
	ssize_t err = B_OK;

	BMallocStore::ptr store(new BMallocStore());
	BPositionIO::ptr io(new BPositionIO(store));
	
	time_t cwhen;
	err = into->GetModificationTime(&cwhen);
	if (err >= B_OK) err = io->Write(&cwhen, sizeof(cwhen));

	if (err >= B_OK && from) err = from->Flatten(io);

	if (err >= B_OK)
	{
		err = into->WriteAttr(name, B_RAW_TYPE, 0, store->Buffer(), store->Size());
		if (err != sizeof(store->Size())) err = B_DEVICE_FULL;
	}

	if (err >= B_OK) into->SetModificationTime(cwhen);

	return err;
}

// --------------------------- AddOnSearchPath ---------------------------

namespace B {
namespace Private {

class AddOnSearchPath : public BHandler, public BSearchPath
{
public:
	AddOnSearchPath(BAddOnManager& manager, const char* name)
			: fManager(&manager)
	{
	}

	virtual ~AddOnSearchPath()
	{
	}


	void DetachManager()
	{
		fAccess.Lock();
		fManager = NULL;
		fAccess.Unlock();
	}

	virtual status_t HandleMessage(const BMessage &msg)
	{
#warning "?????????????"
//		if (HandleNodeMessage(msg) == B_OK)
//		{
//			fAccess.Lock();
//			if (fManager)
//				fManager->send_notices();
//			fAccess.Unlock();
//			return B_OK;
//		}

		return BHandler::HandleMessage(msg);
	}

protected:
	virtual void EntryCreated(const node_ref* node, const entry_ref* entry)
	{
		fAccess.Lock();
		if (fManager)
		{
			BAutolock lock(fManager->Locker()->Lock());
#if DEBUG_NODE_MONITOR
			printf("Installing add-on: dev=%ld, node=%Ld, dir=%Ld, name=%s\n",
			        node->device, node->node, entry->directory, entry->name);
#endif
			fManager->InstallAddOn(entry, node);
		}
		fAccess.Unlock();
	}

	virtual void EntryMoved(const node_ref* node, const entry_ref* entry,
	        const entry_ref* /*oldEntry*/)
	{
		fAccess.Lock();
		if (fManager)
		{
			BAutolock lock(fManager->Locker()->Lock());
			BAddOnHandle* addon = fManager->FindAddOn(node);
			if (addon)
			{
#if DEBUG_NODE_MONITOR
				printf("Moving add-on: dev=%ld, node=%Ld, newdir=%Ld, newname=%s\n",
				        node->device, node->node, entry->directory, entry->name);
#endif
				addon->SetEntryRef(entry);
				addon->Release();
				fManager->mark_changed(false);
			}
		}
		fAccess.Unlock();
	}

	virtual void EntryRemoved(const node_ref* node, const entry_ref* /*entry*/)
	{
		fAccess.Lock();
		if (fManager)
		{
			BAutolock lock(fManager->Locker()->Lock());
			BAddOnHandle* addon = fManager->FindAddOn(node);
			if (addon)
			{
#if DEBUG_NODE_MONITOR
				printf("Removing add-on: dev=%ld, node=%Ld\n",
				        node->device, node->node);
#endif
				fManager->RemoveAddOn(addon);
				addon->Release();
			}
		}
		fAccess.Unlock();
	}

private:
	BLocker fAccess;
	BAddOnManager *fManager;
};

}
} // namespace B::Private;

// --------------------------- BAddOnManager ---------------------------
// #pragma mark -

BAddOnManager::BAddOnManager(const char* name)
		: fLock(name), fSearchPath(NULL), fScanned(false), fRunning(false)
{
	BString handlerName(name);
	handlerName += "_handler";
	fSearchPath = new AddOnSearchPath(*this, handlerName.String());
	fSearchPath->Acquire();
}

BAddOnManager::~BAddOnManager()
{
	Shutdown();
}

status_t BAddOnManager::AddDirectory(const BEntry* dir)
{
	BAutolock lock(fLock.Lock());
	return fSearchPath->AddDirectory(dir);
}

status_t BAddOnManager::AddDirectory(const entry_ref* dir)
{
	BAutolock lock(fLock.Lock());
	return fSearchPath->AddDirectory(dir);
}

status_t BAddOnManager::AddDirectory(const char* dir, const char* leaf)
{
	BAutolock lock(fLock.Lock());
	return fSearchPath->AddDirectory(dir, leaf);
}

status_t BAddOnManager::AddDirectory(directory_which which, const char* leaf)
{
	BAutolock lock(fLock.Lock());
	return fSearchPath->AddDirectory(which, leaf);
}

status_t BAddOnManager::AddSearchPath(const char* path, const char* leaf)
{
	BAutolock lock(fLock.Lock());
	return fSearchPath->AddSearchPath(path, leaf);
}

status_t BAddOnManager::AddEnvVar(const char* name, const char* leaf,
        const char* defEnvVal)
{
	BAutolock lock(fLock.Lock());
	return fSearchPath->AddEnvVar(name, leaf, defEnvVal);
}

status_t BAddOnManager::Scan()
{
	if (IsScanned() || IsRunning()) return B_OK;

	BAutolock lock(fLock.Lock());
	if (IsScanned() || IsRunning()) return B_OK;

	PRINT(("Scanning addon manager\n"));
	if (!fSearchPath) return B_NO_MEMORY;

	status_t res = fSearchPath->Scan();
	if (res < B_OK) return res;

	fScanned = true;

	return B_OK;
}

bool BAddOnManager::IsScanned() const
{
	return fScanned;
}

status_t BAddOnManager::Run()
{
	if (IsScanned() || IsRunning()) return B_OK;

	BAutolock lock(fLock.Lock());
	if (IsScanned() || IsRunning()) return B_OK;

	PRINT(("Starting addon manager\n"));
	if (!fSearchPath) return B_NO_MEMORY;

#warning "Fix me BAddOnManager::Run"
//	status_t res = fSearchPath->StartHandling(BMessenger(fSearchPath));
//	if (res < B_OK) return res;

	fRunning = true;

	return B_OK;
}

bool BAddOnManager::IsRunning() const
{
	return fRunning;
}

static bool unload_addon_func(void* item)
{
	PRINT(("Unloading addon %p\n", item));
	static_cast<BAddOnHandle*>(item)->Flush();
	return false;
}

static bool force_unload_addon_func(void* item)
{
	PRINT(("Unloading addon %p\n", item));
	static_cast<BAddOnHandle*>(item)->Flush(B_FLUSH_KEEP_LOADED|B_FLUSH_DYNAMIC|B_FLUSH_REFERENCED);
	return false;
}

static bool free_addon_func(void* item)
{
	PRINT(("Deleting addon %p\n", item));
	static_cast<BAddOnHandle*>(item)->Release();
	return false;
}

static bool free_watcher_func(void* item)
{
#warning "Fix free_watcher_func"
//	delete static_cast<BMessenger*>(item);
	return false;
}

void BAddOnManager::Shutdown(bool force_unload)
{
	PRINT(("Shutting down BAddOnManager, with %ld add-ons\n", fAddOns.CountItems()));

	// Atomically grab the search path and remove it from this object.
	fLock.Lock();
	AddOnSearchPath* sp = fSearchPath;
	fSearchPath = NULL;
	fLock.Unlock();

	if (sp)
	{
		sp->Stop();
		sp->DetachManager();
		sp->Release();

		{
			BAutolock lock(fLock.Lock());
#warning "I hate this"
//			BList addons = fAddOns;
//			fAddOns.MakeEmpty();
//			if (force_unload)
//				addons.DoForEach(force_unload_addon_func);
//			else
//				addons.DoForEach(unload_addon_func);
//			addons.DoForEach(free_addon_func);
//			fWatchers.DoForEach(free_watcher_func);
//			fWatchers.MakeEmpty();
		}

		fScanned = false;
		fRunning = false;
	}
	else
	{
		// Work around a potential race condition here:  If two
		// different threads call Shutdown(), one could still be
		// in the process of shutting down the AddOnSearchPath
		// while the other blazes through.  So wait here until it
		// is completely finished.
		while (fRunning || fScanned) sleep(20000);
	}
}

#warning "Fix me BAddOnManager::StartWatching"
//status_t BAddOnManager::StartWatching(BMessenger receiver)
//{
//	BAutolock l(Locker());
//
//	if (!IsRunning()) return B_NO_INIT;
//
//	const int32 N = fWatchers.CountItems();
//	for (int32 i=0; i<N; i++) {
//		BMessenger* m = (BMessenger*)fWatchers.ItemAt(i);
//		if (m && *m == receiver) return B_OK;
//	}
//
//	fWatchers.AddItem(new BMessenger(receiver));
//	return B_OK;
//}

#warning "Fix me BAddOnManager::StopWatching"
//status_t BAddOnManager::StopWatching(BMessenger receiver)
//{
//	BAutolock lock(fLock.Lock());
//
//	if (!IsRunning()) return B_NO_INIT;
//
//	const int32 N = fWatchers.CountItems();
//	for (int32 i=0; i<N; i++)
//	{
//		BMessenger* m = (BMessenger*)fWatchers.ItemAt(i);
//		if (m && *m == receiver)
//		{
//			fWatchers.RemoveItem(i);
//			delete m;
//			return B_OK;
//		}
//	}
//
//	return B_NAME_NOT_FOUND;
//}

int32 BAddOnManager::CountAddOns() const
{
	return fAddOns.CountItems();
}

BAddOnHandle* BAddOnManager::AddOnAt(int32 i) const
{
	BAutolock lock(fLock.Lock());
	BAddOnHandle *handle = static_cast<BAddOnHandle*>(fAddOns.ItemAt(i));
	return handle;
}

BAddOnHandle* BAddOnManager::FindAddOn(const node_ref* node) const
{
	if (!node) return NULL;

	BAutolock lock(fLock.Lock());

	for (int32 i = 0 ; i < fAddOns.CountItems() ; i++)
	{
		BAddOnHandle *handle = AddOnAt(i);
		if (handle && handle->NodeRef() == *node)
			return handle;
	}

	return NULL;
}

int32 BAddOnManager::IndexOfAddon(const BAddOnHandle *handle) const
{		
	BAutolock lock(fLock.Lock());

	for (int32 i = 0 ; i < fAddOns.CountItems() ; i++)
	{
		PRINT(("Comparing (%p)@%ld with (%p)\n", AddOnAt(i), i, handle));
		if (AddOnAt(i) == handle)
			return i;
	}
	
	return -1;
}

BAddOnHandle* BAddOnManager::InstallAddOn(const entry_ref* entry, const node_ref* node)
{
	BAddOnHandle* addon = InstantiateHandle(entry, node);
	if (addon) InstallAddOn(addon);
	return addon;
}

void BAddOnManager::InstallAddOn(BAddOnHandle* addon)
{
	if (addon)
	{
		fLock.Lock();
		addon->Acquire();
		fAddOns.AddItem(addon);
		fLock.Unlock();

		mark_changed();
	}
}

bool BAddOnManager::RemoveAddOn(BAddOnHandle *addon)
{
	fLock.Lock();
	bool res = false;
#warning "Fix BAddOnManager::RemoveAddOn"
//	if (addon)
//	{
//		res = fAddOns.RemoveItemsAt(addon);
//		if (res)
//		{
//			addon->Release();
//			mark_changed();
//		}
//	}
	fLock.Unlock();

	if (res) mark_changed();
	return res;
}

BLocker* BAddOnManager::Locker() const
{
	return &fLock;
}

size_t BAddOnManager::GetMemoryUsage() const
{
	BAutolock lock(fLock.Lock());

	size_t amount = 0;
	for (int32 i = fAddOns.CountItems()-1 ; i >= 0 ; i--)
	{
		BAddOnHandle *handle = const_cast<BAddOnManager*>(this)->AddOnAt(i);
		if (handle) amount += handle->GetMemoryUsage();
	}

	return amount;
}

void BAddOnManager::UsingAddOn(int32 i)
{
	// NOTE: Since we are doing the easy thing and using a BList
	// to manage the add-ons, this is pretty brain-dead.  But for
	// a small set of add-ons, it shouldn't be a problem.
	// In the future, maybe the add-on handlers should be stored in
	// a doubly-linked list.
#warning "fix me BAddOnManager::UsingAddOn"
//	if (i > 0)
//	{
//		BAutolock lock(fLock.Lock());
//		fAddOns.MoveItem(i, 0);
//	}
}

ssize_t BAddOnManager::PruneAddOnMemory(ssize_t memory_needed)
{
	BAutolock lock(fLock.Lock());

	PRINT(("Attempting to prune %.1fk of addon memory.\n", (float) memory_needed / 1024));
	for (int32 i = fAddOns.CountItems()-1 ; i >= 0 ; i--)
	{
		BAddOnHandle *handle = AddOnAt(i);
		if (handle && handle->IsLoaded() && !handle->KeepLoaded() && !handle->IsDynamic())
		{
			const ssize_t amount = handle->GetMemoryUsage();
			const bool flushed = handle->Flush();
			PRINT(("Attempted to prune %p (%s): amount=%.1fk, flushed=%ld\n",
			        handle, handle->EntryRef().name, (float) amount / 1024, (int32)flushed));

			if (flushed)
			{
				PRINT(("Pruned add-on %p (%s), using %.1fk\n",
				        handle, handle->EntryRef().name, (float) amount / 1024));
				if (memory_needed != -1)
				{
					if (amount > memory_needed) return 0;
					memory_needed -= amount;
				}
			}

		}
	}

	return memory_needed;
}

int32 BAddOnManager::PruneAddOnTime(time_t min_seconds, int32 growth)
{
	BAutolock lock(fLock.Lock());

	const time_t base_seconds = min_seconds;
	int32 count = 0;

	PRINT(("Attempting to prune add-ons at least %ld secs old.\n", min_seconds));
	for (int32 i = fAddOns.CountItems()-1 ; i >= 0 ; i--)
	{
		BAddOnHandle *handle = AddOnAt(i);
		if (handle && handle->IsLoaded() && !handle->KeepLoaded() && !handle->IsDynamic())
		{
			const time_t age = handle->SecondsSinceOpen();
			const bool flushed = age >= min_seconds ? handle->Flush() : false;
			PRINT(("Thought about pruning %p (%s): age=%ld, min_seconds=%ld, flushed=%ld\n",
			        handle, handle->EntryRef().name, age, min_seconds, (int32)flushed));

			if (flushed)
			{
				PRINT(("Pruned add-on %p (%s) at index %ld with age %ld (min=%ld)\n",
				        handle, handle->EntryRef().name, i, age, min_seconds));
				count++;
			}

			min_seconds += (base_seconds*growth)/100;
		}
	}

	PRINT(("Pruned %ld add-ons\n", count));

	return count;
}

BAddOnHandle* BAddOnManager::InstantiateHandle(const entry_ref* entry, const node_ref* node)
{
	return new BAddOnHandle(entry, node);
}

void BAddOnManager::mark_changed(bool needWakeup)
{
#warning "Fix BAddOnManager::mark_changed"
//	if (IsRunning() && atomic_or(&fChanged, 1) == 0 && needWakeup)
//	{
//		fLock.Lock();
//		BMessenger m(fSearchPath);
//		fLock.Unlock();
//		m.SendMessage(B_ADD_ONS_CHANGED);
//	}
}


void BAddOnManager::send_notices()
{
#warning "Fix BAddOnManager::send_notices"
//	if (atomic_and(&fChanged, 0) != 0)
//	{
//		BAutolock l(Locker());
//		BValue msg(B_ADD_ONS_CHANGED);
//		const int32 N = fWatchers.CountItems();
//		int32 j=0;
//		for (int32 i=0; i<N; i++)
//		{
//			BMessenger* m = (BMessenger*)fWatchers.ItemAt(i);
//			if (m && m->IsValid())
//			{
//				m->SendMessage(&msg);
//				if (j < i) fWatchers.ReplaceItemAt(j, fWatchers.ItemAt(i));
//				j++;
//			}
//			else
//			{
//				delete m;
//			}
//		}
//		if (j < N) fWatchers.RemoveItems(j, N-j);
//	}
}

// -------------------------------- FBC --------------------------------

// #pragma mark -
void BAddOnHandle::_HoldTheAddOnHandle1() {}
void BAddOnHandle::_HoldTheAddOnHandle2() {}
void BAddOnHandle::_HoldTheAddOnHandle3() {}
void BAddOnHandle::_HoldTheAddOnHandle4() {}
void BAddOnHandle::_HoldTheAddOnHandle5() {}
void BAddOnHandle::_HoldTheAddOnHandle6() {}
void BAddOnHandle::_HoldTheAddOnHandle7() {}
void BAddOnHandle::_HoldTheAddOnHandle8() {}
void BAddOnHandle::_HoldTheAddOnHandle9() {}
void BAddOnHandle::_HoldTheAddOnHandle10() {}
void BAddOnHandle::_HoldTheAddOnHandle11() {}
void BAddOnHandle::_HoldTheAddOnHandle12() {}
void BAddOnHandle::_HoldTheAddOnHandle13() {}
void BAddOnHandle::_HoldTheAddOnHandle14() {}
void BAddOnHandle::_HoldTheAddOnHandle15() {}
void BAddOnHandle::_HoldTheAddOnHandle16() {}
// #pragma mark -
void BAddOnManager::_AmyTheAddOnManager1() {}
void BAddOnManager::_AmyTheAddOnManager2() {}
void BAddOnManager::_AmyTheAddOnManager3() {}
void BAddOnManager::_AmyTheAddOnManager4() {}
void BAddOnManager::_AmyTheAddOnManager5() {}
void BAddOnManager::_AmyTheAddOnManager6() {}
void BAddOnManager::_AmyTheAddOnManager7() {}
void BAddOnManager::_AmyTheAddOnManager8() {}
void BAddOnManager::_AmyTheAddOnManager9() {}
void BAddOnManager::_AmyTheAddOnManager10() {}
void BAddOnManager::_AmyTheAddOnManager11() {}
void BAddOnManager::_AmyTheAddOnManager12() {}
void BAddOnManager::_AmyTheAddOnManager13() {}
void BAddOnManager::_AmyTheAddOnManager14() {}
void BAddOnManager::_AmyTheAddOnManager15() {}
void BAddOnManager::_AmyTheAddOnManager16() {}
