/*
	IMTransportHandle.h
*/
#ifndef IM_TRANSPORT_HANDLE_H
#define IM_TRANSPORT_HANDLE_H

#include <AddOnManager.h>
#include "IMTransport.h"

class IMTransportHandle : public BAddOnHandle {
	public:
								IMTransportHandle(const entry_ref *entry = NULL, const node_ref *node = NULL);
		virtual					~IMTransportHandle();
								// BAddOnHandle public virtuals
		virtual bool			KeepLoaded() const;
		virtual bool			IsDynamic() const;
		virtual size_t			GetMemoryUsage() const;
								// IMTransportHandle public functions
		IMTransport *			InstantiateTransport();
		
	protected:
		virtual void			ImageLoaded(image_id image);
		virtual status_t		LoadIdentifiers(BMessage *into, image_id from);
		virtual void			ImageUnloading(image_id image);
		virtual const char *	AttrBaseName() const;
		
	private:
		IMTransport *fTransport;							
	
};

#endif
// End of IMTransportHandle.h
