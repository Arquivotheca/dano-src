#include "PageView.h"
#include <functional>
#include <algorithm>
#include "NumControl.h"

bool 
PageView::Annot::operator<(const Annot &other) const
{
	return fFrame.top < other.fFrame.top ||
		((fFrame.top == other.fFrame.top) && (fFrame.left < other.fFrame.left));
}


PageView::PageView(PDFObject *page, BRect frame, const char *name) :
	BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_SUBPIXEL_PRECISE ),
	fEngine(new RenderEngine(page)), fXscale(1.0), fYscale(1.0), fRawAnnotsArray(0),
	fLastAnnot(-1)
{
}


PageView::~PageView()
{
	fRawAnnotsArray->Release();
	delete fEngine;
}

void 
PageView::SetTo(PDFObject *page)
{
	if ((fEngine && fEngine->Page() != page))
	{
		fRawAnnotsArray->Release();
		fAnnots.clear();
		fRawAnnotsArray = 0;
		delete fEngine;
		fEngine = new RenderEngine(page, fXscale, fYscale);
		fTransform = fEngine->DTM();
		ProcessAnnotations();
		Invalidate();
		NumControl *nc = dynamic_cast<NumControl *>(Parent()->FindView("NumControl"));
		if (nc) nc->SetValue(fEngine->Document()->GetPageNumber(page));
	}
	else page->Release();
}

void 
PageView::SetZoom(float x, float y)
{
	if ((x != fXscale) || (y != fYscale))
	{
		fEngine->SetScale(fXscale = x, fYscale = y);
		fTransform = fEngine->DTM();
		ProcessAnnotations();
		Invalidate();
	}
}

#if 0
void 
PageView::FrameResized(float newWidth, float newHeight)
{
	BRect frame(0, 0, newWidth, newHeight);
	frame.OffsetTo(LeftTop());
	fEngine->SetFrame(frame);
	BView::FrameResized(newWidth, newHeight);
}

void 
PageView::ScrollTo(BPoint where)
{
	fEngine->ScrollTo(where);
	BView::ScrollTo(where);
}
#endif

void 
PageView::Draw(BRect updateRect)
{
	BRegion clip;
	GetClippingRegion(&clip);
	fEngine->Display(this, &clip);
#ifndef NDEBUG
	Flush();
	for (uint i = 0; i < fAnnots.size(); i++)
	{
		BRect rect(fAnnots[i].fFrame);
		//printf("%u ", i); rect.PrintToStream();
		if (updateRect.Intersects(rect))
			InvertRect(rect);
	}
#endif
}

int32 
PageView::HitCheckAnnots(const BPoint &where) const
{
	int32 ii = -1;
	if (fAnnots.size())
	{
		less<Annot> annot_less;
		Annot annot;
		annot.fFrame.Set(where.x, where.y, where.x, where.y);
		vector<Annot>::const_iterator i = lower_bound(fAnnots.begin(), fAnnots.end(), annot, annot_less);
		// i points to the one _after_
		ii = (i - fAnnots.begin()) - 1;
		if ((ii < 0) || (ii >= (int32)(fAnnots.size())) || !fAnnots[ii].fFrame.Contains(where))
			ii = -1;
	}
	return ii;
}

void 
PageView::MouseDown(BPoint where)
{
	int32 i = HitCheckAnnots(where);
	if (i >= 0)
	{
		PDFObject *dest = 0;
		PDFDocument *doc = fEngine->Document();
		// get dictionary
		PDFObject *dict = fAnnots[i].fDict;
		//printf("MouseDown()\n"); dict->PrintToStream(3); printf("\n");
		PDFObject *o = dict->Find(PDFAtom.Subtype);
		if (o->GetCharPtr() == PDFAtom.Link)
		{
			o = dict->Find(PDFAtom.Dest);
			if (o->IsString())
			{
				// look up the named dest in the
				PDFObject *ref = doc->GetNamedObject(PDFAtom.Dests, o);
				PDFObject *destdict = ref->Resolve();
				//printf(" * * * * *\n");destdict->PrintToStream(3);printf("\n * * * * *\n");
				if (destdict->IsDictionary())
				{
					dest = destdict->Find(PDFAtom.D);
					dest->Acquire();
				}
				else
				{
#ifndef NDEBUG
					printf("destdict not a dictionary!\n");
					destdict->PrintToStream(); printf("\n");
#endif
				}
				destdict->Release();
				ref->Release();
			}
			else if (o->IsArray())
			{
				dest = o;
				dest->Acquire();
			}
			else
			{
#ifndef NDEBUG
				printf("/Dest not a string or dictionary!\n");
				o->PrintToStream(); printf("\n");
#endif
			}
		}
		else
		{
#ifndef NDEBUG
			printf("/Subtype not /Link\n");
			o->PrintToStream(); printf("\n");
#endif
		}

		if (dest->IsArray())
		{
			// [0] should contain page reference, either number or ref
			PDFObject *page = (*(dest->Array()))[0];
#ifndef NDEBUG
			printf("Link page %p is ", page); page->PrintToStream(); printf("\n");
#endif
			if (page->IsReference())
				SetTo(doc->GetPage(page));
			else if (page->IsNumber())
				SetTo(doc->GetPage(page->GetInt32()));
			else
			{
#ifndef NDEBUG
				printf("page neither reference nor number\n");
				page->PrintToStream(); printf("\n");
#endif
			}
		}
		else
		{
#ifndef NDEBUG
			printf("dest not an array!\n");
			dest->PrintToStream(); printf("\n");
#endif
		}
		dest->Release();
	}
}

void 
PageView::MouseMoved(BPoint, uint32, const BMessage *)
{
}

void 
PageView::GetPreferredSize(float *width, float *height)
{
	if (fEngine)
	{
		*width = fEngine->Width();
		*height = fEngine->Height();
	}
	else
	{
		*width = *height = 1;
	}
}

void 
PageView::ResizeToPreferred()
{
	float width = 1;
	float height = 1;
	if (fEngine)
	{
		width = fEngine->Width();
		height = fEngine->Height();
	}
	ResizeTo(width, height);
	FrameResized(width, height);
}

void 
PageView::ProcessAnnotations(void)
{
	// throw away existing annotation data
	fRawAnnotsArray->Release();
	fAnnots.clear();
	// get the new annotations, if any
	if ((fRawAnnotsArray = fEngine->Annotations()))
	{
		// we need all of the rectangles
		fRawAnnotsArray->ResolveArrayOrDictionary();
		// size our array appropriately
		fAnnots.reserve(fRawAnnotsArray->size());
		// walk the array, plucking each rect and dict into our sortable vector
		for (object_array::iterator i = fRawAnnotsArray->begin(); i < fRawAnnotsArray->end(); i++)
		{
			PDFObject *rect = (*i)->Find(PDFAtom.Rect)->AsArray();
			if (rect)
			{
				Annot annot;
				object_array *oa = rect->Array();
				annot.fFrame.Set((*oa)[0]->GetFloat(), (*oa)[1]->GetFloat(), (*oa)[2]->GetFloat(), (*oa)[3]->GetFloat());
				// store hit-test rect in view coordinates
				fTransform.Transform((BPoint*)&annot.fFrame, 2);
				// ensure normalized rectangles
				if (annot.fFrame.left > annot.fFrame.right) swap(annot.fFrame.left, annot.fFrame.right);
				if (annot.fFrame.top > annot.fFrame.bottom) swap(annot.fFrame.top, annot.fFrame.bottom);
				annot.fDict = *i;
				fAnnots.push_back(annot);
				rect->Release();
			}
		}
		if (fAnnots.size())
		{
			less<Annot> annot_less;
			sort(fAnnots.begin(), fAnnots.end(), annot_less);
#if 0
			for (vector<Annot>::iterator i = fAnnots.begin(); i < fAnnots.end(); i++)
			{
				i->fFrame.PrintToStream();
				i->fDict->PrintToStream();
				printf("\n");
			}
#endif
		}
	}
}
