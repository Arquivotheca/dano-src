
#ifndef _RENDER2_RENDERSTATE_H_
#define _RENDER2_RENDERSTATE_H_

#include <render2/RenderDefs.h>
#include <render2/Color.h>
#include <render2/Point.h>
#include <render2/Font.h>
#include <render2/2dTransform.h>
#include <render2/Region.h>
#include <raster2/RasterRegion.h>
#include <support2/String.h>
namespace B {
namespace Render2 {

using namespace Support2;

// --------------------------------------------------------------------------

struct BRenderState
{
					BRenderState();
					BRenderState(const BRenderState& copy);
	BRenderState&	operator = (const BRenderState& copy);

	BRenderState *	Prev() const	{ return m_prev; }
	BRenderState *	Next() const	{ return m_next; }
	static void		Push(BRenderState **stack, BRenderState *object);
	static void		Pop(BRenderState **stack);

	void					Revert();
	const B2dTransform&		Transform();
	void 					ApplyTransform(const B2dTransform& transform);

	// ---- dumb ass implementation ----
	BPoint			path[16];
	int32			cntpt;
	bool			closed;
	BString			string;
	// ---------------------------------

private:
	B2dTransform	m_transform;

private:
	mutable BRenderState *m_prev;
	mutable BRenderState *m_next;
};


} }	// namespace B::Render2

#endif	// _RENDER2_RENDERSTATE_H_
