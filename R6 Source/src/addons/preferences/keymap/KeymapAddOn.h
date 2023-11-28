#include "PPAddOn.h"
#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <stdio.h>
#include "PrefsAppExport.h"

class KeymapAddOn : public PPAddOn {
public:
	KeymapAddOn(image_id i,PPWindow*w);

	BView* MakeView();
	bool UseAddOn();

	BBitmap* Icon();
	char* Name();
	char* InternalName();
};

class KeymapView : public BView {
public:
	KeymapView(BRect);

	void AllAttached();
};

class UnicodeView : public BView {
public:
	UnicodeView(BRect);

	void Draw(BRect);
	void MouseDown(BPoint);
	void MessageReceived(BMessage*);

	void StringFor(int,char*);

	int32 page;
	float voffset;

	bool has_glyph[65536];
};

class KeyView : public BView {
public:
	KeyView(BRect);

	void Draw(BRect);
	void MouseDown(BPoint);

	BRect keyframe[23][7];
	int linelength[7];

	void GenerateLayout(int32 flags=0);

	enum {
		LAYOUT_102=1,
		LAYOUT_104=2,
		LAYOUT_BS_ROW2=4,
		LAYOUT_BS_ROW3=8,
		LAYOUT_BS_ROW4=12,
		LAYOUT_BS_MASK=12,
	};
};

class KeyEditView : public BView {
public:
	KeyEditView(BRect);

	void Draw(BRect);
};