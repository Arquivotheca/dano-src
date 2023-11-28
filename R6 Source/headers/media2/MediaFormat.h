/***************************************************************************
//
//	File:			media2/MediaFormat.h
//
//	Description:	Extends BValue to represent a fully-negotiated format.
//					Adds conversion from BMediaConstraintAlternative
//					(provided it contains no wildcards) and some convenience
//					methods.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIAFORMAT_H_
#define _MEDIA2_MEDIAFORMAT_H_

#include <media2/MediaDefs.h>
#include <support2/Value.h>

namespace B {
namespace Media2 {

using namespace Support2;

class BMediaConstraintAlternative;

class BMediaFormat : public BValue
{
public:
									BMediaFormat();
									BMediaFormat(const BMediaConstraintAlternative & alternative);
									BMediaFormat(const BValue & value);
									
									BMediaFormat(const media_format & format);
			status_t				as_media_format(media_format * outFormat) const;
	
			status_t				SetTo(const BMediaConstraintAlternative & alternative);
	
	// returns a buffer size if applicable to the stored format, or 0 if not 
			size_t					BufferSize() const;

private:
};

}; }; // namespace B::Media2

#endif // _MEDIA2_MEDIAFORMAT_H_
