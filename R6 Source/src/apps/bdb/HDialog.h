/*
	HDialog
*/

#ifndef HDIALOG_H
#define HDIALOG_H

#include "HStream.h"
#include "HAppResFile.h"

#include <Window.h>

struct dRect {
	short left, top, right, bottom;
	
	BRect ToBe();
};

template <class T>
class DialogCreator {
public:
static	T* CreateDialog(BWindow *owner);
static	T* CreateDialog(BWindow *owner, BPositionIO& tmpl);
};

template <class T>
T* DialogCreator<T>::CreateDialog(BWindow *owner)
{
	size_t size;
	const void *p = HResources::GetResource('DLOG', T::sResID, size);
	if (!p) throw HErr("missing resource");
	BMemoryIO buf(p, size);
	
	T *d = CreateDialog(owner, buf);
	
	return d;
} /* CreateDialog */

template <class T>
T* DialogCreator<T>::CreateDialog(BWindow *owner, BPositionIO& tmpl)
{
	T::RegisterFields();

	dRect r;
	char n[256];
	window_type t;
	int f;

	tmpl >> r >> n >> t >> f;
	return new T(GetPosition(r, owner), n, t, f, owner, tmpl);
} /* DialogCreator<T>::CreateDialog */

template <class T>
void MakeDialog(BWindow* owner, T*& dlog)
{
	dlog = DialogCreator<T>::CreateDialog(owner);
} // MakeDialog

typedef void (*FieldCreator)(int kind, BPositionIO& data, BView*& inside);

class HDialog : public BWindow {
public:
			HDialog(BRect frame, const char *name, window_type type, int flags,
				BWindow *owner, BPositionIO& data);
			~HDialog();
	
			enum { sResID = 100 };
	
			void CreateField(int kind, BPositionIO& data, BView*& inside);
virtual	void MessageReceived(BMessage *msg);

virtual	bool OKClicked();
virtual	bool CancelClicked();
virtual	void UpdateFields();

			bool IsOn(const char *name) const;
			void SetOn(const char *name, bool on = true);
			
			const char* GetText(const char *name) const;
			void SetText(const char *name, const char *text);
			
			const char* GetLabel(const char *name) const;
			void SetLabel(const char *name, const char *label);
			
			void SetEnabled(const char *name, bool enabled = true);
			
			bool GetDouble(const char *name, double& d) const;

			int GetValue(const char *id) const;
			void SetValue(const char *id, int v);
			
static		void RegisterFieldCreator(int kind, FieldCreator fieldCreator);
static		void RegisterFields();

protected:
			void BuildIt(BPositionIO& data);
	
			BView *fMainView;
			BWindow *fOwner;
};

BRect GetPosition(dRect dr, BWindow *owner);
extern float gFactor;

#endif
