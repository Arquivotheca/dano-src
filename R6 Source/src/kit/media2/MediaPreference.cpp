
#include <media2/MediaPreference.h>

using B::Support2::BValue;

namespace B {
namespace Media2 {

BMediaPreference::BMediaPreference()
{
}

BMediaPreference::BMediaPreference(const BValue &value)
{
	mData = value;
}

BMediaPreference::~BMediaPreference()
{
}

status_t 
BMediaPreference::AddItem(const BValue &key, const BValue &value, float weight)
{
	BValue v = mData[key];
	v.Overlay(value, BValue::Float(weight));
	mData.Overlay(key, v);
	return B_OK;
}

status_t 
BMediaPreference::GetNextKey(void **cookie, BValue *outKey) const
{
	BValue key;
	status_t err = mData.GetNextItem(cookie, &key, 0);
	if (err < B_OK) return err;
	if (outKey) *outKey = key;
	return err;
}

status_t 
BMediaPreference::GetNextItem(
	const BValue &key, void **cookie, BValue *outValue, float *outWeight) const
{
	BValue v, w;
	status_t err = mData[key].GetNextItem(cookie, &v, &w);
	if (err < B_OK) return err;
	if (outValue) *outValue = v;
	if (outWeight) w.GetFloat(outWeight);
	return err;
}

status_t 
BMediaPreference::RemoveKey(const BValue &key)
{
	return mData.RemoveItem(value_ref(key));
}

status_t 
BMediaPreference::RemoveItem(const BValue &key, const BValue &value)
{
	BValue v = mData[key];
	if (!v) return B_BAD_INDEX;
	status_t err = v.RemoveItem(value_ref(value));
	if (err < B_OK) return err;
	mData.Overlay(key, v);
	return B_OK;
}

status_t 
BMediaPreference::Overlay(const BMediaPreference &prefs)
{
	mData.Overlay(prefs.mData);
	return B_OK;
}

void 
BMediaPreference::Undefine()
{
	mData.Undefine();
}

bool 
BMediaPreference::IsDefined() const
{
	return mData.IsDefined();
}

BValue 
BMediaPreference::AsValue() const
{
	return mData;
}

} } // B::Media2
