#ifndef C_MEDIA_EXTRACTOR_H

#define C_MEDIA_EXTRACTOR_H

#include "IMediaExtractor.h"

#include <media2/MediaNode.h>
#include <media2/MediaProducer.h>
#include <support2/Binder.h>

namespace B {
namespace Private {

class LMediaExtractor : public B::Support2::LInterface<B::Media2::IMediaExtractor>
{
	protected:
		virtual ~LMediaExtractor() {};
	
	public:
		virtual	status_t Called (BValue &in,
									const BValue &outBindings,
									BValue &out);		
};

} } // B::Private

namespace B {
namespace Media2 {

class BMediaExtractor : public B::Media2::BMediaNode,
						public B::Media2::BMediaProducer,
						public B::Private::LMediaExtractor
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BMediaExtractor)
		
		BMediaExtractor (const char *name);
		virtual	BValue	Inspect(const BValue &which, uint32 flags = 0);
};

} } // B::Media2

#endif
