#ifndef _MEDIA2_MEDIACONTROLLABLE_INTERFACE_
#define _MEDIA2_MEDIACONTROLLABLE_INTERFACE_

#include <support2/Binder.h>
#include <support2/String.h>
#include <support2/Value.h>

#include <media2/MediaDefs.h>

namespace B {
namespace Media2 {

using namespace Support2;

class IMediaControllable : public IInterface
{
public:
	B_DECLARE_META_INTERFACE(MediaControllable)
	
	// to access the whole set, pass BValue::null as key

	virtual	BValue					Control(const BValue & key) const = 0;
			BValue					operator[](const BValue & key) const;

	// ** should fail if no value currently exists, or if the data type passed is
	//    incompatible with that of the current value.
	virtual	status_t				SetControl(const BValue & key, const BValue & value) = 0;
	virtual	BValue					ControlInfo(const BValue & key) const = 0;

	// convenience stuff
	
			BValue					AllControls() const;
			status_t				SetAllControls(const BValue & map);

	// BValue-style access for commonly-used types
			
			status_t				SetInt32(const BValue & key, int32 value);
			status_t				SetInt64(const BValue & key, int64 value);
			status_t				SetDouble(const BValue & key, double value);
			status_t				SetString(const BValue & key, const char * value);
			status_t				SetString(const BValue & key, const BString & value);

	// +++ should these convert implicitly to the given type if possible?

			int32					AsInt32(const BValue & key, int32 def = 0) const;
			int64					AsInt64(const BValue & key, int64 def = 0) const;
			double					AsDouble(const BValue & key, double def = 0) const;
			BString					AsString(const BValue & key, const char * def = "") const;

	// +++ observation? (or will the new BBinder have enough observation built in?)
};

} } // B::Media2
#endif //_MEDIA2_MEDIACONTROLLABLE_INTERFACE_
