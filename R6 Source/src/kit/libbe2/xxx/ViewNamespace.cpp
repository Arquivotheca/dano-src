
#include <GroupNamespace.h>
#include <ResourceCache.h>

#define checkpoint printf("thid=%ld (%08lx) -- %s:%d -- %s\n",(int32)find_thread(NULL),(uint32)this,__FILE__,__LINE__,__PRETTY_FUNCTION__);

static const char *gehmlGroupNamespaceProps[] = {
	"baseURL",
	"resolveURL",
	"securityGroup",
	NULL
};

status_t 
GroupNamespace::OpenProperties(void **cookie, void *copyCookie)
{
	int32 *i = new int32;
	if (copyCookie) *i = *((int32*)copyCookie);
	else *i = 0;
	*cookie = i;
	return B_OK;
}

status_t 
GroupNamespace::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	int32 *i = (int32*)cookie;
	if (!gehmlGroupNamespaceProps[*i]) return ENOENT;
	strncpy(nameBuf,gehmlGroupNamespaceProps[*i],*len);
	nameBuf[*len - 1] = 0;
	return B_OK;
}

status_t 
GroupNamespace::CloseProperties(void *cookie)
{
	int32 *i = (int32*)cookie;
	delete i;
	return B_OK;
}

status_t 
GroupNamespace::WillRewritten(binder_node disgruntledAncestor, uint32 event, const char *name)
{
	checkpoint

	if (m_relURL.Length() &&
		(!name || ((event & B_VALUE_CHANGED) && name && !strcmp(name,"baseURL")))) {

		m_lock.Lock();
		property myBase = disgruntledAncestor->Property("baseURL");
		if (myBase.IsString()) {
			Wagner::URL url(myBase.String().String());
			m_base.SetTo(url,m_relURL.String());
		} else
			m_base.SetTo(m_relURL.String(),false);
		m_securityGroup = securityManager.GetGroupID(m_base);
		printf("urls2: %s --> %s\n",myBase.String().String(),m_relURL.String());
		m_base.PrintToStream();
		m_lock.Unlock();

		NotifyListeners(B_PROPERTY_CHANGED,"baseURL");
		NotifyListeners(B_PROPERTY_CHANGED,"securityGroup");
	}
	
	BinderContainer::WillRewritten(disgruntledAncestor,event,name);
}

put_status_t 
GroupNamespace::WriteProperty(const char *name, const property &prop)
{
	if (!strcmp(name,"baseURL")) {
		property myBase;

		m_lock.Lock();
		m_relURL = prop;
		BinderNode::ReadProperty("baseURL",myBase);
		if (myBase.IsString()) {
			Wagner::URL url(myBase.String().String());
			m_base.SetTo(url,m_relURL.String());
		} else
			m_base.SetTo(m_relURL.String(),false);
		printf("urls: %s --> %s\n",myBase.String().String(),m_relURL.String());
		m_base.PrintToStream();
		m_securityGroup = securityManager.GetGroupID(m_base);
		m_lock.Unlock();

		NotifyListeners(B_PROPERTY_CHANGED,"baseURL");
		NotifyListeners(B_PROPERTY_CHANGED,"securityGroup");
	} else
		return BinderContainer::WriteProperty(name,prop);

	return B_OK;
}

get_status_t 
GroupNamespace::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (m_base.IsValid() && !strcmp(name,"resolveURL")) {
		Wagner::URL url;
		char buf[1024];
		if (!args.Count()) return B_ERROR;
		url.SetTo(m_base,args[0].String().String());
		url.GetString(buf,1024);
		prop = buf;
	} else if (!strcmp(name,"loadURL")) {
		printf("base "); m_base.PrintToStream();
		get_status_t gst;

		m_lock.Lock();
		if (!m_base.IsValid()) {
			m_lock.Unlock();
			gst = BinderNode::ReadProperty(name,prop,args);
			if (gst.error != ENOENT) {
//				debugger("woohoo1");
				return gst;
			}
			m_lock.Lock();
		}

		char buf[1024];
		Wagner::URL url;
		if (args.Count() < 3) {
			m_lock.Unlock();
//			debugger("woohoo3");
			return B_ERROR;
		}

		if (!m_base.IsValid()) url.SetTo(args[1].String().String(),false);
		else url.SetTo(m_base,args[1].String().String());
		if (!url.IsValid()) {
			m_lock.Unlock();
//			debugger("woohoo2");
			return B_ERROR;
		}
		Wagner::GroupID securityGroup = m_securityGroup;

		m_lock.Unlock();

		int32 id = (int32)args[2].Number();
		GHandler *handler = dynamic_cast<GHandler*>((BinderNode*)args[0].Object());
		printf("loading ");url.PrintToStream();
		Wagner::resourceCache.NewContentInstance(url,id,handler,0,BMessage(),securityGroup,NULL,(const char*)NULL);
		sprintf(buf,"%08x",handler);
		prop = buf;
	} else if (m_base.IsValid() && !strcmp(name,"baseURL")) {
		char buf[1024];
		m_base.GetString(buf,1024);
		prop = buf;
	} else if (m_base.IsValid() && !strcmp(name,"securityGroup")) {
		prop = m_securityGroup;
	} else
		return BinderContainer::ReadProperty(name,prop,args);
		
	return B_OK;
}
