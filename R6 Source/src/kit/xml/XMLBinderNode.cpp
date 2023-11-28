
#include <ctype.h>
#include <image.h>
#include <Directory.h>
#include <File.h>
#include <List.h>
#include <BufferIO.h>
#include <Path.h>
#include <SearchPath.h>
#include "xml/BXMLBinderNode.h"

using namespace B::XML;

#define SAVE_TIMEOUT 5*1000000LL
#define NODE_CLEAN 0x7FFFFFFFFFFFFFFFLL

enum {
	linkageNone		= 0,
	linkageInherit	= 1,
	linkageOverlay	= 2
};

#define checkpoint printf("XMLBinderNode: thid=%ld -- %s:%d -- %s\n",(int32)find_thread(NULL),__FILE__,__LINE__,__FUNCTION__);

class BinderParser : public BParser
{
	public:
						BinderParser(XMLBinderNode *object, bool include=false);
	virtual				~BinderParser();

	virtual status_t	StartTag(BString &name, BStringMap &attributes, BParser **newParser);
	virtual status_t	EndTag(BString &name);
	virtual status_t	TextData(const char *data, int32 size);

	BSearchPath					m_search;
	bool						m_include;
	XMLBinderNode *				m_object;
	BString						m_name;
	BString						m_storage;
	BinderNode::property::type	m_type;
	uint16						m_readingPerms;
	int32						m_behaviorPending;
	char *						m_data;
	int32						m_bufSize;
	int32						m_dataSize;
};

BinderParser::BinderParser(XMLBinderNode *object, bool include)
{
	m_search.AddEnvVar("ADDON_PATH", "binder");
	m_object = object;
	m_type = BinderNode::property::null;
	m_data = (char*)malloc(1);
	m_bufSize = 1;
	m_dataSize = 0;
	m_behaviorPending = -1;
	m_include = include;
}

BinderParser::~BinderParser()
{
	if (m_data) free(m_data);
}

uint16 string_to_perms(const char *p)
{
	uint32 perms = 0;
	while (*p) {
		switch (tolower(*p++)) {
			case 'r': perms |= permsRead; break;
			case 'w': perms |= permsWrite; break;
			case 'c': perms |= permsCreate; break;
			case 'd': perms |= permsDelete; break;
			case 'm': perms |= permsMount; break;
		}
	}
	
	return perms;
}

void perms_to_string(uint16 perms, char *p)
{
	if (perms & permsRead) *p++ = 'r';
	if (perms & permsWrite) *p++ = 'w';
	if (perms & permsCreate) *p++ = 'c';
	if (perms & permsDelete) *p++ = 'd';
	if (perms & permsMount) *p++ = 'm';
	*p = 0;
}

typedef BinderNode * (*return_binder_node_type)();
typedef BinderNode * (*return_binder_node_etc_type)(const char *);

BinderNode * load_node(BSearchPath& path, const char *handler, const char *arg = NULL)
{
	const image_id image = path.LoadAddOn(handler);
	
	if (image >= 0) {
		void *rrn;
		if (arg && get_image_symbol(image,"return_binder_node_etc",B_SYMBOL_TYPE_TEXT,&rrn) == B_OK) {
			printf("Loaded image for '%s' with argument '%s'\n", handler, arg);
			return (*((return_binder_node_etc_type)rrn))(arg);
		} else if (get_image_symbol(image,"return_binder_node",B_SYMBOL_TYPE_TEXT,&rrn) == B_OK) {
			printf("Loaded image for '%s'\n",handler);
			return (*((return_binder_node_type)rrn))();
		}

	}
	
	printf("Could not load image for '%s'\n",handler);
	
	return NULL;
}

status_t 
BinderParser::StartTag(BString &name, BStringMap &attributes, BParser **newParser)
{
	BString *s = attributes.Find("name");
	if (s) m_name = *s;
	else m_name = "<undefined>";

	uint32 perms = permsInherit;
	m_readingPerms = 0;
	s = attributes.Find("perms");
	if (s) perms = string_to_perms(s->String());

	if (name == "prototype") {
		s = attributes.Find("storage");
		if (s && (*s != "")) {
			m_object->SetPathname(s->String());
			BFile file(s->String(),O_RDONLY);
			printf("Setting storage to '%s' %ld\n",s->String(),file.InitCheck());
			if (!file.InitCheck()) {
				m_object->Load(file);
				return B_ERROR;
			}
		}

		if (perms == permsInherit) perms = permsRead;
		m_object->SetPermissions(perms);
		s = attributes.Find("stacking");
		if (s && strcasecmp(s->String(),"false")) m_object->m_canStack = true;

		s = attributes.Find("ordered");
		if (s && strcasecmp(s->String(),"false")) m_object->SetOrdered(true);

		s = attributes.Find("transient");
		if (s && strcasecmp(s->String(),"false")) m_object->m_transient = true;

		*newParser = new BParser();
		return B_OK;
	}

	if (name == "include") {
		s = attributes.Find("storage");
		if (s) {
			BFile file(s->String(),O_RDONLY);
			printf("Including '%s' %ld\n",s->String(),file.InitCheck());
			if (!file.InitCheck()) {
				BXMLDataIOSource source(&file);
				BDocumentParser parse(new BinderParser(m_object,true));
				ParseXML(&parse,&source);
			}
		}
		*newParser = new BParser();
		return B_OK;
	}

	if ((name == "skel") || (name == "object") || (name == "inherit") || (name == "overlay")) {
		BinderNode *newNode;
		if (m_include) {
			status_t err = B_ERROR;
			binder_node subNode;
			/* Find the object we have to recurse into... */
			if (name == "object") {
				BinderNode::property nodeProp;
				err = m_object->GetProperty(m_name.String(),nodeProp);
				if (!err) subNode = nodeProp;
//				printf("recursing into '%s'\n",m_name.String());
			} else if ((name == "inherit") || (name == "overlay")) {
				SmartArray<XMLBinderNode::linkage> &links = m_object->m_inherits;
				if (name == "overlay") links = m_object->m_overlays;
				for (int32 i=0;i<links.CountItems();i++) {
					BString *s = links[i].attributes.Find("name");
					if (s && (*s == m_name)) {
						subNode = links[i].node;
						err = B_OK;
						break;
					}
				}
			} else {
				subNode = m_object->m_skels.Lookup(m_name.String());
				if (subNode) err = B_OK;
			}
			
			if (!err) {
				BExpressible* exp = dynamic_cast<BExpressible*>((BinderNode*)subNode);
				BParser *p = NULL;
				if (exp) {
					BinderParser *bp;
					exp->Parse(&p);
					if (!p) p = new BParser();
					else if ((bp = dynamic_cast<BinderParser*>(p)))
						bp->m_include = true;
//					printf("calling parser on  '%s'\n",m_name.String());
				}
				*newParser = p;
				return B_OK;
			}
		}

		if ((s = attributes.Find("handler"))) {
			/* This object is implemented in an add-on */
			BString *arg = attributes.Find("argument");
			newNode = load_node(m_search, s->String(), (arg?arg->String():NULL));
		} else {
			XMLBinderNode *node = new XMLBinderNode(m_object);
			newNode = node;
		}

		BExpressible *xmlObject = dynamic_cast<BExpressible*>(newNode);
		if (xmlObject) xmlObject->Parse(newParser);

		if (name == "inherit") {
			int32 i = m_object->m_inherits.AddItem();
			m_object->m_inherits[i].node = newNode;
			m_object->m_inherits[i].attributes = attributes;
			if (newNode) m_object->InheritFrom(newNode);
			else *newParser = new BParser();
		} else if (name == "overlay") {
			int32 i = m_object->m_overlays.AddItem();
			m_object->m_overlays[i].node = newNode;
			m_object->m_overlays[i].attributes = attributes;
			if (newNode) newNode->StackOnto(m_object);
			else *newParser = new BParser();
		} else {
			if (newNode) {
				if (name == "object") m_object->BinderContainer::AddProperty(m_name.String(),newNode,perms);
				else m_object->m_skels.Insert(m_name,newNode);
			} else {
				int32 i = m_object->m_externalProperties.AddItem();
				m_object->m_externalProperties[i].attributes = attributes;
				*newParser = new BParser();
			}
		}

		return B_OK;
	}
	
	m_readingPerms = perms;
	if (name == "string") m_type = BinderNode::property::string;
	else if (name == "number") m_type = BinderNode::property::number;
	else if (name == "undefined") m_type = BinderNode::property::null;
	else if (name == "mountpoint") {
		m_type = BinderNode::property::null;
		m_readingPerms = permsRead | permsMount;
	} else
		m_readingPerms = 0;


	return B_OK;
}

status_t 
BinderParser::EndTag(BString &name)
{
	(void)name;
	if (m_readingPerms && (!m_include || !m_object->HasProperty(m_name.String()))) {
		if (m_type == BinderNode::property::string) {
			m_data[m_dataSize] = 0;
			m_object->BinderContainer::AddProperty(m_name.String(),m_data,m_readingPerms);
		} else if (m_type == BinderNode::property::number) {
			double d;
			m_data[m_dataSize] = 0;
			sscanf(m_data,"%lf",&d);
			m_object->BinderContainer::AddProperty(m_name.String(),d,m_readingPerms);
		} else if (m_type == BinderNode::property::null) {
			m_object->BinderContainer::AddProperty(m_name.String(),BinderNode::property::undefined,m_readingPerms);
		}
	}

	m_type = BinderNode::property::null;
	m_dataSize = 0;
	m_readingPerms = 0;

	return B_OK;
}

status_t 
BinderParser::TextData(const char *data, int32 size)
{
	if ((m_type == BinderNode::property::object) || 
		(m_type == BinderNode::property::null)) return B_OK;

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

/*********************************************************************/

status_t 
XMLBinderNode::Code(BCodifier *stream)
{
	return InternalCode(stream,false);
}

status_t 
XMLBinderNode::InternalCode(BCodifier *stream, bool amRoot)
{
	char *typeStr = NULL;
	char permStr[64];
	BStringMap map;
	BExpressible *xml;
	uint32 formatting;
	int32 count;

	m_listLock.Lock();

	perms_to_string(Permissions(),permStr);
	map.Add("perms",permStr);
	if ((m_pathname != "") && !amRoot) map.Add("storage",m_pathname);
	if (m_canStack) map.Add("stacking","true");
	if (Ordered()) map.Add("ordered","true");
	if (m_transient) map.Add("transient","true");
	stream->StartTag("prototype",map,stfLeaf);
	stream->EndTag("prototype");

	if (!amRoot && (m_pathname != "")) {
		bool needSave = false;
		{
			BFile file(m_pathname.String(),O_RDONLY);
			if (file.InitCheck()) needSave = true;
		}
		
		m_listLock.Unlock();
		if (needSave) Save();
	} else {
		count = m_overlays.Count();
		for (int32 i=0;i<count;i++) {
			stream->StartTag("overlay",m_overlays[i].attributes,stfCanAddWS);
			binder_node obj = m_overlays[i].node;
			if ((xml = dynamic_cast<BExpressible*>((BinderNode*)obj))) xml->Code(stream);
			stream->EndTag("overlay");
		}
	
		count = m_inherits.Count();
		for (int32 i=0;i<count;i++) {
			stream->StartTag("inherit",m_inherits[i].attributes,stfCanAddWS);
			binder_node obj = m_inherits[i].node;
			if ((xml = dynamic_cast<BExpressible*>((BinderNode*)obj))) xml->Code(stream);
			stream->EndTag("inherit");
		}
	
		count = m_externalProperties.Count();
		for (int32 i=0;i<count;i++) {
			stream->StartTag("object",m_externalProperties[i].attributes,stfCanAddWS);
			stream->EndTag("object");
		}

		count = m_skels.Count();
		for (int32 i=0;i<count;i++) {
			map.MakeEmpty();
			map.Add("name",m_skels[i].key);
			stream->StartTag("skel",map,stfCanAddWS);
			if ((xml = dynamic_cast<BExpressible*>((BinderNode*)m_skels[i].value))) xml->Code(stream);
			stream->EndTag("skel");
		}

		// Transience
		//
		// If the current node is marked transient, we must write out
		// all prototypes and skeletons (done above) to our private
		// backing store so that they may be reconstituted later.
		// However, any *properties* of a transient node -- this includes
		// objects created from our (transient) skeleton -- must NOT be
		// written out to the XML file.  These are in-memory properties
		// only, and should not be reconstituted when the
		// XMLBinderNode is reloaded from the XML backing store.
		//
		// A nontransient object acts as you would expect (it writes
		// out all of its properties).
		
		if ( ! m_transient ) { // only write properties if we (and therefore they) are not transient
			count = m_propertyList.Count();
			for (int32 i=0;i<count;i++) {
				formatting = stfLeaf;
				uint16 perms = m_propertyList[i].perms;
				switch (m_propertyList[i].value.Type()) {
					case property::null   			: 	if (perms == (permsRead|permsMount)) {
															typeStr = "mountpoint"; perms = permsInherit;
														} else {
															typeStr = "undefined";
														}
														break;
					case property::object 			: 	if ((perms == (permsRead|permsMount)) && m_propertyList[i].value.IsRemoteObject()) {
															typeStr = "mountpoint"; perms = permsInherit;
														} else {
															typeStr = "object"; formatting = stfCanAddWS;
														}
														break;
					case property::string 			: 	typeStr = "string"; break;
					case property::number 			: 	typeStr = "number"; break;
					case property::remote_object	:	typeStr = "undefined"; break;
					case property::typed_raw		:	break; // unhandled ???
				}
		
				map.MakeEmpty();
				map.Add("name",m_propertyList[i].name);
		
				if (perms != permsInherit) {
					perms_to_string(perms,permStr);
					map.Add("perms",permStr);
				}
		
				stream->StartTag(typeStr,map,formatting);
				if (m_propertyList[i].value.Type() == property::object) {
					binder_node obj = m_propertyList[i].value.Object();
					if ((xml = dynamic_cast<BExpressible*>((BinderNode*)obj)))
						xml->Code(stream);
				} else if (	(m_propertyList[i].value.Type() != property::remote_object) &&
							!m_propertyList[i].value.IsUndefined() ) {
					BString s = m_propertyList[i].value.String();
					stream->TextData(s.String(),s.Length());
				}
				stream->EndTag(typeStr);
			}
		}
		m_listLock.Unlock();
	}

	
	return B_OK;
}

status_t 
XMLBinderNode::Parse(BParser **stream)
{
	*stream = new BinderParser(this);
	return B_OK;
}

status_t 
XMLBinderNode::SetParent(XMLBinderNode *parent)
{
	m_parent = parent;
	return B_OK;
}

XMLBinderNode *
XMLBinderNode::Parent()
{
	return m_parent;
}

XMLBinderNode::XMLBinderNode(XMLBinderNode *parent)
{
	m_parent = parent;
	m_saveTime = NODE_CLEAN;
	m_canStack = false;
	m_transient = false;
}

XMLBinderNode::~XMLBinderNode()
{
}

get_status_t 
XMLBinderNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (name) {
		switch (name[0]) {
			case '@': // internal property
				{
					char const *internal_cmd = &(name[1]);
					if (!strcmp(internal_cmd, "sync")) { // @sync : flush node and children to disk
						
						// Sync to disk.  We perform a preorder traversal of all
						// child properties.  Each child *object* is collected while
						// the list is locked.  (We would *copy* the bindernodes,
						// but they would lose their parent information, and there
						// isn't a virtual public BinderNode::SetParent() that can be
						// used to preserve that information.)  When all the children are
						// collected, @sync() them (binder recursion!) and delete.
						
						BString response("");
						
						XMLBinderNode *p = this;
						while ((p->m_pathname == "") && p->m_parent) p = p->m_parent;

						p->m_saveLock.Lock();
						if (p->m_pathname != "" && p->m_saveTime != NODE_CLEAN) { // our parent is dirty
							#if DEBUG
								printf("XMLBinderNode::ReadProperty: <<command:@sync>> ** syncing dirty XML to \"%s\" on request\n",
									p->m_pathname.String());
							#endif

							p->m_saveTime = NODE_CLEAN;
							
							status_t result;
							if ((result = p->Save()) == B_OK)
								response << "ok";
							else
								response << "err : sync error on \"" << p->m_pathname << "\": " << strerror(result) << "; ";
						} else {
							response << "ok";
						}
						p->m_saveLock.Unlock();

						SmartArray<binder_node> childObjects;

						m_listLock.Lock();
						int32 count = m_propertyList.Count();
						for (int32 i=0;i<count;i++) {
							if (m_propertyList[i].value.Type() == property::object) {
								childObjects.AddItem(m_propertyList[i].value.Object());
							}
						}
						m_listLock.Unlock();

						binder_node node;
						property result;
						count = childObjects.Count();
						for(int32 i=0; i<count; i++) {
							childObjects[i]->GetProperty("@sync", result);
							if (result != BinderNode::property::undefined
								&& (strncmp(result.String().String(), "ok", 2) != 0))
							{
								response << result.String(); // append child errors
							}
						}
						
						prop = response.String();
						
						return B_OK;
					}
				} break;
			case '+': // clone skeleton
				{
					binder_node node;
					if (m_skels.Count()) {
						if (name[1] == 0) node = m_skels[0].value;
						else node = m_skels.Lookup(name+1);
					}
					if (node) {
						prop = node->Copy(true);
			
						// TU: Make sure that new
						// nodes get their parent
						// set. Otherwise nothing
						// will be saved
						XMLBinderNode *xmlnode = dynamic_cast<XMLBinderNode *>((BinderNode *)prop.Object());
						if(xmlnode)
							xmlnode->SetParent(this);
			
						return B_OK;
					}
				} break;
		}
	}
	get_status_t r = BinderContainer::ReadProperty(name,prop,args);
	return r;
}

put_status_t 
XMLBinderNode::WriteProperty(const char *name, const property &prop)
{
	put_status_t r = BinderContainer::WriteProperty(name,prop);
	if ((r.error == B_OK) && !m_transient) MarkDirty();
	return r;
}

status_t 
XMLBinderNode::Load(BDataIO &file)
{
	BXMLDataIOSource source(&file);
	BDocumentParser parse(new BinderParser(this));
	return ParseXML(&parse,&source);
}

void create_path(BPath &path)
{
	BPath parent;
	path.GetParent(&parent);
	if (parent.InitCheck()) return;

	create_path(parent);

	BDirectory dir(parent.Path()),other;
	if (dir.InitCheck()) return;
	dir.CreateDirectory(path.Leaf(),&other);
}

status_t 
XMLBinderNode::Save()
{
	if (m_pathname == "") {
		if (!m_parent) {
			printf("Node with no backing store (prabably the root) tried to save!\n");
			return B_OK;
		}
		XMLBinderNode *p = this;
		while ((p->m_pathname == "") && p->m_parent) p = p->m_parent;
		return p->Save();
	}

	BPath path(m_pathname.String());
	create_path(path);

	status_t err;
	BString fn(m_pathname);
	fn << ".new";
	BFile file(fn.String(),O_RDWR|O_TRUNC|O_CREAT);
	if ((err = file.InitCheck())) return err;

	//writing the XML file (token by token) straight to a BFile is
	//_really_ slow, so we'll buffer it (note that, since 'buffer'
	//doesn't own the stream, we must ensure it is deleted before
	//'file', so we explicitly control its lifespan with {}s)
	{
		BBufferIO buffer(&file, BBufferIO::DEFAULT_BUF_SIZE, false);

		BOutputStream stream(buffer,true);
		BStringMap map;
		stream.StartTag("binder-object",map,stfCanAddWS);
		if ((err = InternalCode(&stream,true))) return err;
		stream.EndTag("binder-object");
	}

	//write the file
	file.Unset();

	return rename(fn.String(),m_pathname.String());
}

void 
XMLBinderNode::MarkDirty()
{
	XMLBinderNode *p = this;
	while ((p->m_pathname == "") && !p->m_transient && p->m_parent) p = p->m_parent;
	// We hit a transient node along the way up to the node with
	// backing store.  Therefore, we (as the WriteProperty call
	// or whatever invoked this method) aren't allowed to trigger
	// a dirty flag -- we're supposed to be transient!
//	#if DEBUG
		if (p->m_transient)
			printf("XMLBinderNode::MarkDirty: Node with transient ancestor tried to MarkDirty!\n");
//	#endif
	if (p->m_transient) return;

	p->m_saveLock.Lock();
	if (p->m_saveTime == NODE_CLEAN) {
		p->m_saveTime = system_time() + SAVE_TIMEOUT;
		p->PostMessageAtTime(BMessage('save'),p->m_saveTime);
	}
	p->m_saveLock.Unlock();
}

status_t 
XMLBinderNode::HandleMessage(BMessage *msg)
{
	if (msg->what == 'save') {
		m_saveLock.Lock();
		if (m_saveTime != NODE_CLEAN) {
			if (m_saveTime < system_time()) {
				m_saveTime = NODE_CLEAN;
				Save();
			} else {
				PostMessageAtTime(BMessage('save'),m_saveTime);
			}
		}
		m_saveLock.Unlock();
		return B_OK;
	}
	return B_ERROR;
}

BPath 
XMLBinderNode::Pathname()
{
	return BPath(m_pathname.String());
}

status_t 
XMLBinderNode::SetPathname(const char *pathname, bool load, bool save)
{
	printf("SetPathname('%s',%s,%s)\n",
		pathname,load?"true":"false",save?"true":"false");

	if (load) {
		BFile file(pathname,O_RDONLY);
		if (!file.InitCheck()) Load(file);
	}

	m_saveLock.Lock();
	m_pathname = pathname;
	if (save) Save();
	m_saveLock.Unlock();

	return B_OK;
}

XMLBinderNode::XMLBinderNode(XMLBinderNode *copyFrom, bool deepCopy) : BinderContainer(copyFrom,deepCopy)
{
	m_parent = NULL;
	m_saveTime = NODE_CLEAN;
	m_canStack = false;

	m_inherits.AssertSize(copyFrom->m_inherits.Count());
	for (int32 i=0;i<m_inherits.Count();i++)
		m_inherits[i] = copyFrom->m_inherits[i];

	m_overlays.AssertSize(copyFrom->m_overlays.Count());	
	for (int32 i=0;i<m_overlays.Count();i++)
		m_overlays[i] = copyFrom->m_overlays[i];

	m_externalProperties.AssertSize(copyFrom->m_externalProperties.Count());	
	for (int32 i=0;i<m_externalProperties.Count();i++)
		m_externalProperties[i] = copyFrom->m_externalProperties[i];

	m_skels.AssertSize(copyFrom->m_skels.Count());	
	for (int32 i=0;i<m_skels.Count();i++)
		m_skels[i] = copyFrom->m_skels[i];
		
	m_transient = copyFrom->m_transient;
}

binder_node XMLBinderNode::Copy(bool deep)
{
	return new XMLBinderNode(this,deep);
}

XMLBinderNode::XMLBinderNode(const char *pathname)
{
	m_parent = NULL;
	m_saveTime = NODE_CLEAN;
	m_canStack = false;
	m_transient = false;
	if (pathname) SetPathname(pathname,true);
}
