#ifndef _CONTENT_MANAGER_H
#define _CONTENT_MANAGER_H

#include <storage2/AddOnManager.h>
#include <storage2/Entry.h>
#include <storage2/Node.h>

namespace B {
namespace WWW2 {

using namespace B::Storage2;
using namespace B::Support2;
class B::Support2::BString;

class Content;
class ContentFactory;
class Protocol;
class ProtocolFactory;

class ContentHandle : public BAddOnHandle
{
public:
	// Content add-on handle.
	ContentHandle(const entry_ref* entry=NULL, const node_ref* node=NULL);
	virtual ~ContentHandle();
	
	virtual bool KeepLoaded() const;
	virtual size_t GetMemoryUsage() const;
	
	Content* InstantiateContent(const char* mime_type, const char* extension=0);
	Protocol* InstantiateProtocol(const char* scheme);
	
protected:
	status_t LoadIdentifiers(BMessage* into, image_id from);
	virtual void ImageUnloading(image_id image);
	virtual const char* AttrBaseName() const;
	
private:
	ContentFactory* MakeContentFactory(image_id from, int32 index) const;
	ProtocolFactory* MakeProtocolFactory(image_id from, int32 ) const;
	
	status_t DoRegister(BMessage* into, image_id from);
	
	int32 fAddOnType;
	
	ContentFactory* fContentFactory;
	ProtocolFactory* fProtocolFactory;
	
	bool fAlwaysKeepLoaded;
	bool fRegistered;
};

class ContentManager : public BAddOnManager
{
public:
	ContentManager();
	virtual ~ContentManager();

	ContentHandle* FindContentHandle(const char* name, const char* value,
									 bool quick=false,
									 bool will_use=true);
									 
	Content* InstantiateContent(const char* mime_type, const char* extension=0);
	Protocol* InstantiateProtocol(const char* scheme);
	
	void GetUserInfo(BString* into);
	
	static ContentManager& Default();
	
protected:
	virtual BAddOnHandle* InstantiateHandle(const entry_ref* entry, const node_ref* node);
};

} } // namespace B::WWW2

#endif
