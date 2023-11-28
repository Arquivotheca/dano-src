
#ifndef _XML2_XMLROOTSPLAY_H
#define _XML2_XMLROOTSPLAY_H

#include <support2/IValueStream.h>
#include <xml2/XMLOStr.h>
#include <support2/Stack.h>
#include <support2/IRoot.h>

namespace B {
namespace XML {

using namespace Support2;

/**************************************************************************************/

class BXMLRootSplay : public CXMLOStr
{
	public:

								BXMLRootSplay(IValueOutput::arg stream);
	
		virtual status_t		StartTag(BString &name, BValue &attributes);
		virtual status_t		EndTag(BString &name);
		virtual status_t		Content(const char	*data, int32 size);

	protected:

								BXMLRootSplay(IValueOutput *stream);
								~BXMLRootSplay();
								
		virtual	status_t		Acquired(const void* id);
		virtual	status_t		Released(const void* id);

	private:

				void			Out(const BValue &val);
				void			PushMapping(const value_ref &val);
				void			PushStream(IValueOutput::arg stream);
				void			Pop();

				struct parse_context {
					IValueOutput::ptr		stream;
					BValue				mapping;
					parse_context *		next;
				};

				parse_context *	m_context;
				IValueOutput *	m_root;
				BString			m_name;
				type_code		m_type;
				char *			m_data;
				int32			m_bufSize;
				int32			m_dataSize;
				BStack<IRoot::ptr> m_rootObjects;
};

/**************************************************************************************/

} } // namespace B::XML

#endif // _XML2_XMLROOTSPLAY_H
