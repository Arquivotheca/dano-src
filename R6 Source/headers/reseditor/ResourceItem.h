/***************************************************************************
//
//	File:			ResourceItem.h
//
//	Description:	This class represents a single block of resource data.
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

/**************************************************************************
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// This code is experimental.  Please do not use it in your own
// applications, as future versions WILL break this interface.
//
***************************************************************************/

#ifndef _RESOURCE_ITEM_H
#define _RESOURCE_ITEM_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#include <SupportDefs.h>
#include <Archivable.h>
#include <DataIO.h>
#include <String.h>

namespace BResEditor {

enum {
	B_RES_TYPE_CHANGED		= 0x00000001,
	B_RES_ID_CHANGED		= 0x00000002,
	B_RES_NAME_CHANGED		= 0x00000004,
	B_RES_DATA_CHANGED		= 0x00000008,
	B_RES_SIZE_CHANGED		= 0x00000010,
	B_RES_SYMBOL_CHANGED	= 0x00000020,
	B_RES_FILE_CHANGED		= 0x00000040,
	B_RES_LINE_CHANGED		= 0x00000080,
	B_RES_ALL_CHANGED		= 0xFFFFFFFF
};

enum {
	B_TYPE_STRING_STRICT		= 0x00000001,
	B_TYPE_STRING_CHAR_CONTEXT	= 0x00000002,
	B_TYPE_STRING_NUM_CONTEXT	= 0x00000004
};

class BResourceItem : public BPositionIO, public BArchivable
{
public:
	BResourceItem();
	BResourceItem(type_code t, int32 i,
				  const char* name, const char* symbol = NULL);
	BResourceItem(const BResourceItem& o);
	virtual ~BResourceItem();
	
	BResourceItem& operator=(const BResourceItem& o);
	
	virtual void UpdateFrom(const BResourceItem* item, uint32 which);
	void UpdateFrom(const BResourceItem* item);
	
	// Resource data.
	type_code Type() const;
	int32 ID() const;
	bool HasName() const;
	const char* Name() const;
	const void* Data() const;
	size_t Size() const;
	
	// Contextual information.
	bool HasSymbol() const;
	const char* Symbol() const;
	bool HasFile() const;
	const char* File() const;
	int32 Line() const;
	
	// Modification.
	virtual void SetType(type_code type);
	virtual void SetID(int32 id);
	virtual void SetName(const char* name);
	virtual status_t SetData(const void* data, size_t size);
	virtual void SetSymbol(const char* symbol);
	virtual void SetFile(const char* file);
	virtual void SetLine(int32 line);
	
	// Data modification.  In the future, these will allow the storing
	// of diffs for undo; when that happens, all other data changing
	// functions will be implemented in terms of these.  These will
	// also probably be merged into one "replace" operation.
	virtual status_t DeleteData(off_t pos, size_t amount);
	virtual status_t InsertData(off_t pos, const void* data, size_t amount);
	
	// Manipulating types as strings.
	const char* GetType(BString* out_type,
						uint32 flags = B_TYPE_STRING_CHAR_CONTEXT|B_TYPE_STRING_NUM_CONTEXT) const;
	void SetType(const char* type, uint32 flags = 0);
	
	// Create a "nice" label from us -- either the item's name, its
	// deconstructed symbol, or "" if neither is defined.  If both are
	// defined, the name is preferred.
	const char* CreateLabel(BString* out) const;
	
	// Set this item's name and/or symbol, based on the given label.
	void ApplyLabel(const char* l);

	// Keeping track of object changes.  Be aware that NoteChange() is
	// called -before- the actual information is modified, so that you
	// have a chance to pull out the old value.
	virtual void NoteChange(uint32 what);
	void ClearChanges();
	uint32 Changes() const;
	
	// BPositionIO interface
	virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
	virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);
	
	virtual	off_t		Seek(off_t pos, uint32 seek_mode);
	virtual off_t		Position() const;
	virtual status_t	SetSize(off_t size);

	// New IO interface
	virtual ssize_t		WriteStream(BDataIO* stream);
	
	// BArchivable interface
						BResourceItem(BMessage *from);
	virtual status_t	Archive(BMessage *into, bool deep = true) const;
	
	// Utility functions
	static const char* TypeToString(type_code type, BString* out,
									uint32 flags = B_TYPE_STRING_CHAR_CONTEXT|B_TYPE_STRING_NUM_CONTEXT);
	static const char* TypeIDToString(type_code type, int32 id, BString* out);
	static type_code StringToType(const char* str, uint32 flags = 0);
	
private:
	BMallocIO fData;
	type_code fType;
	int32 fID;
	BString fName;
	
	BString fSymbol;
	BString fFile;
	int32 fLine;
	
	uint32 fChanges;
};

}	// namespace BResEditor
using namespace BResEditor;

#endif
