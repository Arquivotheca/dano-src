
#ifndef UPDATED_NODE_H
#define UPDATED_NODE_H

#include <URL.h>
#include <Content.h>
#include <Binder.h>

class MCABinderNode : public BinderContainer
{
	public:
											MCABinderNode();
		virtual								~MCABinderNode();
		
		void								RemoveConnection(const char *name);
		BinderNode::property 				Connect(const char *name, Wagner:: URL &url);

	protected:

		virtual	get_status_t				ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
		virtual	void						Cleanup();

	private:

		Gehnaphore							m_lock;
		AtomPtr<BinderContainer>			m_connections;
		uint32								m_flags;
};

#endif
