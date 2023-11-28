// SourceList.cpp

#include "SourceList.h"

#include <ByteOrder.h>

#include <MediaRoster.h>
#include <MediaNode.h>
#include <MediaAddOn.h>

#include <cstdlib>
#include <cstdio>

using namespace BPrivate;

struct _source_list_item_wrapper {
	_source_list_item_wrapper() {}
	_source_list_item_wrapper(
		const char* _name, const media_format* _format, const source_list_item* _item) :
		name(_name),
		format(*_format),
		item(*_item) {}

	BString name;
	media_format format;
	source_list_item item;
};

//#pragma mark ctor/dtor

BSourceList::BSourceList() :
	_mSources(new BList()) {}
	
BSourceList::~BSourceList() {
	_clearSources();
	delete _mSources;
}

//#pragma mark operations


// populate list with sources matching this format
// [[[Desktop]]]

status_t BSourceList::Fetch(
	const media_format& format,
	bool physicalSourcesOnly) {

	status_t err;

	BMediaRoster* r = BMediaRoster::Roster();
	_clearSources();

	// scan live nodes
	int32 liveNodesFound;
	int32 liveNodeBufSize = 16;
	live_node_info* liveNodeBuf = 0;
	
	BList liveNodesAddons;
	
	// +++++ hardcoded max number of outputs supported +++++
	const int32 outputBufSize = 16;
	media_output outputBuf[outputBufSize];
	
	do {
		// [re-]allocate buffer
		if(liveNodeBuf) {
			liveNodeBufSize <<= 2;
			liveNodeBuf = (live_node_info*)realloc(
				liveNodeBuf,
				liveNodeBufSize*sizeof(live_node_info));
		}
		else
			liveNodeBuf = (live_node_info*)malloc(
				liveNodeBufSize*sizeof(live_node_info));

		liveNodesFound = liveNodeBufSize;
		uint64 kinds = B_BUFFER_PRODUCER;
		if(physicalSourcesOnly)
			kinds |= B_PHYSICAL_INPUT;
		err = r->GetLiveNodes(
			liveNodeBuf,
			&liveNodesFound,
			0,
			&format,
			0,
			kinds);
		if(err < B_OK) {
			liveNodesFound = 0;
			break;
		}
	} while(liveNodesFound == liveNodeBufSize);
	
	// walk found nodes, picking matching outputs from each
	for(int32 nodeIndex = 0; nodeIndex < liveNodesFound; ++nodeIndex) {
		int32 outputsFound;
		err = r->GetFreeOutputsFor(
			liveNodeBuf[nodeIndex].node,
			outputBuf,
			outputBufSize,
			&outputsFound);
		if(err < B_OK || !outputsFound)
			continue;
		
		bool outputsMatched = false;

		for(int32 outputIndex = 0; outputIndex < outputsFound; ++outputIndex) {
			if(!format_is_compatible(outputBuf[outputIndex].format, format))
				continue;

			outputsMatched = true;
					
			// describe the node/output
			source_list_item item;
			item.type = B_SLIT_LIVE_NODE;
			item.live.node_id = liveNodeBuf[nodeIndex].node.node;
			item.live.kind = liveNodeBuf[nodeIndex].node.kind;
			item.live.source_id = outputBuf[outputIndex].source.id;
	
			BString name;
			if(*liveNodeBuf[nodeIndex].name)
				name << liveNodeBuf[nodeIndex].name;
			if(*outputBuf[outputIndex].name) {
				if(name.Length())
					name << ": ";
				name << outputBuf[outputIndex].name;
			}
			AddItem(
				name.String(),
				&outputBuf[outputIndex].format,
				&item);
		}
			
		// fetch dormant-node info if any
		dormant_node_info dormantInfo;
		err = r->GetDormantNodeFor(
			liveNodeBuf[nodeIndex].node,
			&dormantInfo);
		if(err == B_OK)
			liveNodesAddons.AddItem(new dormant_node_info(dormantInfo));
	}
	
	// scan dormant nodes; skip those matching flavors in liveNodesAddons
	int32 dormantNodesFound;
	int32 dormantNodeBufSize = 16;
	dormant_node_info* dormantNodeBuf = 0;

	do {
		// [re-]allocate buffer
		if(dormantNodeBuf) {
			dormantNodeBufSize <<= 2;
			dormantNodeBuf = (dormant_node_info*)realloc(
				dormantNodeBuf,
				dormantNodeBufSize*sizeof(dormant_node_info));
		}
		else
			dormantNodeBuf = (dormant_node_info*)malloc(
				dormantNodeBufSize*sizeof(dormant_node_info));

		dormantNodesFound = dormantNodeBufSize;
		uint64 kinds = B_BUFFER_PRODUCER | B_PHYSICAL_INPUT;

		err = r->GetDormantNodes(
			dormantNodeBuf,
			&dormantNodesFound,
			0,
			&format,
			0,
			kinds);
		if(err < B_OK) {
			dormantNodesFound = 0;
			break;
		}
	} while(dormantNodesFound == dormantNodeBufSize);

	for(int32 addonIndex = 0; addonIndex < dormantNodesFound; ++addonIndex) {
		int32 i;
		dormant_node_info* current = &dormantNodeBuf[addonIndex];
		for(i = 0; i < liveNodesAddons.CountItems(); ++i) {
			dormant_node_info* existing = (dormant_node_info*)liveNodesAddons.ItemAt(i);
			if(
				existing->addon == current->addon &&
				existing->flavor_id == current->flavor_id)
				break;
		}
		if(i < liveNodesAddons.CountItems())
			// found live node corresponding to this add-on flavor
			continue;
		
		// describe dormant node
		source_list_item item;
		item.type = B_SLIT_DORMANT_NODE;
		item.dormant.addon = current->addon;
		item.dormant.flavor_id = current->flavor_id;

		// fetch flavor description
		dormant_flavor_info flavorInfo;
		err = r->GetDormantFlavorInfoFor(
			*current,
			&flavorInfo);
		if(err < B_OK)
			continue;
			
		// find matching flavor
		media_format flavorFormat;
		for(i = 0; i < flavorInfo.out_format_count; ++i) {
			if(format_is_compatible(flavorInfo.out_formats[i], format)) {
				flavorFormat = flavorInfo.out_formats[i];
				break;
			}
		}
		if(i == flavorInfo.out_format_count)
			// no matching formats found -- the add-on lied
			continue;

		AddItem(
			current->name,
			&flavorFormat,
			&item);
	}
	
	// clean up
	free(liveNodeBuf);	
	free(dormantNodeBuf);	
	for(int32 i = 0; i < liveNodesAddons.CountItems(); ++i)
		delete (dormant_node_info*)liveNodesAddons.ItemAt(i);
	
	return B_OK;
}


status_t BSourceList::GetItemAt(
	int32 index,
	BString* outName,
	media_format* outFormat,
	source_list_item* outItem) const {
	
	if(index < 0 || index >= _mSources->CountItems())
		return B_BAD_INDEX;
	
	_source_list_item_wrapper* i =
		(_source_list_item_wrapper*)_mSources->ItemAt(index);

	if(outName)
		*outName = i->name;
	if(outFormat)
		*outFormat = i->format;
	if(outItem)
		*outItem = i->item;
	return B_OK;
}

status_t BSourceList::FindItem(
	const char* name,
	int32* outIndex) const {

	int32 c = _mSources->CountItems();
	for(int32 i = 0; i < c; ++i) {
		if(((_source_list_item_wrapper*)_mSources->ItemAt(i))->name == name) {
			*outIndex = i;
			return B_OK;
		}
	}
	
	return B_NAME_NOT_FOUND;
}

int32 BSourceList::CountItems() const {
	return _mSources->CountItems();
}

bool BSourceList::IsEmpty() const {
	return _mSources->IsEmpty();
}

//#pragma mark internal ops

void BSourceList::AddItem(
	const char* name,
	const media_format* format,
	const source_list_item* item) {

	_mSources->AddItem(
		(void*)new _source_list_item_wrapper(name, format, item));
}

void BSourceList::_clearSources() {

	int32 c = _mSources->CountItems();
	for(int32 i = 0; i < c; ++i) {
		_source_list_item_wrapper* item =
			(_source_list_item_wrapper*)_mSources->ItemAt(i);
		delete item;
	}
	_mSources->MakeEmpty();
}


// END -- SourceList.cpp --
