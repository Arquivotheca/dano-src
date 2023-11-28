#ifndef _MEDIA2_MEDIACOLLECTIVE_INTERFACE_
#define _MEDIA2_MEDIACOLLECTIVE_INTERFACE_

#include <support2/IInterface.h>
#include <support2/IBinder.h>
#include <media2/MediaDefs.h>

namespace B {
namespace Media2 {

using namespace Support2;

class IMediaCollective : public IInterface
{
public:
	B_DECLARE_META_INTERFACE(MediaCollective)

	virtual	ssize_t				ListNodes(BMediaNodeVector * outNodes) const = 0;
};

}; }; // namespace B::Media2
#endif //_MEDIA2_MEDIACOLLECTIVE_INTERFACE_
