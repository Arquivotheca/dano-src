#ifndef _B_XMLBINDERNODE_H
#define _B_XMLBINDERNODE_H


#include <string.h>
#include <SmartArray.h>
#include <AssociativeArray.h>
#include <Messenger.h>
#include <GHandler.h>
#include <Binder.h>
#include <unistd.h>

#include <xml/BParser.h>
#include <xml/BExpressible.h>

class BPath;

class XMLBinderNode : public BinderContainer, public B::XML::BExpressible
{
	public:
										XMLBinderNode(const char *pathname=NULL);
										XMLBinderNode(XMLBinderNode *parent);
										XMLBinderNode(XMLBinderNode *copyFrom, bool deepCopy);
		virtual							~XMLBinderNode();

		BPath							Pathname();
		status_t						SetPathname(const char *pathname, bool load=false, bool save=false);

		status_t						Load(BDataIO &stream);
		status_t						Save();

		XMLBinderNode *					Parent();
		status_t						SetParent(XMLBinderNode *parent);

		virtual status_t				Code(B::XML::BCodifier *stream);
		virtual status_t				Parse(B::XML::BParser **stream);

		virtual	put_status_t			WriteProperty(const char *name, const property &prop);
		virtual	get_status_t			ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		virtual	status_t				HandleMessage(BMessage *msg);
		virtual	void					MarkDirty();

		virtual	binder_node				Copy(bool deep=false);

	private:

				status_t				InternalCode(B::XML::BCodifier *stream, bool amRoot);

		friend class BinderParser;

		struct linkage {
			binder_node node;
			B::XML::BStringMap attributes;
		};

		atom<XMLBinderNode>						m_parent;
		SmartArray<linkage>						m_inherits;
		SmartArray<linkage>						m_overlays;
		SmartArray<linkage>						m_externalProperties;
		AssociativeArray<BString,binder_node>	m_skels;
		BString									m_pathname;
		NestedGehnaphore						m_saveLock;
		bigtime_t								m_saveTime;
		bool									m_canStack:1;
		bool									m_transient:1;
		uint16									m_flags:14;
};

#endif // _B_XMLBINDERNODE_H
