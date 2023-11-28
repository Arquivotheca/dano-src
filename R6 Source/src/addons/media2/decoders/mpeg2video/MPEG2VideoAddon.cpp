#include "MPEG2VideoAddon.h"
#include "MPEG2VideoDecoder.h"

#include <media2/MediaDefs.h>
#include <media2/MediaConstraint.h>

#include <string.h>
#include <malloc.h>

using namespace B::Media2;

CMPEG2VideoAddon::CMPEG2VideoAddon(image_id image)
	: BMediaAddon(image)
{
	memset(&mFlavor,0,sizeof(mFlavor));
	
	mFlavor.name=strdup("mpeg2video.decoder");
	mFlavor.info=strdup("a node capable of decoding streamed mpeg2-data to planar yuv");
	mFlavor.kinds=0;
	mFlavor.flavor_flags=0;
	mFlavor.internal_id=0;
	mFlavor.possible_count=0;

	BMediaConstraint c(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_MULTIPLE_OF,BValue::Int32(16));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_MULTIPLE_OF,BValue::Int32(16));
	c.And(B_FORMATKEY_MEDIA_TYPE,BMediaConstraintItem::B_EQ,BValue::Int32(B::Media2::B_MEDIA_ENCODED_VIDEO));

	c.And(B_FORMATKEY_ENCODING,BValue()
								.Overlay(BValue::String("mpeg1"))
								.Overlay(BValue::String("mpeg2")));

	mFlavor.in_formats=new BMediaConstraint(c);
	
	c=BMediaConstraint(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_COLORSPACE,BValue::Int32(::B_YUV9));
	c.And(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B::Media2::B_MEDIA_RAW_VIDEO));

	mFlavor.out_formats=new BMediaConstraint(c);
}

CMPEG2VideoAddon::~CMPEG2VideoAddon()
{
	delete mFlavor.out_formats;
	delete mFlavor.in_formats;
	free(mFlavor.info);
	free(mFlavor.name);
}

status_t 
CMPEG2VideoAddon::InitCheck(const char **out_failure_text)
{
	static const char *kOkText="everything is just fine.";
	
	*out_failure_text=kOkText;
	
	return B_OK;
}

int32 
CMPEG2VideoAddon::CountFlavors()
{
	return 1;
}

status_t 
CMPEG2VideoAddon::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	if (n>0)
		return B_ENTRY_NOT_FOUND;
		
	*out_info=&mFlavor;
	
	return B_OK;
}

BMediaNode *
CMPEG2VideoAddon::InstantiateNodeFor(const flavor_info *info, status_t *out_error)
{
	if (info!=&mFlavor)
	{
		*out_error=B_BAD_VALUE;
		return NULL;
	}
		
	*out_error=B_OK;
	
	return new CMPEG2VideoDecoder;	
}

BMediaAddon *
make_media_addon (image_id image)
{
	return new CMPEG2VideoAddon(image);
}

