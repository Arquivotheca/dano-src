/*
	MimeMessage.h
*/
#ifndef _MIME_MESSAGE_H
#define _MIME_MESSAGE_H
#include <List.h>
#include <String.h>
#include <xml/BWriter.h>

using namespace B::XML;

enum {
	kMessageAnswered = 1,
	kMessageFlagged = 2,
	kMessageDeleted = 4,
	kMessageSeen = 8,
	kMessageDraft = 16,
	kMessageRecent = 32
};

struct MessagePart {
	MessagePart();
	MessagePart(const MessagePart &inPart);
	MessagePart &operator=(const MessagePart &inPart);
	~MessagePart();
	
	// Message Info
	BString type;
	BString id;
	BString characterSet;
	BString encoding;
	BString name;
	BString contentID;
	int size;
	int32 startOffset;
	
	// Container info
	bool isContainer;
	enum ContainerType {
		kNotContainer = 0,
		kMixed,
		kAlternative,
		kRelated
	} containerType;

	enum DispositionType {
		kNoDisposition = 0,
		kInline = 1,
		kAttached = 2
	} disposition;
	
	BList subParts;
	void Print();
	
	static uint32 CharsetConversion(BString &charset);
};

class MimeMessage {
	public:
								MimeMessage();
								MimeMessage(const MimeMessage &);
								~MimeMessage();
		MimeMessage &			operator=(const MimeMessage &inMessage);
		
		void					AcquireReference();
		void					ReleaseReference();

		void 					CopySubParts(const MessagePart *source, MessagePart *destination);
		const MessagePart *		GetRoot() const;
		MessagePart * 			GetMainDoc(const char *preferredType);
		MessagePart * 			FindPart(const char *name);
		MessagePart * 			FindPartByContentID(const char *contentID);

		const char * 			GetContentType() const;
		const char * 			GetMainPartID() const;
		const char * 			GetMailbox() const;
		const char * 			GetUid() const;
		const char * 			GetDate() const;
		const char * 			GetSubject() const;
		const char * 			GetFrom() const;
		const char * 			GetFromName() const;
		const char * 			GetSender() const;
		const char * 			GetSenderName() const;
		const char * 			GetRecipient() const;
		const char * 			GetRecipientName() const;
		const char * 			GetCc() const;
		const char *			GetCcName() const;
		const char * 			GetReplyTo() const;
		const char * 			GetReplyToName() const;
		const char * 			GetInReplyTo() const;
	
		int 					GetSize() const;
		uint32 					GetFlags() const;
		const char *			GetMessageID() const;
		bool 					IsMultipartMixed() const;

		void 					SetRoot(MessagePart*);
		void 					SetContentType(const char *);
		void 					SetMainPartID(const char *);
		void 					SetMailbox(const char*);
		void 					SetUid(const char *);	
		void 					SetDate(const char*);
		void 					SetSubject(const char *);
		void 					AddFrom(const char*);
		void 					AddFromName(const char*);
		void 					AddSender(const char *);
		void 					AddSenderName(const char *);
		void 					AddRecipient(const char *);
		void					SetRecipient(const char *);
		void 					AddRecipientName(const char *);
		void 					AddCc(const char *);
		void 					AddCcName(const char *);
		void 					AddReplyTo(const char *);
		void 					AddReplyToName(const char *);
		void 					SetInReplyTo(const char *);
		void 					SetSize(int size);
		void 					SetFlags(uint32);
		void 					SetMessageID(const char *);

		status_t 				WriteToStream(BDataIO *stream);
		status_t				WriteToXmlStream(B::XML::BCodifier *stream, bool isDirty);
		status_t 				WritePartsToStream(BDataIO *stream, MessagePart *part);
		status_t				WritePartsToXmlStream(B::XML::BCodifier *stream, MessagePart *part);
		
		status_t 				LoadFromBuffer(BDataIO &stream);
		status_t 				LoadPartsFromBuffer(BDataIO &stream, MessagePart *part);
		inline void				LoadField(BDataIO &stream, BString &field);
		inline void 			ExtractPart(BDataIO &stream, MessagePart *part);
		
		void Print(bool deep=false);

	private:
		void 					DumpPart(MessagePart *part, int depth, bool deep);
		MessagePart* 			SearchForMain(MessagePart *parent, const char *preferredType);
		MessagePart* 			SearchForContentID(MessagePart *part, const char *contentID);

		enum message_fields {
			kDate = 0,
			kSubject = 1,
			kFrom = 2,
			kFromName = 3,
			kSender = 4,
			kSenderName = 5,
			kRecipient = 6,
			kRecipientName = 7,
			kCc = 8,
			kCcName = 9,
			kReplyTo = 10,
			kReplyToName = 11,
			kInReplyTo = 12,
			kMessageId = 13,
			kUid = 14,
			kMailbox = 15,
			kContentType = 16,
			kMainPartId = 17
		};
		BString fFields[18];
		
#if 0
		BString fDate;
		BString fSubject;
		BString fFrom;
		BString fFromName;
		BString fSender;
		BString fSenderName;
		BString fRecipient;
		BString fRecipientName;
		BString fCc;
		BString fCcName;
		BString fReplyTo;
		BString	fReplyToName;
		BString fInReplyTo;
		BString fMessageID;
		BString fUid;
		BString fMailbox;
		BString fContentType;
		BString fMainPartID;
#endif
		
		int fSize;
		uint32 fFlags;
		MessagePart *fRoot;
		mutable int32 fReferenceCount;
};

/*

MimeMessage Versioning
----------------------

The first version of MimeMessage wrote no version info when saving its
persistent state.  That one is hereby dubbed Version Zero.  As of June 2001
I am changing the state in the following ways:

The very first int32 read from the stream will be a "magic cookie" that
also indicates the version.  See the top of MimeMessage.cpp for the value.
In version zero the first int32 is loaded into fFlags, which uses the lowest
six bits with the rest zeroes, so it is easily distinguishable from a
magic cookie.  If it doesn't follow either scheme then it's assumed we are
not at the start of a valid MimeMessage so we stop reading.

Each MessagePart now has a new field, startOffset.  It indicates the buffer
index from the BDataIO where a MIME section starts.  It won't be present
in Version Zero of course so it's set to zero.

Version One can read the older Version Zero format but once written to disk
again it becomes Version One.

*/

#endif
