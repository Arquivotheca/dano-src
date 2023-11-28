#include "GraphicButton.h"
#include "DarkenBitmap.h"

#include "BufPlay.h"

extern char *gClickSound;
extern long gClickSoundSize;
extern char *gClickUpSound;
extern long gClickUpSoundSize;

bool GraphicButton::picsInited = FALSE;
BPicture *GraphicButton::dButtonPicture = NULL;


GraphicButton::GraphicButton(	BRect frame,
								const char *name,
								const char *label,
								BMessage *msg,
								BBitmap *bmapOff,
								BBitmap *bmapPressed,
								ulong resizeMask)
	: 	BPictureButton(	frame,
						name,
						new BPicture(),
						new BPicture(),
						msg, 
						B_ONE_STATE_BUTTON,
						resizeMask ),
		offBitmap(bmapOff),
		onBitmap(bmapPressed),
		madeOnBitmap(FALSE)
{
	SetLabel(label);
	///player = new BufPlay();
	// playerUp = new BufPlay();
}


GraphicButton::~GraphicButton()
{
	if (madeOnBitmap)
		delete OnBitmap();

	//delete player;
	// delete playerUp;
}


void GraphicButton::MouseDown(BPoint where)
{
	//player->Play(gClickSound,gClickSoundSize);
	BPictureButton::MouseDown(where);

	ulong btns;
	GetMouse(&where,&btns);
	BRect r = Bounds();
	if (r.Contains(where)) {
		//player->Play(gClickUpSound,gClickUpSoundSize);
	}
}

void GraphicButton::DrawBackground(BView *v)
{
	return;
	
	BRect cur = Bounds();
	
	v->SetHighColor(v->ViewColor());
	v->FillRect(cur);

	
	v->SetHighColor(188,188,188);
	v->SetPenSize(2.0);
	// topline
	v->StrokeLine(BPoint(cur.left+1,cur.top+1),BPoint(cur.right-1,cur.top+1));
	// bottomline
	v->StrokeLine(BPoint(cur.left+1,cur.bottom),BPoint(cur.right-1,cur.bottom));
	// leftline
	v->StrokeLine(BPoint(cur.left+1,cur.top+1),BPoint(cur.left+1,cur.bottom-1));
	// right
	v->StrokeLine(BPoint(cur.right,cur.top+1),BPoint(cur.right,cur.bottom-1));
	cur.InsetBy(1,1);
	v->SetPenSize(1.0);
	v->SetHighColor(90,90,90);
	v->StrokeLine(BPoint(cur.left+1,cur.top),BPoint(cur.right-1,cur.top));
	v->StrokeLine(BPoint(cur.left+1,cur.bottom),BPoint(cur.right-1,cur.bottom));
	v->StrokeLine(BPoint(cur.left,cur.top+1),BPoint(cur.left,cur.bottom-1));
	v->StrokeLine(BPoint(cur.right,cur.top+1),BPoint(cur.right,cur.bottom-1));
	cur.InsetBy(1,1);
	v->SetHighColor(232,232,232);
	v->StrokeLine(BPoint(cur.left,cur.top),BPoint(cur.right,cur.top));
	v->StrokeLine(BPoint(cur.left,cur.top),BPoint(cur.left,cur.bottom));
	
	BRect c = cur;
	c.left++;
	c.top++;
	v->SetHighColor(148,148,148);
	v->StrokeLine(BPoint(c.left,c.bottom),BPoint(c.right,c.bottom));
	v->StrokeLine(BPoint(c.right,c.top),BPoint(c.right,c.bottom));

	c.bottom--;
	c.right--;
	v->SetHighColor(255,255,255);
	v->FillRect(c);
	
	c.left++;
	c.top++;
	v->SetHighColor(188,188,188);
	v->StrokeLine(BPoint(c.left,c.bottom),BPoint(c.right,c.bottom));
	v->StrokeLine(BPoint(c.right,c.top),BPoint(c.right,c.bottom));
	
	c.InsetBy(1,1);
	v->SetHighColor(232,232,232);
	v->FillRect(c);
}

void GraphicButton::DrawGraphic(BView *v, BBitmap *bmap)
{
	if (!bmap)
		return;

	BRect	cur = Bounds();
	BRect	dst = bmap->Bounds();

	dst.OffsetTo(cur.left + (cur.Width() - dst.Width())/2.0,
					cur.top + ((cur.Height()-(Label() ? 12 : 0)) - dst.Height())/2.0);
	v->SetDrawingMode(B_OP_OVER);
	v->DrawBitmap(bmap,dst);
	v->SetDrawingMode(B_OP_COPY);
}

void GraphicButton::DrawText(BView *v)
{
	if (Label()) {
		BRect	cur = Bounds();
		
		v->SetDrawingMode(B_OP_OVER);
	
		v->SetFont(be_bold_font);
		float w = v->StringWidth(Label());
	
		v->SetHighColor(0,0,0);	
		v->DrawString(Label(),BPoint(cur.left + (cur.Width() - w)/2.0,
								  cur.bottom - 6.0));
		v->SetDrawingMode(B_OP_COPY);
	}
}

void GraphicButton::AttachedToWindow()
{
	BPictureButton::AttachedToWindow();
	
	BPicture	*offPicture = NULL;
	BPicture	*onPicture;

	SetViewColor(B_TRANSPARENT_32_BIT);

	// render the bitmap
	BView	*v = Parent();
	// assert(v);
	
	BRect src, dst;
	if (offBitmap) {
		v->BeginPicture(new BPicture());
		// draws background
		DrawBackground(v);
		// draw graphic
		DrawGraphic(v,offBitmap);
		// draw text
		DrawText(v);
		//set picture	
		offPicture = v->EndPicture();

	}
	
	if (onBitmap) {
		
		// if on and off are the same then duplictae
		if (onBitmap == offBitmap) {
			onBitmap = new BBitmap(	offBitmap->Bounds(),
									offBitmap->ColorSpace());
			DarkenBitmap(offBitmap,onBitmap);
		}
		
		v->BeginPicture(new BPicture());
		// draws background
		DrawBackground(v);
		// draw text
		DrawText(v);
		BRect r = Bounds();
		r.InsetBy(3,3);
		// invert background and text
		v->InvertRect(r);
		DrawGraphic(v,onBitmap);
		onPicture = v->EndPicture();
	}
	else if (offPicture) {
		
		v->BeginPicture(new BPicture());
			/*
			v->DrawPicture(offPicture);
			BRect r = Bounds();
			r.InsetBy(3,3);
			v->InvertRect(r);
			*/
			// draws background
			DrawBackground(v);
			
			DrawText(v);

			DrawGraphic(v,offBitmap);

			// darken			
			BRect r = Bounds();
			// r.InsetBy(3,3);
			v->SetDrawingMode(B_OP_SUBTRACT);
			v->SetHighColor(60,60,60);
			v->FillRect(r);
			v->SetDrawingMode(B_OP_COPY);
			v->SetHighColor(0,0,0);
		onPicture = v->EndPicture();
	}
	
	if (offPicture) {
		SetEnabledOff(offPicture);
		SetEnabledOn(onPicture);
	}
}

void GraphicButton::Init(BView *v)
{
	if (picsInited)
		return;
		
	// init dummy pictures for the constructor arguments
	v->BeginPicture(new BPicture());	
	GraphicButton::dButtonPicture = v->EndPicture();
}

void GraphicButton::Free()
{
	if (picsInited) {
		delete dButtonPicture;
	}
}
