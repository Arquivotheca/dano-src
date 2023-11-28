
#ifndef _APPSERVER2_DRAWABLE_H
#define _APPSERVER2_DRAWABLE_H

#include <render2/PipeRenderer.h>
#include <appserver2_p/AppServerHostedSurface.h>

namespace B {
namespace AppServer2 {

class BAppServerRenderer : public BPipeRenderer
{
	public:
								BAppServerRenderer(BRenderOutputPipe* pipe);
		virtual					~BAppServerRenderer();
		
		virtual	void			DrawBitmap(const BBitmap *bitmap, BRect srcRect, BRect dstRect);
};

class BAppServerUpdateRenderer : public BAppServerRenderer
{
	public:
									BAppServerUpdateRenderer(
										BAppServerHostedSurface *surface,
										BRenderOutputPipe* pipe) : BAppServerRenderer(pipe), m_surface(surface) {};
		virtual						~BAppServerUpdateRenderer() { m_surface->EndUpdate(); };

		BAppServerHostedSurface *	Surface() { return m_surface; };

	private:
		
		BAppServerHostedSurface *m_surface;
};

} }	// namespace B::AppServer2

using namespace B::AppServer2;

#endif	// _APPSERVER2_DRAWABLE_H
