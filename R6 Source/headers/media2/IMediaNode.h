#ifndef _MEDIA2_MEDIANODE_INTERFACE_
#define _MEDIA2_MEDIANODE_INTERFACE_

#include <support2/IInterface.h>
#include <support2/IBinder.h>
#include <media2/MediaDefs.h>

namespace B {
namespace Media2 {

using namespace Support2;

class IMediaNode : public IInterface
{
public:
	B_DECLARE_META_INTERFACE(MediaNode)

	virtual	BString					Name() const = 0;
	virtual	atom_ptr<IMediaCollective>	Parent() const = 0;

	// Adds endpoints that match the given criteria to "outEndpoints"
	// returns the number of new items in "outEndpoints" or an error
	virtual	ssize_t					ListEndpoints(
										BMediaEndpointVector * outEndpoints,
										int32 type = B_ANY_ENDPOINT_TYPE,
										int32 state = B_ANY_ENDPOINT_STATE) const = 0;

	// Lists endpoints of this node that are internally linked to the given
	// endpoint (which must also belong to this node.)  An internal link
	// between an input and an output indicates that the input and output
	// will share a buffer source (ie. filter buffers in-place.)
	virtual	ssize_t					ListLinkedEndpoints(
										const atom_ptr<IMediaEndpoint> & fromEndpoint,
										BMediaEndpointVector * outEndpoints,
										int32 state = B_ANY_ENDPOINT_STATE) const = 0;
private:
	friend class BMediaCollective;
	friend class LMediaNode;

	virtual	status_t				SetParent(const atom_ptr<IMediaCollective> & parent) = 0;
};

}; }; // namespace B::Media2
#endif //_MEDIA2_MEDIANODE_INTERFACE_
