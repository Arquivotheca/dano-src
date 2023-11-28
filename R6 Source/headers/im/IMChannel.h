/*
	IMChannel.h
*/
#ifndef IM_CHANNEL_H
#define IM_CHANNEL_H

#include <Binder.h>

class IMChannel : public BinderNode {
	public:
								IMChannel();
								~IMChannel();

	protected:
								// BinderNode protected virtuals
		virtual	put_status_t	WriteProperty(const char *name, const property &prop);
		virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
	
	private:
};

#endif

// End of IMChannel.h
