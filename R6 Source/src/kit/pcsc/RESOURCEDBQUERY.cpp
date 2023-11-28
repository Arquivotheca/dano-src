
#include <vector>

#include <Message.h>
#include <StorageKit.h>

#include <ResourceManager.h>
#include <ResourceDBQuery.h>


using std::vector;

const char *kReaderAndGroupsMimeType = "application/x-vnd.Be-meta-pcsc";


RESOURCEDBQUERY::RESOURCEDBQUERY(RESOURCEMANAGER *resmgr)
	: 	resmgr_(resmgr)
{
}


RESOURCEDBQUERY::~RESOURCEDBQUERY()
{
}

RESPONSECODE RESOURCEDBQUERY::ListReaderGroups(STR_LIST& Groups)
{
	BDirectory groups;
	status_t result = GetReadersDirectory(groups);
	if (result != B_OK)
		return result;
	
	BEntry a_group;
	while (groups.GetNextEntry(&a_group) == B_OK)
	{ // Entry must be a directory and a "application/x-vnd.Be-meta-pcsc"
		if (a_group.IsDirectory())
		{
			BString mime(B_EMPTY_STRING);
			GetEntryMimeType(a_group, mime);
			if (mime == BString(kReaderAndGroupsMimeType))
			{	
				char name[B_FILE_NAME_LENGTH]; 
				a_group.GetName(name);		
				Groups.AddItem(BString(name));
			}
		}
	}
	return SCARD_S_SUCCESS;
}

RESPONSECODE RESOURCEDBQUERY::ListReaders(const STR_LIST& Groups, STR_LIST& Readers)
{
	BDirectory readers;
	status_t result = GetReadersDirectory(readers);
	if (result != B_OK)
		return result;

	const int nbGroups = Groups.CountItems();	

	BEntry an_entry;
	while (readers.GetNextEntry(&an_entry) == B_OK)
	{ // Go through all entries
		char name[B_FILE_NAME_LENGTH]; 
		BString mime;
		if (	(GetEntryMimeType(an_entry, mime) == B_OK) &&
				(mime == BString(kReaderAndGroupsMimeType)) &&
				(an_entry.GetName(name) == B_OK))
		{ // Check mimetype and get entry's name
			if (an_entry.IsDirectory())
			{ // If this is a directory, check that it's in the list
				BString groupName(name);
				int i;
				for (i=0 ; i<nbGroups ; i++)
				{ // Group found
					if (Groups[i] == groupName)
						break;
				}
				
				if ((i<nbGroups) || (nbGroups == 0))
				{ // Group found in the list, -or- no list at all : fetch all readers in this group
					BEntry a_reader;
					BDirectory a_group(&an_entry);
					while (a_group.GetNextEntry(&a_reader) == B_OK)
					{ // Go through each entry in this group
						char name[B_FILE_NAME_LENGTH]; 
						BString mime;
						if (	(GetEntryMimeType(a_reader, mime) == B_OK) &&
								(mime == BString(kReaderAndGroupsMimeType)) &&
								(a_reader.GetName(name) == B_OK))
						{ // Check mimetype and get entry's name and add it to the list
							Readers.AddItem(BString(name));
						}
					}
				}
			}
			else if ((an_entry.IsFile()) && (nbGroups == 0))
			{ // Fetch all the readers
				Readers.AddItem(BString(name));
			}
		}
	}



	return SCARD_S_SUCCESS;
}

RESPONSECODE RESOURCEDBQUERY::ListCardTypes(const BYTE *ATR,
											const GUID_LIST& Interfaces,
											STR_LIST& CardTypes)
{
// What we do here is:
// ATR		Interface		Action
// NULL		!NULL			Card Types match interfaces
// !NULL	NULL			Card Types match ATR
// !NULL	!NULL			Card match both ATR and interfaces
// NULL		NULL			Card match nothing (All cards returned)

	vector<CARDINFO> allCards;
	RESPONSECODE result = ListCards(allCards);
	if (result == B_OK)
	{
		CardTypes.MakeEmpty();		

		// Test each card
		for (int i=0 ; i<allCards.size() ; i++)
		{
			bool match = true;

			if (Interfaces.IsEmpty() == false)
			{
				// Does this card supports all supplied interfaces list?
				for (int j=0 ; j<Interfaces.CountItems() ; j++)
				{
					if (HasInterface(allCards[i], Interfaces[j]) == false)
					{ // This card doesn't match, since it doesn't have one of the supplied GUID
						match = false;
						break;
					}
				}
				// An interface list was provided, and the card didn't matched
				if (match == false)
					continue;
			}

			// Does this card match the ATR ? 
			if ((ATR) && (ATR[0]))
			{ // ATR is provided
				for (int j=0 ; j<B_PCSC_MAX_ATR_SIZE ; j++)
				{ // ((A ^ B) & Mask) == 0, means that ATR matches
					if (((ATR[j]) ^ (allCards[i].ATRRefVal[j])) & (allCards[i].ATRMask[j]))
					{	// This card does not match!
						match = false;
						break;
					}
				}
				// An ATR was provided, and the card didn't matched
				if (match == false)
					continue;
			}

			if (match == true)
			{ // This card matches, store it then continue with the next card
				CardTypes.AddItem(allCards[i].CardName);
				continue;
			}
		}
	}
	return result;
}

RESPONSECODE RESOURCEDBQUERY::GetProviderId(const STR& CardName, STR& ProviderId)
{
	// Get the list of all known cards
	vector<CARDINFO> allCards;
	RESPONSECODE result = ListCards(allCards);
	if (result == B_OK)
	{
		for (int i=0 ; i<allCards.size() ; i++)
		{ // Look in the Cards 'database' for this card
			if (allCards[i].CardName == CardName)
			{ // copy the ProviderId
				ProviderId = allCards[i].ProviderId;
				return SCARD_S_SUCCESS;
			}
		}	
	}
	return SCARD_E_UNKNOWN_CARD;
}

RESPONSECODE RESOURCEDBQUERY::ListInterfaces(const STR& CardName, GUID_LIST& Interfaces)
{
	// Get the list of all known cards
	vector<CARDINFO> allCards;
	RESPONSECODE result = ListCards(allCards);
	if (result == B_OK)
	{
		for (int i=0 ; i<allCards.size() ; i++)
		{ // Look in the Cards 'database' for this card
			if (allCards[i].CardName == CardName)
			{ // copy the list of GUID
				Interfaces = allCards[i].Interfaces;
				return SCARD_S_SUCCESS;
			}
		}	
	}
	return SCARD_E_UNKNOWN_CARD;
}





bool RESOURCEDBQUERY::HasInterface(const CARDINFO& a_card, const GUID_t& interface)
{ // Return true if this card has the provided interface
	for (int i=0 ; i<a_card.Interfaces.CountItems() ; i++) {
		if (memcmp(a_card.Interfaces[i].guid, interface.guid, sizeof(interface.guid)) == 0)
			return true;
	}
	return false;
}

status_t RESOURCEDBQUERY::GetCardsFile(BFile& file, int32 openMode)
{
	status_t result;
	BPath dir;
	if ((result = find_directory(B_BEOS_ETC_DIRECTORY, &dir)) != B_OK)
		return result;

	BDirectory pcsc;
	BDirectory etc(dir.Path());

	result = etc.CreateDirectory("pcsc", &pcsc);
	if ((result != B_FILE_EXISTS) && (result != B_OK))
		return result;

	dir.Append("pcsc/__pcsc_data");
	file.SetTo(dir.Path(), openMode);
	return file.InitCheck();
}

status_t RESOURCEDBQUERY::GetReadersDirectory(BDirectory& readers)
{
	status_t result;
	BPath dir;
	if ((result = find_directory(B_BEOS_ETC_DIRECTORY, &dir)) != B_OK)
		return result;

	BDirectory pcsc;
	BDirectory etc(dir.Path());
	result = etc.CreateDirectory("pcsc", &pcsc);
	if ((result != B_FILE_EXISTS) && (result != B_OK))
		return result;

	dir.Append("pcsc");
	readers.SetTo(dir.Path());
	return readers.InitCheck();
}

status_t RESOURCEDBQUERY::GetEntryMimeType(BEntry& entry, BString& mime)
{
	char mimeType[256];
	BNode node(&entry);
	BNodeInfo nodeInfo(&node);

	status_t result = nodeInfo.GetType(mimeType);
	if (result != B_OK)
		return result;

	mime = BString(mimeType);
	return B_OK;
}

RESPONSECODE RESOURCEDBQUERY::ListCards(vector<CARDINFO>& AllCards)
{
	BMessage cards;
	BFile file;
	if ((GetCardsFile(file, B_READ_ONLY) == B_OK) && (cards.Unflatten(&file) == B_OK))
	{
		AllCards.clear();
		BMessage a_card_msg;
		int card_index = 0;
		status_t result;
		while ((result = cards.FindMessage("be:cards", card_index++, &a_card_msg)) == B_OK)
		{
			const char *CardName, *ProviderId;
			const BYTE *ATRRefVal, *ATRMask, *Interfaces;
			ssize_t numb1, numb2, numb3;

			result |= a_card_msg.FindString("be:CardName", &CardName);
			result |= a_card_msg.FindString("be:ProviderId", &ProviderId);
			result |= a_card_msg.FindData("be:ATRRefVal", B_INT8_TYPE, (const void **)&ATRRefVal, &numb1);
			result |= a_card_msg.FindData("be:ATRMask", B_INT8_TYPE, (const void **)&ATRMask, &numb2);
			if (result != B_OK)
				return result;

			CARDINFO a_card;
			a_card.CardName = STR(CardName);
			a_card.ProviderId = STR(ProviderId);
			memset(a_card.ATRRefVal, 0, B_PCSC_MAX_ATR_SIZE);
			memcpy(a_card.ATRRefVal, ATRRefVal, numb1);
			memset(a_card.ATRMask, 0, B_PCSC_MAX_ATR_SIZE);
			memcpy(a_card.ATRMask, ATRMask, numb2);
			a_card.ATRMaskLength = numb2;

			int index = 0;
			while ((a_card_msg.FindData("be:Interfaces", index++, B_INT8_TYPE, (const void **)&Interfaces, &numb3)) == B_OK)
			{ // Retrieve all the interfaces of this card
				GUID_t interface;
				memset(interface.guid, 0, sizeof(interface.guid));
				memcpy(interface.guid, Interfaces, numb3);
				a_card.Interfaces.AddItem(interface);
			}

			AllCards.push_back(a_card);
		}
	}
	return B_OK;
}


status_t RESOURCEDBQUERY::FindReader(const BString& readername, BEntry& entry)
{
	status_t result;
	BDirectory groups;
	if ((result = GetReadersDirectory(groups)) != B_OK)
		return result;

	// TODO: We don't handle reader groups yet.
	if ((result = groups.FindEntry(readername.String(), &entry, false)) != B_OK)
		return result;

	return B_OK;
}

