/*

Entity store is basically a cache of Entities and DTDs.  The idea is that the
Parser Context objects are disposable.  You use one per document that you parse.
If the threading gets more complicated, then you can use an EntityStore, which is
threadsafe (meaning you can access it by many threads without worrying about
unsafe things).

*/

#include <textencoding2/TextEncoding.h>
#include <xml2/BContent.h>
#include <xml2/BDataSource.h>
#include <xml2/BParser.h>
#include <xml2/BEntityParseContext.h>
#include <storage2/Path.h>
#include <storage2/File.h>
#include <storage2/FindDirectory.h>
#include <support2/PositionIO.h>

#include <stdio.h>
#include <parsing.h>

//#define DEBUG 1
//#include <Debug.h>

namespace B {
namespace XML {

using namespace Storage2;

// =====================================================================
BEntityStore::BEntityStore(uint32 parseFlags)
	:_parseFlags(parseFlags)
{
	
}
 

// =====================================================================
BEntityStore::~BEntityStore()
{
	
}


// =====================================================================
status_t
BEntityStore::FetchExternalDTD(const BString & publicID, const BString & systemID,
					BXMLObjectFactory * factory, BDocumentType * dt,
					uint32 parseFlags, BDocumentTypeDefinition * dtd)
{
	status_t err;
	BEntityDecl * decl;
	
	err = FetchEntity(publicID, systemID, &decl, true, factory, parseFlags, dt);
	if (err != B_OK)
		return err;
	
	// Transfer all of the objects from the entity to the dtd
	BVector<BXMLObject *> & set = decl->GetReplacementContent();
	int32 count = set.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BXMLObject * obj = set.ItemAt(0);
		BElementDecl * ed = dynamic_cast<BElementDecl *>(obj);
		BAttributeDecl * ad = dynamic_cast<BAttributeDecl *>(obj);
		BEntityDecl * yd = dynamic_cast<BEntityDecl *>(obj);
		if (ed)
		{
			err = dtd->AddElementDecl(ed);
			if (err != B_OK)
			{
				delete decl;
				return err;
			}
		}
		else if (ad)
		{
			err = dtd->AddAttributeDecl(ad);
			if (err != B_OK)
			{
				delete decl;
				return err;
			}
		}
		else if (yd)
		{
			err = dtd->AddEntityDecl(yd);
			if (err != B_OK)
			{
				delete decl;
				return err;
			}
		}
	}
	delete decl;
	return B_NO_ERROR;
	
	
}


// =====================================================================
status_t
BEntityStore::FetchEntity(const BString & publicID, const BString & systemID,
					BEntityDecl ** returnDecl, bool parameter,
					BXMLObjectFactory * factory, uint32 parseFlags,
					BDocumentType * dt)
{
	printf("ENTER: BEntityStore::FetchEntity\n");
	status_t err;
	
	BEntityDecl * decl = factory->EntityDeclFactory("");
	
	decl->SetType(B_XML_PARSED_ENTITY);
	decl->SetScope(parameter ? B_XML_PARAMETER_ENTITY : B_XML_GENERAL_ENTITY);
	decl->SetStorage(B_XML_EXTERNAL_ENTITY);
	
	// See if we can find the resource
	IByteInput::ptr file;
	err = go_get_document(publicID, systemID, &file);
	if (err != B_OK || (file == NULL))
	{
		delete decl;
		return err;
	}
	
	BXMLEntityParseContext context(decl, parameter, dt, factory, this);
	BXMLIByteInputSource source(file);
	
	err = _do_the_parsing_yo_(&source, &context, parameter, parseFlags);
	if (err != B_OK)
	{
		delete decl;
		return err;
	}
	
	*returnDecl = decl;
	
	printf("EXIT: BEntityStore::FetchEntity\n");
	return B_OK;
}


// =====================================================================
status_t
BEntityStore::go_get_document(const BString & publicID, const BString & systemID,
								IByteInput::ptr *dataIO)
{
	status_t err;
	
	// See if we have it in our directories first
	if (publicID != "")
	{
		err = try_public_id(publicID, dataIO);
		if (err == B_OK)
			return B_OK;
	}
	
	// Okay, we don't, so try go looking for it with the systemID
	err = try_system_id(systemID, dataIO);
	if (err != B_OK)
		return err;
	
	return B_OK;
}


// =====================================================================
status_t
BEntityStore::try_public_id(const BString & publicID, IByteInput::ptr *dataIO)
{
	status_t err;
	BPath	path;
	BFile::ptr file = new BFile();
	*dataIO = new BPositionIO(file);
	
	// Convert the publicID to the disk filename.  The difference is that
	// disk paths replace / with B_UTF8_ELLIPSIS.  This essentially means
	// that publicIDs could conflict if one had an elipsis in it.  Hoepfully
	// that's rare enough that we'll all be okay.
	BString filename = publicID;
	filename.ReplaceAll("/", B_UTF8_ELLIPSIS);
	
	// First, look in the /boot/beos/etc/xml_dtds directory
	err = find_directory(B_BEOS_ETC_DIRECTORY, &path);
	if (err != B_OK)
		goto TRY_USER_DIR;
	path.Append(B_XML_DTD_DIRECTORY);
	path.Append(filename.String());
	
	err = file->SetTo(path.Path(), B_READ_ONLY);
	if (err != B_OK)
		goto TRY_USER_DIR;
	return B_OK;
	
TRY_USER_DIR:

	file->Unset();
	
	// Next, look in the /boot/home/config/xml_dtds directory
	err = find_directory(B_USER_CONFIG_DIRECTORY, &path);
	if (err != B_OK)
		goto GIVE_UP;
	path.Append(B_XML_DTD_DIRECTORY);
	path.Append(filename.String());
	
	err = file->SetTo(path.Path(), B_READ_ONLY);
	if (err != B_OK)
		goto GIVE_UP;
	return B_OK;
	
GIVE_UP:
	// Only reach here on error condition
	*dataIO = NULL;
	return B_ERROR;
}


// =====================================================================
status_t
BEntityStore::GetNamespaceDTD(const BString & nsName,
							BXMLObjectFactory * factory, uint32 parseFlags,
							BDocumentTypeDefinition ** dtd)
{
	*dtd = NULL;
	
	status_t err;
	IByteInput::ptr data;
	
	err = try_public_id(nsName, &data);
	if (err != B_OK)
		return err;
	
	// We're looking for a DTD
	BString dtdName(nsName);
	BDocumentTypeDefinition * d = factory->DTDFactory(dtdName);
	BXMLEntityParseContext context(NULL, true, NULL, factory, this, d);
	BXMLIByteInputSource source(data);
	
	err = _do_the_parsing_yo_(&source, &context, true, parseFlags);
	if (err != B_OK)
	{
		delete d;
		return err;
	}
	
	*dtd = d;
	
	return B_OK;
}


// =====================================================================
status_t
BEntityStore::try_system_id(const BString & systemID, IByteInput::ptr *dataIO)
{
	(void) systemID;
	(void) dataIO;
	return B_ERROR;
}



}; // namespace XML
}; // namespace B

