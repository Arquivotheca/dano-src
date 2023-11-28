
#ifndef	_APPSERVER2_APPSERVERPIPE_H_
#define	_APPSERVER2_APPSERVERPIPE_H_

#include <support2/PortPipe.h>
#include <render2/RenderPipe.h>

namespace B {
namespace AppServer2 {

class BAppServerOutputPipe : public BRenderOutputPipe, public BPortOutputPipe
{
	public:		BAppServerOutputPipe(port_id outputPort, int32 id, int32 bufferSize)
					: BPortOutputPipe(outputPort,id,bufferSize) {};
};

class BAppServerInputPipe : public BRenderInputPipe, public BPortInputPipe
{
	public:		BAppServerInputPipe(port_id inputPort, int32 id)
					: BPortInputPipe(inputPort,id) {};
};

} }	// namespace B::AppServer2

using namespace B::AppServer2;

#endif	/* _APPSERVER2_APPSERVERPIPE_H_ */
