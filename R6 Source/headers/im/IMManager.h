/*
	IMManager.h
*/
#ifndef IM_MANAGER_H
#define IM_MANAGER_H

#include <AddOnManager.h>
#include <Binder.h>
#include <List.h>

class IMUserListener;

class IMManager : public BAddOnManager, public BinderNode {
	public:
		static IMManager *		Manager();
		static void				Teardown();

		virtual status_t		HandleMessage(BMessage *message);
			
	protected:
		virtual BAddOnHandle *	InstantiateHandle(const entry_ref* entry, const node_ref* node);
		
		virtual	put_status_t	WriteProperty(const char *name, const property &prop);
		virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

	private:
								// These are private, you must go through the above static calls.
								IMManager();
		virtual					~IMManager();
								// Do not implement these, they are no-op's
								IMManager(const IMManager &inManager);
		const IMManager &		operator=(const IMManager &inManager);
								
		BList fTransportList;

		static IMManager * fIMManagerInstance;
		static BLocker fInstanceLock;
};

#endif

// End of IMManager.h
