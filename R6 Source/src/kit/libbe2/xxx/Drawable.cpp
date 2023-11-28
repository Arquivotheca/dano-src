
#include <math.h>
#include <Message.h>
#include <Region.h>
#include <InterfaceUtils.h>
#include <View.h>
#include <ViewGroup.h>
#include <Layout.h>

int32 BView::m_nextID = 1;

BView::BView()
{
	Init();
}

bool 
BView::Lock() const
{
	m_lock.Lock();
	return true;
}

void 
BView::Unlock() const
{
	m_lock.Unlock();
}

BLayoutConstraints *
BView::Constraints()
{
	if (!m_constraints) m_constraints = new BLayoutConstraints();
	return m_constraints;
}

BView::BView(const BMessage &attr)
{
	int32 count;
	BString s;

	Init();

	if (attr.FindString("left",s) == B_OK) 		Constraints()->SetLeft(parse_dimth(s));
	if (attr.FindString("right",s) == B_OK)		Constraints()->SetRight(parse_dimth(s));
	if (attr.FindString("top",s) == B_OK)		Constraints()->SetTop(parse_dimth(s));
	if (attr.FindString("bottom",s) == B_OK)	Constraints()->SetBottom(parse_dimth(s));

	if (attr.FindString("width",s) == B_OK) {
		dimth values[3];
		count = parse_dimths(s,values,3);
		if (count == 1) {
			Constraints()->SetWidth(values[0],true);
		} else if (count == 2) {
			Constraints()->axis[B_HORIZONTAL].min = values[0];
			Constraints()->axis[B_HORIZONTAL].max = values[1];
		} else if (count == 3) {
			Constraints()->axis[B_HORIZONTAL].min = values[0];
			Constraints()->axis[B_HORIZONTAL].pref = values[1];
			Constraints()->axis[B_HORIZONTAL].max = values[2];
		}
	}

	if (attr.FindString("height",s) == B_OK) {
		dimth values[3];
		count = parse_dimths(s,values,3);
		if (count == 1) {
			Constraints()->SetHeight(values[0],true);
		} else if (count == 2) {
			Constraints()->axis[B_VERTICAL].min = values[0];
			Constraints()->axis[B_VERTICAL].max = values[1];
		} else if (count == 3) {
			Constraints()->axis[B_VERTICAL].min = values[0];
			Constraints()->axis[B_VERTICAL].pref = values[1];
			Constraints()->axis[B_VERTICAL].max = values[2];
		}
	}

	if (attr.FindString("prefw",s) == B_OK) {
		dimth value;
		parse_dimths(s,&value,1);
		Constraints()->SetWidth(value,false);
	}

	if (attr.FindString("prefh",s) == B_OK) {
		dimth value;
		parse_dimths(s,&value,1);
		Constraints()->SetHeight(value,false);
	}

	#if DEBUG_LAYOUT
	if (attr.FindString("debug",m_debugName) == B_OK) {
		printf("debug(%s):H ",m_debugName.String()); Constraints()->axis[B_HORIZONTAL].PrintToStream(); printf("\n");
		printf("debug(%s):V ",m_debugName.String()); Constraints()->axis[B_VERTICAL].PrintToStream(); printf("\n");
	}
	#endif
}

status_t 
BView::SetNamespace(binder ns)
{
	binder oldNS;

	Lock();
	oldNS = m_namespace;
	m_namespace = ns;
	if (oldNS != NULL) oldNS->RenounceAncestry();
	if (m_namespace != NULL) {
		property parent = Parent();
		if (!parent.IsUndefined()) {
			m_namespace->InheritFrom(parent["namespace"]);
		}
	}
	Unlock();
}

BBinder::property 
BView::Namespace()
{
	property prop;

	Lock();
	if (m_namespace == NULL) {
		m_namespace = new BBinder();
		property parent = Parent();
		if (!parent.IsUndefined()) {
			m_namespace->InheritFrom(parent["namespace"]);
		}
	}
	prop = m_namespace;
	Unlock();
	
	return prop;
}

void
BView::Acquired()
{
	BBinder::Acquired();
}

void
BView::Init()
{
	m_ID = atomic_add(&m_nextID,1);
	m_constraints = NULL;
	m_needs = 0;
}

void
BView::Trace(const char *msg, ...)
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
BView::Position(layoutbuilder_t , BRegion &)
{
	return false;
}

void 
BView::Clean(layout_t layout, BRegion &dirty)
{
	dirty.Include(layout.Bounds());
}

void 
BView::Draw(layout_t , BDrawable &, const BRegion &)
{
}

bool
BView::Constrain()
{
	return false;
}

void
BView::GetConstraints(int32 axis, BLayoutConstraint &constraint) const
{
	if (m_constraints) constraint = m_constraints->axis[axis];
}

int32
BView::TendToNeeds()
{
	return atomic_and(&m_needs,0);
}

int32 
BView::PropagateNeeds(int32 needFlags, const view_ptr &)
{
	int32 oldNeeds = atomic_or(&m_needs,needFlags);
	if (needFlags) {
		viewgroup_ptr parent = LocalParent();
		if (parent != NULL) parent->PropagateNeeds(nfDescend,this);
	}

	return oldNeeds;
}

void 
BView::ConstraintsChanged()
{
	viewgroup_ptr parent = LocalParent();
	if (parent != NULL) parent->PropagateNeeds(nfConstrain|nfLayout,this);
}

void 
BView::SetSize(BPoint size)
{
	Constraints()->axis[B_HORIZONTAL].pref = size.x;
	Constraints()->axis[B_VERTICAL].pref = size.y;
	ConstraintsChanged();
}

void 
BView::SetPosition(BPoint location)
{
	Constraints()->axis[B_HORIZONTAL].pos = location.x;
	Constraints()->axis[B_VERTICAL].pos = location.y;
	ConstraintsChanged();
}

void 
BView::SetFrame(BRect frame)
{
	SetSize(BPoint(frame.right-frame.left+1,frame.bottom-frame.top+1));
	SetPosition(BPoint(frame.left,frame.top));
}

viewgroup_ptr 
BView::LocalParent() const
{
	return m_parent.acquire();
}

BBinder::property 
BView::Parent()
{
	property prop;
	Lock();
	prop = m_parent.acquire().ptr();
	Unlock();
	return prop;
}

status_t 
BView::SetParent(const binder &parent)
{
	BViewGroup *localParent = dynamic_cast<BViewGroup*>(parent.ptr());
	if (parent.ptr() && !localParent) return B_ERROR;

	if (Lock()) {
		viewgroup_ptr oldParent = LocalParent();
		m_parent = localParent;
		if (m_namespace != NULL) {
			m_namespace->RenounceAncestry();
			if (localParent != NULL) m_namespace->InheritFrom(localParent->Property("namespace"));
		}
		Unlock();
		if (oldParent != NULL) oldParent->PropagateNeeds(nfRemoveChild,this);
		if (localParent) localParent->PropagateNeeds(nfAddChild,this);
	}
	
	return B_OK;
}

BView::~BView()
{
}

const char *gehmlBinderProps[] = {
	"namespace",
	NULL
};

status_t 
BView::OpenProperties(void **cookie, void *copyCookie)
{
	int32 *i = new int32;
	if (copyCookie) *i = *((int32*)copyCookie);
	else *i = 0;
	*cookie = i;
	return B_OK;
}

status_t 
BView::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	int32 *i = (int32*)cookie;
	if (!gehmlBinderProps[*i]) return ENOENT;
	strncpy(nameBuf,gehmlBinderProps[*i],*len);
	nameBuf[*len - 1] = 0;
	return B_OK;
}

status_t 
BView::CloseProperties(void *cookie)
{
	int32 *i = (int32*)cookie;
	delete i;
	return B_OK;
}

put_status_t 
BView::WriteProperty(const char *name, const property &prop)
{
	if (!strcmp(name,"parent")) {
		SetParent(prop);
	} else
		return EPERM;
		
	return B_OK;
}

get_status_t 
BView::ReadProperty(const char *name, property &prop, const property_list &)
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
BView::DumpInfo(int32 indent)
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
	BLayoutConstraints cnst;
	GetXYConstraints(cnst);
	cnst.PrintToStream();

	printf("\n");
}
#endif
