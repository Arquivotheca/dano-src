
#include "PDFPageView.h"

#if PDF_PRINTING > 0
#include "PDFPrintJob.h"
#include <print/PrintJobEditSettings.h>
#include <Bitmap.h>
#endif

#include <functional>
#include <algorithm>


PDFPageView::PDFPageView(BRect frame, const char *name, PDFContentInstance *instance, PDFDocument *doc, uint32 openPage, float scale) :
	DocView(frame, name),
	fInstance(instance),
	fDoc(doc),
	fEngine(NULL),
	fScaleX(scale),
	fScaleY(scale),
#if PDF_PRINTING > 0
	fPrintThread(-1),
#endif
	fRawAnnotsArray(NULL),
	fLastAnnot(-1)
{
	uint32 flags = Flags();
	SetFlags(flags | B_SUBPIXEL_PRECISE | B_FULL_UPDATE_ON_RESIZE);
	if (fDoc && fDoc->InitCheck() == B_OK) {
		fPageCount = doc->PageCount();
		GoToPage(openPage);
	}
	SetViewColor(B_TRANSPARENT_COLOR);
}


PDFPageView::~PDFPageView()
{
	if (fRawAnnotsArray)
		fRawAnnotsArray->Release();
	delete fEngine;
}

const char *
PDFPageView::Title()
{
	return NULL;
//	if (fDoc)
//		return fDoc->Title();
}

status_t 
PDFPageView::ScaleToFit(BRect frame, uint32 scaleFlags)
{
	if (fEngine) {
		float cw = fEngine->ContentWidth();
		float ch = fEngine->ContentHeight();
		
		float wScale = frame.Width()/ cw ;
		float hScale = frame.Height()/ ch ;
		
		float scale = 1.0;
		
		if ((scaleFlags & SCALE_TO_WIDTH) && (scaleFlags & SCALE_TO_HEIGHT)) {
			scale = min_c(wScale, hScale);
		}
		else if (scaleFlags & SCALE_TO_WIDTH)
			scale = wScale;
		else if (scaleFlags & SCALE_TO_HEIGHT)
			scale = hScale;
		
		return SetScale(scale, scale);
	}	
	else
		return B_ERROR;
}

status_t 
PDFPageView::SetScale(float x, float y)
{
	bool scaleChanged = false;
	if (fScaleX != x) {
		fScaleX = x;
		scaleChanged = true;
	}
	if (fScaleY != y) {
		fScaleY = y;
		scaleChanged = true;
	}
	
	if (scaleChanged && fEngine) {
		fEngine->SetScale(fScaleX, fScaleY);
		fTransform = fEngine->DTM();
		ProcessAnnotations();
		Invalidate();
		Framework()->DocSizeChanged(fScaleY);
		return B_OK;
	}
	else
		return B_ERROR;
}

status_t 
PDFPageView::GetScale(float *x, float *y) const
{
	*x = fScaleX;
	*y = fScaleY;
	return B_OK;
}

uint32 
PDFPageView::GoToPage(PDFObject *page)
{
	if (!fEngine || fEngine->Page() != page) {
		fRawAnnotsArray->Release();
		fAnnots.clear();
		fRawAnnotsArray = 0;
		if (fEngine) {
			fEngine->SetContent(page);
			fEngine->SetScale(fScaleX, fScaleY);
		}
		else
			fEngine = new PDFDisplay(page, fScaleX, fScaleY);
		fTransform = fEngine->DTM();
		ProcessAnnotations();
		ScrollTo(B_ORIGIN);
		Invalidate();
		fCurrentPage = fDoc->GetPageNumber(page);
		// protect against setting the page before we get added to the framework
		if (Framework()) Framework()->DocPageChanged(fCurrentPage);
	}
	return fCurrentPage;
}

uint32 
PDFPageView::GoToPage(uint32 pageNum)
{
	if (!fDoc || pageNum > fPageCount) {
		return fCurrentPage;
	}
	
	PDFObject * pageObj = fDoc->GetPage(pageNum);
	if (!pageObj)
		return fCurrentPage;
	else
		return GoToPage(pageObj);
}

void 
PDFPageView::FrameResized(float newWidth, float newHeight)
{
	return DocView::FrameResized(newWidth, newHeight);
}

#if PDF_PRINTING > 0
void 
PDFPageView::Print()
{
	if (fPrintThread == -1) {
		fPrintThread = spawn_thread(print_entry, "pdf_printer_thread", B_LOW_PRIORITY, this);
		resume_thread(fPrintThread);
	}
}
bool 
PDFPageView::IsPrinting()
{
	return (fPrintThread != -1);
}


int32 
PDFPageView::print_entry(void *arg)
{
	return ((PDFPageView *)arg)->PrintFunc();
}

int32 
PDFPageView::PrintFunc()
{
	PDFPrintJob job(fDoc);
	status_t status = job.InitCheck();
	if (status == B_OK) {
		uint32 count = 1;
		BMessage *settings = job.Settings();
		BPrintJobEditSettings printSettings(*settings);
		delete settings;
		printSettings.SetFirstPage(fCurrentPage);
		printSettings.SetLastPage(fCurrentPage + count);
		BMessage *nuSettings = new BMessage(printSettings.Message());
		job.SetSettings(nuSettings);	
		job.StartJob(count, B_BITMAP_ACCEPTS_VIEWS); 		
	}
	fPrintThread = -1;
	return status;
}
#endif

void 
PDFPageView::ScrollTo(BPoint where)
{
	return DocView::ScrollTo(where);
}

void 
PDFPageView::Draw(BRect updateRect)
{
#if PDF_PRINTING > 0
#else

	if (BView::IsPrinting()) {
		SetHighColor(255,255,255);
		FillRect(Bounds(), B_SOLID_HIGH);
		if (fDoc)
			fDoc->ThrowAlert(B_NO_PRINTER);
		return;
	}
#endif
	if (!fEngine) {
		return;
	}
	BRegion clip;
	GetClippingRegion(&clip);
	fEngine->Display(this, &clip);
#ifndef NDEBUG
	Flush();
	BPoint offset(fEngine->ViewOffset());
	for (uint i = 0; i < fAnnots.size(); i++)
	{
		BRect rect(fAnnots[i].fFrame);
		rect.OffsetBy(offset);
		//printf("%d ", i); rect.PrintToStream();
		if (updateRect.Intersects(rect))
			InvertRect(rect);
	}
#endif
}

void 
PDFPageView::MouseDown(BPoint where)
{
	MakeFocus(true);
	SetExplicitFocus(true);
	
	//DocView::MouseDown(where);
	if (!fEngine)
		return;
	where -= fEngine->ViewOffset();
	int32 i = HitCheckAnnots(where);
	if (i >= 0)
	{
		PDFObject *dest = 0;
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
				PDFObject *ref = fDoc->GetNamedObject(PDFAtom.Dests, o);
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
			else if (o->IsName())
			{
#ifndef NDEBUG
				printf("Named destination "); o->PrintToStream(); printf("\n");
#endif
				dest = fDoc->GetNamedDestination(o);
#ifndef NDEBUG
				dest->PrintToStream(2); printf("\n");
#endif
				if (dest->IsDictionary())
				{
					PDFObject *tmp = dest;
					dest = tmp->Find(PDFAtom.D)->AsArray();
					tmp->Release();
				}
			}
			else
			{
#ifndef NDEBUG
				printf("/Dest not a name, string, or dictionary!\n");
				o = dict->Find(PDFAtom.A)->Resolve();
				o->PrintToStream(); printf("\n");
				o->Release();
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
				GoToPage(fDoc->GetPage(page));
			else if (page->IsNumber())
				GoToPage(fDoc->GetPage(page->GetInt32()));
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
PDFPageView::MouseMoved(BPoint where, uint32 code, const BMessage *msg)
{
	DocView::MouseMoved(where, code, msg);
}

void 
PDFPageView::GetPreferredSize(float *width, float *height)
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
PDFPageView::ProcessAnnotations(void)
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

int32 
PDFPageView::HitCheckAnnots(const BPoint &where) const
{
	int32 ii = -1;
	if (fAnnots.size())
	{
		less<Annot> annot_less;
		Annot annot;
		annot.fFrame.Set(where.x, where.y, where.x, where.y);
		ii = (lower_bound(fAnnots.begin(), fAnnots.end(), annot, annot_less) - fAnnots.begin()) - 1;
		while (ii >= 0)
		{
			if (fAnnots[ii].fFrame.Contains(where)) break;
			if (where.y < fAnnots[ii].fFrame.top) ii = 0;
			ii--;
		}
	}
	return ii;
}

bool 
PDFPageView::Annot::operator<(const Annot &other) const
{
	return (fFrame.top < other.fFrame.top) ||
		((fFrame.top == other.fFrame.top) && (fFrame.left < other.fFrame.left));
}

