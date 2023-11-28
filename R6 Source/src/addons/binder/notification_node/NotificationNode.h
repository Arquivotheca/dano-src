/*
	NotificationNode.h
*/
#ifndef _NOTIFICATION_NODE_H_
#define _NOTIFICATION_NODE_H_
#include <Binder.h>
#include <SmartArray.h>
	
enum {
	kNormalPriority = 0,
	kAlphaPriority = 10,
	kBetaPriority = 20,
	kCharliePriority = 50,
	kDeltaPriority = 100
};

class NotificationContainer;
class NotificationNode : public BinderNode
{
	public:
								NotificationNode();
								~NotificationNode();
								
		virtual	put_status_t	WriteProperty(const char *name, const property &prop);
		virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		void					PrintToStream();
		
	private:
		status_t				Post(property &outProperty, const property_list &inArgs);
		void					Next(property &outProperty, const property_list &inArgs);
		void					Peek(property &outProperty, const property_list &inArgs);
		void					Count(property &outProperty, const property_list &inArgs);
		void					Clear();
		
		SmartArray< atom<class NotificationContainer> > fAlerts;
};

class NotificationContainer : public BinderContainer
{
	public:
								NotificationContainer();
								
		virtual	put_status_t	WriteProperty(const char *name, const property &prop);
		virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
		
		BString					Code();	
		int32					Priority();
		int32					ParameterCount() const;
		time_t					Timestamp();
			
	private:
		int32 fParameterCount;
};

#endif
