#include "widget.h"

#include <CheckBox.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <string.h>

extern "C" {
	#include <intel_map.h>
}

#define TOGGLE_ENABLED	'Mr.T'
#define NAME_MODIFIED	'B.A.'
#define SET_COLOR		'Face'

TTextView::TTextView(BRect r, const char *text) :
	BTextView(r, NULL, BRect(0,0,r.Width(),r.Height()), 0, B_WILL_DRAW)
{
	if (text) SetText(text);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	MakeSelectable(false);
	MakeEditable(false);
}

TButton::TButton(BRect r, const char *label, uint32 what, int32 cookie,
	bool enabled) : BButton(r, label, label, NULL)
{
	BMessage *message = new BMessage(what);
	message->AddInt32("cookie", cookie);
	SetMessage(message);
	SetEnabled(enabled);
}

status_t TButton::Invoke(BMessage *message)
{
	return BButton::Invoke(message);
}

PListItem::PListItem(Partition *p) : partition(p)
{
	enabled = true;
	color = 0;
	
	const char *text = partition->VolumeName();
	if (!text || !(text[0])) text = "Unknown";
	strcpy(name, text);

	if (!strcmp(partition->FileSystemShortName(), "unknown")) {
		int32 i;
		for (i=0;types[i].id != -1;i++)
			if (types[i].id == partition->Data()->partition_code)
				break;
		if (types[i].id != -1)
			strcpy(name, types[i].name);
		else
			sprintf(name, "id 0x%2.2x", partition->Data()->partition_code);
	}
}

PView::PView(BRect frame, const char *name, BList *p) :
		BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW), Partitions(p)
{
	SetViewColor(255,255,255);
}

void PView::AllAttached(void)
{
	GetFont(&font);
	
	font_height h;
	font.GetHeight(&h);
	fheight = h.ascent + h.descent;
	itemheight = fheight + 10;

	((BScrollView *)Parent())->ScrollBar(B_HORIZONTAL)->SetRange(0, 220);
	((BScrollView *)Parent())->ScrollBar(B_HORIZONTAL)->SetSteps(10.0, 50.0);
	((BScrollView *)Parent())->ScrollBar(B_VERTICAL)->SetRange(0, (Partitions->CountItems() - Bounds().Height() / itemheight + 1) * itemheight);
	((BScrollView *)Parent())->ScrollBar(B_VERTICAL)->SetSteps(itemheight, 3 * itemheight);

	/* add controls */
	for (int32 i=0;i<Partitions->CountItems();i++) {
		PListItem *item = (PListItem *)(Partitions->ItemAt(i));
		BRect f(2,i*itemheight+2,itemheight-2,(i+1)*itemheight-2);
		BCheckBox *box = new BCheckBox(f, NULL, "",
				NewBMessageWithCookie(TOGGLE_ENABLED, i));
		box->SetTarget(this);
		if (item->enabled) box->SetValue(B_CONTROL_ON);
		AddChild(box);
		f = BRect(itemheight + 2,i*itemheight+2,itemheight + 2 + 80,(i+1)*itemheight-2);
		BTextControl *text = new BTextControl(f, NULL, NULL, item->name,
				NewBMessageWithCookie(NAME_MODIFIED, i));
		text->SetModificationMessage(NewBMessageWithCookie(NAME_MODIFIED, i));
		text->TextView()->SetMaxBytes(32);
		text->SetTarget(this);
		AddChild(text);
	}
}

void PView::DrawTruncatedString(float w,
		const char *text, uint32 mode)
{
	char ttext[256], *ptext = ttext;
	font.GetTruncatedStrings(&text, 1, mode, w, &ptext);
	DrawString(ttext);
}

void PView::DrawItem(PListItem *item, BRect frame)
{
	const char *text;
	Partition *partition = item->partition;

	float fmiddle = (frame.bottom + frame.top + fheight) / 2;
	float spacing = itemheight + 2 + 80 + 2;

	SetHighColor(0,0,250);
	StrokeLine(
			BPoint(spacing,frame.top),
			BPoint(spacing,frame.bottom));

	text = partition->FileSystemShortName();
	char _[40];
	if (!strcmp(text, "unknown")) {
		text = _;
		int32 i;
		for (i=0;types[i].id != -1;i++)
			if (types[i].id == partition->Data()->partition_code)
				break;
		if (types[i].id != -1)
			strcpy(_, types[i].name);
		else
			sprintf(_, "id 0x%2.2x", partition->Data()->partition_code);
	}

	spacing += 5;

	SetHighColor(0,0,0);
	MovePenTo(spacing, fmiddle);
	DrawTruncatedString(65, text);

	spacing += 70;
	SetHighColor(0,0,250);
	StrokeLine(
			BPoint(spacing,frame.top),
			BPoint(spacing,frame.bottom));

	spacing += 5;
	
	SetHighColor(0,0,0);
	MovePenTo(spacing, fmiddle);
	char temp[128], name[128], *p;
	strcpy(temp, partition->GetDevice()->Name());
	p = strrchr(temp, '/'); *(p+1) = 0;
	sprintf(name, "%s%ld_%ld", temp,
			partition->GetSession()->Index(), partition->Index());
	DrawTruncatedString(200, name, B_TRUNCATE_BEGINNING);

	spacing += 205;
	SetHighColor(0,0,250);
	StrokeLine(
			BPoint(spacing,frame.top),
			BPoint(spacing,frame.bottom));

	spacing += 5;

	SetHighColor(0,0,0);
	MovePenTo(spacing, fmiddle);
	if (partition->Blocks() > 1024LL * 1024 * 1024 * 2)
		sprintf(temp, "%.1f TB", partition->Blocks() / (2. * 1024. * 1024. * 1024.));
	else if (partition->Blocks() > 1024 * 1024 * 2)
		sprintf(temp, "%.1f GB", partition->Blocks() / (2. * 1024. * 1024.));
	else if (partition->Blocks() > 1024 * 2)
		sprintf(temp, "%.1f MB", partition->Blocks() / (2. * 1024.));
	else
		sprintf(temp, "%.1f KB", partition->Blocks() / 2.);

	DrawString(temp);
}

void PView::Draw(BRect update)
{
	SetHighColor(255,255,255);
	FillRect(update);

	for (int32 i=0;i<Partitions->CountItems();i++)
		DrawItem((PListItem *)(Partitions->ItemAt(i)),
				BRect(0, i * itemheight, 300, (i + 1) * itemheight));

	BRect b = Bounds();
	SetHighColor(0,0,200);
	int32 i;
	for (i=0;i<Partitions->CountItems();i++)
		StrokeLine(BPoint(b.left, i * itemheight), BPoint(b.right, i * itemheight));
	StrokeLine(BPoint(b.left, i * itemheight), BPoint(b.right, i * itemheight));
}

BMessage *PView::NewBMessageWithCookie(uint32 what, int32 cookie1, int32 cookie2)
{
	BMessage *m = new BMessage(what);
	m->AddPointer("this", this);
	m->AddInt32("cookie", cookie1);
	m->AddInt32("cookie", cookie2);
	return m;
}

void PView::MouseDown(BPoint p)
{
	BPoint point;
	uint32 buttons;

	GetMouse(&point, &buttons);
/*
	int32 which = (int32)(p.y / itemheight);
	PListItem *item = (PListItem *)(Partitions->ItemAt(which));
	if (item && (buttons & B_SECONDARY_MOUSE_BUTTON)) {
		BPopUpMenu *menu = new BPopUpMenu(NULL, false);
		menu->AddItem(new BMenuItem("Red",
				NewBMessageWithCookie(SET_COLOR, 1)));
		menu->AddItem(new BMenuItem("Blue",
				NewBMessageWithCookie(SET_COLOR, 2)));
		menu->AddItem(new BMenuItem("Green", 
				NewBMessageWithCookie(SET_COLOR, 3)));
		menu->SetTargetForItems(this);
		menu->Go(ConvertToScreen(p) - BPoint(3,3),
				true, true, false);
	}
*/
}

void PView::MessageReceived(BMessage *message)
{
	int32 index, cookie;
	PListItem *item;
	if (message->FindInt32("cookie", 0, &index) != B_OK)
		index = -1;
	if (message->FindInt32("cookie", 1, &cookie) != B_OK)
		cookie = 0;
	item = (PListItem *)Partitions->ItemAt(index);
	switch (message->what) {
		case TOGGLE_ENABLED:
			item->enabled = !item->enabled;
			break;
		case NAME_MODIFIED:
		{
			BTextControl *src;
			if (message->FindPointer("source", 0, (void **)&src) == B_OK) {
				strcpy(item->name, src->Text());
			}
			break;
		}
		case SET_COLOR:
			item->color = cookie;
			break;
		default:
			BView::MessageReceived(message);
	}
}
