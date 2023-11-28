#include "CCITTDecoderAddon.h"
#include "CCITTDecoder.h"

#include <media2/MediaDefs.h>
#include <media2/MediaConstraint.h>

#include <string.h>
#include <malloc.h>

using namespace B::Media2;

CCCITTADPCMDecoderAddon::CCCITTADPCMDecoderAddon(image_id image)
	: BMediaAddon(image)
{
	memset(&mFlavor,0,sizeof(mFlavor));
	
	mFlavor.name=strdup("ccittadpcm.decoder");
	mFlavor.info=strdup("this node decodes ccittadpcm encoded audio and outputs raw audio");
	mFlavor.kinds=0;
	mFlavor.flavor_flags=0;
	mFlavor.internal_id=0;
	mFlavor.possible_count=0;

	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));

	c.And(B_FORMATKEY_ENCODING,BValue()
							.Overlay(BValue::String("be:g721_4"))
							.Overlay(BValue::String("be:g723_3"))
							.Overlay(BValue::String("be:g723_5")));

	mFlavor.in_formats=new BMediaConstraint(c);

	c=BMediaConstraint(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));

	c.And(B_FORMATKEY_BYTE_ORDER,BValue::Int32(B_MEDIA_HOST_ENDIAN));
	c.And(B_FORMATKEY_RAW_AUDIO_TYPE,BValue::Int32(B_AUDIO_INT16));

	mFlavor.out_formats=new BMediaConstraint(c);
}

CCCITTADPCMDecoderAddon::~CCCITTADPCMDecoderAddon()
{
	delete mFlavor.out_formats;
	delete mFlavor.in_formats;
	free(mFlavor.info);
	free(mFlavor.name);
}

status_t 
CCCITTADPCMDecoderAddon::InitCheck(const char **out_failure_text)
{
	static const char *kOkText="everything is just fine.";
	
	*out_failure_text=kOkText;
	
	return B_OK;
}

int32 
CCCITTADPCMDecoderAddon::CountFlavors()
{
	return 1;
}

status_t 
CCCITTADPCMDecoderAddon::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	if (n>0)
		return B_ENTRY_NOT_FOUND;
		
	*out_info=&mFlavor;
	
	return B_OK;
}

BMediaNode *
CCCITTADPCMDecoderAddon::InstantiateNodeFor(const flavor_info *info, status_t *out_error)
{
	if (info!=&mFlavor)
	{
		*out_error=B_BAD_VALUE;
		return NULL;
	}
		
	*out_error=B_OK;
	
	return new B::Private::CCITTDecoder;	
}

BMediaAddon *
make_media_addon (image_id image)
{
	return new CCCITTADPCMDecoderAddon(image);
}

