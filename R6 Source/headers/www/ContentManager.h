#ifndef _CONTENT_MANAGER_H
#define _CONTENT_MANAGER_H

#ifndef _ADD_ON_MANAGER_H
#include <AddOnManager.h>
#endif

#ifndef _CONTENT_H
#include <Content.h>
#endif

#ifndef _PROTOCOL_H
#include <Protocol.h>
#endif

class BString;

namespace Wagner {

class ContentHandle : public BAddOnHandle
{
public:
	// Content add-on handle.
	ContentHandle(const entry_ref* entry=NULL, const node_ref* node=NULL);
	virtual ~ContentHandle();
	
	virtual bool KeepLoaded() const;
	virtual size_t GetMemoryUsage() const;
	
	Content* InstantiateContent(const char* mime_type,
								const char* extension=0);
	Protocol* InstantiateProtocol(const char* scheme);
	
protected:
	status_t LoadIdentifiers(BMessage* into, image_id from);
	virtual void ImageUnloading(image_id image);
	virtual const char* AttrBaseName() const;
	
private:
	ContentFactory* MakeContentFactory(image_id from, int32 index) const;
	ProtocolFactory* MakeProtocolFactory(image_id from, int32 index) const;
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
									 
	Content* InstantiateContent(const char* mime_type,
								const char* extension=0);
	Protocol* InstantiateProtocol(const char* scheme);
	
	void GetUserInfo(BString* into);
	
	static ContentManager& Default();
	
protected:
	virtual BAddOnHandle* InstantiateHandle(const entry_ref* entry, const node_ref* node);
};

};

#endif
