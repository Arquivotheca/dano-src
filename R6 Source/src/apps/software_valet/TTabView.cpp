#include <string.h>
#include "TTabView.h"
#include "colors.h"
#include <List.h>
#include <Window.h>


TTabView::TTabView(BRect inFrame, const char *inName, ulong inResizingMode, ulong inFlags)
	: BView(inFrame, inName, inResizingMode, inFlags)
{
	mLabels = new BList();
	mViews = new BList();
	mCurrent =0;
	mBehavior =2;
}

TTabView::~TTabView()
{
	
	// Deallocate those strings we have been allocating
	while ( !mLabels->IsEmpty() )
	{
		delete[] mLabels->RemoveItem(0L);
	}

	delete mLabels;
	delete mViews;
}

void
TTabView::Draw(BRect inRect)
{
	inRect;
	
	float	theTotalWidth = 0;
	float	theLabelWidth = 0;
	float	last = 0;
	char	*theLabel;
	BRect	theRect;
	
	float	left,right,bottom,top;
	
	if (mBehavior == 1)
	{
		theRect = Bounds();
		theRect.top = 0;
		theRect.bottom = 19;
		SetHighColor(white);
		FillRect(theRect);
		
		// Start drawing first label
		for(long i=0;i<mLabels->CountItems();i++)
		{
			theLabel = (char*)mLabels->ItemAt(i);
			theLabelWidth = StringWidth(theLabel);
			
			left=theTotalWidth+1;
			top=2;
			right=theTotalWidth + theLabelWidth + 14;
			bottom=18;
			
			// Make the Rectangle
			SetDrawingMode(B_OP_COPY);
			
			if ( i == mCurrent )
			{
				BeginLineArray(7);
				AddLine(BPoint(last,bottom),BPoint(left,bottom), dkGray);
				AddLine(BPoint(last,bottom+1),BPoint(left+1,bottom+1), white);
	
				AddLine(BPoint(left,bottom),BPoint(left,top), dkGray);
				AddLine(BPoint(left,top),BPoint(right,top), dkGray);
				AddLine(BPoint(right,top),BPoint(right,bottom), dkGray);
				
				AddLine(BPoint(left+1,bottom),BPoint(left+1,top+1), white);
				AddLine(BPoint(left+1,top+1),BPoint(right-1,top+1), white);
				EndLineArray();
		
				theRect.Set(left+2,top+2,right-1,bottom+1);
				SetHighColor(ViewColor());
				FillRect(theRect);
			}
			else
			{
				BeginLineArray(6);
				AddLine(BPoint(last,bottom),BPoint(left,bottom), dkGray);
				AddLine(BPoint(last,bottom+1),BPoint(right,bottom+1), white);
				
				AddLine(BPoint(left,bottom),BPoint(left,top), dkGray);
				AddLine(BPoint(left,top),BPoint(right,top), dkGray);
				AddLine(BPoint(right,top),BPoint(right,bottom), dkGray);
				AddLine(BPoint(right,bottom),BPoint(left,bottom), dkGray);
				
	
				EndLineArray();			
				
				BeginLineArray(2);
			//	SetDrawingMode(B_OP_BLEND);
			//	AddLine(BPoint(left+1,bottom-1),BPoint(left+1,top+1), dkGray);
			//	AddLine(BPoint(left+2,top+1),BPoint(right-1,top+1), dkGray);
				AddLine(BPoint(left+1,bottom-1),BPoint(right-1,bottom-1), white);
				AddLine(BPoint(right-1,bottom-2),BPoint(right-1,top+1), white);
		
				EndLineArray();
		
				theRect.Set(left+2,top+2,right-2,bottom-2);
				SetHighColor(ViewColor());
				FillRect(theRect);
			}
			
			// Draw the String
			SetDrawingMode(B_OP_OVER);
			SetHighColor(black);
			MovePenTo(left + 6 ,14);
			DrawString(theLabel);
			
			last = right;
			
			theTotalWidth += theLabelWidth + 15;
			
		}
		
		BeginLineArray(2);
		AddLine(BPoint(last,bottom),BPoint(Bounds().right,bottom),dkGray);
		AddLine(BPoint(last,bottom+1),BPoint(Bounds().right,bottom+1),white);
		EndLineArray();
	}
	else
	// Behavior II
	{
			theRect = Bounds();
		theRect.top = 0;
		theRect.bottom = 19;
		//SetHighColor(white);
		//FillRect(theRect);
		
		// Start drawing first label
		for(long i=0;i<mLabels->CountItems();i++)
		{
			theLabel = (char*)mLabels->ItemAt(i);
			theLabelWidth = StringWidth(theLabel);
			
			left=theTotalWidth;
			top=0;
			right=theTotalWidth + theLabelWidth + 14;
			bottom=18;
			
			if (right > Bounds().right)
				right = Bounds().right;
			
			// Make the Rectangle
			SetDrawingMode(B_OP_COPY);
			
			theRect.Set(left,top,right,bottom);
			SetHighColor(ViewColor());
			FillRect(theRect);
			
			// Draw the String
			SetDrawingMode(B_OP_OVER);
			SetHighColor(black);
			MovePenTo(left + 6 ,14);
			DrawString(theLabel);
		
			if ( i == mCurrent )
			{
				BeginLineArray(5);
				AddLine(BPoint(last,bottom),BPoint(left,bottom), white);
	
				AddLine(BPoint(left,bottom),BPoint(left,top), white);
				AddLine(BPoint(left,top),BPoint(right-1,top), white);
				AddLine(BPoint(right,top+1),BPoint(right,bottom), black);
				AddLine(BPoint(right-1,top+1),BPoint(right-1,bottom), dkGray);
				
				EndLineArray();
		
			}
			else
			{
				top = 2;
				BeginLineArray(6);
				AddLine(BPoint(last,bottom),BPoint(left,bottom), white);
	
				AddLine(BPoint(left,bottom),BPoint(left,top), white);
				AddLine(BPoint(left,top),BPoint(right-1,top), white);
				AddLine(BPoint(right,top+1),BPoint(right,bottom-1), black);
				AddLine(BPoint(right-1,top+1),BPoint(right-1,bottom-1), dkGray);
				AddLine(BPoint(left,bottom),BPoint(right,bottom), white);
				
				EndLineArray();
						
			//	theRect.Set(left+2,top+2,right-2,bottom-2);
			//	SetHighColor(ViewColor());
			//	FillRect(theRect);
			}
			
				
			last = right;
			
			theTotalWidth += theLabelWidth + 15;
			
		}
		
		SetDrawingMode(B_OP_COPY);
		BeginLineArray(1);
		AddLine(BPoint(last,bottom),BPoint(Bounds().right,bottom),white);
		EndLineArray();
	
		// Draw outline around view
		top = Bounds().top + 18;
		left = Bounds().left;
		right = Bounds().right;
		bottom = Bounds().bottom;	
	
		BeginLineArray(5);
		AddLine(BPoint(left,top),BPoint(left,bottom),white);
		AddLine(BPoint(left+1,bottom),BPoint(right,bottom),black);
		AddLine(BPoint(right,bottom),BPoint(right,top+1),black);
		AddLine(BPoint(left+1,bottom-1),BPoint(right-1,bottom-1),dkGray);
		AddLine(BPoint(right-1,bottom-1),BPoint(right-1,top+1),dkGray);
		EndLineArray();
	}
}

void
TTabView::AddChild(BView* inView, const char *inLabel)
{
	mViews->AddItem(inView);
	
	char *theLabel = new char [ strlen(inLabel)+1 ] ;
	strcpy(theLabel,inLabel);
	
	mLabels->AddItem(theLabel);
	
	inView->ResizeTo( Bounds().Width()-3.0, Bounds().Height() - 21.0);
	inView->MoveTo(1,19);
	BView::AddChild(inView);
	
	if (mViews->CountItems() != 1)
	{
		inView->Hide();
	}
}

void
TTabView::AddChild(BView* inView)
{
	AddChild(inView,inView->Name());	
}

void
TTabView::AttachedToWindow()
{
	//SetFontName("Emily");
	//SetFontSize(12);
	
}

void
TTabView::ActivateView(long i)
{
	BRect allButtons;
	allButtons = Bounds();
	allButtons.top=0;
	allButtons.bottom=19;
	BView *v;
	
	if (i < mViews->CountItems()) { // sanity check on the index
		v = (BView*)mViews->ItemAt(mCurrent);
		if (v != NULL) {
			v->Hide();
		}

		mCurrent = i;
		Draw(allButtons);

		v = (BView *)mViews->ItemAt(mCurrent);
		if (v != NULL) {
			v->Show();
			v->MakeFocus(TRUE);
		}
		// Window()->UpdateIfNeeded();
	}
}

long
TTabView::CurrentView()
{
	return mCurrent;
}

void
TTabView::MouseDown(BPoint inPoint)
{
	inPoint;
	
	float	theTotalWidth = 0;
	float	theLabelWidth;
	char	*theLabel;
	BRect	theButton;
	BPoint	cursor;
	ulong	buttons;
	long	theSave = mCurrent;
	long	theFound;
	
	// float	left,right,bottom,top;
	
	GetMouse(&cursor, &buttons);

	while (buttons)
	{
		theFound = 0;
		theTotalWidth=0;
		for(long i=0;i<mLabels->CountItems();i++)
		{
			theLabel = (char*)mLabels->ItemAt(i);
			theLabelWidth = StringWidth(theLabel);
			
			theButton.Set(theTotalWidth+1,2,theTotalWidth + theLabelWidth + 15,18);
	
			if ( theButton.Contains(cursor) )
			{
				// Draw selected button
				if ( mCurrent != i)
				{
					ActivateView(i);
					Window()->UpdateIfNeeded();
				}
				theFound = 1;
				break;	
			}
			
			theTotalWidth += theLabelWidth + 15;
		}
		
		if (theFound == 0)
		{
			if ( mCurrent != theSave)
			{
				ActivateView(theSave);
				Window()->UpdateIfNeeded();
			}
		}
		
		snooze(20000);
		GetMouse(&cursor, &buttons);
	}
			
			
				
}

	
