#include "ImagePanel.h"
#include "PreviewView.h"
#include <Path.h>
#include <Node.h>
#include <File.h>
#include <Messenger.h>
#include <Button.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <Window.h>

#include <stdio.h>

ImageFilter::ImageFilter()
{
	roster = BTranslatorRoster::Default();
}

bool ImageFilter::Filter(const entry_ref *ref, BNode *node, struct stat *, const char *mime)
{
	status_t		err = B_ERROR;
	translator_info	info;

	// show all directories
	if(node->IsDirectory())
		return true;

	BFile	file(ref, O_RDONLY);

	// First try to identify the data using the hint, if any
	if(mime)
		err = roster->Identify(&file, 0, &info, 0, mime);

	// If not identified, try without a hint
	if(err)
		err = roster->Identify(&file, 0, &info);

	return err == B_OK && info.group == B_TRANSLATOR_BITMAP;
}

class PreviewZone : public BView
{
	PreviewView	*prev;
	BStringView	*typetext;
	BStringView *restext;
	BRect		prevbounds;

public:
	PreviewZone(BRect frame, const char *name, uint32 resizingMode, uint32 flags);
	void AttachedToWindow();
	void MessageReceived(BMessage *msg);
	void LoadBitmap(const char *path);
	void ClearBitmap();
};

PreviewZone::PreviewZone(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
 : BView(frame, name, resizingMode, flags)
{
	prev = 0;
	prevbounds = Bounds();
	prevbounds.InsetBy(5, 5);
	prevbounds.right = prevbounds.left + (prevbounds.bottom - prevbounds.top) / 3 * 4;
	typetext = new BStringView(BRect(prevbounds.right + 10, prevbounds.top, Bounds().right - 5, prevbounds.top + 14), "", "");
	AddChild(typetext);
	restext = new BStringView(BRect(prevbounds.right + 10, prevbounds.top + 16, Bounds().right - 5, prevbounds.top + 30), "", "");
	AddChild(restext);
}

void PreviewZone::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
}

void PreviewZone::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case 'errr' :
			prev->ClearBitmap();
			break;

		case 'load' :
			// display info
			{
			const char *desc;
			int32 x;
			int32 y;
			char tmp[32];
			if(msg->FindString("type", &desc) == B_OK &&
				msg->FindInt32("sizex", &x) == B_OK &&
				msg->FindInt32("sizey", &y) == B_OK)
			{
				typetext->SetText(desc);
				sprintf(tmp, "Resolution: %ldx%ld", x, y);
				restext->SetText(tmp);
			}
			}
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

void PreviewZone::LoadBitmap(const char *path)
{
	if(! prev)
	{
		// add preview view
		prev = new PreviewView(prevbounds, "preview", 1, new BMessage('errr'));
		AddChild(prev);

		// setup preview view
		prev->SetViewColor(ViewColor());
		prev->SetLoadNotification(this, new BMessage('load'));
		prev->SetTarget(this);
		prev->ScaleToFit();
		prev->SetAspectConstrain(true);
	}

	// clear previous image and image info
	prev->ClearBitmap();
	typetext->SetText("");
	restext->SetText("");

	if(! prev->LoadBitmap(path))
		prev->ClearBitmap();
}

void PreviewZone::ClearBitmap()
{
	if(prev)
	{
		prev->ClearBitmap();
		RemoveChild(prev);
		delete prev;
		prev = 0;
	}
	typetext->SetText("");
	restext->SetText("");
}

ImagePanel::ImagePanel(BMessenger *target, entry_ref *start_directory,
	BMessage *message, BRefFilter *rf)
 : BFilePanel(B_OPEN_PANEL, target, start_directory, B_FILE_NODE,
 	false, message, rf, false, true), prev(0)
{
	BWindow	*w = Window();
	if(w->Lock())
	{
		BView *panelback = w->ChildAt(0);
		SetButtonLabel(B_DEFAULT_BUTTON, "Select");

		BView *pose = w->FindView("PoseView");
		BView *count = w->FindView("CountVw");
		BView *vsc = w->FindView("VScrollBar");
		BView *hsc = w->FindView("HScrollBar");

#define HSPACE	70

		if(pose && count && vsc && hsc)
		{
			w->ResizeBy(0, HSPACE);
			pose->ResizeBy(0, -HSPACE);
			vsc->ResizeBy(0, -HSPACE);
			count->MoveBy(0, -HSPACE);
			hsc->MoveBy(0, -HSPACE);

			prev = new PreviewZone(BRect(10, panelback->Bounds().bottom - 100, 250, panelback->Bounds().bottom - 10),
				"previewzone", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, 0);
			panelback->AddChild(prev);
		}

		w->Unlock();
	}
}

void ImagePanel::SelectionChanged()
{
	if(prev)
	{
		entry_ref	r;
		Rewind();
		if(GetNextSelectedRef(&r) == 0)
		{
			BEntry e(&r);
			BPath p;
			e.GetPath(&p);
			prev->LoadBitmap(p.Path());
		}
		else
			prev->ClearBitmap();
	}

	BFilePanel::SelectionChanged();
}
