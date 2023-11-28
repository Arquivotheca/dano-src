#ifndef C_MEDIA_EXTRACTOR_INTERFACE_H

#define C_MEDIA_EXTRACTOR_INTERFACE_H

#include <support2/IByteStream.h>

using B::Support2::atom_ptr;
using B::Support2::atom_ref;
using B::Support2::BValue;
using B::Support2::IBinder;

namespace B {
namespace Media2 {

class IMediaExtractor : public ::B::Support2::IInterface
{
	public:
		B_DECLARE_META_INTERFACE(MediaExtractor)
		
		virtual status_t Sniff (::B::Support2::IByteInput::arg stream,
								float *quality) = 0;

		virtual status_t AttachedToStream (::B::Support2::IByteInput::arg stream) = 0;
		virtual void DetachedFromStream() = 0;
};

} } // B::Media2

#endif
