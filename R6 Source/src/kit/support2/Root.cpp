#include <support2/Root.h>
#include <support2/String.h>
#include <support2/Looper.h>
#include <support2/Debug.h>
#include <OS.h>
#include <image.h>
#include <stdio.h>

namespace B {
namespace Support2 {

static const BValue g_key_LoadObject("LoadObject");
static const BValue g_key_LoadObject_pathname("LoadObject_pathname");
static const BValue g_key_LoadObject_params("LoadObject_params");



class RRoot : public RInterface<IRoot>
{
	public:
		RRoot(IBinder::arg o) : RInterface<IRoot>(o) {};
		
		virtual IBinder::ptr	LoadObject(BString pathname, const BValue & params);
};


BImage::BImage(image_id id)
	:m_image(id)
{
}

BImage::~BImage()
{
	printf("BImage destructor called (area %d)\n", (int) m_image);
	BLooper * me = BLooper::This();
	if (!me) {
		debugger("going to leak an addon!");
		// The last reference to this add-on is gone,
		// but nobody is going to unload the add-on
		// because we can't find a BLooper to do it
		// for us
	} else {
		me->m_dyingAddons.AddItem(m_image);
	}
}

BImage::ptr BImageRef::Image() const
{
	return m_image;
}

status_t
LRoot::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	BValue val;
	
	// ------- LoadObject ------------------------------------------------
	if ((val = in[g_key_LoadObject])) {
		BString pathname = val[g_key_LoadObject_pathname].AsString();
		BValue params = val[g_key_LoadObject_params];
		IBinder::ptr loadedObjectBinder = this->LoadObject(pathname, params);
		BValue result(g_key_LoadObject, loadedObjectBinder);
		out += outBindings * result;
	}
	
	return B_OK;
}


IBinder::ptr
RRoot::LoadObject(BString pathname, const BValue & params)
{
	BValue arg;
	arg.Overlay(g_key_LoadObject_pathname, pathname);
	arg.Overlay(g_key_LoadObject_params, params);
	BValue returned = Remote()->Invoke(arg, g_key_LoadObject);
	return returned.AsBinder();
}


BRoot::BRoot()
{
}

BRoot::~BRoot()
{
	printf("BRoot destructor called\n");
}

IBinder::ptr
BRoot::LoadObject(BString pathname, const BValue & params)
{
	printf("using new BRoot::LoadObject(%s)\n", pathname.String());
	IBinder::ptr (*startRootFunc)(const BValue &, image_id);
	image_id addon = load_add_on(pathname.String());
	if (addon < 0) return NULL;

	status_t err = get_image_symbol(addon,"_start_root_",B_SYMBOL_TYPE_TEXT,(void**)&startRootFunc);
	if (err < 0) return NULL;
	return startRootFunc(params, addon);
}

const BValue IRoot::descriptor(BValue::TypeInfo(typeid(IRoot)));
B_IMPLEMENT_META_INTERFACE(Root)

} } // namespace B::Support2

