
#include <interface2/SurfaceManager.h>

namespace B {
namespace Interface2 {

class RSurfaceManager : public RInterface<ISurfaceManager>
{
	public:

									RSurfaceManager(IBinder::arg remote) : RInterface<ISurfaceManager>(remote) {};

		virtual	ISurface::ptr		RootSurface();
		virtual	ISurface::ptr		CreateSurface(int32 width, int32 height, color_space format);
};

ISurface::ptr 
RSurfaceManager::RootSurface()
{
	return ISurface::AsInterface(Remote()->Get(value_ref("root")));
}

ISurface::ptr 
RSurfaceManager::CreateSurface(int32 /*width*/, int32 /*height*/, color_space /*format*/)
{
	return NULL;
}

B_IMPLEMENT_META_INTERFACE(SurfaceManager)

status_t 
LSurfaceManager::Told(value &/*in*/)
{
	return B_OK;
}

status_t 
LSurfaceManager::Asked(const value &outBindings, value &out)
{
	out += (outBindings *
			(	BValue(descriptor, BValue::Binder(this))
				.Overlay(BValue("root", RootSurface()->AsBinder())) ) );
	return B_OK;
}

} }	// namespace B::Interface2
