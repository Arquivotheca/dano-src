#include "MediaExtractor.h"

using namespace B::Support2;

namespace B {
namespace Media2 {

const BValue IMediaExtractor::descriptor(BValue::TypeInfo(typeid(IMediaExtractor)));

} // Media2

namespace Private {

class RMediaExtractor : public RInterface<B::Media2::IMediaExtractor>
{
	friend class LMediaExtractor;
	
	static const BValue kKeySniff;
	static const BValue kKeySniffStream;
	static const BValue kKeySniffQuality;
	static const BValue kKeySniffResult;
	
	static const BValue kKeyAttached;
	static const BValue kKeyAttachedStream;
	static const BValue kKeyAttachedResult;
	
	static const BValue kKeyDetached;
	
	public:
		RMediaExtractor (IBinder::arg o)
			: RInterface<IMediaExtractor>(o)
		{
		}
		
		virtual status_t Sniff (::B::Support2::IByteInput::arg stream,								
								float *quality)
		{
			BValue params(kKeySniff,BValue::Bool(true));
			params.Overlay(kKeySniffStream,BValue(stream->AsBinder()));

			BValue result=Remote()->Invoke(params);
			
			*quality=result[kKeySniffQuality].AsFloat();
			
			return result[kKeySniffResult].AsInt32();
		}						

		virtual status_t AttachedToStream (::B::Support2::IByteInput::arg stream)
		{
			return Remote()->Invoke(BValue(kKeyAttached,BValue::Bool(true))
									.Overlay(kKeyAttachedStream,BValue(stream->AsBinder())))
									[kKeyAttachedResult].AsInt32();
		}
		
		virtual void DetachedFromStream()
		{
			Remote()->Invoke(BValue(kKeyDetached,BValue::Bool(true)));
		}
};

const BValue RMediaExtractor::kKeySniff("Sniff");
const BValue RMediaExtractor::kKeySniffStream("SniffStream");
const BValue RMediaExtractor::kKeySniffQuality("SniffQuality");
const BValue RMediaExtractor::kKeySniffResult("SniffResult");

const BValue RMediaExtractor::kKeyAttached("Attached");
const BValue RMediaExtractor::kKeyAttachedStream("AttachedStream");
const BValue RMediaExtractor::kKeyAttachedResult("AttachedResult");

const BValue RMediaExtractor::kKeyDetached("Detached");

} // Private

namespace Media2 {

using namespace B::Private;

B_IMPLEMENT_META_INTERFACE(MediaExtractor)

} // Media2

namespace Private {

status_t 
LMediaExtractor::Called(BValue &in, const BValue &, BValue &)
{
	if (in[RMediaExtractor::kKeySniff].IsDefined())
	{
		float quality;
		status_t result=Sniff(IByteInput::AsInterface(in[RMediaExtractor::kKeySniffStream].AsBinder()),
								&quality);

		Push(BValue(RMediaExtractor::kKeySniffQuality,BValue::Float(quality)));
		Push(BValue(RMediaExtractor::kKeySniffResult,BValue::Int32(result)));
	}
	
	if (in[RMediaExtractor::kKeyAttached].IsDefined())
	{
		Push(BValue(RMediaExtractor::kKeyAttachedResult,
				BValue::Int32(
						AttachedToStream(IByteInput::AsInterface(in[RMediaExtractor::kKeyAttachedStream].AsBinder()))
				)));				
	}
	
	if (in[RMediaExtractor::kKeyDetached].IsDefined())
		DetachedFromStream();
	
	return B_OK;
}

} // Private

namespace Media2 {

BMediaExtractor::BMediaExtractor(const char *name)
	: BMediaNode(name)
{
}

BValue 
BMediaExtractor::Inspect(const BValue &in, uint32 flags)
{
	return BMediaNode::Inspect(in, flags)
			.Overlay(BMediaProducer::Inspect(in, flags))
			.Overlay(LMediaExtractor::Inspect(in, flags));
}

} // Media2
} // B
