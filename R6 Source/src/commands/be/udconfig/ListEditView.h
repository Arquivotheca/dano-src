//
// Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#ifndef _LIST_EDIT_VIEW_H_
#define _LIST_EDIT_VIEW_H_

#include <View.h>

class BTextControl;
class BInvoker;

class ListEditView : public BView {
public:
	ListEditView(BRect frame, const char *name, uint32 resizingMode, uint32 flags, BList *slist);
	~ListEditView();

	void AttachedToWindow();
	void MessageReceived(BMessage*);

	void SetChangedInvoker(BInvoker *invoker);

	void GetPreferredSize(float *width, float *height);
	void GetItemHeight(float *height);

	void Revert();
	void Commit();

	int DirtyCount() { return fDirty; }

private:
	void CookieToName(int32 cookie, char *oname, size_t len);
	void NameToCookie(const char *name, int32 *cookie);

	void AddItem(BRect&, int32 cookie);
	void BuildList();
	void PopulateList();

	void ItemChanged(int32);
	void SetDirty(BTextControl*, bool);

	BList *fSettingsList;
	float fEm;
	float fBEm;
	float fFEm;

	float fPrefWidth;
	float fPrefHeight;
	float fItemHeight;

	int fDirty;
	BInvoker *fChangedInvoker;
};

#endif //_LIST_EDIT_VIEW_H_
