#ifndef _SCREENSAVER_CONTROLLER_NODE_H_
#define _SCREENSAVER_CONTROLLER_NODE_H_

#include <Binder.h>
#include <Locker.h>
#include <Messenger.h>

// property names
extern const char* kQuitBlankerProp;
extern const char* kIdleTimeProp;
extern const char* kIsBlankingProp;

// message field names
extern const char* kIdleTimeField;
extern const char* kQuitBlankerResultField;
extern const char* kIsBlankingField;
extern const char* kEventMaskField;
extern const char* kPropertyNameField;

class ScreenSaverControllerNode : public BinderNode {
public:
	ScreenSaverControllerNode(BMessenger target);
	virtual ~ScreenSaverControllerNode(void);
	
	virtual status_t HandleMessage(BMessage* message);
	
protected:
	virtual	status_t OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t CloseProperties(void *cookie);

	virtual	put_status_t WriteProperty(const char *name, const property &prop);
	virtual	get_status_t ReadProperty(const char *name, property &prop,
		const property_list &args = empty_arg_list);
		
private:
	BMessenger m_target;
	BLocker m_locker;

	struct store_cookie {
		int32 index;
	};
};

#endif /* #ifndef _SCREENSAVER_CONTROLLER_NODE_H_ */
