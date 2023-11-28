/*
	MailCacheNode.h
*/
#ifndef MAILCACHE_NODE_H
#define MAILCACHE_NODE_H

#include <Binder.h>
#include <String.h>
#include <List.h>

enum update_type {
	kAddEntryType = 0,
	kTouchEntryType,
	kContainsEntryType,
	kRemoveEntryType,
	kResizeEntryType,
	kRemoveMessagesType
};

class CacheContainer;

class MailCacheNode : public XMLBinderNode
{
	public:
								MailCacheNode();
								~MailCacheNode();
								// GHandler virtuals
		virtual status_t		HandleMessage(BMessage *message);
								// BinderContainer virtuals
		virtual	put_status_t	WriteProperty(const char *name, const property &prop);
		virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
								// BXmlKit::BExpressible virtuals
		virtual status_t		Parse(B::XML::BParser **stream);
								// MailCacheNode calls
		status_t				HandleCacheUpdate(const property_list &args, update_type type);
		status_t				SaveContainers();
		void					BuildContainerList();
		void					AddContainer(const char *username);
		void					RemoveContainer(const char *username);
		CacheContainer *		FindContainer(const char *username);
		
		off_t					CalculateCurrentCacheSize();
		void					SetContainerMaximumSize(int32 size);
		void					PrintToStream(const char *username = NULL);
		
	private:

		BList fList;
		bool fDirty;
		BList fContainers;
		off_t fMaxContainerSize;
};

class MailCacheXMLParser : public BParser
{
	public:
								MailCacheXMLParser(MailCacheNode *node);
								
		virtual status_t		StartTag(BString &name, BStringMap &attributes, BParser **newParser);
		virtual status_t		EndTag(BString &name);
		virtual status_t		TextData(const char	* data, int32 size);
	
	private:
		MailCacheNode *fNode;
		BString fLastTagName;
};

#endif
