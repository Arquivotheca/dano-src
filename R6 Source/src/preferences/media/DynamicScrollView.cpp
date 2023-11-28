#include "DynamicScrollView.h"

#include <Debug.h>
#include <TabView.h>
#include <Window.h>

#include <stdio.h>


BDynamicScrollView::BDynamicScrollView(const BRect area, const char *name,
										uint32 resize, uint32 flags, border_style border)
	:	BView(area, name, resize, flags | B_FULL_UPDATE_ON_RESIZE),
		mVBar(NULL),
		mHBar(NULL),
		mTarget(NULL),
		mScrollHeight(0),
		mScrollWidth(0),
		mBorderStyle(border)
{
	SetViewColor(216, 216, 216);
	BRect rect(Bounds());
	rect.right--;
	rect.left = rect.right - B_V_SCROLL_BAR_WIDTH;
	rect.bottom--;
	rect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	mVBar = new BScrollBar(rect, "vertical", NULL, 0, 0, B_VERTICAL);
	AddChild(mVBar);
	mVBar->Hide();
	rect = Bounds();
	rect.bottom--;
	rect.top = rect.bottom - B_H_SCROLL_BAR_HEIGHT;
	rect.right--;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	mHBar = new BScrollBar(rect, "horizontal", NULL, 0, 0, B_HORIZONTAL);
	AddChild(mHBar);
	mHBar->Hide();
}

BDynamicScrollView::~BDynamicScrollView()
{
}

BView *
BDynamicScrollView::Target()
{
	return mVBar->Target();
}

void 
BDynamicScrollView::SetTarget(BView *newTarget)
{
	if(mTarget)
	{
		ASSERT(mTarget->Window());
		ASSERT(mTarget->Window() != (BWindow *)(-1L));
		ASSERT(mTarget->Window()->IsLocked());
		
		mTarget->ResizeTo(mTargetBounds.Width(), mTargetBounds.Height());
		RemoveChild(mTarget);
		ASSERT(mTarget->Window() != (BWindow *)(-1L));
	}
	mTarget = newTarget;
	if (!mTarget)
		return;
	mTargetBounds = mTarget->Bounds();
	//mTargetBounds.PrintToStream();
	BRect rect(Bounds());
	//rect.PrintToStream();
	AddChild(mTarget);
	mVBar->SetTarget(mTarget);
	mHBar->SetTarget(mTarget);
	mTarget->MoveTo(1,2);
	rect = Bounds();
	FrameResized(rect.Width(), rect.Height());
	ASSERT(mTarget->Window() != (BWindow *)(-1L));
}

void
BDynamicScrollView::SetTargetBounds(BRect bounds)
{
	mTargetBounds = bounds;
	BRect rect(Bounds());
	FrameResized(rect.Width(), rect.Height());
}

void 
BDynamicScrollView::DiscardTarget()
{
	mTarget = NULL;
}

void 
BDynamicScrollView::FrameResized(float width, float height)
{
//	if(width < mTargetBounds.Width() && mHBar->IsHidden())
//	{
//		mHBar->Show();
//		mScrollHeight = B_H_SCROLL_BAR_HEIGHT;
//		Invalidate();
//	}
//	else if(width >= mTargetBounds.Width() && !mHBar->IsHidden())
	{
		mHBar->Hide();
		mScrollHeight = 0;
		Invalidate();
		mHBar->Invalidate();
		mVBar->Invalidate();
		if(mTarget)
			mTarget->Invalidate();
	}

//	if(height < mTargetBounds.Height() && mVBar->IsHidden())
//	{
//		mVBar->Show();
//		mScrollWidth = B_V_SCROLL_BAR_WIDTH;
//		Invalidate();
//	}
//	else if(height >= mTargetBounds.Height() && !mVBar->IsHidden())
	{
		mVBar->Hide();
		mScrollWidth = 0;
		Invalidate();
		mHBar->Invalidate();
		mVBar->Invalidate();
		if(mTarget)
			mTarget->Invalidate();
	}
	height -= mScrollHeight;
	width -= mScrollWidth;

	if(mTarget)
		mTarget->ResizeTo(width-3, height-4);

	mHBar->ResizeTo(width - 1, B_H_SCROLL_BAR_HEIGHT);
//	mHBar->SetRange(0, mTargetBounds.Width() - width);
	mHBar->SetRange(0, 0);
	mHBar->SetProportion(width / mTargetBounds.Width());
	mVBar->ResizeTo(B_V_SCROLL_BAR_WIDTH, height - 1);
//	mVBar->SetRange(0, mTargetBounds.Height() - height);
	mVBar->SetRange(0, 0);
	mVBar->SetProportion(height / mTargetBounds.Height());
}

void 
BDynamicScrollView::Draw(BRect /*area*/)
{
	switch(mBorderStyle)
	{
		case B_FANCY_BORDER:
		{
			SetHighColor(128, 128, 128);
			BPoint rt = Bounds().RightTop();
			BPoint lt = Bounds().LeftTop();
			if(mHBar->IsHidden() && mVBar->IsHidden())
			{				
				if(!dynamic_cast<BTabView *>(mTarget))
				{
					StrokeLine(lt, rt);
					lt.y++;
					rt.y++;
					SetHighColor(255, 255, 255);
					StrokeLine(lt, rt);
				}
				break;
			}
			BPoint rb = Bounds().RightBottom();
			BPoint lb = Bounds().LeftBottom();
			BTabView *tabVu = dynamic_cast<BTabView *>(mTarget);
			if(!tabVu)
			{
				StrokeLine(lt, rt);
			}
			StrokeLine(lb, lt);
			BPoint a, b, c;
			a = b = c = rb;
			a.y -= mScrollHeight;
			c = rb;
			c.x -= mScrollWidth;
			b.x = c.x;
			b.y = a.y;
			SetHighColor(255, 255, 255);
			StrokeLine(rt, a);
			StrokeLine(a, b);
			StrokeLine(b, c);
			StrokeLine(c, lb);
			break;
		}
		case B_NO_BORDER:
		default:
			break;
	}
}

