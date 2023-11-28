#include "MSADPCMDecoderAddon.h"
#include "MSADPCMDecoder.h"

#include <media2/MediaDefs.h>
#include <media2/MediaConstraint.h>

#include <string.h>
#include <malloc.h>

using namespace B::Media2;

CMSADPCMDecoderAddon::CMSADPCMDecoderAddon(image_id image)
	: BMediaAddon(image)
{
	memset(&mFlavor,0,sizeof(mFlavor));
	
	mFlavor.name=strdup("msadpcm.decoder");
	mFlavor.info=strdup("this node decodes msadpcm encoded audio and outputs raw audio");
	mFlavor.kinds=0;
	mFlavor.flavor_flags=0;
	mFlavor.internal_id=0;
	mFlavor.possible_count=0;

	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));

	c.And(B_FORMATKEY_INFO,BMediaConstraintItem::B_NE,BValue::undefined);

	BMediaConstraint c1(B_FORMATKEY_ENCODING,BValue::String("be:avi:msadpcm"));

	BMediaConstraint c2(B_FORMATKEY_ENCODING,BValue::String("be:wav:msadpcm"));
	c2.And(B_FORMATKEY_DECODED_BUFFER_SIZE,BMediaConstraintItem::B_GE,BValue::Int32(1));
	
	c1.Or(c2);
	c.And(c1);

	mFlavor.in_formats=new BMediaConstraint(c);

	c=BMediaConstraint(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_BYTE_ORDER,BValue::Int32(B_MEDIA_HOST_ENDIAN));

	c.And(B_FORMATKEY_RAW_AUDIO_TYPE,BValue()
										.Overlay(BValue::Int32(B_AUDIO_INT16))
										.Overlay(BValue::Int32(B_AUDIO_UINT8)));

	mFlavor.out_formats=new BMediaConstraint(c);
}

CMSADPCMDecoderAddon::~CMSADPCMDecoderAddon()
{
	delete mFlavor.out_formats;
	delete mFlavor.in_formats;
	free(mFlavor.info);
	free(mFlavor.name);
}

status_t 
CMSADPCMDecoderAddon::InitCheck(const char **out_failure_text)
{
	static const char *kOkText="everything is just fine.";
	
	*out_failure_text=kOkText;
	
	return B_OK;
}

int32 
CMSADPCMDecoderAddon::CountFlavors()
{
	return 1;
}

status_t 
CMSADPCMDecoderAddon::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	if (n>0)
		return B_ENTRY_NOT_FOUND;
		
	*out_info=&mFlavor;
	
	return B_OK;
}

BMediaNode *
CMSADPCMDecoderAddon::InstantiateNodeFor(const flavor_info *info, status_t *out_error)
{
	if (info!=&mFlavor)
	{
		*out_error=B_BAD_VALUE;
		return NULL;
	}
		
	*out_error=B_OK;
	
	return new B::Private::CMSADPCMDecoder;	
}

BMediaAddon *
make_media_addon (image_id image)
{
	return new CMSADPCMDecoderAddon(image);
}

