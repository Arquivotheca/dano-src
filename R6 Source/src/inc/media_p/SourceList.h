// SourceList.h

#if !defined(_SOURCE_LIST_H)
#define _SOURCE_LIST_H

#include <MediaDefs.h>
#include <String.h>

namespace BPrivate {

enum source_list_item_type {
	B_SLIT_UNKNOWN, 
	B_SLIT_LIVE_NODE, 
	B_SLIT_DORMANT_NODE, 
	B_SLIT_DEVICE 
}; 
 
struct source_list_item { 
	int32 type; 
	union { 
		struct {
			media_node_id node_id;
			uint32 kind;
			int32 source_id;
		} live;
		
		struct {
			media_addon_id addon;
			int32 flavor_id;
		} dormant;

		char device[128]; 
		char _reserved[252]; 
	}; 
}; 

class BSourceList {
public:
	BSourceList();
	virtual ~BSourceList();

	// populate list with sources matching this format
	status_t Fetch(
		const media_format& format,
		bool physicalSourcesOnly=true);

	status_t GetItemAt(
		int32 index,
		BString* outName,
		media_format* outFormat,
		source_list_item* outItem=0) const;
	
	status_t FindItem(
		const char* name,
		int32* outIndex) const;

	int32 CountItems() const;
	bool IsEmpty() const;

protected:
	void AddItem(
		const char* name,
		const media_format* format,
		const source_list_item* item);

private:	
	BList* const _mSources;
	
	void _clearSources();
};

}; // BPrivate

#endif //_SOURCE_LIST_H
