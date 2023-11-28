
#include "RenderEngine.h"
#include "ObjectParser.h"
#include <View.h>
#include <Window.h>
#include <Bitmap.h>
#include <Shape.h>
#include <float.h>
#include <stdlib.h>
#include <ctype.h>
#include "PDFKeywords.h"
#include "PushContents.h"
#include "ImageChain.h"
#include "FontEngine.h"
#include <math.h>
#include <ResourceCache.h>
#include <Binder.h>
#include <Debug.h>
#include "pdf_doc.h"
using namespace Wagner;
#include <stdio.h>
#if 0
#include <stdio.h>
#include <OS.h>
//#define DFN(a) if (PDF_##a != fCurrentKey) { char buffer[64]; sprintf(buffer, #a"Fn() mismatch: %ld vs. %d\n", fCurrentKey, PDF_##a); debugger(buffer); }
#define DFN(a) { printf(#a"Fn() : %d\n", PDF_##a); }
#else
#define DFN(a)
#endif

namespace BPrivate {

struct TState {
					TState();
					TState(const TState &clone);
					~TState();
	void			Reset();
	TState &		operator=(const TState& rhs);
	float			Tc;			//character spacing - unscaled by Tfs
	float			Tw;			//word spacing - unscaled by Tfs
	float			Th;			//horizontal scaling
	float			Tl;			//leading - unscaled by Tfs
	FontEngine *	Tf;			//font dictionary
	float			Tfs;		//text font size
	uint8			Tmode;		//text rendering mode
	float			Trise;		//text rise - unscaled by Tfs
};

TState::TState() :
	Tc(0), Tw(0), Th(1), Tl(0), Tf(NULL), Tfs(0), Tmode(0), Trise(0)
{
}


TState::TState(const TState &clone)
{
	*this = clone;
}


TState::~TState()
{
}


void
TState::Reset()
{
	Tc = 0;
	Tw = 0;
	Th = 1;
	Tl = 0;
	Tf = NULL;
	Tfs = 0;
	Tmode = 0;
	Trise = 0;
}

TState &
TState::operator=(const TState &rhs)
{
	Tc = rhs.Tc;
	Tw = rhs.Tw;
	Th = rhs.Th;
	Tl = rhs.Tl;
	Tfs = rhs.Tfs;
	Tmode = rhs.Tmode;
	Trise = rhs.Trise;
	
	Tf = rhs.Tf;
	return *this;
}

//#pragma mark -
struct ColorInfo {
	PDFObject				*space;
	rgb_color				color;
	float					whitepoint[3];
	float					range[3][2];
	void					SetXYZColor(float x, float y, float z);
	void 					SetLab(PDFObject *labspace);
	float					LabG(float x);
	void					SetLabColor(float l, float a, float b);
};


static float xyz[3][3] = {
	{  3.2410, -1.5374, -0.4986},
	{ -0.9692,  1.8760,  0.0416},
	{  0.0556, -0.2040,  1.0570}
};	


void 
ColorInfo::SetXYZColor(float x, float y, float z)
{
	// set color from XYZ values
	float R = xyz[0][0] * x  +  xyz[0][1] * y  +  xyz[0][2] * z;
	float G = xyz[1][0] * x  +  xyz[1][1] * y  +  xyz[1][2] * z;
	float B = xyz[2][0] * x  +  xyz[2][1] * y  +  xyz[2][2] * z;
	if ((R < 0.00304) && (G < 0.00304) && (B < 0.00304))
	{
		R *= 12.92;
		G *= 12.92;
		B *= 12.92;
	}
	else
	{
		double gamma = 1.0/2.4;
		R = 1.055 * pow(R, gamma);
		G = 1.055 * pow(G, gamma);
		B = 1.055 * pow(B, gamma);
	}
	color.red = (uint8)(R < 0.0 ? 0 : (R > 1.0 ? 255 : R * 255));
	color.green = (uint8)(G < 0.0 ? 0 : (G > 1.0 ? 255 : G * 255));
	color.blue = (uint8)(B < 0.0 ? 0 : (B > 1.0 ? 255 : B * 255));
	color.alpha = 255;
}

void 
ColorInfo::SetLab(PDFObject *labspace)
{
	// find required WhitePoint
	PDFObject *obj = labspace->Find(PDFAtom.WhitePoint);
	object_array *oa = obj->Array();
	whitepoint[0] = ((*oa)[0])->GetFloat();
	whitepoint[1] = ((*oa)[1])->GetFloat();
	whitepoint[2] = ((*oa)[2])->GetFloat();
	range[1][0] = 0;
	range[1][1] = 100;
	obj = labspace->Find(PDFAtom.Range);
	if (obj)
	{
		oa = obj->Array();
		range[0][0] = ((*oa)[0])->GetFloat();
		range[0][1] = ((*oa)[1])->GetFloat();
		range[2][0] = ((*oa)[2])->GetFloat();
		range[2][1] = ((*oa)[3])->GetFloat();
	}
	else
	{
		range[0][0] = -100;
		range[0][1] =  100;
		range[2][0] = -100;
		range[2][1] =  100;
	}
}

float 
ColorInfo::LabG(float x)
{
	if (x > (6.0/29.0)) return x * x * x;
	return (108.0 / 841.0) * (x - (4.0/29.0));
}

void 
ColorInfo::SetLabColor(float l, float a, float b)
{
	// clamp to range
	if (a < range[0][0]) l = range[0][0];
	if (a > range[0][1]) l = range[0][1];
	if (l < range[1][0]) l = range[1][0];
	if (l > range[1][1]) l = range[1][1];
	if (b < range[2][0]) l = range[2][0];
	if (b > range[2][1]) l = range[2][1];
	float M = (l + 16.0)/116.0;
	float L = M + (a/500.0);
	float N = M + (b/200.0);
	float X = whitepoint[0] * LabG(L);
	float Y = whitepoint[1] * LabG(M);
	float Z = whitepoint[2] * LabG(N);
	SetXYZColor(X, Y, Z);
}

//#pragma mark -

// graphic state
struct GState {
	BShape *				fShape;	// the current shape
	bool					fShapeTransformed;
	int8					fShapeVisibility;
	BPoint					fCurrentPoint;
	BPoint					fSubPathStart;
	PDFObject				*fLineDash;
	float					fLineDashPhase;
	uint8					fFlatness;
	uint8					fLineCap;
	uint8					fLineJoin;
	float					fLineWidth;
	float					fMiterLimit;
	PDFObject				*fRenderingIntent;
	ColorInfo				fFillColor;
	ColorInfo				fStrokeColor;
	Transform2D				fCTM;

	TState					fTS;

	static GState *			Push(const Transform2D& ctm);
	GState *				Push(void);
	GState *				Pop(void);
	
	
private:
	GState *				fPrev;	// link to previous graphics state
							GState(const Transform2D& ctm);
							GState(GState *prev);
							GState();
							GState(const GState &prev);
							~GState();
	void					InitState(const Transform2D &ctm);
	GState &				operator=(const GState& rhs);
};

void
GState::InitState(const Transform2D &ctm)
{
	fPrev = 0;
	fShapeTransformed = false;
	fCTM = ctm;
	fCurrentPoint.Set(0,0);
	fFillColor.color.red = fFillColor.color.blue = fFillColor.color.green = 0;
	fFillColor.color.alpha = 255;
	fStrokeColor.color.red = fStrokeColor.color.blue = fStrokeColor.color.green = 0;
	fStrokeColor.color.alpha = 255;
	fFillColor.space = PDFObject::makeName(PDFAtom.DeviceGray);
	fStrokeColor.space = PDFObject::makeName(PDFAtom.DeviceGray);
	fRenderingIntent = PDFObject::makeName(PDFAtom.RelativeColorimetric);
	fLineDash = PDFObject::makeArray();
	fLineDashPhase = 0;
	fFlatness = 0;
	fLineCap = B_BUTT_CAP;
	fLineJoin = B_MITER_JOIN;
	fLineWidth = 1.0;
	fMiterLimit = 10.0;
	fShape = new BShape();
	fShapeVisibility = -1;
	fTS.Reset();
}

GState::GState(const Transform2D &ctm)
{
	InitState(ctm);
}

GState::GState(GState *prev)
{
	//debugger("GState *prev");
	*this = *prev;
	fPrev = prev;
}


GState::~GState()
{
	delete fShape;
	fLineDash->Release();
	fFillColor.space->Release();
	fStrokeColor.space->Release();
	fRenderingIntent->Release();
}

GState &
GState::operator=(const GState &rhs)
{
	//debugger("Copying GState");
	if (this != &rhs)
	{
		fShape = new BShape(*rhs.fShape);
		fShapeTransformed = rhs.fShapeTransformed;
		fShapeVisibility = rhs.fShapeVisibility;
		fCurrentPoint = rhs.fCurrentPoint;
		fSubPathStart = rhs.fSubPathStart;
		fLineDash = rhs.fLineDash;
		fLineDash->Acquire();
		fLineDashPhase = rhs.fLineDashPhase;
		fFlatness = rhs.fFlatness;
		fLineCap = rhs.fLineCap;
		fLineJoin = rhs.fLineJoin;
		fLineWidth = rhs.fLineWidth;
		fMiterLimit = rhs.fMiterLimit;

		fFillColor = rhs.fFillColor;
		fFillColor.space->Acquire();

		fStrokeColor = rhs.fStrokeColor;
		fStrokeColor.space->Acquire();

		fRenderingIntent = rhs.fRenderingIntent;
		fRenderingIntent->Acquire();
	
		fCTM = rhs.fCTM;
		fTS = rhs.fTS;
	}
	return *this;
}

GState *
GState::Push(const Transform2D &ctm)
{
	return new GState(ctm);
}

GState *
GState::Push(void)
{
	return new GState(this);
}

GState *
GState::Pop(void)
{
	GState *prev = fPrev;
	delete this;
	return prev;
}


//#pragma mark -


class TransformIterator : public BShapeIterator {
private:
const Transform2D		&fM;

public:
						TransformIterator(const Transform2D &m) : BShapeIterator(), fM(m) {};
						
virtual	status_t		IterateMoveTo(BPoint *point);
virtual	status_t		IterateLineTo(int32 lineCount, BPoint *linePts);
virtual	status_t		IterateBezierTo(int32 bezierCount, BPoint *bezierPts);

};


status_t 
TransformIterator::IterateMoveTo(BPoint *point)
{
	fM.Transform(point, 1);
#if 0
	point->x = floorf(point->x);
	point->y = floorf(point->y);
	printf("imt %f,%f\n", point->x, point->y);
#endif
	return B_OK;
}

status_t 
TransformIterator::IterateLineTo(int32 lineCount, BPoint *linePts)
{
	fM.Transform(linePts, lineCount);
#if 0
	while (lineCount--)
	{
		linePts->x = floorf(linePts->x);
		linePts->y = floorf(linePts->y);
		printf("ilt %f,%f\n", linePts->x, linePts->y);
		linePts++;
	}
#endif
	return B_OK;
}

status_t 
TransformIterator::IterateBezierTo(int32 bezierCount, BPoint *bezierPts)
{
	fM.Transform(bezierPts, bezierCount * 3);
#if 0
	while (bezierCount--)
	{
		bezierPts->x = floorf(bezierPts->x);
		bezierPts->y = floorf(bezierPts->y);
		printf("ibt %f,%f ", bezierPts->x, bezierPts->y);
		bezierPts++;
		bezierPts->x = floorf(bezierPts->x);
		bezierPts->y = floorf(bezierPts->y);
		printf("%f,%f ", bezierPts->x, bezierPts->y);
		bezierPts++;
		bezierPts->x = floorf(bezierPts->x);
		bezierPts->y = floorf(bezierPts->y);
		printf("%f,%f\n", bezierPts->x, bezierPts->y);
		bezierPts++;
	}
#endif
	return B_OK;
}

}; // end namespace BPrivate

//#pragma mark -

using namespace BPrivate;

#if USE_TEE_PUSHER
#include "TeePusher.h"
#endif


#define BITMAP_WATCHER 0

#if BITMAP_WATCHER > 0
#include <Window.h>
#include <View.h>
static BWindow * bitWindow = NULL;
static BView * bitView = NULL;
static bool bitmap_watcher_init = false;

static
void update_bitmap_watcher(BBitmap *bitmap) {
	if (bitmap_watcher_init == false) {
		bitWindow = new BWindow(BRect(550, 50, 1050, 750), "RenderBitmapWatcher", B_TITLED_WINDOW, 0);
		bitView = new BView(bitWindow->Bounds(), "bitView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
		bitView->SetViewColor(155, 155, 155);
		bitWindow->AddChild(bitView);
		bitView->SetScale(.102564);
		bitWindow->Show();
		bitmap_watcher_init = true;
	}
	
	if (bitWindow && bitView && bitmap) {
		if (bitWindow->Lock()) {
			bitView->DrawBitmap(bitmap, B_ORIGIN);
//			bitView->DrawBitmap(bitmap, bitView->Bounds());
			bitView->Sync();
			bitWindow->Unlock();
		}
	}
}
#endif

#define GLYPH_WATCHER 0

#if GLYPH_WATCHER > 0
#include <Window.h>
#include <View.h>
#include <OS.h>
static BWindow * glyphWindow = NULL;
static BView * glyphView = NULL;
static bool glyph_watcher_init = false;
static BBitmap *glyphBits = NULL;

static
void init_glyph_watcher(int32 width, int32 height) {
	if (glyph_watcher_init == false) {
		BRect rect(0, 0, width, height);
		rect.OffsetTo(550, 50);
		glyphWindow = new BWindow(rect, "GlyphWatcher", B_TITLED_WINDOW, 0);
		glyphView = new BView(glyphWindow->Bounds(), "glyphView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
		glyphView->SetViewColor(155, 155, 155);
		glyphWindow->AddChild(glyphView);
		glyphWindow->Show();
		rect.OffsetTo(B_ORIGIN);
		glyphBits = new BBitmap(rect, B_RGB32);
		memset(glyphBits->Bits(), 0xff, glyphBits->BitsLength());
		glyph_watcher_init = true;
	}
}

static
void resize_glyph_bits(int32 width, int32 height) {
	if (!glyph_watcher_init)
		init_glyph_watcher(width, height);
	else {
		glyphWindow->FrameResized(width, height);
		delete glyphBits; glyphBits = NULL;
		glyphBits = new BBitmap(BRect(0,0,width, height), B_RGB32);
	}
}

static
void update_glyph_watcher() {
	if (glyphWindow && glyphView && glyphBits) {
		if (glyphWindow->Lock()) {
			glyphView->SetLowColor(255, 255, 255);
			glyphView->FillRect(glyphView->Bounds(), B_SOLID_LOW);
			glyphView->DrawBitmap(glyphBits, B_ORIGIN);
			glyphWindow->Unlock();
//			snooze(300000);
		}
	}
}
#endif



RenderEngine::RenderEngine(PDFObject *object, float scaleX, float scaleY) :
	fFinder(object),
	fScaleX(scaleX),
	fScaleY(scaleY),
	fCrop(B_ORIGIN),
	fContentHeight(-1),
	fContentWidth(-1),
	fBitmap(NULL),
	fBitmapView(NULL),
	fGS(NULL),
	fClipPicture(NULL),
	fParent(NULL),
	fUnsupportedDisplayed(false)
{
#if USE_TEE_PUSHER
	fParent = new TeePusher(new ObjectParser(this), "/boot/home/pagedata.txt");
#else
//	printf("NOT USING TEE PUSHER!!!\n");
	fParent = new ObjectParser(this);
#endif
	InitData();
}


RenderEngine::~RenderEngine()
{
	if (fParent) {
		// separate ourselves from our parent
		fParent->SetSink(0);
		// to prevent double destruction when we kill it
		delete fParent;
	}
	// no more state stack
	empty_GS_stack();
	// free up the bitmap view
	if (fBitmapView) {
		delete fBitmapView;
		fBitmapView = NULL;
	}
}

status_t 
RenderEngine::RenderBitmap(BBitmap &inTo, const BPoint &offset, const BRegion &clip)
{
//	printf("RenderEngine::RenderBitmap()========================================\n");
	// set and lock the bitmap
	fBitmap = &inTo;
	fBitSpace = inTo.ColorSpace();
	fBitmap->Lock();

	// bitRect is the bounds of the content to render
	fBitmapFrame = fBitmap->Bounds();
	fBitmapFrame.OffsetBy(offset);
	// fClip is the clipping region of the actual bits to touch
	// we stash it here so we can restore it as a baseline
	fClip.Set(fBitmapFrame);
	fClip.IntersectWith(&clip);
	
	// clear the bitmap
	uint32 *bits = (uint32 *)fBitmap->Bits();
	memset(bits, 0xFF, fBitmap->BitsLength());
	
	// resize, attach and scroll the bitmap view to represent the right bits
	if (!fBitmapView) {
		fBitmapView = new BView(fBitmap->Bounds(), "BitmapView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_SUBPIXEL_PRECISE);
		fBitmapView->SetViewColor(B_TRANSPARENT_COLOR);
	}
	BRect bitViewBounds = fBitmapView->Bounds();
	if (fBitmapFrame.Width() != bitViewBounds.Width() || fBitmapFrame.Height() != bitViewBounds.Height())
		fBitmapView->ResizeTo(fBitmapFrame.Width(), fBitmapFrame.Height());
	fBitmapView->MoveTo(B_ORIGIN);
	fBitmap->AddChild(fBitmapView);
	fBitmapView->ScrollTo(offset);

	// contrain the clipping region of the view
	fBitmapView->ConstrainClippingRegion(&fClip);
	
	// run the content through the grinder

	PDFObject *content = Content();
	if (content) {
		PDFObject *unresolved = content->Find(PDFAtom.Contents);
		if (unresolved) {
			PDFObject *contents = unresolved->Resolve();
			if (contents) {
				PushContent(contents, fParent);
				contents->Release();
			}
		}
			
	}

	// unhook the bitmap view	
	fBitmapView->Sync();
	fBitmap->RemoveChild(fBitmapView);
	
	fBitmap->Unlock();
	// forget the bitmap
	fBitmap = NULL;
	return B_OK;
}

status_t 
RenderEngine::SetContent(PDFObject *content)
{
	status_t status = fFinder.SetBase(content);
	if (status == B_OK) {
		InitData();
		ContentChanged();
	}
	return status;
}

status_t 
RenderEngine::SetScale(float x, float y)
{
	if ((fScaleX != x) || (fScaleY != y))
	{
		fScaleX = x; fScaleY = y;
		fDTM.Set(fScaleX, 0, 0, -fScaleY, fCrop.x * fScaleX, fCrop.y * fScaleY);
		ScaleChanged();
	}
	return B_OK;
}

void 
RenderEngine::ScaleChanged()
{
}

void 
RenderEngine::ContentChanged()
{
}

void 
RenderEngine::InitData()
{
	empty_GS_stack();

	if (fFinder.Base()) {

		PDFObject *obj = Find(PDFAtom.CropBox, true);
		if (!obj)  obj = Find(PDFAtom.MediaBox, true);
		if (!obj)
		{
			// should never happen
//			printf("WHOA!  Couldn't find crop or media box\n");
			obj = PDFObject::makeArray();
			object_array *_a = obj->Array();
			_a->push_back(PDFObject::makeNumber(0.0));
			_a->push_back(PDFObject::makeNumber(0.0));
			_a->push_back(PDFObject::makeNumber( 8.5 * 72));
			_a->push_back(PDFObject::makeNumber(11.0 * 72));
		}
		if (obj->IsArray())
		{
			object_array *crop = obj->Array();
			float llx, lly, urx, ury;
			llx = ((*crop)[0])->GetFloat();
			lly = ((*crop)[1])->GetFloat();
			urx = ((*crop)[2])->GetFloat();
			ury = ((*crop)[3])->GetFloat();
			fCrop.Set(-llx, ury);
			fContentWidth = urx - llx;
			fContentHeight = ury - lly;
			fDTM.Set(fScaleX, 0, 0, -fScaleY, fCrop.x * fScaleX, fCrop.y * fScaleY);
			// initial graphics state: default matrix [1 0 0 1 0 0]
			fGS = GState::Push(Transform2D());
//			fGS = GState::Push(Transform2D(fScaleX, 0, 0, fScaleY, 0, 0), Document());
		}
#ifndef NDEBUG
		else debugger("crop/media_box is not array!");
#endif
		// don't need obj any longer
		obj->Release();
#ifndef NDEBUG
		obj = Find(PDFAtom.Resources, true);
		obj->ResolveArrayOrDictionary();
		printf("Page Resources "); obj->PrintToStream(); printf("\n");
		obj->Release();
#endif
	}
}

void 
RenderEngine::empty_GS_stack()
{
	while (fGS)
		fGS = fGS->Pop();
	fGS = 0;
}

void 
RenderEngine::setrgbcolor(rgb_color &color)
{
	PDFObject *b = PopStack();
	PDFObject *g = PopStack();
	PDFObject *r = PopStack();
	
	color.red = (uint8)((double)255 * (double)r->GetFloat());
	color.green = (uint8)((double)255 * (double)g->GetFloat());
	color.blue = (uint8)((double)255 * (double)b->GetFloat());
	color.alpha = 255;
	r->Release();
	g->Release();
	b->Release();
}

void 
RenderEngine::setgreycolor(rgb_color &color)
{
	PDFObject *g = PopStack();
	uint8 grey = (uint8)((double)255 * g->GetFloat());
	color.red = color.green = color.blue = grey;
	color.alpha = 255;
	g->Release();
}

void 
RenderEngine::setcmykcolor(rgb_color &color)
{
	PDFObject *k = PopStack();
	PDFObject *y = PopStack();
	PDFObject *m = PopStack();
	PDFObject *c = PopStack();
	//printf("cmyk: %f %f %f %f\n", c->GetFloat(), m->GetFloat(), y->GetFloat(), k->GetFloat());
	uint16 k_int = (uint16)(k->GetFloat() * 255);
	if (k_int > 255) k_int = 255;
	uint16 tmp = (uint16)(c->GetFloat() * 255) + k_int;
	if (tmp > 255) tmp = 255;
	color.red = 255 - tmp;
	tmp = (uint16)(m->GetFloat() * 255) + k_int;
	if (tmp > 255) tmp = 255;
	color.green = 255 - tmp;
	tmp = (uint16)(y->GetFloat() * 255) + k_int;
	if (tmp > 255) tmp = 255;
	color.blue = 255 - tmp;
	color.alpha = 255;
	//ASSERT(0 == 1);
	c->Release();
	m->Release();
	y->Release();
	k->Release();
}

void 
RenderEngine::setcolorspace(ColorInfo &color, const char *space)
{
	PDFObject *name_obj = color.space;
	if ((name_obj->GetCharPtr()) != space)
	{
		name_obj->Release();
		name_obj = PDFObject::makeName(space);
		const char *name = name_obj->GetCharPtr();

		if (name == PDFAtom.DeviceGray)// (|| (name == PDFAtom.CalGray))
		{
		}
		else if (name == PDFAtom.Indexed)
		{
		}
		else if ((name == PDFAtom.DeviceRGB) || (name == PDFAtom.CalRGB))
		{
		}
		else if (name == PDFAtom.DeviceCMYK)
		{
		}
		else if (name == PDFAtom.Pattern)
		{
			printf("!!!! non-array'd pattern color space!\n");
		}
		else if (name == PDFAtom.Separation)
		{
			printf("!!!! non-array'd /Separation color space\n");
		}
		else {
			//printf("FIXME! named color space!\n");
			PDFObject *cs_info = FindResource(PDFAtom.ColorSpace, name);
			//printf("named space %s ", name); cs_info->PrintToStream(); printf("\n");
			if (cs_info->IsArray())
			{
				object_array *cs_array = cs_info->Array();
				PDFObject *space_obj = (*cs_array)[0];
				name = space_obj->GetCharPtr();
				if (name == PDFAtom.CalRGB)
				{
					name_obj->Release();
					name_obj = PDFObject::makeName(PDFAtom.DeviceRGB);
				}
				else if (name == PDFAtom.Separation)
				{
					PDFObject *sep_name = (*cs_array)[1];
					name = sep_name->GetCharPtr();
					if (name == PDFAtom.All)
					{
						name_obj->Release();
						name_obj = PDFObject::makeName(PDFAtom.All);
					}
					else if (name == PDFAtom.None)
					{
						name_obj->Release();
						name_obj = PDFObject::makeName(PDFAtom.None);
					}
					else
					{
#ifndef NDEBUG
						printf("separation name: "); sep_name->PrintToStream(); printf("\n");
#endif
					}
				}
				else if (name == PDFAtom.ICCBased)
				{
					PDFObject *icc_stream = (*cs_array)[1]->Resolve();
					PDFObject *N = icc_stream->Find(PDFAtom.N);
					name_obj->Release();
					switch (N->GetInt32())
					{
						case 1:
						{
							// use device gray instead
							name_obj = PDFObject::makeName(PDFAtom.DeviceGray);
						} break;
						case 3:
						{
							// use device gray instead
							name_obj = PDFObject::makeName(PDFAtom.DeviceRGB);
						} break;
						case 4:
						{
							// use device gray instead
							name_obj = PDFObject::makeName(PDFAtom.DeviceCMYK);
						} break;
						default:
						{
							printf("Invalid ICCBased for PDF 1.3\n");
							name_obj = PDFObject::makeName(space);
						} break;
					}
					icc_stream->Release();
				}
				else if (name == PDFAtom.Lab)
				{
					PDFObject *labdict = (*cs_array)[1]->Resolve();
					color.SetLab(labdict);
					labdict->Release();
					name_obj->Release();
					name_obj = PDFObject::makeName(PDFAtom.Lab);
				}
			}
			cs_info->Release();
		}
		color.space = name_obj;
	}
}

void 
RenderEngine::setcolornary(ColorInfo &color)
//void 
//RenderEngine::setcolornary(PDFObject *which, rgb_color &color)
{
	// color space determines the number of stack items to consume
	const char *name = color.space->GetCharPtr();
	if (name == PDFAtom.DeviceGray)// (|| (name == PDFAtom.CalGray))
	{
		setgreycolor(color.color);
	}
	else if (name == PDFAtom.Indexed)
	{
		PDFObject *o = PopStack();
		o->Release();
	}
	else if ((name == PDFAtom.DeviceRGB) || (name == PDFAtom.CalRGB))
	{
		setrgbcolor(color.color);
	}
	else if (name == PDFAtom.DeviceCMYK)
	{
		setcmykcolor(color.color);
	}
	else if (name == PDFAtom.Pattern)
	{
//		printf("FIXME! pattern color space!\n");
		PDFObject *o = PopStack();
		// ARRGH!  Find the underlying color space
		// means I need more parms for this function, but I don't know what they should be yet.
		// Probably a dictionary representing the pattern.
		o->Release();
		eat_stack();
	}
	else if (name == PDFAtom.Separation)
	{
		PDFObject *o = PopStack();
		// tint
		printf("!!!! non-array'd /Separation color space\n");
		o->Release();
	}
	else if (name == PDFAtom.All)
	{
		PDFObject *o = PopStack();
		float oval = 1.0 - o->GetFloat();
		o->Release();
		o = PDFObject::makeNumber(oval);
		PushStack(o);
		setgreycolor(color.color);
	}
	else if (name == PDFAtom.None)
	{
		color.color.alpha = 0;
	}
	else if (name == PDFAtom.Lab)
	{
		PDFObject *o = PopStack();
		float b = o->GetFloat();
		o->Release();
		o = PopStack();
		float a = o->GetFloat();
		o->Release();
		o = PopStack();
		float l = o->GetFloat();
		o->Release();
		color.SetLabColor(l, a, b);
	}
	else {
//		printf("FIXME! named color space!\n");
		PDFObject *cs_info = FindResource(PDFAtom.ColorSpace, name);
//		printf("named space %s ", name); cs_info->PrintToStream(); printf("\n");
		if (cs_info->IsArray())
		{
			object_array *cs_array = cs_info->Array();
			PDFObject *space = (*cs_array)[0];
			name = space->GetCharPtr();
			if (name == PDFAtom.CalRGB)
			{
				setrgbcolor(color.color);
			}
			else if (name == PDFAtom.Separation)
			{
				PDFObject *sep_name = (*cs_array)[1];
				name = sep_name->GetCharPtr();
				if (name == PDFAtom.All)
				{
					PDFObject *o = PopStack();
					float oval = 1.0 - o->GetFloat();
					o->Release();
					o = PDFObject::makeNumber(oval);
					PushStack(o);
					setgreycolor(color.color);
				}
				else if (name == PDFAtom.None)
				{
					printf("separation None\n");
					// this means "transparent"  How to deal with this?
					color.color.alpha = 0;
				}
				else
				{
#ifndef NDEBUG
					printf("separation name: "); sep_name->PrintToStream(); printf("\n");
#endif
				}
			}
			else if (name == PDFAtom.ICCBased)
			{
				PDFObject *icc_stream = (*cs_array)[1]->Resolve();
#ifndef NDEBUG
				printf("setcolornary ICCBased: \n"); icc_stream->PrintToStream(4); printf("\n");
#endif
				icc_stream->Release();
				eat_stack();
			}
			else
			{
			// a "named" color space
			// FIXME: get the named space from the Resources dictionary for this page
			// and determine how many entries to conume, etc.
#ifndef NDEBUG
			printf("named color space %s ", name); cs_info->PrintToStream(); printf("\n");
			while (!StackEmpty())
			{
				PDFObject *o = PopStack();
				o->PrintToStream(2); printf("\n");
				o->Release();
			}
#endif
			eat_stack();
			}
		}
		cs_info->Release();
	}
}

void 
RenderEngine::closepath()
{
//	printf("RenderEngine::closepath\n");
	fGS->fShape->LineTo(fGS->fSubPathStart);
	fGS->fCurrentPoint = fGS->fSubPathStart;
	fGS->fShape->Close();
	fGS->fShapeTransformed = false;
}

void 
RenderEngine::fill()
{
//	printf("RenderEngine::fill\n");
	if (fBitmapView) {
//		printf("fShapeTransformed %s\n", fGS->fShapeTransformed ? "true" : "false");
		if (!fGS->fShapeTransformed) TransformShape();
#if 1
		if (fGS->fShapeVisibility == -1) {
			BRect bounds = fGS->fShape->Bounds();
			fGS->fShapeVisibility = (fClip.Intersects(bounds)) ? 1 : 0;
		}
		if (fGS->fShapeVisibility != 1) return;
#endif
		
		rgb_color fOldColor = fBitmapView->HighColor();
		fBitmapView->SetHighColor(fGS->fFillColor.color);
		fBitmapView->MovePenTo(B_ORIGIN);
		float ps = fBitmapView->PenSize();
		fBitmapView->SetPenSize(0);
		fBitmapView->FillShape(fGS->fShape);
		fBitmapView->StrokeShape(fGS->fShape);
		fBitmapView->SetPenSize(ps);
		fBitmapView->SetHighColor(fOldColor);
		fBitmapView->Sync();
	}
#if BITMAP_WATCHER > 0
	update_bitmap_watcher(fBitmap);
#endif
}

void 
RenderEngine::eofill()
{
//	printf("RenderEngine::eofill\n");
	if (fBitmapView) {
		if (!fGS->fShapeTransformed) TransformShape();
#if 1
		if (fGS->fShapeVisibility == -1) {
			BRect bounds = fGS->fShape->Bounds();
			fGS->fShapeVisibility = (fClip.Intersects(bounds)) ? 1 : 0;
		}
		if (fGS->fShapeVisibility != 1) return;
#endif
		rgb_color fOldColor = fBitmapView->HighColor();
		fBitmapView->SetHighColor(fGS->fFillColor.color);
		fBitmapView->MovePenTo(B_ORIGIN);
		fBitmapView->FillShape(fGS->fShape);
		fBitmapView->SetHighColor(fOldColor);
		fBitmapView->Sync();
	}
#if BITMAP_WATCHER > 0
	update_bitmap_watcher(fBitmap);
#endif
}

void 
RenderEngine::stroke()
{
//	printf("RenderEngine::stroke\n");
	if (fBitmapView) {
		if (!fGS->fShapeTransformed) TransformShape();
#if 1
		if (fGS->fShapeVisibility == -1) {
			BRect bounds = fGS->fShape->Bounds();
			fGS->fShapeVisibility = (fClip.Intersects(bounds)) ? 1 : 0;
		}
		if (fGS->fShapeVisibility != 1) return;
#endif
		rgb_color fOldColor = fBitmapView->HighColor();
		fBitmapView->SetHighColor(fGS->fStrokeColor.color);
		fBitmapView->MovePenTo(B_ORIGIN);
		fBitmapView->StrokeShape(fGS->fShape);
		fBitmapView->SetHighColor(fOldColor);
		fBitmapView->Sync();
	}
#if BITMAP_WATCHER > 0
	update_bitmap_watcher(fBitmap);
#endif
}

void 
RenderEngine::clear()
{
	//printf("RenderEngine::clear\n");
	fGS->fShape->Clear();
	fGS->fCurrentPoint = B_ORIGIN;
	fGS->fSubPathStart = B_ORIGIN;
	fGS->fShapeTransformed = false;
	fGS->fShapeVisibility = -1;
}

void 
RenderEngine::TransformShape(void)
{
	// make the working transform from DTM * CTM
#if 0
	Transform2D wtm(fGS->fCTM);
	wtm *= fDTM;
#else
	Transform2D wtm(fDTM);
	wtm *= fGS->fCTM;
#endif
#if 0
	printf("TransformShape()\nfCTM ");fGS->fCTM.PrintToStream(); printf("\n");
	printf(" wtm "); wtm.PrintToStream(); printf("\n");
#endif
	// apply the CTM to the current shape
	TransformIterator ti(wtm);
	ti.Iterate(fGS->fShape);
	fGS->fShapeTransformed = true;
}

void 
RenderEngine::setclipping()
{
	if (fClipPicture)
	{
		fBitmapView->ClipToPicture(fClipPicture);
		delete fClipPicture;
		fClipPicture = 0;
	}
}

void 
RenderEngine::setlinemode()
{
	if (fBitmapView)
		fBitmapView->SetLineMode((cap_mode)fGS->fLineCap, (join_mode)fGS->fLineJoin, fGS->fMiterLimit);
}


//#pragma mark -
// Text State Operators
void 
RenderEngine::TcFn()
{
	DFN(Tc);
	// character spacing
	PDFObject *charspace = PopStack();
	fGS->fTS.Tc = (charspace->GetFloat());
	charspace->Release();
}

void 
RenderEngine::TwFn()
{
	DFN(Tw);
	// word spacing
	PDFObject *wordspace = PopStack();
	fGS->fTS.Tw = (wordspace->GetFloat());
	wordspace->Release();
}

void 
RenderEngine::TzFn()
{
	DFN(Tz);
	PDFObject *scale = PopStack();
	fGS->fTS.Th = scale->GetFloat() / 100;
	scale->Release();
}

void 
RenderEngine::TLFn()
{
	DFN(TL);
	PDFObject *leading = PopStack();
	fGS->fTS.Tl = leading->GetFloat();
	leading->Release();
}

void 
RenderEngine::TfFn()
{
	DFN(Tf);
	PDFObject *size = PopStack();
	fGS->fTS.Tfs = size->GetFloat();
	if (fGS->fTS.Tfs > 1000)
		fGS->fTS.Tfs = 1;
	size->Release();
	
	PDFObject *fontname = PopStack();
	PDFObject *a_font = FindResource(PDFAtom.Font, fontname->GetCharPtr());
	fontname->Release();
	
	if (a_font) {
		status_t status = B_OK;
		fGS->fTS.Tf = FontEngine::SpecifyEngine(a_font, status);
		if (status < B_OK) {
			Document()->ThrowAlert(status);
		}
	}
	else
		fGS->fTS.Tf = NULL;
//	printf("Tfs = %.3f fontName: %s\n", fGS->fTS.Tfs, (fGS->fTS.Tf) ? fGS->fTS.Tf->FontName() : NULL);	
}

void 
RenderEngine::TrFn()
{
	DFN(Tr);
	PDFObject *mode = PopStack();
	fGS->fTS.Tmode = mode->GetInt32();
	mode->Release();
}

void 
RenderEngine::TsFn()
{
	DFN(Ts);
	PDFObject *rise = PopStack();
	fGS->fTS.Trise = rise->GetFloat();
	rise->Release();
}

// Text Object Operators
void 
RenderEngine::BTFn()
{
	DFN(BT);
	fTObj.Init();
}

void 
RenderEngine::ETFn()
{
	DFN(ET);
	fTObj.Reset();
}


// Text Positioning Operators
void 
RenderEngine::TdFn()
{
	DFN(Td);
	if (!fTObj.isValid)
		return;
	
	PDFObject *ty = PopStack();
	PDFObject *tx = PopStack();
	
	next_text_line(tx->GetFloat(), ty->GetFloat());
	tx->Release();
	ty->Release();
}

void 
RenderEngine::TDFn()
{
	DFN(TD);
	if (!fTObj.isValid)
		return;
	
	PDFObject *ty = PopStack();
	PDFObject *tx = PopStack();
	
	float y = ty->GetFloat();
	
	fGS->fTS.Tl = -y;
	next_text_line(tx->GetFloat(), y);
	tx->Release();
	ty->Release();
}

void 
RenderEngine::TmFn()
{
	DFN(Tm);
	if (!fTObj.isValid)
		return;
		
	PDFObject *f = PopStack();
	PDFObject *e = PopStack();
	PDFObject *d = PopStack();
	PDFObject *c = PopStack();
	PDFObject *b = PopStack();
	PDFObject *a = PopStack();
	
	fTObj.Tm = fTObj.Tlm.Set(a->GetFloat(), b->GetFloat(), c->GetFloat(),
		d->GetFloat(), e->GetFloat(), f->GetFloat());
	
//	printf("TmFn: (%.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", fTObj.Tm.A(), fTObj.Tm.B(), fTObj.Tm.C(), fTObj.Tm.D(), fTObj.Tm.E(), fTObj.Tm.F());
	
	a->Release();
	b->Release();
	c->Release();
	d->Release();
	e->Release();
	f->Release();
}

void 
RenderEngine::TstarFn()
{
	DFN(Tstar);
	if (!fTObj.isValid)
		return;
		
	next_text_line(0, -fGS->fTS.Tl);
}

// Text Showing Operators
void 
RenderEngine::TjFn()
{
	DFN(Tj);
	if (!fTObj.isValid)
		return;

	PDFObject *string = PopStack();
	RenderText(string);
	string->Release();
}

void 
RenderEngine::tickFn()
{
	DFN(tick);
	if (!fTObj.isValid)
		return;

	next_text_line(0, fGS->fTS.Tl);
	
	PDFObject *string = PopStack();
	RenderText(string);
	string->Release();
}

void 
RenderEngine::ticktickFn()
{
	DFN(ticktick);
	if (!fTObj.isValid)
		return;
	PDFObject *string = PopStack();
	
	PDFObject *wordspace = PopStack();
	fGS->fTS.Tw = wordspace->GetFloat();
	wordspace->Release();
	
	PDFObject *charspace = PopStack();
	fGS->fTS.Tc = charspace->GetFloat();
	charspace->Release();
	
	RenderText(string);
	string->Release();
}

void 
RenderEngine::TJFn()
{
	DFN(TJ);
	if (!fTObj.isValid)
		return;
		
	PDFObject *obj = PopStack();
	if (obj) {
		object_array *array = obj->Array();
		int32 size = array->size();
		
		for (int32 ix = 0; ix < size; ix++) {
			PDFObject *item = (*array)[ix];
			if (item->IsString()) RenderText(item);
			else if (item->IsNumber())
			{
				// update the text matrix
				float Tj = item->GetFloat();
				
				bool horizontal = true;
				
				float tx = 0.0;
				float ty = 0.0;
				if (horizontal) {
					tx = ((0 - Tj/1000) * fGS->fTS.Tfs) * fGS->fTS.Th;
				}
				else {
					ty = (0 - Tj/1000) * fGS->fTS.Tfs;
				}
		
				fTObj.Tm.Translate(tx, ty);
			}
		}
	}
	obj->Release();
}

void 
RenderEngine::next_text_line(float x, float y)
{
	fTObj.Tm = fTObj.Tlm.Translate(x, y);
}

// 32 bit source pixel
#define SrcAlpha(a)		((a) >> 24)
#define SrcRed(a)		(((a) >> 16) & 0xFF)
#define SrcGreen(a)		(((a) >> 8) & 0xFF)
#define SrcBlue(a)		((a) & 0xFF)

// 16 bit destination pixel
#define DstAlpha16(b)		255
#define DstRed16(b)			((((b) >> 8) & 0xF8) | (((b) >> 13) & 0x7))
#define DstGreen16(b)		((((b) >> 3) & 0xFC) | (((b) >> 9) & 0x3))
#define DstBlue16(b)		((((b) << 3) & 0xF8) | (((b) >> 2) & 0x7))

// 32 bit destination pixel
#define DstAlpha32(b)		((b) >> 24)
#define DstRed32(b)			(((b) >> 16) & 0xFF)
#define DstGreen32(b)		(((b) >> 8) & 0xFF)
#define DstBlue32(b)		((b) & 0xFF)


#define Blend32to16( srcPixel, dstPixel, srcAlpha) do {\
	uint32 tmpVar1 = ((																\
				(((SrcGreen(srcPixel) << 16) | SrcRed(srcPixel)) * srcAlpha) +		\
				(((DstGreen16(dstPixel) << 16) | DstRed16(dstPixel)) * (256-srcAlpha))	\
			) & 0xFC00F800);														\
	dstPixel = 	(tmpVar1 & 0xFFFF) | (tmpVar1 >> 21) |								\
				(((DstBlue16(dstPixel) * (256-srcAlpha)) +							\
				 (SrcBlue(srcPixel) * srcAlpha)) >> 11); } while (0)				\

inline void blend32to32(uint32 srcPixel, uint32 &dstPixel, uint8 srcAlpha) {
	uint32 tmp1 = ((dstPixel>>8)&0x00FF00FF) * (256-srcAlpha);
	uint32 tmp2 = ((srcPixel>>8)&0x00FF00FF) * srcAlpha;
	uint32 tmp3 = (tmp1 + tmp2) & 0xFF00FF00;
	
	uint32 tmp4 = (srcPixel&0x00FF00FF) * srcAlpha;
	uint32 tmp5 = (dstPixel&0x00FF00FF) * (256-srcAlpha);
	uint32 tmp6 = ((tmp4 + tmp5) >> 8) & 0x00FF00FF;
	dstPixel = tmp3 | tmp6;
}

void 
RenderEngine::RenderText(PDFObject *string, float offset)
{
	if (!fGS->fTS.Tf)
		return;

	const uint8 *text = string->Contents();
	size_t len = string->Length();
	
//	printf("RenderEngine::RenderText: offset: %0.2f %.*s\n", offset, len, text);

	if (fBitSpace != B_RGBA32 && fBitSpace != B_RGB32 && fBitSpace != B_RGB16) {
		printf("invalid fBitSpace: %d\n", fBitSpace);
		return;
	}
	
	// lock down the bitmap
	fBitmap->LockBits();
	uint8 *bits = (uint8 *)fBitmap->Bits();
	int32 bpr = fBitmap->BytesPerRow();
	
	Transform2D tTrm = fDTM;
	tTrm *= fGS->fCTM;
	tTrm *= fTObj.Tm;
	tTrm *= Transform2D(fGS->fTS.Tfs * fGS->fTS.Th, 0, 0, fGS->fTS.Tfs, 0, fGS->fTS.Trise);
	tTrm.RoundBy(4.0);
	Transform2D fontTrm(tTrm.A(), tTrm.B(), tTrm.C(), tTrm.D(), 0, 0);
	
	status_t errCode;
	FontEngine::Strike *strike = fGS->fTS.Tf->StrikeForFont(fontTrm, errCode);
//	printf("fontTrm: (%f,%f,%f,%f,%f,%f) strike: %p\n", fontTrm.A(), fontTrm.B(), fontTrm.C(), fontTrm.D(), fontTrm.E(), fontTrm.F(), strike);

	BRect fontBBox = fontTrm.TransformedBounds(fGS->fTS.Tf->BoundingBox());
	
//	printf("fontBBox: (%.2f, %.2f, %.2f, %.2f)\n", fontBBox.left, fontBBox.top, fontBBox.right, fontBBox.bottom);

	glyph_data glyph;
	for (uint ix = 0; ix < len; ix++) {
		
		// set up the transformation
 		Transform2D Trm = fDTM;
 		
 		Trm *= fGS->fCTM;
 		Trm *= fTObj.Tm;
 		Trm *= Transform2D(fGS->fTS.Tfs * fGS->fTS.Th, 0, 0, fGS->fTS.Tfs, 0, fGS->fTS.Trise);
//		printf("Trm: (%.2f, %.2f, %.2f, %.2f)\n", Trm.A(), Trm.B(), Trm.C(), Trm.D());


		// determine the position to draw at
		BPoint pt;
		Trm.Origin(&pt);
		
		glyph_info ginfo;
		fGS->fTS.Tf->GetGlyphInfo(text[ix], ginfo);

		// see if the glyph could in theory intersect our clipping region
		BRect glyphBBox(fontBBox);
		glyphBBox.OffsetBy(pt);
		
		if (strike && fClip.Intersects(glyphBBox)) {
	
			// render the glyph
			glyph.reset();
			
			strike->RenderGlyph(ginfo.ffcode, glyph);
			
			if (glyph.bits != NULL) {
	//			printf("render glyph: %c (0, 0, %.2f, %.2f) glyph.bits: %p glyph.rowbytes: %ld\n", ginfo.ff_code, glyph.width, glyph.height, glyph.bits, glyph.rowbytes);
				rgb_color color(fGS->fFillColor.color);
				
				BRect bBounds(fBitmapFrame);
				float mx = (uint32)pt.x + (uint32)glyph.lefttop.x;
				float my = (uint32)pt.y - (uint32)glyph.lefttop.y;
				BRect gBounds(mx, my, mx + glyph.width, my + glyph.height);
				
				BRect intersect = bBounds & gBounds;
				BPoint bOffset = intersect.LeftTop() - bBounds.LeftTop();
				BPoint gOffset = intersect.LeftTop() - gBounds.LeftTop();
	//			BPoint gOffset = gBounds.LeftTop() - intersect.LeftTop();
	#if GLYPH_WATCHER > 0
				bool useGlyphWatcher = false;
				resize_glyph_bits((int32)glyph.width, (int32)glyph.height);
	#endif
				if (bBounds.Intersects(gBounds) && !bBounds.Contains(gBounds)) {
	//				printf("%c bitmap: (%.2f, %.2f, %.2f, %.2f) glyph: (%.2f, %.2f, %.2f, %.2f) intersect: (%.2f,%.2f,%.2f,%.2f)\n",
	//					text[ix], bBounds.left, bBounds.top, bBounds.right, bBounds.bottom, gBounds.left, gBounds.top, gBounds.right, gBounds.bottom,
	//					intersect.left, intersect.top, intersect.right, intersect.bottom);
	
	#if GLYPH_WATCHER > 0
					useGlyphWatcher = true;
					BPoint bLT(bBounds.LeftTop());
					BPoint gLT(gBounds.LeftTop());
					BPoint iLT(intersect.LeftTop());
	//				printf("%c b.LT: (%.2f, %.2f) g.LT: (%.2f, %.2f) i.LT: (%.2f, %.2f) bOff: (%.2f, %.2f) gOff: (%.2f, %.2f)\n",
	//					text[ix], bLT.x, bLT.y, gLT.x, gLT.y, iLT.x, iLT.y, bOffset.x, bOffset.y, gOffset.x, gOffset.y
	//				);
	#endif
				}
				
				// determine the right spot in the bitmap to draw into
				uint16 * dstBits16 = (uint16 *) (bits + (2 * (uint32)bOffset.x) + (bpr * (uint32)bOffset.y));
				uint32 * dstBits32 = (uint32 *) (bits + (4 * (uint32)bOffset.x) + (bpr * (uint32)bOffset.y));
				uint8 * srcBits = (uint8 *)	(glyph.bits + (uint32)gOffset.x + (uint32)(glyph.rowbytes * (uint32)gOffset.y));
	
	#if GLYPH_WATCHER > 0
				uint32 *dstGlyphBits = (uint32 *) glyphBits->Bits();
				int32 glyphBpr = glyphBits->BytesPerRow();
				if (useGlyphWatcher)
					memset(dstGlyphBits, 0xff, glyphBits->BitsLength());
	#endif
		
				// draw the glyph
				intersect.OffsetTo(B_ORIGIN);
				for (int yi = (int)intersect.top; yi < (int)intersect.bottom; yi++) {
					for (int xi = (int)intersect.left; xi < (int)intersect.right; xi++) {
						uint8 alpha = srcBits[xi];
						uint8 pixel[4];
						pixel[0] = color.blue;
						pixel[1] = color.green;
						pixel[2] = color.red;
						pixel[3] = alpha;
						if (fBitSpace == B_RGB16) {
							Blend32to16(*(uint32*)pixel, dstBits16[xi], alpha);
						}
						else {
							blend32to32(*(uint32*)pixel, dstBits32[xi], alpha);
	#if GLYPH_WATCHER > 0
							if (useGlyphWatcher) {
								blend32to32(*(uint32*)pixel, dstGlyphBits[xi], alpha);
							}
	#endif
						}
					}
					
					// advance to the next rows
					dstBits16 += bpr >> 1;
					dstBits32 += bpr >> 2;
					srcBits += glyph.rowbytes;
	#if GLYPH_WATCHER > 0
					if (useGlyphWatcher) {
						dstGlyphBits+= glyphBpr >> 2;
					}
	#endif
	#if BITMAP_WATCHER > 0
					update_bitmap_watcher(fBitmap);	
	#endif	
				}
	
	#if GLYPH_WATCHER > 0
				if (useGlyphWatcher)
					update_glyph_watcher();
	#endif
				strike->ReleaseGlyph();
			}
		}
		// update the text matrix
		float Tj = (ix == (len - 1)) ? offset : 0;
		float spacing = fGS->fTS.Tc;
		if (ginfo.isSpace)
			spacing += fGS->fTS.Tw;
		
//		printf("spacing: %.3f Tc: %.3f Tw: %.3f\n", spacing, fGS->fTS.Tc, fGS->fTS.Tw);
		
		bool horizontal = true;
		
		float tx = 0.0;
		float ty = 0.0;
		// translate back into the original space

//		printf("%c ginfo.width: %.3f\n", ginfo.ffcode, ginfo.width);
		
		if (horizontal) {
			tx = ((ginfo.width - Tj/1000) * fGS->fTS.Tfs + spacing) * fGS->fTS.Th;
//			printf("%d %c tx:%f = ((w0:%f - Tj:%f/1000) * Tfs:%f + spacing: %f) * Th: %f\n", text[ix], ginfo.ffcode, tx, ginfo.width, Tj, fGS->fTS.Tfs, spacing, fGS->fTS.Th);
		}
		else {
			ty = (ginfo.width - Tj/1000) * fGS->fTS.Tfs + spacing;
//			printf("%c (%.2f, %.2f) %.2f > %.2f\n", text[ix], pt.x, pt.y, ty, advYIn1000thOfTextSpace);
		}

//		printf("%c Trm: (%.2f,%.2f,%.2f,%.2f,%.2f,%.2f) v.x: %.2f tx: %.2f\n Old Tm: (%.2f,%.2f,%.2f,%.2f,%.2f,%.2f)",
//		text[ix], Trm.A(), Trm.B(), Trm.C(), Trm.D(), Trm.E(), Trm.F(), v.x, tx, fTObj.Tm.A(), fTObj.Tm.B(), fTObj.Tm.C(), fTObj.Tm.D(), fTObj.Tm.E(), fTObj.Tm.F());

//		printf(" Old Tm: (%.2f,%.2f,%.2f,%.2f,%.2f,%.2f)\n", fTObj.Tm.A(), fTObj.Tm.B(), fTObj.Tm.C(), fTObj.Tm.D(), fTObj.Tm.E(), fTObj.Tm.F());
		fTObj.Tm.Translate(tx, ty);
//		printf(" New Tm: (%.2f,%.2f,%.2f,%.2f,%.2f,%.2f)\n", fTObj.Tm.A(), fTObj.Tm.B(), fTObj.Tm.C(), fTObj.Tm.D(), fTObj.Tm.E(), fTObj.Tm.F());
	}
	
#if BITMAP_WATCHER > 0
	update_bitmap_watcher(fBitmap);	
#endif	
	
	fBitmap->UnlockBits();
}
//#pragma mark -

void 
RenderEngine::RenderImage(PDFObject *o)
{
	// convert FROM BView coords
	Transform2D t(fDTM);
	// scale to desired size
	// the current CTM defines the placment and size of the image
	t *= fGS->fCTM;
	// the image is (nominally) a 1x1 matrix
	// make a region from the bounding rect
	BRegion ir;
	ir.Set(t.TransformedBounds(BRect(0.0, 0.0, 1.0, 1.0)));
	// only the intersection between this region and the clipping region is usefull
	BRegion clipping;
	fBitmapView->GetClippingRegion(&clipping);
	ir.IntersectWith(&clipping);
	int32 rects = ir.CountRects();
	if (rects == 0) return;

	PDFObject *oo = o->AsDictionary();
	Pusher *p = MakeImageChain(oo, fBitmap, ir, t, (uint8*)&(fGS->fFillColor.color));
	if (p)
	{
		if (oo->Find(PDFAtom.__document__))
			PushStream(oo, p);
		else
		{
			const uint8 *buffer = reinterpret_cast<const uint8 *>(fString.String());
			ssize_t length = fString.Length();
			//printf("Inline data, %ld bytes\n", length);
			PushBuffer(oo, buffer, length, p);
		}
	}
	oo->Release();
#if BITMAP_WATCHER > 0
	update_bitmap_watcher(fBitmap);
#endif
}

void 
RenderEngine::UnknownKeyword(PDFObject *keyword)
{
	DescriptionEngine::UnknownKeyword(keyword);
}

void 
RenderEngine::bFn()
{
	DFN(b);
	// closepath fill and stroke
	closepath();
	setlinemode();
	fill();
	stroke();
	clear();
	setclipping();
}

void 
RenderEngine::bstarFn()
{
	DFN(bstar);
	// closepath eofill and stroke
	closepath();
	setlinemode();
	eofill();
	stroke();
	clear();
	setclipping();
}

void 
RenderEngine::BFn()
{
	DFN(B);
	// fill and stroke
	setlinemode();
	fill();
	stroke();
	clear();
	setclipping();
}

void 
RenderEngine::BstarFn()
{
	DFN(Bstar);
	// eofill and stroke
	setlinemode();
	eofill();
	stroke();
	clear();
	setclipping();
}

#if 0
void 
RenderEngine::BDCFn()
{
}

void 
RenderEngine::BIFn()
{
}

void 
RenderEngine::BMCFn()
{
}
#endif

#if 0
void 
RenderEngine::BXFn()
{
}
#endif

void 
RenderEngine::cFn()
{
	// curveto
	BPoint pts[3];
	for (int ix = 2; ix >= 0; ix--) {
		PDFObject *y = PopStack();
		PDFObject *x = PopStack();
		pts[ix].Set(x->GetFloat(), y->GetFloat());
		y->Release();
		x->Release();
	}
	fGS->fShape->BezierTo(pts);
	fGS->fShapeTransformed = false;
	fGS->fCurrentPoint = pts[2];
}

void 
RenderEngine::cmFn()
// ( a b c d e f - )
{
	DFN(cm);
	PDFObject *a, *b;
	PDFObject *c, *d;
	PDFObject *e, *f;
	//debugger("cmFn()");
	//printf("cmFn()\nold CTM "); fGS->fCTM.PrintToStream(); printf("\n");
	f = PopStack();
	e = PopStack();
	d = PopStack();
	c = PopStack();
	b = PopStack();
	a = PopStack();
	fGS->fCTM *= Transform2D(
		a->GetFloat(), b->GetFloat(),
		c->GetFloat(), d->GetFloat(),
		e->GetFloat(), f->GetFloat()
		);
	//printf("new CTM "); fGS->fCTM.PrintToStream(); printf("\n");
	a->Release(); b->Release();
	c->Release(); d->Release();
	e->Release(); f->Release();
}

void 
RenderEngine::csFn()
// ( cs - )
{
	DFN(cs);
	PDFObject *cs = PopStack();
	if (cs->IsName())
	{
#ifndef NDEBUG
		printf("setcolorspace(fill) : "); cs->PrintToStream(0); printf("\n");
#endif
		// setcolorspace (fill)
		// set fill color space
		setcolorspace(fGS->fFillColor, cs->GetCharPtr());
	}
	cs->Release();
}

void 
RenderEngine::CSFn()
// ( cs - )
{
	DFN(CS);
	PDFObject *cs = PopStack();
	if (cs->IsName())
	{
		// setcolorspace (stroke)
		setcolorspace(fGS->fStrokeColor, cs->GetCharPtr());
	}
	cs->Release();
}

#if 0
void 
RenderEngine::dFn()
{
}

void 
RenderEngine::d0Fn()
{
}

void 
RenderEngine::d1Fn()
{
}
#endif

void 
RenderEngine::DoFn()
// ( name  - )
{
	DFN(Do);
	// deal with xobject
	PDFObject *name = PopStack();
	// validate it's a name?
	PDFObject *xo = FindResource(PDFAtom.XObject, name->GetCharPtr());
	if (xo)
	{
#ifndef NDEBUG
		printf("XObject "); xo->PrintToStream(3); printf("\n");
#endif
#if 1
		PDFObject *subtype = xo->Find(PDFAtom.Subtype);
		if (subtype->GetCharPtr() == PDFAtom.Image)
		{
			RenderImage(xo);
		}
		else
		{
#if 0
			printf("Do "); name->PrintToStream(); printf("\n");
			printf("XObject "); xo->PrintToStream(3); printf("\n");
#endif
		}
#endif
	}
	xo->Release();
	name->Release();
}

#if 0
void 
RenderEngine::DPFn()
{
}
#endif

#ifndef NDEBUG
static void TwoHex(uint8 b, uint8 *buf)
{
	buf++;
	uint8 h = b & 0x0f;
	if (h > 9) h += 'A' - 10;
	else h += '0';
	*buf-- = h;
	b >>= 4;
	h = b & 0x0f;
	if (h > 9) h += 'A' - 10;
	else h += '0';
	*buf-- = h;
};
static void DumpChunk(uint32 offset, uint8 *inbuf, int32 size)
{
	uint8 buf[16 * 4 + 2];
	while (size)
	{
		memset(buf, ' ', sizeof(buf)-1);
		buf[sizeof(buf)-1] = '\0';
		for (int byte = 0; byte < 16 && size; byte++)
		{
			TwoHex(*inbuf, buf + (byte * 3));
			uint8 code = *inbuf++;
			buf[3 * 16 + 1 + byte] = ((code >= ' ') && (code <= 127)) ? code : '.';
			size--;
		}
		printf("%08lx  %s\n", offset, buf);
		offset += 16;
	}
	
}
#endif

void 
RenderEngine::EIFn()
// End Inlined-Image.  The image dictionary is on the stack
{
	DFN(EI);
	PDFObject *obj = PopStack();
#ifndef NDEBUG
	printf("InlineImage ");obj->PrintToStream();printf("\n");
	printf("CTM ");fGS->fCTM.PrintToStream();printf("\n");
	//printf("inline image data:\n"); DumpChunk(0, fString.String(), fString.Length());
#endif
	RenderImage(obj);
	fString.Clear();
	obj->Release();
}

#if 0
void 
RenderEngine::EMCFn()
{
}
#endif


#if 0
void 
RenderEngine::EXFn()
{
}
#endif

void 
RenderEngine::fFn()
{
	DFN(f);
	// fill, non-zero winding rule
	fGS->fShape->Close();
	setlinemode();
	fill();
	clear();
	setclipping();
}

void 
RenderEngine::fstarFn()
// ( - )
{
	DFN(fstar);
	// eofill - fill, even-odd rule
	setlinemode();
	eofill();
	clear();
	setclipping();
}

#if 0
void 
RenderEngine::FFn()
// ( - )
{
	DFN(F);
	// depricated, do as fFn
	fFn();
}
#endif

void 
RenderEngine::gFn()
// ( gray - )
{
	DFN(g);
	// setgray (fill)
	setgreycolor(fGS->fFillColor.color);
}

void 
RenderEngine::gsFn()
{
	PDFObject *o = PopStack();
#ifndef NDEBUG
	PDFObject *gstate = FindResource(PDFAtom.ExtGState, o->GetCharPtr());
	printf("ExtGState /%s:\n", o->GetCharPtr()); gstate->PrintToStream(2); printf("\n");
	gstate->Release();
#endif
	o->Release();
}

void 
RenderEngine::GFn()
// ( gray - )
{
	DFN(G);
	// setgray (stroke)
	setgreycolor(fGS->fStrokeColor.color);
}

void 
RenderEngine::hFn()
// ( - )
{
	DFN(h);
	// closepath
	closepath();
}

#if 0
void 
RenderEngine::iFn()
{
}

void 
RenderEngine::IDFn()
{
}
#endif

void 
RenderEngine::jFn()
{
	// set line join
	PDFObject * join = PopStack();
	switch (join->GetInt32()) {
		case 0:
			fGS->fLineJoin = B_MITER_JOIN;
			break;
		case 1:
			fGS->fLineJoin = B_ROUND_JOIN;
			break;
		case 2:
			fGS->fLineJoin = B_BEVEL_JOIN;
			break;
	}
	join->Release();
}

void 
RenderEngine::JFn()
{
	// set line cap
	PDFObject * cap = PopStack();
	switch (cap->GetInt32()) {
		case 0:
			fGS->fLineCap = B_BUTT_CAP;
			break;
		case 1:
			fGS->fLineCap = B_ROUND_CAP;
			break;
		case 2:
			fGS->fLineCap = B_SQUARE_CAP;
			break;
	}
	cap->Release();
}

void 
RenderEngine::kFn()
{
	setcmykcolor(fGS->fFillColor.color);
	setcolorspace(fGS->fFillColor, PDFAtom.DeviceCMYK);
}

void 
RenderEngine::KFn()
{
	setcmykcolor(fGS->fStrokeColor.color);
	setcolorspace(fGS->fStrokeColor, PDFAtom.DeviceCMYK);
}

void 
RenderEngine::lFn()
// ( x y - )
{
	DFN(l);
	// lineto
	PDFObject *y = PopStack();
	PDFObject *x = PopStack();
	
	BPoint pt(x->GetFloat(), y->GetFloat());
	//printf("LineTo: %.2f, %.2f\n", pt.x, pt.y);
	fGS->fShape->LineTo(pt);
	fGS->fShapeTransformed = false;
	fGS->fCurrentPoint = pt;
	y->Release();
	x->Release();
}

void 
RenderEngine::mFn()
// ( x y - )
{
	DFN(m);
	// moveto
	PDFObject *y = PopStack();
	PDFObject *x = PopStack();
	//printf("MoveTo x,y: "); x->PrintToStream(); y->PrintToStream(); printf("\n");
	fGS->fCurrentPoint.Set(x->GetFloat(), y->GetFloat());
	fGS->fSubPathStart = fGS->fCurrentPoint;
	fGS->fShape->MoveTo(fGS->fCurrentPoint);
	fGS->fShapeTransformed = false;
	//printf("MoveTo: %.2f, %.2f\n", fGS->fCurrentPoint.x, fGS->fCurrentPoint.y);
	x->Release();
	y->Release();
}

void 
RenderEngine::MFn()
{
	DFN(M);
	// get miter limit
	PDFObject * miterLimit = PopStack();
	fGS->fMiterLimit = miterLimit->GetFloat();
	miterLimit->Release();
}

#if 0
void 
RenderEngine::MPFn()
{
}
#endif

void 
RenderEngine::nFn()
// ( - )
{
	DFN(n);
	// newpath
	fGS->fShape->Clear();
	fGS->fShapeTransformed = false;
	setclipping();
}

#if 0
void 
RenderEngine::PSFn()
{
}
#endif

void 
RenderEngine::qFn()
// ( - )
{
	DFN(q);
//	printf("q: save state\n");
	// save graphics state to graphics state stack
	fGS = fGS->Push();
	fBitmapView->PushState();
}

void 
RenderEngine::QFn()
// ( - )
{
	DFN(Q);
//	printf("Q: restore state\n");
	// restore graphics state from gstate stack
	fGS = fGS->Pop();
	// deal with unballanced q/Q operators
	if (!fGS) fGS = GState::Push(Transform2D());
	fBitmapView->PopState();
	// reset the state
	// FIXME - more to do here
	fBitmapView->SetPenSize(fGS->fLineWidth);
}

void 
RenderEngine::reFn()
// ( x y width height - )
{
	DFN(re);
	// rectangle
	PDFObject *height = PopStack();
	PDFObject *width = PopStack();
	PDFObject *y = PopStack();
	PDFObject *x = PopStack();

	float rx = x->GetFloat();
	float ry = y->GetFloat();
	float rwidth = width->GetFloat();
	float rheight = height->GetFloat();

//	printf("rx: %.2f ry: %.2f rwidth: %.2f rheight: %.2f \n", rx, ry, rwidth, rheight);

	fGS->fCurrentPoint.Set(rx, ry);
	fGS->fSubPathStart.Set(rx, ry);
	fGS->fShape->MoveTo(fGS->fCurrentPoint);
	//fBitmapView->MovePenTo(fGS->fCurrentPoint);
	//printf("start at: (%.2f, %.2f)\n", fGS->fCurrentPoint.x, fGS->fCurrentPoint.y);
	BPoint pt;
	pt.Set(rx + rwidth, ry);
	//printf("line to (%.2f, %.2f)\n", pt.x, pt.y);
	fGS->fShape->LineTo(pt);
	//fBitmapView->StrokeLine(pt);
	pt.Set(rx + rwidth, ry + rheight);
	//printf("line to (%.2f, %.2f)\n", pt.x, pt.y);
	fGS->fShape->LineTo(pt);
	//fBitmapView->StrokeLine(pt);
	pt.Set(rx, ry + rheight);
	//printf("line to (%.2f, %.2f)\n", pt.x, pt.y);
	fGS->fShape->LineTo(pt);
	//fBitmapView->StrokeLine(pt);
	// close the rect
	fGS->fShape->LineTo(fGS->fCurrentPoint);
	//fBitmapView->StrokeLine(fGS->fCurrentPoint);
	fGS->fShapeTransformed = false;

	x->Release();
	y->Release();
	width->Release();
	height->Release();
}

void 
RenderEngine::rgFn()
// ( r g b - )
{
	DFN(rg);
	// setrgbcolor (fill)
	setrgbcolor(fGS->fFillColor.color);
	// set fill color space
	setcolorspace(fGS->fFillColor, PDFAtom.DeviceRGB);
}

void 
RenderEngine::riFn()
// ( intent - )
{
	DFN(ri);
	// rendering intent
	PDFObject *i = PopStack();
	if (i->IsName())
	{
		fGS->fRenderingIntent->Release();
		fGS->fRenderingIntent = i->AsName(); // ups ref count
#ifndef NDEBUG
		printf("New rendering intent: ");
		i->PrintToStream(0);
		printf("\n");
#endif
	}
	// down ref count
	i->Release();
}

void 
RenderEngine::RGFn()
// ( r g b - )
{
	DFN(RG);
	// setrgbcolor (stroke)
	setrgbcolor(fGS->fStrokeColor.color);
	// set stroke color space
	setcolorspace(fGS->fStrokeColor, PDFAtom.DeviceRGB);
}

void 
RenderEngine::sFn()
// ( - )
{
	DFN(s);
	// closepath and stroke
	closepath();
	setlinemode();
	stroke();
	clear();
	setclipping();
}

void 
RenderEngine::scFn()
// ( ? - )
{
	DFN(sc);
	// setcolor (fill)
	setcolornary(fGS->fFillColor);
}

void 
RenderEngine::scnFn()
{
	DFN(scn);
	// setcolor (fill) with pattern support
	setcolornary(fGS->fFillColor);
}

#if 0
void 
RenderEngine::shFn()
// ( name - )
{
	DFN(sh);
	// shading fill
}
#endif

void 
RenderEngine::SFn()
// ( - )
{
	DFN(S);
	// stroke
	setlinemode();
	stroke();
	clear();
	setclipping();
}

void 
RenderEngine::SCFn()
{
	DFN(SC);
	// setcolor (stroke)
	//setcolornary(fGS->fStrokeColorSpace, fGS->fStrokeColor);
	setcolornary(fGS->fStrokeColor);
}

void 
RenderEngine::SCNFn()
{
	DFN(SCN);
	// setcolor
	//setcolornary(fGS->fStrokeColorSpace, fGS->fStrokeColor);
	setcolornary(fGS->fStrokeColor);
}


void 
RenderEngine::vFn()
// ( x2 y2 x3 y3 - )
{
	DFN(v);
	// alternate curveto where x1 y1 is the current point
	BPoint pts[3];

	for (int ix = 2; ix >= 1; ix--) {
		PDFObject *y = PopStack();
		PDFObject *x = PopStack();
		pts[ix].Set(x->GetFloat(), y->GetFloat());
		x->Release();
		y->Release();
	}  

	pts[0] = fGS->fCurrentPoint;

	fGS->fShape->BezierTo(pts);
	fGS->fShapeTransformed = false;
	fGS->fCurrentPoint = pts[2];
}

void 
RenderEngine::wFn()
// ( linewidth - )
{
	DFN(w);
	PDFObject *linewidth = PopStack();
	if (fBitmapView) {
		//printf("SetPenSize: %.2f\n", linewidth->GetFloat());
		fGS->fLineWidth = linewidth->GetFloat();
		fBitmapView->SetPenSize(fGS->fLineWidth);
	}
	linewidth->Release();
}

void 
RenderEngine::WFn()
{
	DFN(W);
	// clip
	// start a picture
	fBitmapView->BeginPicture(new BPicture());
	// fill the current shape into the view
#if 0
	rgb_color save = fGS->fFillColor;
	fGS->fFillColor.red = random();
	fGS->fFillColor.green = random();
	fGS->fFillColor.blue = random();
#endif
	setlinemode();
	fill();
#if 0
	fGS->fFillColor = save;
#endif
	// get the picture
	fClipPicture = fBitmapView->EndPicture();
}

void 
RenderEngine::WstarFn()
// ( - )
{
	DFN(Wstar);
	WFn();
}

void 
RenderEngine::yFn()
// ( x1 y1 x3 y3 - )
{
	DFN(y);
	// alternate curveto where x1 y1 is the current point
	BPoint pts[3];

	for (int ix = 2; ix >= 0; ix--) {
		PDFObject *y = PopStack();
		PDFObject *x = PopStack();
		pts[ix].Set(x->GetFloat(), y->GetFloat());
		x->Release();
		y->Release();
		if (ix == 2) {
			pts[1] = pts[2];
			ix--;
		}
	}  

	fGS->fShape->BezierTo(pts);
	fGS->fShapeTransformed = false;
	fGS->fCurrentPoint = pts[2];
}



