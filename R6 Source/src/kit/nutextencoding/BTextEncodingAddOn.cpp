
#include "BTextEncodingAddOn.h"
#include <add-ons/textencoding/BTextCodec.h>

#include <FindDirectory.h>
#include <Application.h>

using namespace B::TextEncoding;


BTextEncodingAddOn::BTextEncodingAddOn(const entry_ref *entry, const node_ref *node) :
	BAddOnHandle(entry, node),
	fMakeCodec(NULL)
{
}


BTextEncodingAddOn::~BTextEncodingAddOn()
{
}

BTextCodec *
BTextEncodingAddOn::MakeCodec(const char *encoding)
{
	BTextCodec *codec = NULL;
	
	if (!fMakeCodec)
		LoadFunc();
		
	if (fMakeCodec) {
		codec = (*fMakeCodec)(encoding, this);
		Open();
	}
		
	return codec;
}

bool 
BTextEncodingAddOn::HasCodec(const char *encoding)
{
	if (GetIdentifiers(0, true) != B_OK)
		return false;
	bool found = false;
	const char *cur = NULL;
	const BMessage *identifiers = LockIdentifiers();
	for (int32 ix = 0; identifiers->FindString("codec", ix, &cur) == B_OK; ix++) {
		if (strcasecmp(cur, encoding) == 0) {
			found = true;
			break;
		}
	}
	UnlockIdentifiers(identifiers);
	return found;
}

void 
BTextEncodingAddOn::ImageUnloading(image_id)
{
	fMakeCodec = NULL;
}

const char *
BTextEncodingAddOn::AttrBaseName() const
{
	return "be:textencoding";
}

void 
BTextEncodingAddOn::LoadFunc()
{
	image_id id = Open();
	if (id >= B_OK) {
		void *mk_codec;
		if (get_image_symbol(id, "make_codec", B_SYMBOL_TYPE_TEXT, &mk_codec) == B_OK) {
			fMakeCodec = (make_codec_t)mk_codec;
		}
	}
}


// #pragma mark -

static BTextEncodingManager gDefaultMgr;


BTextEncodingManager::BTextEncodingManager() :
	BAddOnManager("TextEncoding Mgr")
{
}


BTextEncodingManager::~BTextEncodingManager()
{
}

BTextEncodingManager &
BTextEncodingManager::Default()
{
	if (!gDefaultMgr.IsScanned()) {
		
//		if (be_app)
			gDefaultMgr.AddEnvVar("ADDON_PATH", "textencoding");
//		else {
//			gDefaultMgr.AddDirectory(B_BEOS_ADDONS_DIRECTORY, "textencoding");
//			gDefaultMgr.AddDirectory(B_COMMON_ADDONS_DIRECTORY, "textencoding");
//		}
		gDefaultMgr.Scan();	
	}
	return gDefaultMgr;
}

BTextCodec *
BTextEncodingManager::MakeCodec(const char *encoding)
{
	BTextCodec *codec = NULL;
	int32 count = CountAddOns();
	for (int32 ix = 0; ix < count; ix++) {
		BTextEncodingAddOn * addOn = dynamic_cast<BTextEncodingAddOn *>(AddOnAt(ix));
		if (addOn->HasCodec(encoding)) {
			codec = addOn->MakeCodec(encoding);
			break;
		}
	}
	return codec;
}


BAddOnHandle *
BTextEncodingManager::InstantiateHandle(const entry_ref *entry, const node_ref *node)
{
	return new BTextEncodingAddOn(entry, node);
}

