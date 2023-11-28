/***************************************************************************
//
//	File:			render2/IRender.h
//
//	Description:	Abstract drawing interface.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _RENDER2_RENDER_H_
#define _RENDER2_RENDER_H_

#include <math.h>
#include <support2/Binder.h>
#include <render2/Color.h>
#include <render2/Point.h>
#include <render2/Rect.h>
#include <render2/RenderDefs.h>
#include <render2/Path.h>
#include <raster2/RasterDefs.h>

namespace B {
namespace Render2 {

using namespace Support2;
using namespace Raster2;

class BFont;

/**************************************************************************************/

/*!
	Rendering is modelled very explicitly as a definition of the value of every pixel
	within the target area.  The API provides a small number of primitives to modify
	the current pixel values, and allows more complex rendering to be done by
	compositing multiple operations.
	
	There are basically two types of drawing primitives: parametric drawing operations
	(path definition, stroking, and filling) and raster drawing operations (blitting).
	In the context of this model, it sometimes makes sense to think about these
	respectively as	"sparse" vs. "dense" definitions of pixel values.
	
	Parametric drawing looks very much like Postscript.  All drawing of parametric
	shapes is modelled as incremental building of a complex path with lines, arcs and
	bezier curves.  This path can have numerous disjoint subpaths.  Any of these
	subpaths can be "stroked" using standard Postscript line capping and joining modes
	to form	more complex paths.  Once a path is completely defined, it can be filled.
	A path can be filled with white (1,1,1,1), a solid color, or a linear color
	gradient.
	
	Raster drawing is very simple.  A call to BeginPixelBlock establishes the pixel
	grid within	which I wish to define samples.  This is followed by one or more calls
	to PlacePixels which define blocks of pixels within that grid.  EndPixelBlock ends the
	definition of samples within the grid (undefined pixels are assumed to be (0,0,0,0)).
	
	In addition, general 2D transformations and color-space transformations are available.
	2D transformations operate on vertices as they are passed into the path generation calls, and
	color-space transformations apply at the time of a Fill(), Color(), Shade() or
	BeginPixelBlock() call.  Both of these transformations are composited with the current
	transformation stack.
	
	State saving and restoring is accomplished with PushState()/PopState().  State includes
	the current 2D and color-space transformations, the current font, stroke style and winding
	rule, and the current path.
	
	Normal rendering involves compositing, using standard alpha blending, any pixels
	generated with these primitives with the current value of each pixel (i.e. the current
	contents of the framebuffer).  Compositions can also be generated by placing normal
	rendering commands within a BeginComposition()/EndComposition() block.  These pixel values
	are pushed onto the transformation stack as position-variant diagonal color
	transformations (i.e. modulations).  In other words, any pixel generated from that point
	forward (until a PopState() restores some earlier state) is multiplied by the pixel
	generated by the modulation block.  Any number of modulation blocks can be placed onto
	the stack, and modulations can be nested.
	
	Note that the implementation of a given composition can be highly optimized depending on
	its contents and the hardware available.  On the one hand, simple clipping is modelled as the
	modulation of an arbitrary white shape with some later drawing, but can be optimized easily to
	use our standard region-based clipping in most cases.  On the other hand, the effect of a
	modulation block is equivalent to that of the GL "modulate" texture environment, and the
	effect of multiple modultation blocks stacked atop one another is supported directly by the
	ARB GL multitexture extension.
*/
	
/**************************************************************************************/

class IRender : public IPath
{
	public:

		B_DECLARE_META_INTERFACE(Render)

		enum {
			B_SYNCHRONOUS = 0,
			B_ASYNCHRONOUS = 1
		};

		virtual	IRender::ptr		Branch(uint32 flags = B_SYNCHRONOUS) = 0;

		/*	An extension of the path-building interface, Text() is equivalent to "charpath". */
		virtual	void				Text(	const char *text,
											int32 len = B_CSTR_LEN,
											const escapements& escapements = B_NO_ESCAPEMENT) = 0;

		/*!	Composite path operations.
			These operate on the current sub-path,
			replacing it with a new path derived from the old one.
			Stroke() is equivalent to postscript's "strokepath" function.
			TextToPath() replaces the current sub-path with the path of the given glyphs
			rendered along the former sub-path. */
		virtual	void				TextOnPath(	const char *text,
												int32 len = B_CSTR_LEN,
												const escapements& escapements = B_NO_ESCAPEMENT,
												uint32 flags = B_JUSTIFY_LEFT|B_VALIGN_BASELINE) = 0;

		/*	Path generation state */
		virtual	void				BeginStroke() = 0;
		virtual	void				EndStroke() = 0;
		virtual	void				SetFont(const BFont &font) = 0;	// effective at time of Text*() call
		virtual	void				SetWindingRule(winding_rule rule) = 0; // effective at time of Fill/Color/Shade() call
		virtual	void				SetStrokeStyle( // effective at time of BeginStroke() call
										coord penWidth,
										cap_mode capping=B_BUTT_CAP,
										join_mode joining=B_BUTT_JOIN,
										float miter=10.0f,
										coord *stippling=NULL) = 0;

		/*	Path filling */
		virtual	void				Fill() = 0;
		virtual	void				Color(const BColor&) = 0;
		virtual	void				Shade(const BGradient&) = 0;

		/*	Composition stacks */
		virtual	void				Transform(const B2dTransform&) = 0;
		virtual	void				Transform(const BColorTransform&) = 0;
		virtual	void				BeginComposition() = 0;
		virtual	void				EndComposition() = 0;

		/*	Save/restore state */
		virtual	void				PushState() = 0;
		virtual	void				PopState() = 0;

		/*	Pixmap/sample support */
		virtual	void				BeginPixelBlock(const BPixelDescription& pixels) = 0;
		virtual	void				PlacePixels(const BRasterPoint& at, const BPixelData& pixels) = 0;
		virtual	void				EndPixelBlock() = 0;

		/*	Drawing an IVisual */
		virtual	void				DisplayCached(	const atom_ptr<IVisual>& view, uint32 flags,
													int32 cache_id) = 0;

		/*	Conveniences */
		virtual	void				TextAt(
										const BPoint&,
										const char *text,
										int32 len=B_CSTR_LEN,
										const escapements& escapements = B_NO_ESCAPEMENT);
};

/**************************************************************************************/

class IVisual : public IInterface
{
	public:

		B_DECLARE_META_INTERFACE(Visual)
	
		virtual	void				Display(const IRender::ptr& into) = 0;
		virtual	void				Draw(const IRender::ptr& into) = 0;
		virtual void				Invalidate(const BUpdate& update) = 0;
		virtual BRect				Bounds() const = 0;
};

/**************************************************************************************/

class LVisual : public LInterface<IVisual>
{
	public:

		virtual	status_t			Told(value &in);
		virtual	status_t			Asked(const BValue &outBindings, BValue &out);
		virtual	status_t			Called(	BValue &in,
											const BValue &outBindings,
											BValue &out);
};

/**************************************************************************************/

} } // namespace B::Render2

#endif	// _RENDER2_RENDER_H_
