
#include <GLooper.h>
#include "GroupNamespace.h"
#include "GehmlDocument.h"
#include "GehmlContent.h"
#include "GehmlLayout.h"
#include "GehmlBlock.h"
#include "GehmlHVGroup.h"
#include "GehmlStack.h"
#include "GehmlWindow.h"
#include "GehmlEmbeddedRoot.h"
#include "BViewDrawable.h"

using namespace Wagner;
using namespace BXmlKit;

class GehmlDocumentPrototype : public Content {

	public:

									GehmlDocumentPrototype(void* handle);
		virtual 					~GehmlDocumentPrototype();
		virtual ssize_t 			Feed(const void *buffer, ssize_t bufferLen, bool done=false);
		virtual	bool				IsInitialized();
		virtual size_t				GetMemoryUsage();

		virtual void				Acquired();

	private:

		virtual status_t			CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);

		friend class GehmlDataSource;

		const void *				m_buffer;
		int32						m_bufferLen;
		sem_id						m_bufferInSem,m_bufferOutSem;
		bool						m_done;
		atom<GehmlDocument>			m_instance;
};

/******************************************************************/

typedef status_t (*gehml_constructor)(BStringMap &attributes, gehml_obj &child, gehml_group &group);
typedef AssociativeArray<BString,gehml_constructor> ConstructorList;

class GehmlParser : public BParser
{
	public:
						GehmlParser(GehmlDocument *doc);
	virtual				~GehmlParser();

	virtual status_t	StartTag(BString &name, BStringMap &attributes, BParser **newParser);
	virtual status_t	EndTag(BString &name);
	virtual status_t	TextData(const char *data, int32 size);

	atom<GehmlDocument>		m_doc;
	gehml_obj				m_object;
	SmartArray<gehml_group>	m_groups;
	ConstructorList			m_constructors;
};

/******************************************************************/

class GehmlDataSource : public BXMLDataSource
{
	public:
	
									GehmlDataSource(GehmlDocumentPrototype *source) : m_source(source) {};
		virtual status_t			GetNextBuffer(size_t * size, uchar ** data, int * done)
		{
			printf("GetNextBuffer %ld\n",*size);
			release_sem(m_source->m_bufferOutSem);
			acquire_sem(m_source->m_bufferInSem);
			*size = m_source->m_bufferLen;
			*data = (uchar*)m_source->m_buffer;
			*done = m_source->m_done;
			release_sem(m_source->m_bufferOutSem);
			return B_OK;
		}

	private:

		GehmlDocumentPrototype *	m_source;
};

/******************************************************************/

enum {
	fLoading = 0x00000001,
	fLayoutPending = 0x00000002
};

GehmlDocument::GehmlDocument(GehmlDocumentPrototype *content)
	: ContentInstance(content,NULL)
{
	SetFlags(Wagner::ContentInstance::Flags() | Wagner::cifDoubleBuffer);
}

void 
GehmlDocument::Acquired()
{
	GehmlRoot::Acquired();
	PostMessage(BMessage('load'));
}

void
GehmlDocument::GoInstanceGo()
{
	char buf[1024];
	GetContent()->GetResource()->GetURL().PrintToStream();
	GetContent()->GetResource()->GetURL().GetString(buf,1024);
	property ns = new GroupNamespace();
	ns["baseURL"] = buf;
	SetNamespace(ns);
}

GehmlDocument::~GehmlDocument()
{
}

status_t
GehmlDocument::GetSize(int32 *width, int32 *height, uint32 *flags)
{
	GehmlConstraints cnst;
	GetConstraints(HORIZONTAL,cnst.axis[HORIZONTAL]);
	GetConstraints(VERTICAL,cnst.axis[VERTICAL]);
	BRect r = FrameInParent();
	r = cnst.Resolve(BPoint(r.right - r.left + 1,r.bottom - r.top + 1));
	*width = (int32)(r.right-r.left+1);
	*height = (int32)(r.bottom-r.top+1);
	*flags = Wagner::STRETCH_HORIZONTAL|Wagner::STRETCH_VERTICAL;
	return B_OK;
}

status_t
GehmlDocument::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
//	newFrame.PrintToStream();
	BRect r = FrameInParent();
	ContentInstance::FrameChanged(newFrame,fullWidth,fullHeight);
//	FrameInParent().PrintToStream();

	BRegion dirty;
	if (r.right < newFrame.right)
		dirty.Include(BRect(r.right+1,newFrame.top,newFrame.right,newFrame.bottom));
	if (r.bottom < newFrame.bottom)
		dirty.Include(BRect(newFrame.left,r.bottom+1,newFrame.right,newFrame.bottom));
	if (dirty.CountRects() != 0) ContentInstance::MarkDirty(dirty);

	SetSize(newFrame.RightBottom() + BPoint(2,2));
	return B_DEFER_LAYOUT;
}

void 
GehmlDocument::ConstraintsChanged()
{
	ContentNotification(new BMessage(bmsgLayoutCompletion));
}

void 
GehmlDocument::MarkDirty(const BRegion &dirty)
{
	ContentInstance::MarkDirty(dirty);
}

GehmlDocumentPrototype *
GehmlDocument::GetContent() const
{
	return (GehmlDocumentPrototype*)ContentInstance::GetContent();
}

status_t
GehmlDocument::HandleMessage(BMessage *msg)
{
//	msg->PrintToStream();
	switch (msg->what) {
		case 'load': {
			ResumeScheduling();
			GLooper::UnregisterThis();
			GehmlDataSource source(GetContent());
			BDocumentParser parse(new GehmlParser(this));
			ParseXML(&parse,&source);
		} break;
		default:
			return GehmlRoot::HandleMessage(msg);
	}
	
	return B_OK;
}

void 
GehmlDocument::Cleanup()
{
	GehmlRoot::Cleanup();
	ContentInstance::Cleanup();
}

status_t 
GehmlDocument::Code(BXmlKit:: BCodifier *)
{
	return B_OK;
}

status_t 
GehmlDocument::Parse(BXmlKit:: BParser **)
{
	return B_OK;
}

const Wagner::GroupID
GehmlDocument::SecurityGroup()
{
	return GetContent()->GetResource()->GetGroupID();
}

void 
GehmlDocument::ResolveURL(const char *relURL, Wagner::URL &url)
{
	url.SetTo(GetContent()->GetResource()->GetURL(),relURL);
}

put_status_t 
GehmlDocument::WriteProperty(const char *name, const property &prop)
{
	return GehmlGroup::WriteProperty(name,prop);
}

get_status_t 
GehmlDocument::ReadProperty(const char *name, property &prop, const property_list &args)
{
	return GehmlObject::ReadProperty(name,prop,args);
}

status_t 
GehmlDocument::Draw(BView *view, BRect exposed)
{
	BRegion dirty;
	dirty.Include(exposed);
	BViewDrawable into(view);
	GetLayout().Draw(into,dirty);
	return B_OK;
}

void 
GehmlDocument::MouseDown(BPoint , const BMessage *)
{
	DumpInfo(0);
}

/******************************************************************/

GehmlParser::GehmlParser(GehmlDocument *doc)
{
	m_doc = doc;
	m_object = doc;
	m_constructors.Insert("content",GehmlContent::Constructor);
	m_constructors.Insert("group",GehmlGroup::Constructor);
	m_constructors.Insert("block",GehmlBlock::Constructor);
	m_constructors.Insert("hgroup",GehmlHVGroup::HConstructor);
	m_constructors.Insert("vgroup",GehmlHVGroup::VConstructor);
	m_constructors.Insert("stack",GehmlStack::Constructor);
	m_constructors.Insert("root",GehmlEmbeddedRoot::Constructor);
	m_constructors.Insert("window",GehmlWindow::Constructor);
	m_groups.AddItem(doc);
}

GehmlParser::~GehmlParser()
{
}

status_t 
GehmlParser::StartTag(BString &name, BStringMap &attributes, BParser **newParser)
{
	gehml_obj child;
	gehml_group group;
	gehml_constructor func = m_constructors.Lookup(name);
	if (func) {
//		printf("Constructing <%s>\n",name.String());
		if (!func(attributes,child,group)) {
			m_groups[m_groups.Count()-1]->AddChild(child,"name");
			if (group) m_groups.AddItem(group);
			else *newParser = new BParser();
		}
	}
	return B_OK;
}

status_t 
GehmlParser::EndTag(BString &)
{
	m_groups.RemoveItem(m_groups.Count()-1);
	return B_OK;
}

status_t 
GehmlParser::TextData(const char *, int32 )
{
	return B_OK;
}

/******************************************************************/

GehmlDocumentPrototype::GehmlDocumentPrototype(void *handle) : Content(handle)
{
	m_bufferInSem = create_sem(0,"bufferInSem");
	m_bufferOutSem = create_sem(0,"bufferOutSem");
}

void 
GehmlDocumentPrototype::Acquired()
{
}

GehmlDocumentPrototype::~GehmlDocumentPrototype()
{
	delete_sem(m_bufferInSem);
	delete_sem(m_bufferOutSem);
}

ssize_t 
GehmlDocumentPrototype::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	if (!m_instance) {
		m_instance = new GehmlDocument(this);
		m_instance->GoInstanceGo();
	}

	acquire_sem(m_bufferOutSem);
	m_buffer = buffer;
	m_bufferLen = bufferLen;
	m_done = done;
	release_sem_etc(m_bufferInSem,1,B_DO_NOT_RESCHEDULE);
	acquire_sem(m_bufferOutSem);
	return bufferLen;
}

bool 
GehmlDocumentPrototype::IsInitialized()
{
	return true;
}

status_t 
GehmlDocumentPrototype::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &)
{
	m_instance->SetHandler(handler);
	*outInstance = m_instance;
	return B_OK;
}

size_t 
GehmlDocumentPrototype::GetMemoryUsage()
{
	return 16;
}

/******************************************************************/

class GehmlContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-gehml");
		into->AddString(S_CONTENT_EXTENSIONS, "gml");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		(void)mime;
		(void)extension;
		printf("bleh2\n");
		return new GehmlDocumentPrototype(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id , uint32 , ...)
{
	printf("bleh1\n");
	if( n == 0 ) return new GehmlContentFactory;
	return 0;
}
