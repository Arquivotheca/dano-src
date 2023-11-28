#include "TimeSource.h"

#include <support2/Debug.h>

using namespace B::Support2;

namespace B {

namespace Media2 {

const BValue ITimeSource::descriptor(BValue::TypeInfo(typeid(ITimeSource)));

class RTimeSource : public RInterface<B::Media2::ITimeSource>
{
	friend class LTimeSource;
	
	static const BValue kKeyNow;
	static const BValue kKeyNowResult;
	
	static const BValue kKeySnoozeUntil;
	static const BValue kKeySnoozeUntilWhen;
	static const BValue kKeySnoozeUntilResult;
	
	public:
		RTimeSource (IBinder::arg o)
			: RInterface<ITimeSource>(o)
		{
		}
		
		virtual bigtime_t Now()
		{
			return Remote()->Invoke(BValue(kKeyNow,BValue::Bool(true)))
						[kKeyNowResult].AsTime();
		}
		
		virtual status_t SnoozeUntil (bigtime_t when)
		{
			return Remote()->Invoke(BValue(kKeySnoozeUntil,BValue::Bool(true))
									.Overlay(BValue(kKeySnoozeUntilWhen,BValue::Time(when))))
									[kKeySnoozeUntilResult].AsInt32();
		}		
};

const BValue RTimeSource::kKeyNow("Now");
const BValue RTimeSource::kKeyNowResult("NowResult");

const BValue RTimeSource::kKeySnoozeUntil("SnoozeUntil");
const BValue RTimeSource::kKeySnoozeUntilWhen("SnoozeUntilWhen");
const BValue RTimeSource::kKeySnoozeUntilResult("SnoozeUntilResult");

B_IMPLEMENT_META_INTERFACE(TimeSource)

status_t 
LTimeSource::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	if (in[RTimeSource::kKeyNow].IsDefined())
		out += outBindings * (BValue(RTimeSource::kKeyNowResult,BValue::Time(Now())));

	if (in[RTimeSource::kKeySnoozeUntil].IsDefined())
	{
		out += outBindings * (BValue(RTimeSource::kKeySnoozeUntilResult,
				BValue::Int32(SnoozeUntil(in[RTimeSource::kKeySnoozeUntilWhen].AsTime()))));
	}
	
	return B_OK;
}

} } // B::Media2

