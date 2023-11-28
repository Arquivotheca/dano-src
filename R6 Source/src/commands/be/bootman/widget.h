#ifndef _BOOTMAN_WIDGET_H_
#define _BOOTMAN_WIDGET_H_

#include <Button.h>
#include <ListView.h>
#include <TextView.h>

#include <private/storage/DeviceMap.h>

class TRect : public BRect
{
public:
	TRect(BRect r, BRect f) : BRect(r.left*f.Width(), r.top*f.Height(),
			r.right*f.Width(), r.bottom*f.Height()) { }
};

class TTextView : public BTextView
{
public:
	TTextView(BRect r, const char *text);
	virtual ~TTextView() {}
};

class TButton : public BButton
{
public:
	TButton(BRect r, const char *label, uint32 what, 
			int32 cookie = 0, bool enabled = true);
	virtual ~TButton() {}
	virtual status_t Invoke(BMessage *message = NULL);
};

struct PListItem
{
public:
	PListItem(Partition *p);
	
	Partition *partition;
	bool enabled;
	int32 color;
	char name[80];
};

class PView : public BView
{
	BList *Partitions;
	BFont font;
	float fheight, itemheight;

	BMessage *NewBMessageWithCookie(uint32 what,
			int32 cookie1, int32 cookie2 = 0);
	void DrawTruncatedString(float w,
		const char *text, uint32 mode = B_TRUNCATE_END);
	void DrawItem(PListItem *item, BRect frame);
public:
	PView(BRect frame, const char *name, BList *Partitions);

	virtual void AllAttached(void);	
	virtual void MouseDown(BPoint point);
	virtual void Draw(BRect update);
	virtual void MessageReceived(BMessage *message);
};

#endif
