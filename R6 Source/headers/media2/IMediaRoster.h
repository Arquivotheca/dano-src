#ifndef C_MEDIA_ROSTER_INTERFACE_H

#define C_MEDIA_ROSTER_INTERFACE_H

#include <media2/IMediaNode.h>
#include <media2/IMediaEndpoint.h>
#include <media2/ITimeSource.h>

namespace B {
namespace Media2 {

using B::Support2::atom_ptr;
using B::Support2::atom_ref;
using B::Support2::BValue;
using B::Support2::IBinder;

class IMediaRoster : public ::B::Support2::IInterface
{
	public:
		B_DECLARE_META_INTERFACE(MediaRoster)
		
		virtual status_t GetAudioMixer (B::Media2::IMediaNode::ptr *node) = 0;
		virtual status_t SetAudioMixer (B::Media2::IMediaNode::arg node) = 0;

		virtual status_t GetTimeSource (B::Media2::ITimeSource::ptr *node) = 0;
		virtual status_t SetTimeSource (B::Media2::ITimeSource::arg node) = 0;
		
		virtual status_t GetDecoderChain (B::Media2::IMediaOutput::arg starting_at,
											B::Media2::IMediaNode::ptr *collective) = 0;
											
};

} } // B::Media2

#endif
