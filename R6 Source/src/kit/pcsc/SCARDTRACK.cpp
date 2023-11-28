
#include <OS.h>
#include <stdio.h>

#include <ResourceManager.h>
#include <ResourceDBQuery.h>
#include <SmartCardTrack.h>
#include <SmartCardComm.h>

#include "Reader.h"

using std::vector;

SCARDTRACK::SCARDTRACK(RESOURCEMANAGER *resmgr)
	: resmgr_(resmgr)
{
}

SCARDTRACK::~SCARDTRACK()
{
}

RESPONSECODE SCARDTRACK::LocateCards(	const STR_LIST& Cards,
										SCARD_READERSTATE_LIST& ReaderStates)
{
	return LocateCards(Cards, ReaderStates, true);
}



// Private Version of this function may or may not perfom the ATR matching
RESPONSECODE SCARDTRACK::LocateCards(	const STR_LIST& Cards,
										SCARD_READERSTATE_LIST& ReaderStates,
										bool try_atr_match)
{
	// Establish a connexion with the Smart Card of each reader of interrest
	SCARDCOMM scard(resmgr_);
	
	for (int i=0 ; i<ReaderStates.size() ; i++)
	{
		// Clear the output flags
		ReaderStates[i].CurrentState &= ~(SCARD_STATE_CHANGED);

		if (ReaderStates[i].CurrentState & SCARD_STATE_IGNORE)
		{ // User is not intersted in this reader
			ReaderStates[i].EventState = SCARD_STATE_IGNORE; 
			continue;
		}
		else if (ReaderStates[i].CurrentState == SCARD_STATE_UNAWARE)
		{ // User know nothing about this card and would like to know
			ReaderStates[i].EventState = SCARD_STATE_UNAWARE; 
		}


		// Default state is SCARD_UNKNOWN
		DWORD state = SCARD_UNKNOWN;
		BYTE AnswerToReset[B_PCSC_MAX_ATR_SIZE];
		RESPONSECODE result;

		// We need to connect to the card, because we'll need the ATR
		DWORD protocol_dummy;
		result = scard.Connect(	ReaderStates[i].Reader,
								SCARD_SHARE_SHARED,
								SCARD_PROTOCOL_T0,
								&protocol_dummy);
		
		switch (result)
		{
			case SCARD_S_SUCCESS:
				if (try_atr_match)
					scard.Status(NULL, &state, NULL, AnswerToReset);
				break;

			case SCARD_W_REMOVED_CARD:
			case SCARD_E_NO_SMARTCARD:
				state = SCARD_ABSENT;
				break;

			case SCARD_W_UNPOWERED_CARD:
			case SCARD_W_UNRESPONSIVE_CARD:
			case SCARD_W_UNUPPORTED_CARD:
				state = SCARD_PRESENT;
				try_atr_match = false; // We can't try ATR matching
				break;

			case SCARD_E_SHARING_VIOLATION:
				ReaderStates[i].EventState = SCARD_STATE_EXCLUSIVE;
				goto not_connected;

			case SCARD_E_READER_UNAVAILLABLE: // The reader exists, but we couldn't find the driver
			case SCARD_E_UNKNOWN_READER:
				ReaderStates[i].EventState = SCARD_STATE_UNKNOWN;
				return result;
		}
 
		switch (state)
		{
			case SCARD_ABSENT:
				ReaderStates[i].EventState = SCARD_STATE_EMPTY;
				break;

			case SCARD_PRESENT:
			case SCARD_SWALLOWED:
			case SCARD_POWERED:
			case SCARD_NEGOTIABLE:
			case SCARD_SPECIFIC:
				ReaderStates[i].EventState = SCARD_STATE_PRESENT;

				if (try_atr_match)
				{ // Find the cards that matches this reader's ATR
					GUID_LIST Interfaces(0); // We want to match on ATR only
					STR_LIST CardTypes;
					RESOURCEDBQUERY query(resmgr_);	// We need a RESOURCEDBQUERY to find the cards

					if (query.ListCardTypes(AnswerToReset, Interfaces, CardTypes) == SCARD_S_SUCCESS)
					{						
						if ((Cards.CountItems() == 0) && (CardTypes.CountItems() > 0))
						{ // User didn't provided a Cards list, so considere it as a Jocker
							ReaderStates[i].EventState |= SCARD_STATE_ATRMATCH;
						}
						else
						{ // Must verify that the card that matched is in the user's list
							for (int j=0 ; j<Cards.CountItems() ; j++)
							{
								for (int k=0 ; k<CardTypes.CountItems() ; k++)
								{
									if (Cards[j] == CardTypes[k])
									{
										ReaderStates[i].EventState |= SCARD_STATE_ATRMATCH;
										// What's strange here, is that we don't store the name of the CardType!!
										goto card_found;
									}
								}
							}
						}
					}
				}				
				break;

			default:
			case SCARD_UNKNOWN:
				ReaderStates[i].EventState = SCARD_STATE_UNAVAILABLE;
				break;
		}

card_found:
		// Disconnect from the card
		scard.Disconnect(SCARD_LEAVE_CARD);

not_connected:
		// Verify if the state has changed
		if ((ReaderStates[i].CurrentState & SCARD_STATE_ALL) != (ReaderStates[i].EventState & SCARD_STATE_ALL))
			ReaderStates[i].EventState |= SCARD_STATE_CHANGED;
		
		// If there is a state change, we must return
		if (ReaderStates[i].EventState & SCARD_STATE_CHANGED)
			break;
	}
	return SCARD_S_SUCCESS;
}


RESPONSECODE SCARDTRACK::GetStatusChange(	SCARD_READERSTATE_LIST& ReaderStates,
											DWORD msTimeout)
{
	status_t sysresult;
	RESPONSECODE result;
	STR_LIST Cards(0);
	bigtime_t endtime = system_time() + (msTimeout*1000);
	bigtime_t wait = 1000; // start with 1 ms interval
	fCancelSem = create_sem(1, "SCARDTRACK::GetStatusChange()");
	
	/*
	** Open all driver of the readers of interrest
	*/
	
	// reader list of interrest
	vector<active_reader_t> readers;

	for (int i=0 ; i<ReaderStates.size() ; i++)
	{ // Open each reader of interest
		active_reader_t a_reader;
		if (ReaderStates[i].CurrentState & SCARD_STATE_IGNORE)
		{ // User is not intersted in this reader, don't open it
			a_reader.result = SCARD_S_SUCCESS;
			a_reader.reader = NULL;
		}
		else
		{ // Open this reader
			a_reader.result = resmgr_->GetReader(ReaderStates[i].Reader, 0, &(a_reader.reader));
		}
		readers.push_back(a_reader);

		// The unknown reader case is special, we must exit

		if ((a_reader.result == SCARD_E_READER_UNAVAILLABLE) ||
			(a_reader.result == SCARD_E_UNKNOWN_READER))
		{ // In these cases, we must exit immidiately with an error and SCARD_STATE_UNKNOWN
			ReaderStates[i].EventState = SCARD_STATE_UNKNOWN;
			goto exit;
		}
	}

	/*
	** Test state of each reader
	*/

	do
	{
		for (int i=0 ; i<ReaderStates.size() ; i++)
		{
			// Clear the output flags
			ReaderStates[i].CurrentState &= ~(SCARD_STATE_CHANGED);

			if (ReaderStates[i].CurrentState & SCARD_STATE_IGNORE)
			{ // User is not intersted in this reader
				ReaderStates[i].EventState = SCARD_STATE_IGNORE; 
				continue;
			}
			else if (ReaderStates[i].CurrentState == SCARD_STATE_UNAWARE)
			{ // User know nothing about this card and would like to know
				ReaderStates[i].EventState = SCARD_STATE_UNAWARE; 
			}

			// Default state is SCARD_UNKNOWN
			DWORD state = SCARD_UNKNOWN;
			result = readers[i].result;

			if (result == SCARD_S_SUCCESS)
			{ // Is the card in the reader ?
				BYTE iccPresence = 0;
				result = readers[i].reader->IFD_get_capabilities(TAG_IFD_ICC_PRESENCE, &iccPresence);
				state = ((iccPresence) ? (SCARD_PRESENT) : (SCARD_ABSENT));
			}

			switch (result)
			{
				case SCARD_S_SUCCESS:
					break; // Nothing to do
	
				case SCARD_W_REMOVED_CARD:
				case SCARD_E_NO_SMARTCARD:
					state = SCARD_ABSENT;
					break;
	
				case SCARD_W_UNPOWERED_CARD:
				case SCARD_W_UNRESPONSIVE_CARD:
				case SCARD_W_UNUPPORTED_CARD:
					state = SCARD_PRESENT;
					break;
	
				case SCARD_E_SHARING_VIOLATION:
					ReaderStates[i].EventState = SCARD_STATE_EXCLUSIVE;
					result = SCARD_S_SUCCESS;
					goto not_connected;	
			}
 
			// Now we considere the success of the operation
			result = SCARD_S_SUCCESS;

			switch (state)
			{
				case SCARD_ABSENT:
					ReaderStates[i].EventState = SCARD_STATE_EMPTY;
					break;
	
				case SCARD_PRESENT:
				case SCARD_SWALLOWED:
				case SCARD_POWERED:
				case SCARD_NEGOTIABLE:
				case SCARD_SPECIFIC:
					ReaderStates[i].EventState = SCARD_STATE_PRESENT;
					break;
	
				default:
				case SCARD_UNKNOWN:
					ReaderStates[i].EventState = SCARD_STATE_UNAVAILABLE;
					break;
			}

not_connected:

			// Verify if the state has changed
			if ((ReaderStates[i].CurrentState & SCARD_STATE_ALL) != (ReaderStates[i].EventState & SCARD_STATE_ALL))
				ReaderStates[i].EventState |= SCARD_STATE_CHANGED;
			
			// If there is a state change, we must return
			if (ReaderStates[i].EventState & SCARD_STATE_CHANGED)
				break;
		}

		// -------------------------------------------------------
		// Process the result and timeout stuffs		


		// If timeout is NULL or if there is an error
		// we have to return immediately and report the error code
		if ((result != SCARD_S_SUCCESS) || (msTimeout == 0))
			goto exit;
		
		for (int i=0 ; i<ReaderStates.size() ; i++)
		{ // Figure out if the state of one of the reader has changed
			if (ReaderStates[i].EventState & SCARD_STATE_CHANGED)
				goto exit;
		}

		sysresult = acquire_sem_etc(fCancelSem, 1, B_RELATIVE_TIMEOUT, wait);
		if (sysresult == B_BAD_SEM_ID)
		{ // here, the sem has been deleted
			result = SCARD_E_CANCELLED;
			goto exit;
		}
		else if (sysresult == B_INTERRUPTED)
		{ // The sem is not deleted here, but the system interrupted the operation
			result = SCARD_E_SYSTEM_CANCELLED;
			goto exit;
		}
		
		// Timeout expired, wait a little more next time
		wait *= 2;
		if (wait > 1000000)
			wait = 1000000;
	} while ((system_time() < endtime) || (msTimeout == INFINITE));

	result = SCARD_E_TIMEOUT;


exit:
	delete_sem(fCancelSem);

	for (int i=0 ; i<readers.size() ; i++)
	{ // Close each opened reader
		if (readers[i].reader)
			resmgr_->ReleaseReader(readers[i].reader);
	}

	return result;
}


RESPONSECODE SCARDTRACK::Cancel()
{
	delete_sem(fCancelSem);
	return SCARD_S_SUCCESS;
}

