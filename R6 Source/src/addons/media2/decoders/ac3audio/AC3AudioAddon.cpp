#include "AC3AudioAddon.h"
#include "AC3Decoder.h"

#include <media2/MediaDefs.h>
#include <media2/MediaConstraint.h>

#include <string.h>
#include <malloc.h>

using namespace B::Media2;

CAC3AudioAddon::CAC3AudioAddon(image_id image)
	: BMediaAddon(image)
{
	memset(&mFlavor,0,sizeof(mFlavor));
	
	mFlavor.name=strdup("ac3audio.decoder");
	mFlavor.info=strdup("a node capable of decoding streamed ac3-data to raw audio");
	mFlavor.kinds=0;
	mFlavor.flavor_flags=0;
	mFlavor.internal_id=0;
	mFlavor.possible_count=0;

	mFlavor.in_formats=new BMediaConstraint(B_FORMATKEY_MEDIA_TYPE,BMediaConstraintItem::B_EQ,BValue::Int32(B::Media2::B_MEDIA_ENCODED_AUDIO));
	mFlavor.in_formats->And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));
	mFlavor.in_formats->And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	mFlavor.in_formats->And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));
	mFlavor.in_formats->And(B_FORMATKEY_ENCODING,BValue::String("be:ac3audio"));
	
	mFlavor.out_formats=new BMediaConstraint(B_FORMATKEY_MEDIA_TYPE,BMediaConstraintItem::B_EQ,BValue::Int32(B::Media2::B_MEDIA_RAW_AUDIO));

	mFlavor.out_formats->And(B_FORMATKEY_BUFFER_FRAMES,BMediaConstraintItem::B_EQ,BValue::Int32(6*256));
	mFlavor.out_formats->And(B_FORMATKEY_BYTE_ORDER,BMediaConstraintItem::B_EQ,BValue::Int32(B::Media2::B_MEDIA_HOST_ENDIAN));
	mFlavor.out_formats->And(B_FORMATKEY_RAW_AUDIO_TYPE,BMediaConstraintItem::B_EQ,BValue::Int32(B_AUDIO_INT16));
}

CAC3AudioAddon::~CAC3AudioAddon()
{
	delete mFlavor.out_formats;
	delete mFlavor.in_formats;
	free(mFlavor.info);
	free(mFlavor.name);
}

status_t 
CAC3AudioAddon::InitCheck(const char **out_failure_text)
{
	static const char *kOkText="everything is just fine.";
	
	*out_failure_text=kOkText;
	
	return B_OK;
}

int32 
CAC3AudioAddon::CountFlavors()
{
	return 1;
}

status_t 
CAC3AudioAddon::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	if (n>0)
		return B_ENTRY_NOT_FOUND;
		
	*out_info=&mFlavor;
	
	return B_OK;
}

BMediaNode *
CAC3AudioAddon::InstantiateNodeFor(const flavor_info *info, status_t *out_error)
{
	if (info!=&mFlavor)
	{
		*out_error=B_BAD_VALUE;
		return NULL;
	}
		
	*out_error=B_OK;
	
	return new CAC3Decoder;	
}

BMediaAddon *
make_media_addon (image_id image)
{
	return new CAC3AudioAddon(image);
}

