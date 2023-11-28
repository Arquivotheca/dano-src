
#include <View.h>
#include <Region.h>
#include <ResourceCache.h>
#include "BDrawable.h"
#include "BViewDrawable.h"
#include "BAppSessionDrawable.h"
#include "GehmlContent.h"
#include "GehmlLayout.h"

using namespace Wagner;

GehmlContent::GehmlContent(BStringMap &attr) : GehmlObject(attr)
{
	m_id = 0;
	m_size[0] = m_size[1] = 0;
	attr.Find("src",m_src);
}

GehmlContent::~GehmlContent()
{
}

void 
GehmlContent::Acquired()
{
	GehmlObject::Acquire();
	StartListening(Namespace());
//	Overheard(Namespace(),B_PROPERTY_CHANGED,"baseURL");
}

status_t 
GehmlContent::Overheard(binder_node node, uint32 observed, BString propertyName)
{
	URL url;
	property This = this;
	property id = m_id;
	property myURL = m_src.String();
	property result = node->Property("loadURL",&This,&myURL,&id,NULL);
	printf("result: %s\n",result.String().String());
}

status_t 
GehmlContent::HandleMessage(BMessage *msg)
{
	uint32 flags;

	switch (msg->what) {
		case bmsgContentDirty: {
			if (m_content) {
				int32 width,height;
				m_content->GetSize(&width,&height,&flags);
				if ((width != m_size[0]) || (height != m_size[1])) {
					m_size[0] = width;
					m_size[1] = height;
					ConstraintsChanged();
				}
				NeedDraw();
			}
		} break;
		case NEW_CONTENT_INSTANCE: {
			if (msg->FindAtom("instance", m_content) == B_OK) {
				m_content->GetSize(&m_size[0],&m_size[1],&flags);
//				printf("NEW_CONTENT_INSTANCE %s\n",DebugName().String());
				ConstraintsChanged();
				NeedDraw();
			}
		} break;
		case CONTENT_INSTANCE_ERROR: {
			printf("CONTENT_INSTANCE_ERROR\n");
		} break;
	}
	return B_OK;
}

bool 
GehmlContent::Position(layoutbuilder_t layout, BRegion &outDirty)
{
	outDirty.Include(layout.OldBounds());
	outDirty.Include(layout.Bounds());
	return GehmlObject::Position(layout,outDirty);
}

void 
GehmlContent::Clean(layout_t layout, BRegion &dirty)
{
	BRect bounds = layout.Bounds();
	BRegion reg1(bounds),reg2;
	if (m_content) m_content->FetchDirty(reg2);
	if (!reg2.CountRects())
		dirty.Include(&reg1);
	else {
		reg2.OffsetBy(bounds.left,bounds.top);
		reg1.IntersectWith(&reg2);
		dirty.Include(&reg1);
	}
}

void 
GehmlContent::Draw(layout_t layout, BDrawable &into, const BRegion &dirty)
{
	BRect bounds = layout.Bounds();
	GehmlObject::Draw(layout,into,dirty);

	if (m_content) {
//		if (DebugName().Length()) printf("%s drawing with content!\n",DebugName().String());
		m_content->FrameChanged(bounds,bounds.Width()+1,bounds.Height()+1);

		BViewDrawable *view = dynamic_cast<BViewDrawable*>(&into);
		if (view) m_content->Draw(view->View(),dirty.Frame());
		else {
			BAppSessionDrawable *asd = dynamic_cast<BAppSessionDrawable*>(&into);
			if (asd) m_content->Draw((::BView*)asd->HackView(),dirty.Frame());
		}
	} else {
		into.SetHighColor(make_color(0,0,0));
		into.StrokeRect(bounds);
	}
}

void 
GehmlContent::GetConstraints(int32 axis, GehmlConstraint &c) const
{
	GehmlObject::GetConstraints(axis,c);
	if (c.pref.is_undefined()) c.pref = dimth(m_size[axis],dimth::pixels);
/*
		if (!(flags & Wagner::STRETCH_HORIZONTAL)) {
			if (isnan(constraints.absLimits[HORIZONTAL][MIN_PARAM]))
				constraints.absLimits[HORIZONTAL][MIN_PARAM] = width;
			if (isnan(constraints.absLimits[HORIZONTAL][MAX_PARAM]))
				constraints.absLimits[HORIZONTAL][MAX_PARAM] = width;
		}
*/
}

/*
status_t 
GehmlContent::SetParent(const binder_node &parent)
{
	status_t err;
	if (!(err = GehmlObject::SetParent(parent)))
		Overheard(this, B_SOMETHING_CHANGED, NULL);
	
	return err;
}
*/

status_t 
GehmlContent::Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &)
{
	child = new GehmlContent(attributes);
	return B_OK;
}
