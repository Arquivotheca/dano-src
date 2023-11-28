
#include <ResourceCache.h>
#include "BDrawable.h"
#include "GehmlBlock.h"
#include "GehmlLayout.h"
#include "GehmlUtils.h"

GehmlBlock::GehmlBlock(BStringMap &attr) : GehmlObject(attr)
{
	BString *s = attr.Find("color");
	if (s) m_color = decode_color(s->String());
	NeedDraw();
}

GehmlBlock::~GehmlBlock()
{
}

bool 
GehmlBlock::Position(layoutbuilder_t layout, BRegion &outDirty)
{
	BRegion reg;

	reg.Include(layout.Bounds());
	reg.Exclude(layout.OldBounds());
	outDirty.Include(&reg);

	reg.MakeEmpty();

	reg.Include(layout.OldBounds());
	reg.Exclude(layout.Bounds());
	outDirty.Include(&reg);

	#if DEBUG_LAYOUT
	if (DebugName().Length()) {
		outDirty.Include(layout.Bounds());
	}
	#endif
	
	return false;
}

void 
GehmlBlock::Draw(layout_t layout, BDrawable &into, const BRegion &)
{
	BRect bounds = layout.Bounds();
//	printf("%s: ",DebugName().String()); bounds.PrintToStream();
	into.SetHighColor(m_color);
	into.FillRect(bounds);
	#if DEBUG_LAYOUT
	if (DebugName().Length()) {
		into.SetHighColor(make_color(0,0,0));
		into.StrokeLine(bounds.LeftTop(),bounds.RightBottom());
		into.StrokeLine(bounds.RightTop(),bounds.LeftBottom());
		into.StrokeRect(bounds);
	}
	#endif
}

status_t 
GehmlBlock::Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &)
{
	child = new GehmlBlock(attributes);
	return B_OK;
}

