
#include <math.h>
#include <View.h>
#include <Region.h>
#include "GehmlObject.h"
#include "GehmlGroup.h"
#include "GehmlUtils.h"
#include "GehmlLayout.h"

int32 GehmlObject::m_nextID = 1;

GehmlObject::GehmlObject()
{
	Init();
}

bool 
GehmlObject::Lock() const
{
/*
	// This version, interestingly enough, triggers a compiler bug
	const_cast<GehmlObject>(*this).m_lock.Lock();
	return true;
*/
	GehmlObject *obj = (GehmlObject*)((void*)this);
	obj->m_lock.Lock();
	return true;
}

void 
GehmlObject::Unlock() const
{
/*
	// As does this!
	const_cast<GehmlObject>(*this).m_lock.Unlock();
*/
	GehmlObject *obj = (GehmlObject*)((void*)this);
	obj->m_lock.Unlock();
}

GehmlConstraints *
GehmlObject::Constraints()
{
	if (!m_constraints) m_constraints = new GehmlConstraints();
	return m_constraints;
}

GehmlObject::GehmlObject(BStringMap &attr)
{
	int32 count;
	BString *s;

	Init();

	if ((s = attr.Find("left"))) 	Constraints()->SetLeft(parse_dimth(s->String()));
	if ((s = attr.Find("right"))) 	Constraints()->SetRight(parse_dimth(s->String()));
	if ((s = attr.Find("top"))) 	Constraints()->SetTop(parse_dimth(s->String()));
	if ((s = attr.Find("bottom"))) 	Constraints()->SetBottom(parse_dimth(s->String()));

	if ((s = attr.Find("width"))) {
		dimth values[3];
		count = parse_dimths(s->String(),values,3);
		if (count == 1) {
			Constraints()->SetWidth(values[0],true);
		} else if (count == 2) {
			Constraints()->axis[HORIZONTAL].min = values[0];
			Constraints()->axis[HORIZONTAL].max = values[1];
		} else if (count == 3) {
			Constraints()->axis[HORIZONTAL].min = values[0];
			Constraints()->axis[HORIZONTAL].pref = values[1];
			Constraints()->axis[HORIZONTAL].max = values[2];
		}
	}

	if ((s = attr.Find("height"))) {
		dimth values[3];
		count = parse_dimths(s->String(),values,3);
		if (count == 1) {
			Constraints()->SetHeight(values[0],true);
		} else if (count == 2) {
			Constraints()->axis[VERTICAL].min = values[0];
			Constraints()->axis[VERTICAL].max = values[1];
		} else if (count == 3) {
			Constraints()->axis[VERTICAL].min = values[0];
			Constraints()->axis[VERTICAL].pref = values[1];
			Constraints()->axis[VERTICAL].max = values[2];
		}
	}

	if ((s = attr.Find("prefw"))) {
		dimth value;
		parse_dimths(s->String(),&value,1);
		Constraints()->SetWidth(value,false);
	}

	if ((s = attr.Find("prefh"))) {
		dimth value;
		parse_dimths(s->String(),&value,1);
		Constraints()->SetHeight(value,false);
	}

	#if DEBUG_LAYOUT
	attr.Find("debug",m_debugName);
	if (m_debugName.Length()) {
		printf("debug(%s):H ",m_debugName.String()); Constraints()->axis[HORIZONTAL].PrintToStream(); printf("\n");
		printf("debug(%s):V ",m_debugName.String()); Constraints()->axis[VERTICAL].PrintToStream(); printf("\n");
	}
	#endif
}

status_t 
GehmlObject::SetNamespace(binder_node ns)
{
	binder_node oldNS;

	Lock();
	oldNS = m_namespace;
	m_namespace = ns;
	if (oldNS) oldNS->RenounceAncestry();
	if (m_namespace) {
		checkpoint
		property parent = Parent();
		if (!parent.IsUndefined()) m_namespace->InheritFrom(parent["namespace"]);
	}
	Unlock();
}

BinderNode::property 
GehmlObject::Namespace()
{
	property prop;

	Lock();
	if (!m_namespace) {
		checkpoint
		m_namespace = new BinderNode();
		property parent = Parent();
		if (!parent.IsUndefined()) {
			checkpoint
			m_namespace->InheritFrom(parent["namespace"]);
		}
	}
	prop = m_namespace;
	Unlock();
	
	return prop;
}

void
GehmlObject::Acquired()
{
	BinderNode::Acquired();
}

void
GehmlObject::Init()
{
	m_ID = atomic_add(&m_nextID,1);
	m_constraints = NULL;
	m_needs = 0;
}

void
GehmlObject::Trace(const char *msg, ...)
{
#if DEBUG_LAYOUT
	if (m_debugName.Length()) {
		char buf[1024];
		va_list pvar;
		va_start(pvar,msg);
		sprintf(buf,"debug(%s): ",m_debugName.String());
		strcat(buf,msg);
		vprintf(buf,pvar);
		va_end(pvar);
	}
#endif
}

bool 
GehmlObject::Position(layoutbuilder_t , BRegion &)
{
	return false;
}

void 
GehmlObject::Clean(layout_t layout, BRegion &dirty)
{
	dirty.Include(layout.Bounds());
}

void 
GehmlObject::Draw(layout_t , BDrawable &, const BRegion &)
{
}

bool
GehmlObject::Constrain()
{
	return false;
}

void
GehmlObject::GetConstraints(int32 axis, GehmlConstraint &constraint) const
{
	if (m_constraints) constraint = m_constraints->axis[axis];
}

int32
GehmlObject::TendToNeeds()
{
	return atomic_and(&m_needs,0);
}

int32 
GehmlObject::PropagateNeeds(int32 needFlags, const gehml_obj &)
{
	int32 oldNeeds = atomic_or(&m_needs,needFlags);
	if (needFlags) {
		gehml_group_ref parent = LocalParent();
		if (parent) parent->PropagateNeeds(nfDescend,this);
	}

	return oldNeeds;
}

void 
GehmlObject::ConstraintsChanged()
{
	gehml_group_ref parent = LocalParent();
	if (parent) parent->PropagateNeeds(nfConstrain|nfLayout,this);
}

void 
GehmlObject::SetSize(BPoint size)
{
	Constraints()->axis[HORIZONTAL].pref = size.x;
	Constraints()->axis[VERTICAL].pref = size.y;
	ConstraintsChanged();
}

void 
GehmlObject::SetPosition(BPoint location)
{
	Constraints()->axis[HORIZONTAL].pos = location.x;
	Constraints()->axis[VERTICAL].pos = location.y;
	ConstraintsChanged();
}

void 
GehmlObject::SetFrame(BRect frame)
{
	SetSize(BPoint(frame.right-frame.left+1,frame.bottom-frame.top+1));
	SetPosition(BPoint(frame.left,frame.top));
}

gehml_group_ref 
GehmlObject::LocalParent() const
{
	return static_cast<GehmlGroup*>((BinderNode*)m_parent);
}

BinderNode::property 
GehmlObject::Parent()
{
	property prop;
	Lock();
	if (m_parent) prop = (GehmlGroup*)m_parent;
	Unlock();
	return prop;
}

status_t 
GehmlObject::SetParent(const binder_node &parent)
{
	GehmlGroup *localParent = dynamic_cast<GehmlGroup*>((BinderNode*)parent);
	if (parent && !localParent) return B_ERROR;

	if (Lock()) {
		gehml_group_ref oldParent = static_cast<GehmlGroup*>((BinderNode*)m_parent);
		m_parent = localParent;
		if (m_namespace) {
			m_namespace->RenounceAncestry();
			m_namespace->InheritFrom(m_parent->Property("namespace"));
		}
		Unlock();
		if (oldParent) oldParent->PropagateNeeds(nfRemoveChild,this);
		if (localParent) localParent->PropagateNeeds(nfAddChild,this);
	}
	
	return B_OK;
}

GehmlObject::~GehmlObject()
{
}

const char *gehmlBinderProps[] = {
	"namespace",
	NULL
};

status_t 
GehmlObject::OpenProperties(void **cookie, void *copyCookie)
{
	int32 *i = new int32;
	if (copyCookie) *i = *((int32*)copyCookie);
	else *i = 0;
	*cookie = i;
	return B_OK;
}

status_t 
GehmlObject::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	int32 *i = (int32*)cookie;
	if (!gehmlBinderProps[*i]) return ENOENT;
	strncpy(nameBuf,gehmlBinderProps[*i],*len);
	nameBuf[*len - 1] = 0;
	return B_OK;
}

status_t 
GehmlObject::CloseProperties(void *cookie)
{
	int32 *i = (int32*)cookie;
	delete i;
	return B_OK;
}

put_status_t 
GehmlObject::WriteProperty(const char *name, const property &prop)
{
	if (!strcmp(name,"parent")) {
		checkpoint
		SetParent(prop);
	} else
		return EPERM;
		
	return B_OK;
}

get_status_t 
GehmlObject::ReadProperty(const char *name, property &prop, const property_list &)
{
	if (!strcmp(name,"namespace")) {
		prop = Namespace();
	} else if (!strcmp(name,"parent")) {
		prop = Parent();
	} else
		return ENOENT;
		
	return B_OK;
}

#if DEBUG_LAYOUT
void 
GehmlObject::DumpInfo(int32 indent)
{
	while (indent--) printf("  ");
	printf("debug(%s): ",m_debugName.String());
	if (m_needs) {
		printf("(");
		if (m_needs & nfDraw) printf("nfDraw|");
		if (m_needs & nfConstrain) printf("nfConstrain|");
		if (m_needs & nfPosition) printf("nfPosition|");
		if (m_needs & nfLayout) printf("nfLayout|");
		if (m_needs & nfDescend) printf("nfDescend|");
		if (m_needs & nfAddChild) printf("nfAddChild|");
		if (m_needs & nfRemoveChild) printf("nfRemoveChild|");
	} else
		printf("<none>");

	printf(" ");
	GehmlConstraints cnst;
	GetXYConstraints(cnst);
	cnst.PrintToStream();

	printf("\n");
}
#endif
