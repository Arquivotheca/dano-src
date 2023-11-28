//==================================================================
//	MKeyBindingsListView.h
//	Copyright 97 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MKEYBINDINGSLISTVIEW_H
#define _MKEYBINDINGSLISTVIEW_H

#include "MTriangleListView.h"
#include "CString.h"
#include "MKeyBindingManager.h"

class MPreferencesView;
class MPlugInBuilder;

struct ListRowData
{
						ListRowData(
							const char *		inName)
							: name(inName)
								{}

	virtual void		Draw(
							MTriangleListView*			inView,
							BRect						inArea,
							const MKeyBindingManager&	inManager);

	String				name;
};

struct GroupRowData : public ListRowData
{
						GroupRowData(
							const char *		inName)
							: ListRowData(inName)
								{}
	virtual void		Draw(
							MTriangleListView*			inView,
							BRect						inArea,
							const MKeyBindingManager&	inManager);
};

struct BindingRowData : public ListRowData
{
						BindingRowData(
							KeyBindingInfo& 	inBinding,
							KeyBindingContext	inContext,
							const char *		inName)
							: ListRowData(inName), info(inBinding), context(inContext)
								{}

	virtual void		Draw(
							MTriangleListView*			inView,
							BRect						inArea,
							const MKeyBindingManager&	inManager);
	virtual void		UpdateBinding(
							const KeyBindingInfo&		inInfo,
							MKeyBindingManager&			inManager);

	KeyBindingInfo		info;
	KeyBindingContext	context;
};

struct PrefixRowData : public BindingRowData
{
						PrefixRowData(
							KeyBindingInfo& 	inBinding,
							const char *		inName,
							int32				inIndex)
							: BindingRowData(inBinding, kBindingGlobal, inName),
							  fIndex(inIndex)
								{}

	virtual void		Draw(
							MTriangleListView*			inView,
							BRect						inArea,
							const MKeyBindingManager&	inManager);

	virtual void		UpdateBinding(
							const KeyBindingInfo&		inInfo,
							MKeyBindingManager&			inManager);

	int32		fIndex;
};

class MKeyBindingsListView : public MTriangleListView
{
public:
								MKeyBindingsListView(
									BRect			inFrame,
									const char*		inName,
									BView*			inOwner);
								~MKeyBindingsListView();

virtual	void					DrawRow(
									int32 index,
									void * data,
									BRect rowArea,
									BRect intersectionRect);

virtual	void					InvokeRow(
									int32 	inRow);
virtual	void					DeleteItem(
									void * item);
	void						BuildBindingList();

	virtual void				UpdateBinding(
									KeyBindingContext			inContext,
									const KeyBindingInfo&		inInfo,
									MKeyBindingManager&			inManager);
	void						UpdateAllBindings(
									const MKeyBindingManager&	inManager);
	void						SetBindingManager(
									MKeyBindingManager*		inManager);

private:

	BView*				fOwner;
	MKeyBindingManager*	fManager;
	int32				fInvokedRow;
	int32				fFirstPrefixRow;

	BindingRowData*				NewRowObject(
									CommandT		inCommand,
									const char *	inName,
									int32			inGroupID,
									int32			inIndex);

};

#endif
