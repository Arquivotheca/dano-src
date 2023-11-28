/*
	Copyright 2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Debug.h>
#include <ByteOrder.h>
#include <Entry.h>
#include <Message.h>
#include <Entry.h>
#include <ByteOrder.h>
#include "DProxy.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Path.h>

extern "C" status_t DumpVariable(DProxy& proxy);

void
HexDump(const void *buf, long length)
{
	const int32 kBytesPerLine = 16;
	long offset;
	unsigned char *buffer = (unsigned char *)buf;

	for (offset = 0; ; offset += kBytesPerLine, buffer += kBytesPerLine) {
		long remain = length;
		int index;

		printf( "0x%06x: ", (int)offset);

		for (index = 0; index < kBytesPerLine; index++) {

			if (remain-- > 0)
				printf("%02x%c", buffer[index], remain > 0 ? ',' : ' ');
			else
				printf("   ");
		}

		remain = length;
		printf(" \'");
		for (index = 0; index < kBytesPerLine; index++) {

			if (remain-- > 0)
				printf("%c", buffer[index] > ' ' ? buffer[index] : '.');
			else
				printf(" ");
		}
		printf("\'\n");

		length -= kBytesPerLine;
		if (length <= 0)
			break;

	}
	fflush(stdout);
}


class TmpStorage {
public:
	TmpStorage()
		:	ptr(0)
		{}
	~TmpStorage()
		{ free(ptr); }
		
	void *Use(size_t size)
		{
			free(ptr);
			return malloc(size);
		}

private:
	void *ptr;
};

// cruft from Message.cpp

struct dyn_array {
	int32 fLogicalBytes;
	int32 fPhysicalBytes;
	int32 fChunkSize;
	int32 fCount;
	int32 fEntryHdrSize;	
};

struct entry_hdr : public dyn_array {
	entry_hdr *fNext;
	uint32 fType;
	uchar fNameLength;	
	char fName[1];
};

struct var_chunk {
	int32 fDataSize;				
	char fData[1];
};

// overlay to help walking a message
struct DebugMessagOverlay {
	uint32 what;
	BMessage *link;
	int32 fTarget;	
	BMessage *fOriginal;
	uint32 fChangeCount;
	int32 fCurSpecifier;
	uint32 fPtrOffset;
	uint32 _reserved[3];

	entry_hdr *fEntries;

	struct {
		port_id port;
		int32 target;
		team_id team;
		bool preferred;
	} fReplyTo;

	bool fPreferred;
	bool fReplyRequired;
	bool fReplyDone;
	bool fIsReply;
	bool fWasDelivered;
	bool fReadOnly;
	bool fHasSpecifiers;	
};

// more cruft from Message.cpp

#define convert(a) 	((a)?(entry_hdr *)(((uint32)(a)) + fPtrOffset) : 0)	

inline char *
da_start_of_data(DProxy& proxy, const dyn_array *realEntry, const dyn_array *mappedEntry,
	TmpStorage &tmp)
{
	uint32 alignedSize = ((mappedEntry->fChunkSize * mappedEntry->fCount + 3) >> 2) << 2;
	char *result = (char *)tmp.Use(alignedSize);
	
	if (proxy.ReadData((ptr_t)((char *)realEntry) + (sizeof(dyn_array) + mappedEntry->fEntryHdrSize),
		result, alignedSize) != B_OK) {
		printf("error reading entry data from addr %p, size %ld, entry %p, headerSize %ld\n",
			((char *) realEntry) + (sizeof(dyn_array) + mappedEntry->fEntryHdrSize),
			alignedSize, realEntry, mappedEntry->fEntryHdrSize);
		return 0;
	}
	return result;
}

inline var_chunk *
get_chunk(DProxy& proxy, const var_chunk *nextChunk, TmpStorage &tmp)
{
	var_chunk tmpVarChunk;
	if (proxy.ReadData((ptr_t)nextChunk, &tmpVarChunk, sizeof(var_chunk)) != B_OK)
		return 0;

	uint32 size = (((tmpVarChunk.fDataSize + sizeof(int32)) + 3) >> 2) << 2;

	char *result = (char *)tmp.Use(size);
	if (proxy.ReadData((ptr_t)nextChunk, result, size) != B_OK) {
		printf("error reading variable size entry data from addr %p, size %ld\n",
			nextChunk, size);
		return 0;
	}
	
	return (var_chunk *) result;
}

inline var_chunk *
da_first_chunk(DProxy& proxy, const dyn_array *realEntry, const dyn_array *mappedEntry,
	const var_chunk *&realChunk, TmpStorage &tmp)
{
	realChunk = (const var_chunk *)((char *)realEntry + sizeof(dyn_array) + mappedEntry->fEntryHdrSize);
	var_chunk *result = get_chunk(proxy, realChunk, tmp);
#if 0
	if (result) {
		printf("chunk at %x\n", realChunk);
		HexDump(result, 40);
	}
#endif
	return result;
}

inline var_chunk *
da_next_chunk(DProxy& proxy, const var_chunk *&realChunk, const var_chunk *mappedChunk, TmpStorage &tmp)
{
	realChunk = (const var_chunk *)((uint32)realChunk + ((mappedChunk->fDataSize + sizeof(int32) + 7) & ~7));
#if 0
	if (realChunk) 
		printf("reading chunk at %x\n", realChunk);
#endif
 	var_chunk *result = get_chunk(proxy, realChunk, tmp);
	return result;
}

inline int32
da_chunk_hdr_size()
{
	return sizeof(int32);
}

inline int32
da_chunk_size(var_chunk *v)
{
	return (v->fDataSize + da_chunk_hdr_size() + 7) & ~7;
}

status_t DumpVariable(DProxy& proxy)
{
	ptr_t addr = proxy.VariableAddress();
	
	if (proxy.IsPointer())
	{
		proxy.Dereference();
		addr = proxy.VariableAddress();
	}
	
	size_t size = proxy.VariableSize();
	void *p = malloc(size);
	if (p && proxy.ReadData (addr, p, size) == B_OK)
	{

		const char *spaces =
	//   11234567892123456789312345678941234567895123456789612345678961234567896
		"                                                                       ";
	
		int	sl = strlen(spaces);

		union {
			uint32 value;
			char string[4];
		} swappedValue;
		
		char buffer[256];

		DebugMessagOverlay *message = reinterpret_cast<DebugMessagOverlay *>(p);
		swappedValue.value = B_BENDIAN_TO_HOST_INT32(message->what);
		const char *messageWhat = 0;
		switch (message->what) {
			case B_ABOUT_REQUESTED:
				messageWhat = "B_ABOUT_REQUESTED";
				break;
			case B_WINDOW_ACTIVATED:
				messageWhat = "B_WINDOW_ACTIVATED";
				break;
//			case B_APP_ACTIVATED:
//				messageWhat = "B_APP_ACTIVATED";
//				break;
			case B_ARGV_RECEIVED:
				messageWhat = "B_ARGV_RECEIVED";
				break;
			case B_QUIT_REQUESTED:
				messageWhat = "B_QUIT_REQUESTED";
				break;
//			case B_CLOSE_REQUESTED:
//				messageWhat = "B_CLOSE_REQUESTED";
//				break;
			case B_CANCEL:
				messageWhat = "B_CANCEL";
				break;
			case B_KEY_DOWN:
				messageWhat = "B_KEY_DOWN";
				break;
			case B_KEY_UP:
				messageWhat = "B_KEY_UP";
				break;
			case B_MODIFIERS_CHANGED:
				messageWhat = "B_MODIFIERS_CHANGED";
				break;
			case B_MINIMIZE:
				messageWhat = "B_MINIMIZE";
				break;
			case B_MOUSE_DOWN:
				messageWhat = "B_MOUSE_DOWN";
				break;
			case B_MOUSE_MOVED:
				messageWhat = "B_MOUSE_MOVED";
				break;
			case B_MOUSE_ENTER_EXIT:
				messageWhat = "B_MOUSE_ENTER_EXIT";
				break;
			case B_MOUSE_UP:
				messageWhat = "B_MOUSE_UP";
				break;
			case B_OPEN_IN_WORKSPACE:
				messageWhat = "B_OPEN_IN_WORKSPACE";
				break;
			case B_PULSE:
				messageWhat = "B_PULSE";
				break;
			case B_READY_TO_RUN:
				messageWhat = "B_READY_TO_RUN";
				break;
			case B_REFS_RECEIVED:
				messageWhat = "B_REFS_RECEIVED";
				break;
			case B_SCREEN_CHANGED:
				messageWhat = "B_SCREEN_CHANGED";
				break;
			case B_VALUE_CHANGED:
				messageWhat = "B_VALUE_CHANGED";
				break;
			case B_VIEW_MOVED:
				messageWhat = "B_VIEW_MOVED";
				break;
			case B_VIEW_RESIZED:
				messageWhat = "B_VIEW_RESIZED";
				break;
			case B_WINDOW_MOVED:
				messageWhat = "B_WINDOW_MOVED";
				break;
			case B_WINDOW_RESIZED:
				messageWhat = "B_WINDOW_RESIZED";
				break;
			case B_WORKSPACES_CHANGED:
				messageWhat = "B_WORKSPACES_CHANGED";
				break;
			case B_WORKSPACE_ACTIVATED:
				messageWhat = "B_WORKSPACE_ACTIVATED";
				break;
			case B_ZOOM:
				messageWhat = "B_ZOOM";
				break;
			case _APP_MENU_:
				messageWhat = "_APP_MENU_";
				break;
			case _BROWSER_MENUS_:
				messageWhat = "_BROWSER_MENUS_";
				break;
			case _MENU_EVENT_:
				messageWhat = "_MENU_EVENT_";
				break;
			case _PING_:
				messageWhat = "_PING_";
				break;
			case _QUIT_:
				messageWhat = "_QUIT_";
				break;
			case _VOLUME_MOUNTED_:
				messageWhat = "_VOLUME_MOUNTED_";
				break;
			case _VOLUME_UNMOUNTED_:
				messageWhat = "_VOLUME_UNMOUNTED_";
				break;
			case _MESSAGE_DROPPED_:
				messageWhat = "_MESSAGE_DROPPED_";
				break;
			case _DISPOSE_DRAG_:
				messageWhat = "_DISPOSE_DRAG_";
				break;
			case _MENUS_DONE_:
				messageWhat = "_MENUS_DONE_";
				break;
			case _SHOW_DRAG_HANDLES_:
				messageWhat = "_SHOW_DRAG_HANDLES_";
				break;
			case _EVENTS_PENDING_:
				messageWhat = "_EVENTS_PENDING_";
				break;
			case _UPDATE_:
				messageWhat = "_UPDATE_";
				break;
			default:
				break;
		}
		
		if (messageWhat)
			printf("BMessage: what = \'%s\' (0x%lx)\n", messageWhat,
				message->what);
		else
			printf("BMessage: what = \'%.4s\' (0x%lx, or %li)\n", swappedValue.string, 
				message->what, message->what);
		
		uint32 fPtrOffset = message->fPtrOffset;
		entry_hdr *fEntries = message->fEntries;
		const entry_hdr *entry = convert(fEntries);
		while (entry) {
			entry_hdr tmp;
			if (proxy.ReadData((ptr_t)entry, &tmp, sizeof(tmp)) != B_OK)
				break;
			
			uint32 newSize = sizeof(tmp) + tmp.fNameLength;
			entry_hdr *newEntry = (entry_hdr *)malloc(newSize);
			if (proxy.ReadData((ptr_t)entry, newEntry, newSize) != B_OK) {
				free(newEntry);
				break;
			}

			const entry_hdr *mappedEntry = newEntry;

			swappedValue.value = B_BENDIAN_TO_HOST_INT32(mappedEntry->fType);

			sprintf(buffer, "    entry %14s, type=\'%.4s\', c=%li", mappedEntry->fName, swappedValue.string, mappedEntry->fCount);
			// These types handle their own output will handle their own 'size' output....
			if( (mappedEntry->fType != B_STRING_TYPE) && (mappedEntry->fType != B_REF_TYPE) )
				sprintf(buffer, "%s, size=%2li, ", buffer, 	mappedEntry->fChunkSize);

			int offset = sl - strlen(buffer);
			if(offset < 0)
				offset = 0;
			
			printf(buffer);

			TmpStorage store;
			switch (mappedEntry->fType) {
				case B_BOOL_TYPE: {
					bool *val = (bool*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							printf("%sdata[%li]: %d\n",
								i > 0 ? spaces + offset : "", i, *val);
							val++;
						}
					break;
				}
				case B_INT8_TYPE: {
					int8 *val = (int8*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							printf("%sdata[%li]: 0x%x (%hd)\n",
								i > 0 ? spaces + offset : "", i,
								*val, *val);
							val++;
						}
					break;
				}
				case B_INT16_TYPE: {
					int16 *val = (int16*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							swappedValue.value = B_BENDIAN_TO_HOST_INT16(*val);
							printf("%sdata[%li]: 0x%x (%i, \'%.4s\')\n",
								i > 0 ? spaces + offset : "", i,
								*val, *val, swappedValue.string);
							val++;
						}
					break;
				}
				case B_INT32_TYPE: {
					int32 *val = (int32*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							swappedValue.value = B_BENDIAN_TO_HOST_INT32(*val);
							printf("%sdata[%li]: 0x%lx (%li, \'%.4s\')\n",
								i > 0 ? spaces + offset : "", i,
								*val, *val, swappedValue.string);
							val++;
						}
					break;
				}
				case B_INT64_TYPE: {
					int64 *val = (int64*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							swappedValue.value = B_BENDIAN_TO_HOST_INT64(*val);
							printf("%sdata[%li]: 0x%Lx (%Ld, \'%.8s\')\n",
								i > 0 ? spaces + offset : "", i,
								*val, *val, swappedValue.string);
							val++;
						}
					break;
				}
				case B_FLOAT_TYPE: {
					float *val = (float*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							printf("%sdata[%ld]: %.4f\n",
								i > 0 ? spaces + offset : "", i, *val);
							val++;
						}
					break;
				}
				case B_DOUBLE_TYPE: {
					double *val = (double*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							printf("%sdata[%ld]: %.8f\n",
								i > 0 ? spaces + offset : "", i, *val);
							val++;
						}
					break;
				}
				case B_POINT_TYPE: {
					BPoint *val = (BPoint*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							printf("%sdata[%ld]: ",
								i > 0 ? spaces + offset : "", i);
							val++->PrintToStream();
						}
					break;
				}
				case B_RECT_TYPE: {
					BRect *val = (BRect*) da_start_of_data(proxy, entry, mappedEntry, store);
					if (val)
						for (int32 i = 0; i < mappedEntry->fCount; i++) {
							printf("%sdata[%ld]: ",
								i > 0 ? spaces + offset : "", i);
							val++->PrintToStream();
						}
					break;
				}
				case B_POINTER_TYPE: {
				/*
					const var_chunk *realChunk;
					var_chunk *mappedChunk = da_first_chunk(proxy, entry, mappedEntry, realChunk, store);
					if (mappedChunk)
						for (int32 i = 0;; i++) {
							printf("%s, size=%li, data[%li]: %px\n",i > 0 ? spaces + offset: "", mappedChunk->fDataSize,i , mappedChunk->fData);
							if (i >= mappedEntry->fCount - 1)
								break;
							
							mappedChunk = da_next_chunk(proxy, realChunk, mappedChunk, store);
							if (!mappedChunk)
								break;
						}
				*/
					printf("\n");
					break;
				}
				case B_STRING_TYPE: {
					const var_chunk *realChunk;
					var_chunk *mappedChunk = da_first_chunk(proxy, entry, mappedEntry, realChunk, store);
					if (mappedChunk)
						for (int32 i = 0;; i++) {
							printf("%s, size=%li, data[%li]: \"%s\"\n",i > 0 ? spaces + offset: "", mappedChunk->fDataSize,i , mappedChunk->fData);
							if (i >= mappedEntry->fCount - 1)
								break;
							
							mappedChunk = da_next_chunk(proxy, realChunk, mappedChunk, store);
							if (!mappedChunk)
								break;
						}

					break;
				}
				case B_REF_TYPE: {
					const var_chunk *realChunk;
					var_chunk *mappedChunk = da_first_chunk(proxy, entry, mappedEntry, realChunk, store);
					if (mappedChunk)
						for (int32 i = 0;; i++) {
							entry_ref ref;
							ref.device = *(int32 *)mappedChunk->fData;
							ref.directory = *((int64 *)(mappedChunk->fData + sizeof(int32)));
							ref.set_name(mappedChunk->fData + sizeof(int32) + sizeof(int64));
							
							// Find the path...
							BEntry bentry(&ref);
							BPath path;
							if(bentry.InitCheck() == B_OK)
								bentry.GetPath(&path);
								
							printf("%s, size=%li, data[%li]: device=%li, directory=%Li, name=\"%s\", path=\"%s\"\n",i > 0 ? spaces + offset: "",
								mappedChunk->fDataSize,
								i ,
								*(int32 *)mappedChunk->fData,
								*((int64 *)(mappedChunk->fData + sizeof(int32))),
								mappedChunk->fData + sizeof(int32) + sizeof(int64),
								path.Path());
							if (i >= mappedEntry->fCount - 1)
								break;
							
							mappedChunk = da_next_chunk(proxy, realChunk, mappedChunk, store);
							if (!mappedChunk)
								break;
						}

					break;
				}
				case B_MESSAGE_TYPE: {
				/*				
					const var_chunk *realChunk;
					var_chunk *mappedChunk = da_first_chunk(proxy, entry, mappedEntry, realChunk, store);
					if (mappedChunk)
						for (int32 i = 0;; i++) {
						//	printf("%s, size=%li, data[%li]: \"%s\"\n",i > 0 ? spaces + offset: "", mappedChunk->fDataSize,i , mappedChunk->fData);
						//	DumpVariable (proxy); ----------
							if (i >= mappedEntry->fCount - 1)
								break;
							
							mappedChunk = da_next_chunk(proxy, realChunk, mappedChunk, store);
							if (!mappedChunk)
								break;
						}
				*/
					break;
				}
				case B_RAW_TYPE: {
					// Do a HexDump()...
//					const var_chunk *realChunk;
//					var_chunk *mappedChunk = da_first_chunk(proxy, entry, mappedEntry, realChunk, store);
					void *data = (void*)da_start_of_data(proxy, entry, mappedEntry, store);
					if (data) {
						HexDump(data,mappedEntry->fChunkSize);
				/*		for (int32 i = 0;; i++) {
							printf("%s, size=%li, data[%li]: \n",i > 0 ? spaces + offset: "", mappedChunk->fDataSize,i);
							HexDump(mappedChunk->fData, mappedChunk->fDataSize);
							if (i >= mappedEntry->fCount - 1)
								break;
							
							mappedChunk = da_next_chunk(proxy, realChunk, mappedChunk, store);
							if (!mappedChunk)
								break;
						}
					*/
					}
					break;
				}
				default: {
					printf("\n");
					break;
				}
			}

			entry = convert(mappedEntry->fNext);
			free(newEntry);
		}

		free(p);
	}
	else
		puts("Insufficient memory");
	
	return B_OK;
} // DumpVariable
