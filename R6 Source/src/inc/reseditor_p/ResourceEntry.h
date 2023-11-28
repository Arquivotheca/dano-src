#ifndef _RESOURCE_ENTRY_H
#define _RESOURCE_ENTRY_H

#include <List.h>

#include <ResourceEditor.h>
#include <Ref.h>

namespace BPrivate {
	class ResourceEntry : public BResourceItem, public BRefable
	{
	public:
		ResourceEntry(BResourceCollection* collection = 0);
		ResourceEntry(BResourceCollection* collection,
					  type_code type, int32 id, const char* name, const char* sym);
		ResourceEntry(BResourceCollection* collection, BMessage* from);
		ResourceEntry(const BResourceItem& data);
		
		void SetOwner(void* owner);
		void* Owner() const;
	
		void SetCollection(BResourceCollection* collection);
		BResourceCollection* Collection() const;
		
		status_t AddSubscriber(BResourceHandle* who);
		status_t RemSubscriber(BResourceHandle* who);
		
		void SetChangeOriginator(BResourceAddonBase* who);
		BResourceAddonBase* ChangeOriginator() const;
		
		virtual void NoteChange(uint32 what);
		virtual status_t ReportChange();
		
	protected:
		virtual ~ResourceEntry();
	
	private:
		void* fOwner;
		BResourceCollection* fCollection;
		BResourceAddonBase* fChangeOriginator;
		BList fSubscribers;				// list of BResourceHandle objs.
		bool fMultiOriginator;
	};
}	// namespace BPrivate

using namespace BPrivate;

#endif
