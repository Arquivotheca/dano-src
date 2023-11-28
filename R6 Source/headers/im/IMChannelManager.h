/*
	IMChannelManager.h
*/
#ifndef _IM_CHANNEL_MANAGER_H
#define _IM_CHANNEL_MANAGER_H

#include <Binder.h>

class IMChannelManager : public BinderNode {
	public:
								IMChannelManager();
								~IMChannelManager();

	protected:
								// BinderNode protected virtuals
		virtual	put_status_t	WriteProperty(const char *name, const property &prop);
		virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
	
	private:
};

#endif

// End of IMChannelManager.h
