/******************************************************************************
/
/	File:			ResourceDBQuery.h
/
/	Description:	PC/SC Defined types.
/
/	Copyright 1993-2000, Be Incorporated
/
******************************************************************************/

#ifndef RESOURCEDBQUERY_H
#define RESOURCEDBQUERY_H

#include <vector>

#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <File.h>

#include <SmartCardDefs.h>
#include <ResourceManager.h>


class RESOURCEDB;

class RESOURCEDBQUERY
{
public:

				RESOURCEDBQUERY(RESOURCEMANAGER *resmgr);
	virtual		~RESOURCEDBQUERY();

	RESPONSECODE	ListReaderGroups	(	STR_LIST& Groups);

	RESPONSECODE	ListReaders			(	const STR_LIST& Groups,
											STR_LIST& Readers);

	RESPONSECODE	ListCardTypes		(	const BYTE *ATR,
											const GUID_LIST& Interfaces,
											STR_LIST& CardTypes);

	RESPONSECODE	GetProviderId		(	const STR& CardName,
											STR& ProviderId);

	RESPONSECODE	ListInterfaces		(	const STR& CardName,
											GUID_LIST& Interfaces);

protected:
	friend class READER;
	struct CARDINFO
	{
		STR 		CardName;
		GUID_LIST	Interfaces;
		BYTE		ATRRefVal[B_PCSC_MAX_ATR_SIZE];
		BYTE		ATRMask[B_PCSC_MAX_ATR_SIZE];
		BYTE		ATRMaskLength;
		STR			ProviderId;
	};

	static status_t GetCardsFile(BFile& file, int32 openMode);
	static status_t GetReadersDirectory(BDirectory& readers);
	static status_t GetEntryMimeType(BEntry& entry, BString& mime);
	static status_t FindReader(const BString& readername, BEntry& entry);

private:
	RESOURCEDBQUERY();
	RESOURCEDBQUERY(const RESOURCEDBQUERY&);
	RESOURCEDBQUERY& operator=(const RESOURCEDBQUERY&);
 
 	RESOURCEMANAGER *resmgr_;
	bool HasInterface(const CARDINFO& a_card, const GUID_t& interface);
	RESPONSECODE ListCards(std::vector<CARDINFO>& AllCards);
	uint32 _reserved_RESOURCEDBQUERY_[16];
};

#endif
