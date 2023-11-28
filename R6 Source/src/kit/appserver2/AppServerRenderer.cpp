
#include <render2/RenderProtocol.h>
#include <appserver2_p/AppServerBitmap.h>
#include <appserver2_p/AppServerRenderer.h>

BAppServerRenderer::BAppServerRenderer(BRenderOutputPipe *pipe)
	: BPipeRenderer(pipe)
{
}


BAppServerRenderer::~BAppServerRenderer()
{
}

void 
BAppServerRenderer::DrawBitmap(const BBitmap *bitmap, BRect srcRect, BRect dstRect)
{
	const BAppServerBitmap *asbmp = dynamic_cast<const BAppServerBitmap*>(bitmap);

	if (asbmp) {
		fSession->write32(GR_SCALE_BITMAP1_A);
		fSession->writeRect(srcRect);
		fSession->writeRect(dstRect);
		fSession->write32(asbmp->ServerToken());
	} else
		BPipeRenderer::DrawBitmap(bitmap,srcRect,dstRect);
}
