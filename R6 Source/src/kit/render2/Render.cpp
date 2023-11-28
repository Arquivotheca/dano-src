//******************************************************************************
//
//	File:			Render.cpp
//
//	Description:	RRender, RVisual & LVisual implementation
//	
//	Written by:		George Hoffman, Mathias Agopian
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>

#include <support2_p/BinderKeys.h>
#include <render2_p/RenderProtocol.h>
#include <render2_p/RenderOutputPipe.h>

#include <render2/Render.h>
#include <render2/Region.h>
#include <render2/Color.h>
#include <render2/Font.h>
#include <render2/Update.h>
#include <support2/StdIO.h>

namespace B {
namespace Render2 {

const escapements B_NO_ESCAPEMENT(0, 0);

using namespace B::Private;

/******************************************************************************/
// #pragma mark RRender

class RRender : public RInterface<IRender>
{
	public:	
										RRender(const IBinder::ptr& remote);
		virtual							~RRender();
		virtual	IRender::ptr			Branch(uint32 flags = B_SYNCHRONOUS);

		virtual	void					MoveTo(const BPoint& pt);
		virtual	void					LinesTo(const BPoint *points, int32 lineCount);
		virtual	void					BeziersTo(const BPoint *points, int32 bezierCount);
		virtual	void					ArcsTo(const BPoint *points, coord radius, int32 arcCount);
		virtual	void					Arc(const BPoint& center, coord radius, float startAngle=0, float arcLen=M_2_PI, bool connected=false);
		virtual	void					Text(const char *text, int32 len=B_CSTR_LEN, const escapements& escapements=B_NO_ESCAPEMENT);
		virtual	void					Close();
		virtual	void					BeginStroke();
		virtual	void					EndStroke();
		virtual	void					TextOnPath(	const char *text,
													int32 len = B_CSTR_LEN,
													const escapements& escapements = B_NO_ESCAPEMENT,
													uint32 flags = B_JUSTIFY_LEFT|B_VALIGN_BASELINE);
		virtual	void					SetFont(const BFont &font);
		virtual	void					SetWindingRule(winding_rule rule);
		virtual	void					SetStrokeStyle(
											coord penWidth,
											cap_mode capping=B_BUTT_CAP,
											join_mode joining=B_BUTT_JOIN,
											float miter=10.0f,
											coord *stippling=NULL);
		virtual	void					Fill();
		virtual	void					Color(const BColor&);
		virtual	void					Shade(const BGradient&);
		virtual	void					Transform(const B2dTransform&);
		virtual	void					Transform(const BColorTransform&);
		virtual	void					BeginComposition();
		virtual	void					EndComposition();
		virtual	void					PushState();
		virtual	void					PopState();
		virtual	void					BeginPixelBlock(const BPixelDescription& pixels);
		virtual	void					PlacePixels(const BRasterPoint& at, const BPixelData& pixels);
		virtual	void					EndPixelBlock();
		virtual	void					DisplayCached(	const atom_ptr<IVisual>& view, uint32 flags,
														int32 cache_id);

	protected:
		virtual	atom_ptr<IBinder>		AsBinderImpl()			{ return NULL; }
		virtual	atom_ptr<const IBinder>	AsBinderImpl() const	{ return NULL; }
				
				BRenderOutputPipe		*fSession;
};

/******************************************************************************/

B_IMPLEMENT_META_INTERFACE(Render)
; // for Eddie

/******************************************************************************/

RRender::RRender(IBinder::arg remote)
	 :	RInterface<IRender>(remote),
	 	fSession(new BRenderOutputPipe(remote, 1024, 512))
{
}

RRender::~RRender()
{
	fSession->writeOp(GRP_FINISHED);
	fSession->Close();
	delete fSession;
}

IRender::ptr RRender::Branch(uint32 flags)
{
	// Create the renderstream in the server
	BValue value("flags", flags);
	BValue val = Remote()->Invoke(value, g_keyBranch);
	int32 id = val["id"].AsInteger();
	IRender::ptr render = IRender::AsInterface(val["stream"]);
	// now insert it in the render stream
	if (id) { // synchronous branch
		fSession->writeOp(GRP_BRANCH);
		fSession->write(id);
	}
	return render;
}

// --------------------------------------------------------------
// #pragma mark -
// #pragma mark Primitive path creation and manipulation

void RRender::MoveTo(const BPoint& pt)
{
	fSession->writeOp(GRP_MOVE_TO);
	fSession->write(pt);
}

void RRender::LinesTo(const BPoint* points, int32 lineCount)
{
	while (lineCount--) {
		fSession->writeOp(GRP_LINE_TO);
		fSession->write(*points++);
	}
}

void RRender::BeziersTo(const BPoint* points, int32 bezierCount)
{
	while (bezierCount--) {
		fSession->writeOp(GRP_BEZIER_TO);
		fSession->assert_size(sizeof(*points)*3);
		fSession->write(points[0]);
		fSession->write(points[1]);
		fSession->write(points[2]);
		points += 3;
	}
}

void RRender::ArcsTo(const BPoint *points, coord radius, int32 arcCount)
{
	while (arcCount--) {
		fSession->writeOp(GRP_ARC_TO);
		fSession->assert_size(sizeof(*points)*2 + sizeof(radius));
		fSession->write(*points++);
		fSession->write(*points);
		fSession->write(radius);
	}
}

void RRender::Arc(const BPoint& center, coord radius, float startAngle, float arcLen, bool connected=false)
{
	if (connected)	fSession->writeOp(GRP_ARC_CONECTED);
	else			fSession->writeOp(GRP_ARC);
	fSession->assert_size(sizeof(center)+sizeof(radius)+sizeof(startAngle)+sizeof(arcLen));
	fSession->write(center);
	fSession->write(radius);
	fSession->write(startAngle);
	fSession->write(arcLen);
}

void RRender::Text(const char *text, int32 len, const escapements& e)
{
	len = (len == B_CSTR_LEN) ? (strlen(text)) : (len);
	if (e == B_NO_ESCAPEMENT) {
		fSession->writeOp(GRP_TEXT);
	} else {
		fSession->writeOp(GRP_TEXT_ESCAPEMENT);
		fSession->assert_size(sizeof(e)+sizeof(len));
		fSession->write(e);
	}
	fSession->write(len);
	fSession->write_align(reinterpret_cast<const uint8 *>(text), len);
}

void RRender::Close()
{
	fSession->writeOp(GRP_CLOSE);
}

// --------------------------------------------------------------
// #pragma mark -
// #pragma mark Composite path operations
void 
RRender::BeginStroke()
{
}

void 
RRender::EndStroke()
{
	fSession->writeOp(GRP_STROKE);
}

void RRender::TextOnPath(const char *text, int32 len, const escapements& escapements, uint32 flags)
{
	len = (len == B_CSTR_LEN) ? (strlen(text)) : (len);
	fSession->writeOp(GRP_TEXT_ON_PATH);
	fSession->assert_size(sizeof(escapements)+sizeof(flags)+sizeof(len));
	fSession->write(escapements);
	fSession->write((int32)flags);
	fSession->write(len);
	fSession->write_align(reinterpret_cast<const uint8 *>(text), len);
}

// --------------------------------------------------------------
// #pragma mark -
// #pragma mark Path generation state

void RRender::SetFont(const BFont &v)
{
	fSession->writeOp(GRP_SET_FONT);
	fSession->write(v);
}

void RRender::SetWindingRule(winding_rule rule)
{
	fSession->writeOp(GRP_SET_WINDING_RULE);
	fSession->write((int32)rule);
}

void RRender::SetStrokeStyle(coord, cap_mode, join_mode, float, coord *)
{
	fSession->writeOp(GRP_SET_STROKE);
}

// --------------------------------------------------------------
// #pragma mark -
// #pragma mark Path filling

void RRender::Fill()
{
	fSession->writeOp(GRP_FILL);
}

void RRender::Color(const BColor& c)
{
	fSession->writeOp(GRP_COLOR);
	fSession->write(c);
}

void RRender::Shade(const BGradient& gradient)
{
	fSession->writeOp(GRP_SHADE);
	fSession->write(gradient);
}


// --------------------------------------------------------------
// #pragma mark -
// #pragma mark Composition stacks

void RRender::Transform(const B2dTransform& transform)
{
	fSession->writeOp(GRP_TRANSFORM_2D);
	fSession->write(transform);
}

void RRender::Transform(const BColorTransform& transform)
{
	fSession->writeOp(GRP_TRANSFORM_COLOR);
	fSession->write(transform);
}

void RRender::BeginComposition()
{
	fSession->writeOp(GRP_BEGIN_COMPOSE);
}

void RRender::EndComposition()
{
	fSession->writeOp(GRP_END_COMPOSE);
}

// --------------------------------------------------------------
// #pragma mark -
// #pragma mark Save/restore state

void RRender::PushState() {
	fSession->writeOp(GRP_PUSH_STATE);
}
void RRender::PopState() {
	fSession->writeOp(GRP_POP_STATE);
}


// --------------------------------------------------------------
// #pragma mark -
// #pragma mark Pixmap/sample support

void RRender::BeginPixelBlock(const BPixelDescription& pixels)
{
	fSession->writeOp(GRP_BEGIN_PIXELS);	
	fSession->assert_size(3*sizeof(int32) + sizeof(pixel_format));
	fSession->write((int32)pixels.Width());
	fSession->write((int32)pixels.Height());
	fSession->write((int32)pixels.Flags());
	fSession->write(pixels.ColorSpace());
}

void RRender::PlacePixels(const BRasterPoint& at, const BPixelData& pixels)
{
	if (pixels.IsValid() == false)
		debugger("AddPixelBlock: BPixelData is not valid");
	fSession->writeOp(GRP_PLACE_PIXELS);	
	fSession->assert_size(sizeof(at) + 3*sizeof(int32) + sizeof(pixel_format) + sizeof(int32));
	fSession->write(at);
	fSession->write((int32)pixels.Width());
	fSession->write((int32)pixels.Height());
	fSession->write((int32)pixels.BytesPerRow());
	fSession->write(pixels.ColorSpace());
	fSession->write((int32)pixels.Size());
	fSession->write_align(reinterpret_cast<const uint8 *>(pixels.Data()), pixels.Size());
}

void RRender::EndPixelBlock()
{
	fSession->writeOp(GRP_END_PIXELS);	
}

// --------------------------------------------------------------
// #pragma mark -
// #pragma mark Drawing a visual

void RRender::DisplayCached(const atom_ptr<IVisual>& visual, uint32 flags, int32 cache_id)
{
	fSession->writeOp(GRP_DISPLAY_CACHED);
	fSession->write(visual->AsBinder());
	fSession->write((int32)flags);
	fSession->write(cache_id);
}

/******************************************************************************/
// #pragma mark -

void IRender::TextAt(	const BPoint& at,
						const char *text,
						int32 len,
						const escapements& e)
{
	MoveTo(at);
	Text(text, len, e);
}


/******************************************************************************/
// #pragma mark -
// #pragma mark RVisual

class RVisual : public RInterface<IVisual>
{
	public:

										RVisual(const IBinder::ptr& remote) : RInterface<IVisual>(remote) {};

		virtual	void					Display(const IRender::ptr& into);
		virtual	void					Draw(const IRender::ptr& into);
		virtual void					Invalidate(const BUpdate& update);
		virtual BRect					Bounds() const;
};

/******************************************************************************/

void RVisual::Display(const IRender::ptr& into) {
	Remote()->Put(BValue(g_keyDisplay, into->AsBinder()));
}

void RVisual::Draw(const IRender::ptr& into) {
	Remote()->Put(BValue(g_keyDraw, into->AsBinder()));
}

void RVisual::Invalidate(const BUpdate& update) {
	Remote()->Put(BValue(g_keyInvalidate, update.AsValue()));
}

BRect RVisual::Bounds() const {
	return BRect(Remote()->Get(BValue(g_keyBounds)));
}

/******************************************************************************/

B_IMPLEMENT_META_INTERFACE(Visual)
; // for Eddie

/******************************************************************************/

status_t 
LVisual::Told(BValue &map)
{
	BValue val;
	if (val = map[g_keyDisplay]) {
		IRender::ptr into = IRender::AsInterface(val);
		Display(into);
	}
	if (val = map[g_keyDraw]) {
		IRender::ptr into = IRender::AsInterface(val);
		Draw(into);
	}
	if (val = map[g_keyInvalidate])		Invalidate(BUpdate(val));
	return B_OK;
}

status_t
LVisual::Called(BValue &/*in*/, const BValue &/*outBindings*/, BValue &/*out*/)
{
	return B_OK;
}

status_t 
LVisual::Asked(const BValue &outBindings, BValue &out)
{
	BValue props;
	props.
		Overlay(g_keyBounds,		Bounds().AsValue());
	out += outBindings * props;
	return B_OK;
}


} }	// namespace B::Render2
