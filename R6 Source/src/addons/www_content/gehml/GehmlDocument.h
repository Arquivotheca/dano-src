
#ifndef _GEHMLDOCUMENT_H_
#define _GEHMLDOCUMENT_H_

#include <Content.h>
#include <XMLParser.h>
#include "GehmlRoot.h"
#include "GehmlLayout.h"

class GehmlDocumentPrototype;
class GehmlDocument :
	public Wagner::ContentInstance,
	public BXmlKit::BExpressible,
	public GehmlRoot
{
	public:
										GehmlDocument(GehmlDocumentPrototype *content);
		virtual							~GehmlDocument();

		virtual status_t				Code(BXmlKit::BCodifier *stream);
		virtual status_t				Parse(BXmlKit::BParser **stream);

		virtual void					ResolveURL(const char *relURL, Wagner::URL &url);
		virtual const Wagner::GroupID	SecurityGroup();

		virtual	status_t				HandleMessage(BMessage *msg);
				void					GoInstanceGo();

		virtual void					Acquired();
		virtual	void					Cleanup();

		virtual	put_status_t			WriteProperty(const char *name, const property &prop);
		virtual	get_status_t			ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		/* ContentInstance overrides */
		virtual	status_t				Draw(BView *into, BRect exposed);
		virtual status_t				GetSize(int32 *width, int32 *height, uint32 *flags);
		virtual	status_t				FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
		virtual	void					MarkDirty(const BRegion &dirty);
		virtual	void					ConstraintsChanged();

		virtual	void					MouseDown(BPoint where, const BMessage *event=NULL);

		GehmlDocumentPrototype *		GetContent() const;
		
	private:
	
		GehmlLayout						m_layout;
		Gehnaphore						m_lock;
		int32							m_flags;
};

#endif
