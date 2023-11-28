

#include <OS.h>
#include <stdlib.h>
#include <stdio.h>
#include <Binder.h>
#include <Gehnaphore.h>
#include "enum_recovery.h"

class RecoveryNode : public BinderNode
{
		zrecover_config_t					m_config;
		Gehnaphore							m_lock;
		bool								m_burnScheduled;

	public:
											RecoveryNode();
		virtual								~RecoveryNode();
	
		virtual	status_t					OpenProperties(void **cookie, void *copyCookie);
		virtual	status_t					NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t					CloseProperties(void *cookie);

		virtual	put_status_t				WriteProperty(const char *name, const property &prop);
		virtual	get_status_t				ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		virtual	status_t					HandleMessage(BMessage *msg);
};

RecoveryNode::RecoveryNode()
{
}

RecoveryNode::~RecoveryNode()
{
}

status_t 
RecoveryNode::OpenProperties(void **cookie, void *)
{
	GehnaphoreAutoLock _auto(m_lock);
	int32 *index = new int32;
	*index = -1;
	*cookie = index;
	return B_OK;
}

status_t 
RecoveryNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	GehnaphoreAutoLock _auto(m_lock);
	int32 next,i,*index = (int32*)cookie;
	if (*index == -1) {
		if (!m_config.CountItems()) return ENOENT;
		*index = 0;
		for (i=1;i<m_config.CountItems();i++)
			if (strcmp(m_config.ItemAt(i)->Key(),m_config.ItemAt(*index)->Key()) < 0) *index = i;
	} else {
		next = -1;
		for (i=0;i<m_config.CountItems();i++) {
			if ((strcmp(m_config.ItemAt(i)->Key(),m_config.ItemAt(*index)->Key()) > 0) &&
				((next == -1) || (strcmp(m_config.ItemAt(i)->Key(),m_config.ItemAt(next)->Key()) < 0))) next = i;
		}
		if (next == -1) return ENOENT;
		*index = next;
	}
	strncpy(nameBuf,m_config.ItemAt(*index)->Key(),*len);
	*len = strlen(m_config.ItemAt(*index)->Key());
	return B_OK;
}

status_t 
RecoveryNode::CloseProperties(void *cookie)
{
	int32 *index = (int32*)cookie;
	delete index;
	return B_OK;
}

put_status_t 
RecoveryNode::WriteProperty(const char *name, const property &prop)
{
	GehnaphoreAutoLock _auto(m_lock);

	for (int32 i=0;i<m_config.CountItems();i++) {
		if (!strcmp(m_config.ItemAt(i)->Key(),name)) {
			if (m_config.ItemAt(i)->Kind() == zrecover_config_item_t::ZR_INFO) return EPERM;
			m_config.Configure(name,prop.String().String());
			NotifyListeners(B_PROPERTY_CHANGED,name);
			if (!m_burnScheduled) {
				m_burnScheduled = true;
				PostDelayedMessage(BMessage('burn'),30*1000000L);
			}
			return B_OK;
		}
	}
	return put_status_t(EPERM,true);
}

get_status_t 
RecoveryNode::ReadProperty(const char *name, property &prop, const property_list &)
{
	GehnaphoreAutoLock _auto(m_lock);

	for (int32 i=0;i<m_config.CountItems();i++) {
		if (!strcmp(m_config.ItemAt(i)->Key(),name)) {
			prop = m_config.ItemAt(i)->Value();
			return get_status_t(B_OK,true);
		}
	}
	return ENOENT;
}

status_t 
RecoveryNode::HandleMessage(BMessage *msg)
{
	GehnaphoreAutoLock _auto(m_lock);
	
	if (msg->what == 'burn')
		m_config.Commit();
	else
		return BinderNode::HandleMessage(msg);
		
	return B_OK;
}

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new RecoveryNode();
}
