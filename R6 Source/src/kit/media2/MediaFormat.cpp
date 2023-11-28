#include <media2/MediaDefs.h>
#include <media2/MediaFormat.h>
#include <media2/MediaConstraintAlternative.h>

#include <support2/StdIO.h>

#include "convert_format.h"

namespace B {
namespace Private {

using namespace Support2;
using namespace Media2;

class MediaFormatBuilder : public IValueOutput
{
	BMediaFormat &	_f;
public:
	B_STANDARD_ATOM_TYPEDEFS(MediaFormatBuilder)

					MediaFormatBuilder(BMediaFormat & f) : _f(f) {}
	status_t		Write(const BValue &out)
	{
		_f.Overlay(out);
		return B_OK;
	}
	status_t		End()
	{
		return B_OK;
	}
	
	protected:
		virtual	atom_ptr<IBinder>		AsBinderImpl()			{ return NULL; }
		virtual	atom_ptr<const IBinder>	AsBinderImpl() const	{ return NULL; }
};
} // Private

namespace Media2 {

using B::Private::MediaFormatBuilder;

BMediaFormat::BMediaFormat()
{
}

BMediaFormat::BMediaFormat (const BMediaConstraintAlternative &alternative)
{
	SetTo(alternative);
}

BMediaFormat::BMediaFormat(const media_format &format)
{
	MediaFormatBuilder::ptr fb = new MediaFormatBuilder(*this);
	export_format_values(format, fb);
}

status_t 
BMediaFormat::as_media_format(media_format *outFormat) const
{
	if (!outFormat) return B_BAD_VALUE;
	void * cookie = 0;
	BValue k, v;
	while (GetNextItem(&cookie, &k, &v) >= B_OK)
	{
		status_t err = import_format_value(BValue(k, v), *outFormat);
		if (err < B_OK) return err;
	}
	return B_OK;
}

BMediaFormat::BMediaFormat(const BValue &value) :
	BValue(value)
{
}

status_t 
BMediaFormat::SetTo(const BMediaConstraintAlternative &alternative)
{
	Undefine();

	bool resolved=true;
	for (size_t i=0;resolved && i<alternative.CountConstraintItems();++i)
	{
		if (!alternative.ConstraintItemAt(i).IsOptional() &&
			(alternative.ConstraintItemAt(i).Relation() != BMediaConstraintItem::B_EQ
			|| alternative.ConstraintItemAt(i).Value().CountItems()>1))
		{
			resolved=false;
		}
	}
	
	if (!resolved)
		return B_BAD_VALUE;		// there are still wildcards
		
	for (size_t i=0;resolved && i<alternative.CountConstraintItems();++i)
	{
		if (!alternative.ConstraintItemAt(i).IsOptional())
		{
			Overlay(
				alternative.ConstraintItemAt(i).Key(),
				alternative.ConstraintItemAt(i).Value());
		}
	}
		
	return B_OK;
}

size_t 
BMediaFormat::BufferSize() const
{
	switch (ValueFor(B_FORMATKEY_MEDIA_TYPE).AsInteger())
	{
		case B_MEDIA_RAW_AUDIO:
			return
				(ValueFor(B_FORMATKEY_RAW_AUDIO_TYPE).AsInteger() & B_AUDIO_SIZE_MASK) *
				ValueFor(B_FORMATKEY_CHANNEL_COUNT).AsInteger() *
 				ValueFor(B_FORMATKEY_BUFFER_FRAMES).AsInteger();
		
		default:
			return 0;
	}
}


} } // namespace B::Media2
