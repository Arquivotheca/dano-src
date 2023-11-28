#ifndef C_SOUNDPLAYER_NODE_H

#define C_SOUNDPLAYER_NODE_H

#include <media2/MediaNode.h>

namespace B {
namespace Private {

class BSoundPlayerNode : public ::B::Media2::BMediaNode
{
	::B::Media2::BMediaOutput::ptr mOutput;
	
	public:
		B_STANDARD_ATOM_TYPEDEFS(BSoundPlayerNode);

		BSoundPlayerNode (const char *name);
		
		virtual status_t Acquired (const void *id);
		virtual status_t Released (const void *id);

		::B::Media2::BMediaOutput::ptr Output() const;
		::B::Media2::BMediaFormat Format() const;
		
		void DisconnectSelf();
};

} } // B::Private

#endif
