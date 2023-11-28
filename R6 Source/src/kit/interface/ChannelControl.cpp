
#include <ChannelControl.h>
#include <PropertyInfo.h>
#include <archive_defs.h>

#include <stdlib.h>
#include <string.h>

#include <map>

namespace BPrivate {
	struct _labels {
		char * minLab;
		char * maxLab;
		char * copy(const char * other) {
			if (other == 0) return 0;
			return strdup(other);
		}
		_labels() {
			minLab = maxLab = 0;
		}
		_labels(const char * a, const char * b) {
			minLab = copy(a);
			maxLab = copy(b);
		}
		_labels(const _labels & other) {
			minLab = copy(other.minLab);
			maxLab = copy(other.maxLab);
		}
		~_labels() {
			free(minLab);
			free(maxLab);
		}
		_labels & operator=(const _labels & other)
		{
			free(minLab);
			free(maxLab);
			minLab = copy(other.minLab);
			maxLab = copy(other.maxLab);
			return *this;
		}
	};
}
using namespace BPrivate;

typedef std::map<int32, _labels> label_map;

/*-------------------------------------------------------------*/
	/*
	 BChannelControl supports the following:
	 	GET/SET		"ChannelCount"	DIRECT form only
	 	GET/SET		"CurrentChannel"	DIRECT form only
	 	GET/SET		"MaxLimitLabel"	DIRECT form only
	 	GET/SET		"MinLimitLabel"	DIRECT form only
	*/

static property_info	prop_list[] = {
	{"ChannelCount",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{B_INT32_TYPE},
		{},
		{}
	},
	{"CurrentChannel",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{B_INT32_TYPE},
		{},
		{}
	},
	{"MaxLimitLabel",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{B_STRING_TYPE},
		{},
		{}
	},
	{"MinLimitLabel",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{B_STRING_TYPE},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};

enum {
	kChannelCount = 0,
	kCurrentChannel,
	kMaxLimitLabel,
	kMinLimitLabel
};

/*-------------------------------------------------------------*/


BChannelControl::BChannelControl(
	BRect frame,
	const char * name,
	const char * label,
	BMessage * model,
	int32 channel_count,
	uint32 resize,
	uint32 flags) :
	BControl(frame, name, label, model, resize, flags)
{
	_m_channel_count = channel_count;
	_m_value_channel = 0;
	_m_channel_min = new int32[_m_channel_count];
	_m_channel_max = new int32[_m_channel_count];
	_m_channel_val = new int32[_m_channel_count];
	for (int ix=0; ix<_m_channel_count; ix++) {
		_m_channel_min[ix] = 0;
		_m_channel_max[ix] = 100;
		_m_channel_val[ix] = 0;
	}
	_m_multi_labels = 0;
	fModificationMsg = 0;
}

BChannelControl::BChannelControl(
	BMessage * from) :
	BControl(from)
{
	if (from->FindInt32("be:_m_channel_count", &_m_channel_count)) {
		_m_channel_count = 1;
	}
	if (_m_channel_count < 1) {
		_m_channel_count = 1;
	}
	if (from->FindInt32("be:_m_value_channel", &_m_value_channel)) {
		_m_value_channel = 0;
	}
	if (_m_value_channel < 0 || _m_value_channel >= _m_channel_count) {
		_m_value_channel = 0;
	}
	const char* str;
	if (from->FindString("be:_m_min_label", &str)) {
		_m_min_label = "";
	}
	else {
		_m_min_label = str;
	}
	if (from->FindString("be:_m_max_label", &str)) {
		_m_max_label = "";
	}
	else {
		_m_max_label = str;
	}
	BMessage* msg = new BMessage;
	if (from->FindMessage(S_MOD_MESSAGE, msg)) {
		fModificationMsg = NULL;
		delete msg;
	} else {
		fModificationMsg = msg;
	}
	_m_channel_min = new int32[_m_channel_count];
	for (int ix=0; ix<_m_channel_count; ix++) {
		if (from->FindInt32("be:_m_channel_min", ix, &_m_channel_min[ix])) {
			_m_channel_min[ix] = 0;
		}
	}
	_m_channel_max = new int32[_m_channel_count];
	for (int ix=0; ix<_m_channel_count; ix++) {
		if (from->FindInt32("be:_m_channel_max", ix, &_m_channel_max[ix])) {
			_m_channel_max[ix] = 100;
		}
	}
	_m_channel_val = new int32[_m_channel_count];
	for (int ix=0; ix<_m_channel_count; ix++) {
		if (from->FindInt32("be:_m_channel_val", ix, &_m_channel_val[ix])) {
			_m_channel_val[ix] = _m_channel_min[ix];
		}
	}
}

BChannelControl::~BChannelControl()
{
	delete[] _m_channel_val;
	delete[] _m_channel_min;
	delete[] _m_channel_max;
	delete (label_map *)_m_multi_labels;
}

status_t BChannelControl::Archive(
	BMessage * into,
	bool deep) const
{
	status_t err = BControl::Archive(into, deep);
	if (err == B_OK) {
		into->AddInt32("be:_m_channel_count", _m_channel_count);
		into->AddInt32("be:_m_value_channel", _m_value_channel);
		if (_m_min_label.Length() > 0) {
			into->AddString("be:_m_min_label", _m_min_label.String());
		}
		if (_m_max_label.Length() > 0) {
			into->AddString("be:_m_max_label", _m_max_label.String());
		}
		for (int ix=0; ix<_m_channel_count; ix++) {
			into->AddInt32("be:_m_channel_min", _m_channel_min[ix]);
			into->AddInt32("be:_m_channel_max", _m_channel_max[ix]);
			into->AddInt32("be:_m_channel_val", _m_channel_val[ix]);
		}
	}
	return err;
}


void BChannelControl::Draw(
	BRect area)
{
	BControl::Draw(area);
}

void BChannelControl::MouseDown(
	BPoint where)
{
	BControl::MouseDown(where);
}

void BChannelControl::KeyDown(
	const char * bytes,
	int32 size)
{
	BControl::KeyDown(bytes, size);
}


void BChannelControl::FrameResized(
	float width,
	float height)
{
	BControl::FrameResized(width, height);
}

void BChannelControl::SetFont(
	const BFont * font,
	uint32 mask)
{
	BControl::SetFont(font, mask);
}


void BChannelControl::AttachedToWindow()
{
	BControl::AttachedToWindow();
}

void BChannelControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}

void BChannelControl::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void BChannelControl::GetPreferredSize(
	float * width,
	float * height)
{
	BControl::GetPreferredSize(width, height);
}

void BChannelControl::MessageReceived(
	BMessage * msg)
{
	bool handled = false;
	BMessage	reply(B_REPLY);
	status_t	err;
	
	switch(msg->what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BMessage	specifier;
			int32		form;
			const char	*prop;
			int32		cur;
			int32		i;
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if( !err ) {
				BPropertyInfo	pi(prop_list);
				i = pi.FindMatch(msg, 0, &specifier, form, prop);
			}
			if( err ) break;
			switch (i) {
				case kChannelCount: {
					if (msg->what == B_GET_PROPERTY) {
						reply.AddInt32("result", CountChannels());
						handled = true;
					} else {
						int32 c;
						err = msg->FindInt32("data", &c);
						if (!err) {
							SetChannelCount(c);
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				} break;
				case kCurrentChannel: {
					if (msg->what == B_GET_PROPERTY) {
						reply.AddInt32("result", CurrentChannel());
						handled = true;
					} else {
						int32 c;
						err = msg->FindInt32("data", &c);
						if (!err) {
							SetCurrentChannel(c);
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				} break;
				case kMaxLimitLabel: {
					if (msg->what == B_GET_PROPERTY) {
						reply.AddString("result", MaxLimitLabel());
						handled = true;
					} else {
						const char* v;
						err = msg->FindString("data", &v);
						if (!err) {
							SetLimitLabels(MinLimitLabel(), v);
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				} break;
				case kMinLimitLabel: {
					if (msg->what == B_GET_PROPERTY) {
						reply.AddString("result", MinLimitLabel());
						handled = true;
					} else {
						const char* v;
						err = msg->FindString("data", &v);
						if (!err) {
							SetLimitLabels(v, MaxLimitLabel());
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				} break;
			}
		}
	}
	
	if (handled)
		msg->SendReply(&reply);
	else
		BControl::MessageReceived(msg);
}

BHandler*
BChannelControl::ResolveSpecifier(BMessage *msg,
								 int32 index,
								 BMessage *specifier,
								 int32 form,
								 const char *property)
{
	BHandler	*target = NULL;
	BPropertyInfo	pi(prop_list);
	int32			i;

	if ((i = pi.FindMatch(msg, index, specifier, form, property)) >= 0) {
		target = this;
	} else {
		target = BControl::ResolveSpecifier(msg, index, specifier, form, property);
	}

	return target;
}

status_t
BChannelControl::GetSupportedSuites(BMessage *data)
{
	data->AddString("suites", "suite/vnd.Be-channel-control");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return BControl::GetSupportedSuites(data);
}


void
BChannelControl::SetModificationMessage(BMessage *message)
{
	if (fModificationMsg)
		delete fModificationMsg;
	
	fModificationMsg = message;
}

BMessage*
BChannelControl::ModificationMessage() const
{
	return fModificationMsg;
}

status_t
BChannelControl::Invoke(BMessage* msg)
{
	bool notify = false;
	BMessage clone(InvokeKind(&notify));
	if( msg ) clone = *msg;
	else if( Message() && !notify ) clone = *Message();
	
	// Add custom BChannelControl information.
	clone.AddInt32("be:current_channel", _m_value_channel);
	
	// Note: doing things this way means a message will be sent to the
	// target even if none is supplied!  The only problem here is if
	// Message() is NULL, and Messenger() is valid... but still...
	return BControl::Invoke(&clone);
}

status_t
BChannelControl::InvokeChannel(BMessage* msg,
							   int32 from_channel, int32 channel_count,
							   const bool* in_mask)
{
	bool notify = false;
	BMessage clone(InvokeKind(&notify));
	if( msg ) clone = *msg;
	else if( Message() && !notify ) clone = *Message();
	
	// Add custom BChannelControl information.
	clone.AddInt32("be:current_channel", _m_value_channel);
	if( channel_count < 0 && !in_mask ) channel_count = _m_channel_count;
	for (int ix=0; ix<_m_channel_count; ix++) {
		clone.AddInt32("be:channel_value", _m_channel_val[ix]);
		clone.AddBool("be:channel_changed",
						(ix >= from_channel && ix < (from_channel+channel_count))
							? ( in_mask ? in_mask[ix] : true ) : false );
	}
	
	// Note: doing things this way means a message will be sent to the
	// target even if none is supplied!  The only problem here is if
	// Message() is NULL, and Messenger() is valid... but still...
	return BControl::Invoke(&clone);
}

status_t
BChannelControl::InvokeNotifyChannel(BMessage* msg,
							   uint32 kind,
							   int32 from_channel, int32 channel_count,
							   const bool* in_mask)
{
	BeginInvokeNotify(kind);
	status_t err = InvokeChannel(msg, from_channel, channel_count, in_mask);
	EndInvokeNotify();
	return err;
}

void BChannelControl::SetValue(		/* SetCurrentChannel() determines which channel */
	int32 value)
{
	if (value > _m_channel_max[_m_value_channel]) {
		value = _m_channel_max[_m_value_channel];
	}
	if (value < _m_channel_min[_m_value_channel]) {
		value = _m_channel_min[_m_value_channel];
	}
	if (value != _m_channel_val[_m_value_channel]) {
		(void)StuffValues(_m_value_channel, 1, &value);
		BControl::SetValue(value);
	}
}

status_t BChannelControl::SetCurrentChannel(
	int32 channel)
{
	if (channel < 0) {
		return B_BAD_VALUE;
	}
	if (channel >= _m_channel_count) {
		return B_BAD_VALUE;
	}
	if (_m_value_channel != channel) {
		_m_value_channel = channel;
		BControl::SetValue(_m_channel_val[_m_value_channel]);
	}
	return B_OK;
}

int32 BChannelControl::CurrentChannel() const
{
	return _m_value_channel;
}

int32 BChannelControl::CountChannels() const
{
	return _m_channel_count;
}

int32 BChannelControl::MaxChannelCount() const
{
	return 32;
}

status_t BChannelControl::SetChannelCount(
	int32 channel_count)
{
	if (channel_count < 1 || channel_count > MaxChannelCount()) {
		return B_BAD_VALUE;
	}
	if (_m_channel_count < channel_count) {
		int32 * new_val = new int32[channel_count];
		memcpy(new_val, _m_channel_val, sizeof(int32)*_m_channel_count);
		int32 * new_min = new int32[channel_count];
		memcpy(new_min, _m_channel_min, sizeof(int32)*_m_channel_count);
		int32 * new_max = new int32[channel_count];
		memcpy(new_max, _m_channel_max, sizeof(int32)*_m_channel_count);
		delete[] _m_channel_val;
		delete[] _m_channel_min;
		delete[] _m_channel_max;
		_m_channel_val = new_val;
		_m_channel_min = new_min;
		_m_channel_max = new_max;
		for (int ix=_m_channel_count; ix<channel_count; ix++) {
			_m_channel_val[ix] = _m_channel_val[_m_value_channel];
			_m_channel_min[ix] = _m_channel_min[_m_value_channel];
			_m_channel_max[ix] = _m_channel_max[_m_value_channel];
		}
	}
	_m_channel_count = channel_count;
	return B_OK;
}

int32 BChannelControl::ValueFor(
	int32 channel) const
{
	int32 value = 0;
	status_t err;
	err = GetValue(&value, channel, 1);
	if (err < B_OK) return err;
	return value;
}

int32 BChannelControl::GetValue(
	int32 * out_values,
	int32 from_channel,
	int32 channel_count) const
{
	int32 ok = 0;
	int32 c = _m_channel_count - from_channel;
	if (c > channel_count) c = channel_count;
	while (c > 0) {
		out_values[ok] = _m_channel_val[from_channel+ok];
		ok++;
		c--;
	}
	if (ok > 0) {
		return ok;
	}
	return B_BAD_VALUE;
}

status_t BChannelControl::SetValueFor(
	int32 channel,
	int32 value)
{
	return SetValue(channel, 1, &value);
}

status_t BChannelControl::SetValue(
	int32 from_channel,
	int32 channel_count,
	const int32 * in_values)
{
	return StuffValues(from_channel, channel_count, in_values);
}

status_t BChannelControl::StuffValues(
	int32 from_channel,
	int32 channel_count,
	const int32 * in_values)
{
	int32 ok = 0;
	int32 c = _m_channel_count - from_channel;
	if (c > channel_count) c = channel_count;
	while (c > 0) {
		int32 val = in_values[ok];
		if (val > _m_channel_max[from_channel+ok]) {
			val = _m_channel_max[from_channel+ok];
		}
		if (val < _m_channel_min[from_channel+ok]) {
			val = _m_channel_min[from_channel+ok];
		}
		if (_m_channel_val[from_channel+ok] != val) {
			_m_channel_val[from_channel+ok] = val;
		}
		ok++;
		c--;
	}
	if (ok > 0) {
		if (from_channel <= _m_value_channel && from_channel + ok > _m_value_channel) {
			BControl::SetValue(_m_channel_val[_m_value_channel]);
		}
		return ok;
	}
	return B_BAD_VALUE;
}

status_t BChannelControl::SetAllValue(
	int32 values)
{
	int32 * ptr = new int32[_m_channel_count];
	for (int ix=0; ix<_m_channel_count; ix++) {
		int32 val = values;
		if (val > _m_channel_max[ix]) {
			val = _m_channel_max[ix];
		}
		if (val < _m_channel_min[ix]) {
			val = _m_channel_min[ix];
		}
		ptr[ix] = val;
	}
	status_t err = SetValue(0, _m_channel_count, ptr);
	delete[] ptr;
	return err;
}

status_t BChannelControl::SetLimitsFor(
	int32 channel,
	int32 minimum,
	int32 maximum)
{
	return SetLimitsFor(channel, 1, &minimum, &maximum);
}

status_t BChannelControl::GetLimitsFor(
	int32 channel,
	int32 * minimum,
	int32 * maximum)const 
{
	return GetLimitsFor(channel, 1, minimum, maximum);
}

status_t BChannelControl::SetLimitsFor(
	int32 from_channel,
	int32 channel_count,
	const int32 * minimum,
	const int32 * maximum)
{
	int32 ok = 0;
	int32 c = _m_channel_count - from_channel;
	if (c > channel_count) {
		c = channel_count;
	}
	bool change = false;
	while (c > 0) {
		_m_channel_min[from_channel+ok] = minimum[ok];
		_m_channel_max[from_channel+ok] = maximum[ok];
		if (_m_channel_val[from_channel+ok] < minimum[ok]) {
			change = true;
		}
		if (_m_channel_val[from_channel+ok] > maximum[ok]) {
			change = true;
		}
		c--;
		ok++;
	}
	if (ok > 0) {
		if (change) {	/* limits make some current values illegal */
			int32 * copy = new int32[ok];
			for (int ix=0; ix<ok; ix++) {
				copy[ix] = _m_channel_val[from_channel+ix];
			}
			status_t err = SetValue(from_channel, ok, copy);
			delete[] copy;
			if (err < B_OK) {
				ok = err;
			}
		}
		return ok;
	}
	return B_BAD_VALUE;
}

status_t BChannelControl::GetLimitsFor(
	int32 from_channel,
	int32 channel_count,
	int32 * minimum,
	int32 * maximum) const
{
	int32 ok = 0;
	int32 c = _m_channel_count - from_channel;
	if (c > channel_count) c = channel_count;
	while (c > 0) {
		minimum[ok] = _m_channel_min[from_channel+ok];
		maximum[ok] = _m_channel_max[from_channel+ok];
		ok++;
		c--;
	}
	if (ok > 0) {
		return ok;
	}
	return B_BAD_VALUE;
}


status_t BChannelControl::SetLimits(
	int32 minimum,
	int32 maximum)
{
	int cc = CountChannels();
	int32 * mins = new int32[cc];
	int32 * maxs = new int32[cc];

	for (int ix=0; ix<cc; ix++)
	{
		mins[ix] = minimum;
		maxs[ix] = maximum;
	}
	SetLimitsFor(0, cc, mins, maxs);
	delete mins;
	delete maxs;

	return B_OK;
}

bool
BChannelControl::SupportsIndividualLimits() const
{
	return false;
}

status_t
BChannelControl::GetLimits(
	int32 * outMin,
	int32 * outMax) const
{
	status_t err = GetLimitsFor(0, outMin, outMax);
	if (err < B_OK) return err;
	int c = CountChannels();
	for (int ix=1; ix<c; ix++) {
		int32 a, b;
		err = GetLimitsFor(ix, &a, &b);
		if (err == B_OK) {
			if (a < *outMin) *outMin = a;
			if (b > *outMax) *outMax = b;
		}
	}
	return B_OK;
}


status_t BChannelControl::SetLimitLabels(
	const char * min_label,
	const char * max_label)
{
	bool inval = false;
	status_t err = B_OK;
	if (_m_min_label != min_label)
	{
		_m_min_label = min_label;
		inval = true;
	}
	if (_m_max_label != max_label)
	{
		_m_max_label = max_label;
		inval = true;
	}
	if (inval)
	{
		Invalidate();
	}
	return err;
}


const char * BChannelControl::MinLimitLabel() const
{
	return _m_min_label.String();
}

const char * BChannelControl::MaxLimitLabel() const
{
	return _m_max_label.String();
}

status_t 
BChannelControl::SetLimitLabelsFor(int32 channel, const char *minLabel, const char *maxLabel)
{
	return SetLimitLabelsFor(channel, 1, minLabel, maxLabel);
}

#define LABS (*(label_map *)_m_multi_labels)

status_t 
BChannelControl::SetLimitLabelsFor(int32 from_channel, int32 channel_count, const char *minLabel, const char *maxLabel)
{
	if (from_channel + channel_count > CountChannels()) return B_BAD_INDEX;
	if (!_m_multi_labels) {
		LABS = *new label_map;
	}
	while (channel_count-- > 0) {
		_labels x(minLabel, maxLabel);
		LABS[from_channel] = x;
		from_channel++;
	}
	return B_OK;
}

const char *
BChannelControl::MinLimitLabelFor(int32 channel) const
{
	if (!_m_multi_labels) return 0;
	label_map::iterator ptr(LABS.find(channel));
	if (ptr == LABS.end()) return 0;
	return (*ptr).second.minLab;
}

const char *
BChannelControl::MaxLimitLabelFor(int32 channel) const
{
	if (!_m_multi_labels) return 0;
	label_map::iterator ptr(LABS.find(channel));
	if (ptr == LABS.end()) return 0;
	return (*ptr).second.maxLab;
}

#undef LABS


status_t BChannelControl::_Reserverd_ChannelControl_0(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_1(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_2(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_3(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_4(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_5(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_6(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_7(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_8(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_9(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_10(void *, ...)
{
	return B_ERROR;
}

status_t BChannelControl::_Reserverd_ChannelControl_11(void *, ...)
{
	return B_ERROR;
}

