#include <Autolock.h>
#include <String.h>

#include <Debug.h>

#include "ContentManager.h"
#include "Content.h"
#include "util.h"

#include <new>
#include <stdlib.h>

using namespace Wagner;
using std::nothrow;

enum {
	ADDON_TYPE_UNKNOWN = 0,
	ADDON_TYPE_CONTENT = 1,
	ADDON_TYPE_PROTOCOL = 2
};

struct Identifier {
	BString name;
	BString value;
	Identifier *next;
	BAddOnHandle *handle;
};

const int kIdentifierHashSize = 389;
static Identifier *gIdentifierHash[kIdentifierHashSize];

inline void IdentifierHash(const char *name, const char *value, BAddOnHandle *handle)
{
	unsigned bucket = (HashStringI(name) ^ HashStringI(value)) % kIdentifierHashSize;
	Identifier *identifier = new Identifier;
	identifier->name = name;
	identifier->value = value;
	identifier->handle = handle;	
	identifier->next = gIdentifierHash[bucket];
	gIdentifierHash[bucket] = identifier;
}

inline BAddOnHandle* IdentifierLookup(const char *name, const char *value)
{
	unsigned bucket = (HashStringI(name) ^ HashStringI(value)) % kIdentifierHashSize;
	for (Identifier *identifier = gIdentifierHash[bucket]; identifier; identifier =
		identifier->next) {
		if (identifier->value.ICompare(value) == 0 && identifier->name.ICompare(name) == 0)
			return identifier->handle;
	}
	
	return 0;
}


// --------------------------- ContentHandle ---------------------------

ContentHandle::ContentHandle(const entry_ref* entry, const node_ref* node)
	: BAddOnHandle(entry, node),
	  fAddOnType(ADDON_TYPE_UNKNOWN),
	  fContentFactory(0), fProtocolFactory(0),
	  fAlwaysKeepLoaded(false), fRegistered(false)
{
}

ContentHandle::~ContentHandle()
{
	ImageUnloading(B_ERROR);
}

bool ContentHandle::KeepLoaded() const
{
	if( fContentFactory && fContentFactory->KeepLoaded() ) return true;
	if( fProtocolFactory && fProtocolFactory->KeepLoaded() ) return true;
	return fAlwaysKeepLoaded;
}

size_t ContentHandle::GetMemoryUsage() const
{
	return BAddOnHandle::GetMemoryUsage()
		+ ( fContentFactory ? fContentFactory->GetMemoryUsage() : 0 )
		+ ( fProtocolFactory ? fProtocolFactory->GetMemoryUsage() : 0 );
}

Content* ContentHandle::InstantiateContent(const char* mime_type,
										   const char* extension)
{
	if( fAddOnType != ADDON_TYPE_UNKNOWN && fAddOnType != ADDON_TYPE_CONTENT ) {
		return 0;
	}
	
	image_id image = Open();
	if( image < B_OK ) return 0;
	
	// Short-cut -- already found this is an old style, so don't try
	// to make the factory object.
	if( fRegistered ) return 0;
	
	if( !fContentFactory ) fContentFactory = MakeContentFactory(image, 0);
	
	if( fContentFactory ) {
		fAddOnType = ADDON_TYPE_CONTENT;
		fAlwaysKeepLoaded = false;
		return fContentFactory->CreateContent(this, mime_type, extension);
	} else {
		// This is probably an old-style content, try to register it.
		DoRegister(0, image);
	}
	
	return 0;
}

Protocol* ContentHandle::InstantiateProtocol(const char* scheme)
{
	if( fAddOnType != ADDON_TYPE_UNKNOWN && fAddOnType != ADDON_TYPE_PROTOCOL ) {
		return 0;
	}
	
	image_id image = Open();
	if( image < B_OK ) return 0;
	
	if( !fProtocolFactory ) fProtocolFactory = MakeProtocolFactory(image, 0);
	if( !fProtocolFactory ) return 0;
	
	fAddOnType = ADDON_TYPE_PROTOCOL;
	fAlwaysKeepLoaded = false;
	return fProtocolFactory->CreateProtocol(this, scheme);
}

status_t ContentHandle::LoadIdentifiers(BMessage* into, image_id from)
{
	ContentFactory* cfactory = MakeContentFactory(from, 0);
	if( cfactory ) {
		fAddOnType = ADDON_TYPE_CONTENT;
		cfactory->GetIdentifiers(into);
		if( cfactory->KeepLoaded() ) fAlwaysKeepLoaded = true;
		delete cfactory;
		return B_OK;
	}

	ProtocolFactory* pfactory = MakeProtocolFactory(from, 0);
	if( pfactory ) {
		fAddOnType = ADDON_TYPE_PROTOCOL;
		pfactory->GetIdentifiers(into);
		if( pfactory->KeepLoaded() ) fAlwaysKeepLoaded = true;
		delete pfactory;
		into->AddBool("be:protocol", true);
		return B_OK;
	}

	return DoRegister(into, from);
}

void ContentHandle::ImageUnloading(image_id image)
{
	(void)image;
	delete fContentFactory;
	fContentFactory = 0;
	delete fProtocolFactory;
	fProtocolFactory = 0;
}

const char* ContentHandle::AttrBaseName() const
{
	return "be:content";
}

ContentFactory* ContentHandle::MakeContentFactory(image_id from, int32 ) const
{
	PRINT(("Looking for make_nth_content func...\n"));
	void *maker;
	if(get_image_symbol(from, "make_nth_content", B_SYMBOL_TYPE_TEXT, &maker) == B_OK) {
		return (*((make_nth_content_type)maker))(0, from, 0);
	} else {
		PRINT(("Unable to find make_nth_content func in image %ld\n", from));
	}
	return 0;
}


ProtocolFactory* ContentHandle::MakeProtocolFactory(image_id from, int32 ) const
{
	PRINT(("Looking for make_nth_protocol func...\n"));
	void *maker;
	if(get_image_symbol(from, "make_nth_protocol", B_SYMBOL_TYPE_TEXT, &maker) == B_OK) {
		return (*((make_nth_protocol_type)maker))(0, from, 0);
	} else {
		PRINT(("Unable to find make_nth_protocol func in image %ld\n", from));
	}
	return 0;
}

status_t ContentHandle::DoRegister(BMessage* into, image_id from)
{
	if( fRegistered ) return B_OK;
	
	fRegistered = true;
	
	PRINT(("Looking for register_addons func...\n"));
	void (*register_addons_func)();
	status_t err = get_image_symbol(from, "register_addons", B_SYMBOL_TYPE_TEXT,
									(void**) &register_addons_func);
	if( err == B_OK ) {
		BMessage dummy;
		
		// Ugliness to figure out which MIME types this old-style
		// add-on supports...
		Content::GetRecentlyRegistered(&dummy);
		register_addons_func();
		if( !into ) {
			into = &dummy;
			dummy = BMessage();
		}
		Content::GetRecentlyRegistered(into);
		#if DEBUG
		PRINT(("Found content: ")); into->PrintToStream();
		#endif
		
		// And now, the add-on needs to stay in memory for the
		// rest of the program's life.
		Open();
	}
	
	return err;
}

// --------------------------- ContentManager ---------------------------

static BLocker gAccess;
static ContentManager* gDefault = NULL;

ContentManager::ContentManager()
	: BAddOnManager("WWW ContentManager")
{
}

ContentManager::~ContentManager()
{
}

ContentHandle* ContentManager::FindContentHandle(const char* name,
												 const char* value,
												 bool ,
												 bool )
{
	BAutolock l(Locker());

	return static_cast<ContentHandle*>(IdentifierLookup(name, value));

#if 0	
	const int32 NUM = CountAddOns();
	for( int32 i=0; i<NUM; i++ ) {
		ContentHandle* ch = dynamic_cast<ContentHandle*>(AddOnAt(i));
		if( ch->MatchIdentifier(name, value, quick) ) {
			if( will_use ) UsingAddOn(i);
			return ch;
		}
	}
	
	return 0;
#endif
}

Content* ContentManager::InstantiateContent(const char* mime_type,
											const char* extension)
{
	BAutolock l(Locker());
	PRINT(("Looking for content add-on: mime=%s, ext=%s\n",
			mime_type, extension));
	ContentHandle* ch = 0;

	//
	//	A little black magic.
	//	Determining which content handler to use is a little more
	// 	complex than it might seem.
	//
	
	// 1. application/octet-stream and text/plain can be returned
	// if a server doesn't recognize a file extension.  Also, the server
	// may not return a type at all.  In this case, try to use the file
	// extension to determine the type.  Note that it may actually be
	// text/plain, in which case that type will be used below if the
	// extension can't be matched.
	if (strcasecmp(mime_type, "application/octet-stream") == 0
		|| strcasecmp(mime_type, "text/plain") == 0
		|| strlen(mime_type) == 0) {
		if (extension) {
			if (strcasecmp(mime_type, "application/octet-stream") != 0) {
				// HACK: Avoid these extensions, because they will just show
				//  garbage if we fall through to "text/html" later on.
				const char *kDenyExtensions[] = {
					"arj", "bin", "sea", "rar", "sit", "exe", "wma",
					"mov", "avi", "m2v", "wmv", 0
				};
	
				for (const char **checkExt = kDenyExtensions; *checkExt; checkExt++)
					if (strcasecmp(*checkExt, extension) == 0) 
						return 0;
			}
			
			ch = FindContentHandle(S_CONTENT_EXTENSIONS, extension, false);
			if (!ch)
				ch = FindContentHandle(S_CONTENT_EXTENSIONS, extension, true);
		}
	}

	// 2. Ok, try to find a content handler by mime type.  Look through the
	// loaded handlers first (the false flag).
	if (!ch)
		ch = FindContentHandle(S_CONTENT_MIME_TYPES, mime_type, false);

	// 3. Try by extension (Note that I may have tried already in case 1,
	//  but do it again here just in case I didn't).
	if (!ch && extension)
		ch = FindContentHandle(S_CONTENT_EXTENSIONS, extension, false);

	// 4. Try by content type again, but this time load new add-ons.
	if (!ch)
		ch = FindContentHandle(S_CONTENT_MIME_TYPES, mime_type, true);

	// 5. Try by extension, but load add-ons.
	if (!ch && extension)
		ch = FindContentHandle(S_CONTENT_EXTENSIONS, extension, true);

	// If the content type wasn't specified and we couldn't guess a type
	// based on extension, default to HTML.
	if (!ch && strlen(mime_type) == 0)
		ch = FindContentHandle(S_CONTENT_MIME_TYPES, "text/html", true);	

	if (!ch) {
		PRINT(("Unable to find content add-on for: mime=%s, extension=%s\n",
				mime_type ? mime_type : "--",
				extension ? extension : "--"));
		return 0;
	}
	
	return ch->InstantiateContent(mime_type, extension);
}

Protocol* ContentManager::InstantiateProtocol(const char* scheme)
{
	BAutolock l(Locker());
	
	PRINT(("Looking for protocol add-on: scheme=%s\n", scheme));
	ContentHandle* ch = FindContentHandle(S_PROTOCOL_SCHEMES, scheme, false);
	if( !ch ) ch = FindContentHandle(S_PROTOCOL_SCHEMES, scheme, true);
	if (!ch) {
		PRINT(("Unable to find protocol add-on for: scheme=%s\n",
				scheme ? scheme : "--"));
		return 0;
	}
	
	return ch->InstantiateProtocol(scheme);
}

#if ENABLE_LOG
void ContentManager::GetUserInfo(BString* into)
{
	BAutolock l(Locker());
	
	*into << "<TABLE BORDER=1>\n"
			 "<TR>\n"
			 "<TH>Name\n"
			 "<TH>MIME Types\n"
			 "<TH>Schemes\n"
			 "<TH>Extensions\n"
			 "<TH>Loaded?\n"
			 "<TH>Memory\n"
			 "<TH>Last Opened\n"
			 "</TR>\n";
	
	BString name, buffer, loaded, memory, seconds;
	
	const int32 NUM = CountAddOns();
	for( int32 i=0; i<NUM; i++ ) {
	
		ContentHandle* ch = dynamic_cast<ContentHandle*>(AddOnAt(i));
		if (!ch) debugger("bad content handle");
		
		*into << "<TR>\n"
			  << "<TD>" << escape_for_html(&name, ch->Ref().name, false);
			  
		BMessage ident;
		bool quick = true;
		if( ch->GetIdentifiers(&ident, true) != B_OK ) {
			quick = false;
			ch->GetIdentifiers(&ident, false);
		}
		
		const char* str;
		int32 j;
		
		*into << "\n<TD NOWRAP>";
		for( j=0; ident.FindString(S_CONTENT_MIME_TYPES, j, &str)==B_OK; j++ ) {
			if( j>0 ) *into << "<BR>";
			*into << escape_for_html(&buffer, str, false);
		}
		
		*into << "\n<TD NOWRAP>";
		for( j=0; ident.FindString(S_PROTOCOL_SCHEMES, j, &str)==B_OK; j++ ) {
			if( j>0 ) *into << "<BR>";
			*into << escape_for_html(&buffer, str, false);
		}
		
		*into << "\n<TD NOWRAP>";
		for( j=0; ident.FindString(S_CONTENT_EXTENSIONS, j, &str)==B_OK; j++ ) {
			if( j>0 ) *into << "<BR>";
			*into << escape_for_html(&buffer, str, false);
		}
		
		*into << "\n<TD NOWRAP>";
		*into << (ch->IsLoaded() ? (quick ? (ch->KeepLoaded() ? "Always" : "Yes")
										  : "Just Now")
								 : "No");
		
		memory = "";
		memory << ch->GetMemoryUsage();
		*into << "\n<TD NOWRAP>" << memory;
		
		seconds = "";
		seconds << ch->SecondsSinceOpen();
		*into << "\n<TD NOWRAP>" << seconds << "s";
		
		*into << "\n</TR>\n";
	}
	
	*into << "</TABLE>\n";
}
#endif

ContentManager& ContentManager::Default()
{
	if (gDefault) return *gDefault;
	
	{
		BAutolock _l(gAccess);
		if (!gDefault) {
			ContentManager* def = new ContentManager;
			def->AddEnvVar("ADDON_PATH", "web");
			def->Scan();
			gDefault = def;
		}
	}
	
	return *gDefault;
}

BAddOnHandle* ContentManager::InstantiateHandle(const entry_ref* entry, const node_ref* node)
{
	ContentHandle *handle = new(nothrow) ContentHandle(entry, node);
	if (handle) {
		BMessage message;
		if (handle->GetIdentifiers(&message, false) != B_OK)
			return handle;
			
		for (int32 nameIndex = 0; nameIndex < message.CountNames(B_STRING_TYPE); nameIndex++) {
			const char *name;
			type_code type;
			int32 valueCount;
			message.GetInfo(B_STRING_TYPE, nameIndex, &name, &type, &valueCount);
			for (int32 valueIndex = 0; valueIndex < valueCount; valueIndex++)
				IdentifierHash(name, message.FindString(name, valueIndex), handle);
		}
	}
	
	return handle;
}
