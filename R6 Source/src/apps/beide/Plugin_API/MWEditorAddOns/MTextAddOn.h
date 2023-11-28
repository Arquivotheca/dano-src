// ---------------------------------------------------------------------------
/*
	filename
	
	MTextAddOn.h
	Copyright 1996  Metrowerks Corporation, All Rights Reserved.

	MTextAddOnStorage
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	Author:	John R. Dance
			18 May 2001

*/
// ---------------------------------------------------------------------------
 
#ifndef _MTEXTADDON_H
#define _MTEXTADDON_H

#include <SupportKit.h>

class MIDETextView;
class BWindow;
struct entry_ref;

// ---------------------------------------------------------------------------
//	MTextAddOn
//	The addon must export
//	extern "C" status_t perform_edit(MTextAddOn *addon);
//
//	This is a proxy class used by Editor add_ons.  It does not inherit from BView
//	but provides an abstract interface to a text engine.
// ---------------------------------------------------------------------------

class MTextAddOn
{
public:
								MTextAddOn(
									MIDETextView&	inTextView);
	virtual						~MTextAddOn();
	virtual	const char*			Text();
	virtual	int32				TextLength() const;
	virtual	void				GetSelection(
									int32 *start, 
									int32 *end) const;
	virtual	void				Select(
									int32 newStart, 
									int32 newEnd);
	virtual void				Delete();
	virtual void				Insert(
									const char* inText);
	virtual void				Insert(
									const char* text, 
									int32 length);

	virtual	BWindow*			Window();
	virtual status_t			GetRef(
									entry_ref&	outRef);
	virtual bool				IsEditable();
	
	virtual void				AddError(const char* message);
	virtual void				ShowErrors();
private:

	MIDETextView&				fText;
};

// ---------------------------------------------------------------------------
//	MTextAddOnStorage
//	The addon may export
//	extern "C" status_t edit_addon_startup(MTextAddOnStorage* storage);
//	extern "C" status_t edit_addon_shutdown(MTextAddOnStorage* storage);
//
//	Simple interface to handle storing persistent data on behalf of the addon
//	Specify the contents of the data as a BMessage
//	Name the data with your addon name as a prefix with underscores
//	(ie: For the Comment addon, the data would be _Comment_data)
// ---------------------------------------------------------------------------

class MTextAddOnStorage
{
public:
						MTextAddOnStorage();
	virtual				~MTextAddOnStorage();
	
	virtual bool		DataExists(const char* inName, size_t* outSize = NULL);
	virtual status_t	SaveData(const char* inName, const BMessage& inMessage);
	virtual status_t	GetData(const char* inName, BMessage& outMessage);
	virtual status_t	RemoveData(const char* inName);
};

#endif
