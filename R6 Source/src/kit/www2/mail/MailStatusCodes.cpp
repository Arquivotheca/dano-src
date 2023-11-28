/*
	MailStatusCodes.cpp
*/
#include "MailStatusCodes.h"
#include "PostOffice.h"
#include "MailDebug.h"

using namespace Wagner;

// --------------------- MailStatusProxy ---------------------

MailStatusProxy::MailStatusProxy()
	:	fStatus(0)
{

}

MailStatusProxy::MailStatusProxy(uint32 status)
	:	fStatus(status)
{
	MDB(MailDebug md);
	DB(md.Print("Adding status code '%s'\n", StatusCodeToString(fStatus)));
	PostOffice::MailMan()->AddStatus(fStatus);
}

MailStatusProxy::~MailStatusProxy()
{
	MDB(MailDebug md);
	if (fStatus != 0) {
		DB(md.Print("Removing status code '%s'\n", StatusCodeToString(fStatus)));
		PostOffice::MailMan()->RemoveStatus(fStatus);
	}
}

void MailStatusProxy::SetStatus(uint32 status)
{
	if (fStatus != 0)
		PostOffice::MailMan()->RemoveStatus(fStatus);

	fStatus = status;
	PostOffice::MailMan()->AddStatus(fStatus);
}

// --------------------------------------------------------

const char *kStatusStrings[kNumberOfCodes] = {
	"0",							// Connecting to Server
	"1",							// Authenticating with server
	"2",							// Selecting mailbox on server
	"3",							// Saving summary file
	"4",							// Loading summary file
	"5",							// Syncing mailbox
	"6",							// Expunging mailbox
	"7",							// Fetching section
	"8",							// Sending message
	"9",							// Uploading message
	"10",							// The entire "sync" operation
	"11"							// Download unread message bodies to cache
};

const char *StatusCodeToString(uint32 statusCode)
{
	switch (statusCode) {
		case kStatusConnectingToServer:			return kStatusStrings[0];
		case kStatusAuthenticating:				return kStatusStrings[1];
		case kStatusSelectingMailbox:			return kStatusStrings[2];
		case kStatusSavingSummaryFile:			return kStatusStrings[3];
		case kStatusLoadingSummaryFile:			return kStatusStrings[4];
		case kStatusSyncingMailbox:				return kStatusStrings[5];
		case kStatusExpungingMailbox:			return kStatusStrings[6];
		case kStatusFetchingSection:			return kStatusStrings[7];
		case kStatusSendingMessage:				return kStatusStrings[8];
		case kStatusUploadingMessage:			return kStatusStrings[9];
		case kStatusSyncOperation:				return kStatusStrings[10];
		case kStatusDownloadingUnread:			return kStatusStrings[11];
		default:								break;
	}
	return "";
}

// End of MailStatusCodes.cpp
