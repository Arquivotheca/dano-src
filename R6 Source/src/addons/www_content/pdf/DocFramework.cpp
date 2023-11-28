
#include <Debug.h>
#include "DocFramework.h"
#include <ScrollBar.h>
#include "ToolView.h"
#include <stdio.h>

DocFramework::DocFramework(BRect frame, const char *name, ToolView *toolbar, DocView *doc) :
	BView(frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
	fDocument(NULL),
	fToolbar(toolbar)
{
	SetViewColor(0, 0, 0);
	BRect bounds(frame);
	bounds.OffsetTo(B_ORIGIN);
	
	fToolbar->SetFramework(this);
	fToolbar->ResizeTo(bounds.Width(), stdHeight);
	AddChild(fToolbar);
	
	BRect toolRect(fToolbar->Frame());
	bounds.top = toolRect.bottom + 1;
	
	//printf("bounds: (%.2f, %.2f, %.2f, %.2f)\n", bounds.left, bounds.top, bounds.right, bounds.bottom);
	
	BRect scrollRect(bounds);
	scrollRect.left = bounds.right - B_V_SCROLL_BAR_WIDTH;
	scrollRect.bottom = bounds.bottom - (B_H_SCROLL_BAR_HEIGHT + 1);
	//printf("v scrollRect(%.2f, %.2f, %.2f, %.2f)\n", scrollRect.left, scrollRect.top, scrollRect.right, scrollRect.bottom);
	fVertScroll = new BScrollBar(scrollRect, "FrameworkVScroll", NULL, 0, scrollRect.bottom, B_VERTICAL);
	AddChild(fVertScroll);
	
	scrollRect = bounds;
	scrollRect.top = bounds.bottom - B_H_SCROLL_BAR_HEIGHT;
	scrollRect.right = bounds.right -  B_V_SCROLL_BAR_WIDTH;
	fHorzScroll = new BScrollBar(scrollRect, "FrameworkHScroll", NULL, 0, scrollRect.right, B_HORIZONTAL);
	AddChild(fHorzScroll);
	
	if (doc)
		SetDocument(doc);
}

DocFramework::~DocFramework()
{
}

status_t 
DocFramework::SetDocument(DocView *doc, bool deleteOld)
{
//	LockLooper();
	if (!doc)
		return B_ERROR;
		
	if (fDocument) {
		fDocument->RemoveSelf();
		if (deleteOld)
			delete fDocument;
	}
	fDocument = doc;
	fDocument->SetFramework(this);

	// move the document to just under the
	BRect toolFrame = fToolbar->Frame();
	
	BRect contentFrame = Bounds();
	contentFrame.top += toolFrame.bottom + 1;
	contentFrame.bottom -= (B_H_SCROLL_BAR_HEIGHT + 1);
	contentFrame.right -= (B_V_SCROLL_BAR_WIDTH + 1);
	
	fDocument->MoveTo(contentFrame.LeftTop());
	fDocument->ResizeTo(contentFrame.Width(), contentFrame.Height());	
	
	if (fDocument)
		AddChild(fDocument);

	// retarget the scrollbars
	fVertScroll->SetTarget(fDocument);
	fHorzScroll->SetTarget(fDocument);
	
	// update scroll bar scales
	AdjustScrollBars();
	
	// hide and show scrollbars


//	UnlockLooper();
	return B_OK;
}

DocView *
DocFramework::Document() const
{
	return fDocument;
}

ToolView *
DocFramework::Toolbar() const
{
	return fToolbar;
}

void 
DocFramework::DocSizeChanged(float scale)
{
	// do we need to add any scrollbars?
	AdjustScrollBars();
	if (fToolbar)
		fToolbar->DocSizeChanged(scale);
}

void 
DocFramework::DocPageChanged(uint32 pageNum)
{
	ASSERT(this);
	// notify the toolview
	if (fToolbar)
		fToolbar->DocPageChanged(pageNum);
}

void 
DocFramework::GetPreferredSize(float *width, float *height)
{
	*width = *height = 150;
}

void 
DocFramework::ResizeToPreferred()
{
}

void 
DocFramework::AdjustScrollBars(void)
{
	float x, y;
	fDocument->GetPreferredSize(&x, &y);
	//printf("fDocument's prefered size: %f,%f\n", x, y);
	BRect docBounds(fDocument->Bounds());
	//printf("docBounds "); docBounds.PrintToStream(); printf("\n");
	float size =  docBounds.IntegerHeight() + 1;
	float range = y > size  ? y - size: 0;
	//printf("v size: %f\n", size);
	//printf("v->SetRange(0, %f)\n", range);
	//printf("v->SetProp(%f)\n", size / y);
	fVertScroll->SetRange(0, range);
	fVertScroll->SetProportion(size / y);
	fVertScroll->SetSteps(size/25, size/5);
	size = docBounds.IntegerWidth() + 1;
	range = x > size ? x - size: 0;
	//printf("h size: %f\n", size);
	//printf("h->SetRange(0, %f)\n", range);
	//printf("h->SetProp(%f)\n", size / x);
	fHorzScroll->SetRange(0, range);
	fHorzScroll->SetProportion(size / x);
	fHorzScroll->SetSteps(size/25, size/5);
}

void 
DocFramework::FrameResized(float , float )
{
	//printf("In DocFramework::FrameResized\n");
	AdjustScrollBars();
}

void 
DocFramework::Draw(BRect updateRect)
{
	if (BView::IsPrinting()) {
		SetHighColor(255,255,255);
		FillRect(Bounds(), B_SOLID_HIGH);
		return;
	}
}


void 
DocFramework::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case SCALE_TO_FIT:
		{
			if (fDocument) {
				msg->AddRect("rect", fDocument->Bounds());
				fDocument->MessageReceived(msg);
			}
			break;
		}
		
		case SET_SCALE:
		case GO_TO_PAGE:
		case GO_TO_NEXT:
		case GO_TO_PREV:
		case GO_TO_FIRST:
		case GO_TO_LAST:
		{
			if (fDocument)
			{
				fDocument->MessageReceived(msg);
			}
			break;
		}
		
		default:
			BView::MessageReceived(msg);
	}
}

//#pragma mark -

DocView::DocView(BRect frame, const char *name) :
	BView(frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	fPageCount(0),
	fCurrentPage(0),
	fFramework(NULL)
{
}

DocView::~DocView()
{
}

status_t 
DocView::SetFramework(DocFramework *framework)
{
	fFramework = framework;
	return B_OK;
}

DocFramework *
DocView::Framework() const
{
	return fFramework;
}

const char *
DocView::Title()
{
	return NULL;
}

status_t 
DocView::ScaleToFit(BRect , uint32 )
{
	return B_ERROR;
}

status_t 
DocView::SetScale(float , float )
{
	return B_ERROR;
}

status_t 
DocView::GetScale(float *x, float *y) const
{
	*x = *y = 1.0;
	return B_OK;
}

void 
DocView::GetPreferredSize(float *width, float *height)
{
	*width = *height = 150;
}

void 
DocView::ResizeToPreferred()
{
}

uint32 
DocView::PageCount() const
{
	return fPageCount;
}

uint32 
DocView::CurrentPage() const
{
	return fCurrentPage;
}

uint32 
DocView::GoToPage(uint32 pageNum)
{
	if (pageNum <= fPageCount)
		fCurrentPage = pageNum;
	return fCurrentPage;
}

uint32 
DocView::GoToNextPage()
{
	if (fCurrentPage < fPageCount)
		return GoToPage(fCurrentPage + 1);
	else
		return fCurrentPage;
}

uint32 
DocView::GoToPreviousPage()
{
	if (fCurrentPage > 1)
		return GoToPage(fCurrentPage - 1);
	else
		return fCurrentPage;
}

uint32 
DocView::GoToFirstPage()
{
	return GoToPage(1);
}

uint32 
DocView::GoToLastPage()
{
	return GoToPage(fPageCount);
}

bool 
DocView::IsPrinting()
{
	return false;
}

void 
DocView::Print()
{
}

void 
DocView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case SET_SCALE: {
			float x = 1.0;
			msg->FindFloat("scale", &x);
			SetScale(x, x);
		}
		break;
		
		case SCALE_TO_FIT: {
			BRect rect;
			if (msg->FindRect("rect", &rect) != B_OK)
				return;
			uint32 flags = 0;
			if (msg->FindInt32("flags", (int32 *)&flags) != B_OK)
				return;
			
			ScaleToFit(rect, flags);
		}	
		break;
		
		case GO_TO_PAGE: {
			uint32 page = 1;
			if (msg->FindInt32("page", (int32 *)&page) != B_OK)
				return;
			GoToPage(page);
		}
		break;
			
		case GO_TO_NEXT:
			GoToNextPage();
			break;
		
		case GO_TO_PREV:
			GoToPreviousPage();
			break;
			
		case GO_TO_FIRST:
			GoToFirstPage();
			break;
			
		case GO_TO_LAST:
			GoToLastPage();
			break;
	
		default:
			BView::MessageReceived(msg);
	}
}

