#include "Indeo5Addon.h"
#include "Indeo5.h"

#include <media2/MediaDefs.h>
#include <media2/MediaConstraint.h>

#include <string.h>
#include <malloc.h>

using namespace B::Media2;

CIndeo5DecoderAddon::CIndeo5DecoderAddon(image_id image)
	: BMediaAddon(image)
{
	memset(&mFlavor,0,sizeof(mFlavor));
	
	mFlavor.name=strdup("indeo5.decoder");
	mFlavor.info=strdup("this node decodes encoded indeo5 data producing raw-video");
	mFlavor.kinds=0;
	mFlavor.flavor_flags=0;
	mFlavor.internal_id=0;
	mFlavor.possible_count=0;

	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_VIDEO));
	c.And(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_ENCODING,BValue()
								.Overlay(BValue::String("be:avi:iv50"))
								.Overlay(BValue::String("be:avi:IV50")));
	

	mFlavor.in_formats=new BMediaConstraint(c);

	c=BMediaConstraint(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_VIDEO));
	c.And(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_COLORSPACE,BValue()
				.Overlay(BValue::Int32(B_RGB32))
				.Overlay(BValue::Int32(B_RGB32_BIG))
#if __INTEL__
				.Overlay(BValue::Int32(B_RGB15))
				.Overlay(BValue::Int32(B_RGB16))
#endif
				.Overlay(BValue::Int32(B_CMAP8))
				.Overlay(BValue::Int32(B_RGB24))
				.Overlay(BValue::Int32(B_YCbCr422)));
				
	mFlavor.out_formats=new BMediaConstraint(c);
}

CIndeo5DecoderAddon::~CIndeo5DecoderAddon()
{
	delete mFlavor.out_formats;
	delete mFlavor.in_formats;
	free(mFlavor.info);
	free(mFlavor.name);
}

status_t 
CIndeo5DecoderAddon::InitCheck(const char **out_failure_text)
{
	static const char *kOkText="everything is just fine.";
	
	*out_failure_text=kOkText;
	
	return B_OK;
}

int32 
CIndeo5DecoderAddon::CountFlavors()
{
	return 1;
}

status_t 
CIndeo5DecoderAddon::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	if (n>0)
		return B_ENTRY_NOT_FOUND;
		
	*out_info=&mFlavor;
	
	return B_OK;
}

BMediaNode *
CIndeo5DecoderAddon::InstantiateNodeFor(const flavor_info *info, status_t *out_error)
{
	if (info!=&mFlavor)
	{
		*out_error=B_BAD_VALUE;
		return NULL;
	}
		
	*out_error=B_OK;
	
	return new B::Private::Indeo5Decoder;	
}

BMediaAddon *
make_media_addon (image_id image)
{
	return new CIndeo5DecoderAddon(image);
}

