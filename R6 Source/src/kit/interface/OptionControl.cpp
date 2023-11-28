/*	OptionControl.cpp	*/

#include <OptionControl.h>
#include <string.h>

BOptionControl::BOptionControl(
	BRect frame,
	const char * name,
	const char * label, 
	BMessage * message,
	uint32 resize,
	uint32 flags) :
	BControl(frame, name, label, message, resize, flags)
{
}


BOptionControl::~BOptionControl()
{
}


void
BOptionControl::MessageReceived(
	BMessage * message)
{
	if (message->what == B_OPTION_CONTROL_VALUE) {
		int32 value = 0;
		if (!message->FindInt32("be:value", &value)) {
			SetValue(value);
			Invoke();
		}
	}
	else {
		BControl::MessageReceived(message);
	}
}

status_t
BOptionControl::AddOption(
	const char * name,
	int32 value)
{
	return AddOptionAt(name, value, CountOptions());
}


bool
BOptionControl::GetOptionAt(
	int32 index,
	const char ** out_name,
	int32 * out_value)
{
	index = index;
	out_name = out_name;
	out_value = out_value;
	return false;
}


void
BOptionControl::RemoveOptionAt(
	int32 index)
{
	index = index;
}

int32
BOptionControl::CountOptions() const
{
	return 0;
}

status_t
BOptionControl::AddOptionAt(
	const char * name,
	int32 value,
	int32 index)
{
	name = name;
	value = value;
	index = index;
	return B_ERROR;
}

int32
BOptionControl::SelectedOption(		//	index >= 0 returned directly
	const char ** name,
	int32 * value) const
{
	if (name) *name = 0;
	if (value) *value = 0;
	return 0;
}

status_t 
BOptionControl::SelectOptionFor(int32 target)
{
	int c = CountOptions();
	int32 value;
	const char * name;
	for (int ix=0; ix<c; ix++) {
		if (GetOptionAt(ix, &name, &value) && (target == value)) {
			SetValue(value);
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}

status_t
BOptionControl::SelectOptionFor(const char *target)
{
	int c = CountOptions();
	int32 value;
	const char * name;
	for (int ix=0; ix<c; ix++) {
		if (GetOptionAt(ix, &name, &value) && !strcmp(name, target)) {
			SetValue(value);
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}


BMessage *
BOptionControl::MakeValueMessage(
	int32 value)
{
	BMessage * msg = new BMessage(B_OPTION_CONTROL_VALUE);
	msg->AddInt32("be:value", value);
	return msg;
}


status_t
BOptionControl::_Reserved_OptionControl_0(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_1(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_2(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_3(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_4(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_5(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_6(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_7(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_8(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_9(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_10(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionControl::_Reserved_OptionControl_11(void *, ...)
{
	return B_ERROR;
}

