/*
	MailStatusCodes.h
*/
#ifndef MAIL_STATUS_CODES_H
#define MAIL_STATUS_CODES_H
#include <SupportDefs.h>

const int32 kNumberOfCodes = 12;

enum MailStatusCodes {
	kStatusIdle					= 0x00000000,
	kStatusConnectingToServer 	= 0x00000001, 
	kStatusAuthenticating 		= 0x00000002,
	kStatusSelectingMailbox		= 0x00000004,
	kStatusSavingSummaryFile 	= 0x00000008, 
	kStatusLoadingSummaryFile	= 0x00000010,
	kStatusSyncingMailbox		= 0x00000020,
	kStatusExpungingMailbox		= 0x00000040,
	kStatusFetchingSection		= 0x00000080,
	kStatusSendingMessage		= 0x00000100,
	kStatusUploadingMessage		= 0x00000200,
	kStatusSyncOperation		= 0x00000400,
	kStatusDownloadingUnread	= 0x00000800
//	kUndefined		 			= 0x00001000
//	kUndefined		 			= 0x00002000
//	kUndefined		 			= 0x00004000
//	kUndefined		 			= 0x00008000
//	kUndefined		 			= 0x00010000
//	kUndefined		 			= 0x00020000
//	kUndefined		 			= 0x00040000
//	kUndefined		 			= 0x00080000
//	kUndefined		 			= 0x00100000
//	kUndefined		 			= 0x00200000
//	kUndefined		 			= 0x00400000
//	kUndefined		 			= 0x00800000
//	kUndefined		 			= 0x01000000
//	kUndefined		 			= 0x02000000
//	kUndefined		 			= 0x04000000
//	kUndefined		 			= 0x08000000
//	kUndefined		 			= 0x10000000
//	kUndefined		 			= 0x20000000
//	kUndefined		 			= 0x40000000
//	kUndefined		 			= 0x80000000
};

extern const char *kStatusStrings[kNumberOfCodes];

class MailStatusProxy {
	public:
								MailStatusProxy();
								MailStatusProxy(uint32 status);
								~MailStatusProxy();
	
		void					SetStatus(uint32 status);
		
	private:
								// Do not implement
								MailStatusProxy(const MailStatusProxy &inStatus);
		MailStatusProxy &		operator=(const MailStatusProxy &inStatus);
		
		uint32 fStatus;
};

const char *StatusCodeToString(uint32 statusCode);

#endif

// End of MailStatusCodes.h
