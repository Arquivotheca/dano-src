/*
	IMTransportHandle.cpp
*/
#include "IMTransportHandle.h"
#include <stdio.h>

IMTransportHandle::IMTransportHandle(const entry_ref *entry, const node_ref *node)
	:	BAddOnHandle(entry, node),
		fTransport(0)
{
	printf("Creating new TransportHandle for: '%s'\n", entry != NULL ? entry->name : "entry is null");
}


IMTransportHandle::~IMTransportHandle()
{

}

bool IMTransportHandle::KeepLoaded() const
{
	return true;
}

bool IMTransportHandle::IsDynamic() const
{
	return false;
}

size_t IMTransportHandle::GetMemoryUsage() const
{
	return GetMemoryUsage();
}

IMTransport *IMTransportHandle::InstantiateTransport()
{
	image_id image = Open();
	if (image < B_OK)
		return 0;
		
	if (!fTransport) {
		void *maker;
		if (get_image_symbol(image, "make_nth_transport", B_SYMBOL_TYPE_TEXT, &maker) == B_OK) {
			fTransport = (*((make_nth_transport_type)maker))(0, image, 0);
		} else {
			printf("Unable to find make_nth_transport for func in image %ld\n", image);
		}
	}
	return fTransport;
}

void IMTransportHandle::ImageLoaded(image_id image)
{

}

status_t IMTransportHandle::LoadIdentifiers(BMessage *into, image_id from)
{
	return B_OK;
}

void IMTransportHandle::ImageUnloading(image_id image)
{

}

const char *IMTransportHandle::AttrBaseName() const
{
	return "be:im_transport";
}

// End of IMTransportHandle.cpp
