
#include <malloc.h>
#include <stdio.h>
#include <OS.h>
#include <support2/StdIO.h>
#include <support2/Container.h>
#include <support2/Team.h>
#include <xml2/XMLRootSplay.h>

namespace B {
namespace XML {

BXMLRootSplay::BXMLRootSplay(IValueOutput::arg stream)
	:	m_context(NULL), m_root(stream.ptr()), m_type(0), m_data(NULL),
		m_bufSize(1), m_dataSize()
{
	m_root->Acquire();
}

BXMLRootSplay::BXMLRootSplay(IValueOutput* stream)
	:	m_context(NULL), m_root(stream), m_type(0), m_data(NULL),
		m_bufSize(1), m_dataSize()
{
}

status_t
BXMLRootSplay::Acquired(const void* id)
{
//	m_search.AddEnvVar("ADDON_PATH", "binder");
	m_data = (char*)malloc(m_bufSize);
	PushStream(m_root);
	return CXMLOStr::Acquired(id);
}

status_t
BXMLRootSplay::Released(const void* id)
{
	return CXMLOStr::Released(id);
}

BXMLRootSplay::~BXMLRootSplay()
{
	while (m_context) Pop();
	if (m_data) free(m_data);
	m_root->AttemptRelease();
}
/*
typedef BinderNode * (*return_binder_node_type)();

BinderNode * load_node(BSearchPath& path, const char *handler)
{
	const image_id image = path.LoadAddOn(handler);
	
	if (image >= 0) {
		void *rrn;
		if (get_image_symbol(image,"return_binder_node",B_SYMBOL_TYPE_TEXT,&rrn) == B_OK) {
			printf("Loaded image for '%s'\n",handler);
			return (*((return_binder_node_type)rrn))();
		}
	}
	
	printf("Could not load image for '%s'\n",handler);
	
	return NULL;
}
*/

void fatal_error(const char *format, ...)
{
	va_list args;
	int		res;
	char	buf[1024];
	
	va_start (args, format);
	res = vsprintf(buf,format, args);	
	va_end (args);

	debugger(buf);
}

status_t 
BXMLRootSplay::StartTag(BString &name, BValue &attributes)
{
	BValue addonParamsUnused;
	BValue s = attributes["name"];
	if (s) m_name = s.AsString();
	else m_name = "<undefined>";
	
	berr << "StartTag " << m_name << ": " << attributes << endl;
	
	if (name == "object") {
		IBinder::ptr newNode;
		m_type = B_BINDER_TYPE;
		if ((s = attributes["code"])) {
			if (attributes["remote"].AsBool()) {
				newNode = load_object_remote(s.AsString().String());
				if (newNode == NULL) fatal_error("could not load remote object at '%s'",s.AsString().String());
			} else {
				if (m_rootObjects.CountItems() > 0 && m_rootObjects.Top().ptr()) { // if it's not an IRoot, load the addon in smooved
					newNode = m_rootObjects.Top()->LoadObject(s.AsString(), addonParamsUnused);
				} else {
					newNode = load_object(s.AsString().String());
				}
				if (newNode == NULL) fatal_error("could not load object at '%s'",s.AsString().String());
			}
		} else {
			newNode = (new BContainer)->IValueOutput::AsBinder();
		}
		
		IRoot::ptr newNodeAsRoot = IRoot::AsInterface(newNode);
		m_rootObjects.Push(newNodeAsRoot);
		
		Out(BValue(m_name, BValue::Binder(newNode)));

		IValueOutput::ptr newStream = IValueOutput::AsInterface(newNode);
		if (newStream != NULL) {
			PushStream(newStream);
			return B_OK;
		}
		PushMapping(m_name.String());
	} else if (name == "string") m_type = B_STRING_TYPE;
	else if (name == "number") m_type = B_DOUBLE_TYPE;
	else if (name == "null") m_type = B_NULL_TYPE;
	else {
		PushMapping(m_name.String());
		m_type = 0;
	}
	m_dataSize = 0;

	return B_OK;
}

status_t 
BXMLRootSplay::EndTag(BString & name)
{
	if (m_type == B_STRING_TYPE) {
		m_data[m_dataSize] = 0;
		Out(BValue(m_name, BValue::String((const char*)m_data)));
	} else if (m_type == B_DOUBLE_TYPE) {
		double d;
		m_data[m_dataSize] = 0;
		sscanf(m_data,"%lf",&d);
		Out(BValue(m_name, BValue::Double(d)));
	} else if (m_type == B_NULL_TYPE) {
		Out(BValue(m_name, BValue::Null()));
	} else
		Pop();

	if (name == "object") {
		m_rootObjects.Pop();
	}

	m_type = 0;
	m_dataSize = 0;

	return B_OK;
}

status_t 
BXMLRootSplay::Content(const char *data, int32 size)
{
	if ((m_type == B_BINDER_TYPE) || 
		(m_type == B_NULL_TYPE)) return B_OK;

	int32 newSize = m_dataSize + size;
	if (m_bufSize < newSize+1) {
		m_bufSize = newSize+1;
		char *newAlloc = (char*)malloc(m_bufSize);
		if (m_data) {
			memcpy(newAlloc,m_data,m_dataSize);
			free(m_data);
		}
		m_data = newAlloc;
	}

	memcpy(m_data+m_dataSize,data,size);
	m_dataSize = newSize;

	return B_OK;
}

void 
BXMLRootSplay::Out(const BValue &val)
{
	BValue v(val);
	parse_context *ctxt = m_context;
	while (ctxt) {
		berr << "Out: " << v << endl;
		if (ctxt->stream != NULL) {
			ctxt->stream->Write(v);
			break;
		}
		v = BValue(ctxt->mapping, v);
		ctxt = ctxt->next;
	}
}

void 
BXMLRootSplay::PushMapping(const value_ref &val)
{
	parse_context *ctxt = new parse_context;
	ctxt->mapping = val;
	ctxt->next = m_context;
	m_context = ctxt;
}

void 
BXMLRootSplay::PushStream(IValueOutput::arg stream)
{
	parse_context *ctxt = new parse_context;
	ctxt->stream = stream;
	ctxt->next = m_context;
	m_context = ctxt;
}

void 
BXMLRootSplay::Pop()
{
	parse_context *ctxt = m_context;
	m_context = ctxt->next;
	delete ctxt;
}

}; // namespace XML
}; // namespace B
