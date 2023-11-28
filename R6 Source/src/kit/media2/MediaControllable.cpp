
#include <media2/MediaControllable.h>

#include "shared_properties.h"

#include <support2/Debug.h>
#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Media2 {

using namespace Private;

/**************************************************************************************/

BValue 
IMediaControllable::operator[](const BValue &key) const
{
	return Control(key);
}

BValue 
IMediaControllable::AllControls() const
{
	return Control(BValue::null);
}

status_t 
IMediaControllable::SetAllControls(const BValue &map)
{
	return SetControl(BValue::null, map);
}

status_t 
IMediaControllable::SetInt32(const BValue &key, int32 value)
{
	return SetControl(key, BValue::Int32(value));
}

status_t 
IMediaControllable::SetInt64(const BValue &key, int64 value)
{
	return SetControl(key, BValue::Int64(value));
}

status_t 
IMediaControllable::SetDouble(const BValue &key, double value)
{
	return SetControl(key, BValue::Double(value));
}

status_t 
IMediaControllable::SetString(const BValue &key, const char *value)
{
	return SetControl(key, BValue::String(value));
}

status_t 
IMediaControllable::SetString(const BValue &key, const BString &value)
{
	return SetControl(key, BValue::String(value));
}

int32 
IMediaControllable::AsInt32(const BValue &key, int32 def) const
{
	BValue value = Control(key);
	value.GetInt32(&def);
	return def;
}

int64 
IMediaControllable::AsInt64(const BValue &key, int64 def) const
{
	BValue value = Control(key);
	value.GetInt64(&def);
	return def;
}

double 
IMediaControllable::AsDouble(const BValue &key, double def) const
{
	BValue value = Control(key);
	value.GetDouble(&def);
	return def;
}

BString 
IMediaControllable::AsString(const BValue &key, const char *def) const
{
	BString out(def);
	BValue value = Control(key);
	value.GetString(&out);
	return out;
}

/**************************************************************************************/

class RMediaControllable : public RInterface<IMediaControllable>
{
public:
									RMediaControllable(const IBinder::ptr & binder) : RInterface<IMediaControllable>(binder) {}
	virtual	BValue					Control(const BValue & key) const;
	virtual	status_t				SetControl(const BValue & key, const BValue & data);
	virtual	BValue					ControlInfo(const BValue & key) const;
};

B_IMPLEMENT_META_INTERFACE(MediaControllable)

BValue 
RMediaControllable::Control(const BValue & key) const
{
	BValue out;
	Remote()->Effect(
		BValue(PMETHOD_CONTROL, BValue(PARG_KEY, key)),
		BValue::null,
		PMETHOD_CONTROL,
		&out);
	return out;
}

status_t 
RMediaControllable::SetControl(const BValue & key, const BValue & value)
{
	BValue request;
	request.Overlay(PARG_KEY, key);
	request.Overlay(PARG_VALUE, value);
	BValue out;
	Remote()->Effect(
		BValue(PMETHOD_SET_CONTROL, request),
		BValue::null,
		PMETHOD_SET_CONTROL,
		&out);
	return out.AsInt32();
}

BValue 
RMediaControllable::ControlInfo(const BValue & key) const
{
	BValue out;
	Remote()->Effect(
		BValue(PMETHOD_CONTROL_INFO, BValue(PARG_KEY, key)),
		BValue::null,
		PMETHOD_CONTROL_INFO,
		&out);
	return out;
}


/**************************************************************************************/

status_t 
LMediaControllable::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	BValue v;
	if (v = in[PMETHOD_SET_CONTROL])
	{
		status_t err = SetControl(v[PARG_KEY], v[PARG_VALUE]);
		out += outBindings * BValue(PMETHOD_SET_CONTROL, BValue::Int32(err));
	}
	if (v = in[PMETHOD_CONTROL])
	{
		out += outBindings * BValue(PMETHOD_CONTROL, Control(v[PARG_KEY]));
	}
	if (v = in[PMETHOD_CONTROL_INFO])
	{
		out += outBindings * BValue(PMETHOD_CONTROL_INFO, ControlInfo(v[PARG_KEY]));
	}
	return B_OK;
}

/**************************************************************************************/

BMediaControllable::BMediaControllable()
{
}

BMediaControllable::~BMediaControllable()
{
}

BValue
BMediaControllable::Control(const BValue & key) const
{
	BValue v = mControls.ValueFor(key);
	return (v.IsSpecified()) ? v : BValue::undefined;
}

status_t
BMediaControllable::SetControl(const BValue & key, const BValue & value)
{
	BValue v = mControls.ValueFor(key);
	if (!v.IsSpecified()) return B_BAD_INDEX;
	if (v.Type() != value.Type()) return B_BAD_TYPE;
	mControls.RemoveItem(value_ref(key), value_ref(v));
	mControls.Overlay(key, value);
	return B_OK;
}

BValue
BMediaControllable::ControlInfo(const BValue & key) const
{
	return mInfo.ValueFor(key);
}

status_t
BMediaControllable::AddControl(const BValue & key, const BValue & def, const BValue & info)
{
	if (mControls.HasItem(value_ref(key))) return B_NOT_ALLOWED;
	ASSERT(!mInfo.HasItem(value_ref(key)));
	mControls.Overlay(key, def);
	mInfo.Overlay(key, info);
	return B_OK;
}

status_t
BMediaControllable::AddControl(const BValue & key, const BValue & def)
{
	return AddControl(key, def, BValue::null);
}

status_t
BMediaControllable::SetControlInfo(const BValue & key, const BValue & info)
{
	if (!mControls.HasItem(value_ref(key))) return B_BAD_INDEX;
	mInfo.RemoveItem(value_ref(key));
	mInfo.Overlay(key, info);
	return B_OK;
}

status_t
BMediaControllable::RemoveControl(const BValue & key)
{
	status_t err = mControls.RemoveItem(value_ref(key));
	if (err < B_OK) return err;
	mInfo.RemoveItem(value_ref(key));
	return B_OK;
}

status_t
BMediaControllable::RemoveAllControls()
{
	mControls.Undefine();
	mInfo.Undefine();
	return B_OK;
}

status_t
BMediaControllable::Acquired(const void* id)
{
	return LMediaControllable::Acquired(id);
}

status_t
BMediaControllable::Released(const void* id)
{
	return LMediaControllable::Released(id);
}

} } // B::Media2
