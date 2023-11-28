#include "SoundPlayerNode.h"

using namespace B::Media2;

namespace B {
namespace Private {

BSoundPlayerNode::BSoundPlayerNode(const char *name)
	: BMediaNode(name)
{
}

status_t 
BSoundPlayerNode::Acquired(const void *id)
{
	status_t result=BMediaNode::Acquired(id);
	
	if (result<B_OK)
		return result;
		
	mOutput=new BMediaOutput("audio_out");
	
	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));

	c.And(B_FORMATKEY_BYTE_ORDER,BValue()
				.Overlay(BValue::Int32(B_MEDIA_BIG_ENDIAN))
				.Overlay(BValue::Int32(B_MEDIA_LITTLE_ENDIAN)));

	c.And(B_FORMATKEY_RAW_AUDIO_TYPE,BValue()
				.Overlay(BValue::Int32(B_AUDIO_INT8))
				.Overlay(BValue::Int32(B_AUDIO_UINT8))
				.Overlay(BValue::Int32(B_AUDIO_INT16))
				.Overlay(BValue::Int32(B_AUDIO_INT32))
				.Overlay(BValue::Int32(B_AUDIO_FLOAT)));				

	mOutput->SetConstraint(c);
	AddEndpoint(mOutput);
}

status_t 
BSoundPlayerNode::Released(const void *id)
{
	return BMediaNode::Released(id);
}

BMediaOutput::ptr
BSoundPlayerNode::Output() const
{
	return mOutput;
}

BMediaFormat
BSoundPlayerNode::Format() const
{
	return mOutput->Format();
}

void 
BSoundPlayerNode::DisconnectSelf()
{
	mOutput->Disconnect();
}

} } // B::Private
