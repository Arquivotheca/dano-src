/*
	MimeMessage.cpp
*/
#include <Debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <DataIO.h>
#include <UTF8.h>
#include "BufferedFileAdapter.h"
#include "MimeMessage.h"

static const uint32 kVersionOneCookie = 0xC92F0001;

static const char *kFieldNames[] = {
	"date",
	"subject",
	"from",
	"fromName",
	"sender",
	"senderName",
	"recipient",
	"recipientName",
	"cc",
	"ccName",
	"replyTo",
	"replyToName",
	"inReplyTo",
	"messageID",
	"uid",
	"mailbox",
	"contentType",
	"mainPartID"
};

MessagePart::MessagePart()
	:	size(0),
		startOffset(0),
		isContainer(false),
		containerType(kNotContainer),
		disposition(kNoDisposition)
{

}

MessagePart::MessagePart(const MessagePart &root)
{
	*this = root;
}

MessagePart::~MessagePart()
{
	int32 count = subParts.CountItems() - 1;
	for (int i = count; i >= 0; i--)
		delete static_cast<MessagePart*>(subParts.RemoveItem(i));
}

MessagePart &MessagePart::operator=(const MessagePart &inPart)
{
	if (&inPart != this) {
		type = inPart.type;
		id = inPart.id;
		characterSet = inPart.characterSet;
		encoding = inPart.encoding;
		name = inPart.name;
		contentID = inPart.contentID;
		size = inPart.size;
		isContainer = inPart.isContainer;
		containerType = inPart.containerType;
		disposition = inPart.disposition;
		// Deep copy.
		for (int i = 0; i < subParts.CountItems(); i++) {
			MessagePart *part = static_cast<MessagePart*>(inPart.subParts.ItemAt(i));
			if (part)
				subParts.AddItem(new MessagePart(*part));
			else
				subParts.AddItem(0);
		}
	}
	
	return *this;
}

void MessagePart::Print()
{
	printf("MessagePart:\n");
	printf("\tType:%s\n", type.String());
	printf("\tID:%s\n", id.String());
	printf("\tCharacter Set:%s\n", characterSet.String());
	printf("\tEncoding:%s\n", encoding.String());
	printf("\tName:%s\n", name.String());
	printf("\tContent-ID:%s\n", contentID.String());
	printf("\tSize:%d\n", size);
	printf("\tDisposition:%s\n", disposition == kInline ? "inline" : "attached");
	printf("\tIsContainer:%s\n", isContainer ? "yes" : "no");
	printf("\tContainer Type:");
	switch (containerType) {
		case kNotContainer:		printf("kNotContainer\n");		break;
		case kMixed:			printf("kMixed\n");				break;
		case kAlternative:		printf("kAlternative\n");		break;
		case kRelated:			printf("kRelated\n");			break;
		default:				printf("unknown type\n");		break;
	}
	printf("\tSubParts:%ld\n", subParts.CountItems());
}

uint32 MessagePart::CharsetConversion(BString &charset)
{
	if (charset.ICompare("iso-8859-", 9) == 0) {
		int32 isoNum = strtol(charset.String() + 9, NULL, 10);
		if ((isoNum >= 1) && (isoNum <= 10)) {
			// Offset to zero
			isoNum--;
			return (B_ISO1_CONVERSION + isoNum);
		}
	}
	else if (charset.ICompare("iso-2022-jp", 11) == 0)
		return B_JIS_CONVERSION;
	else if (charset.ICompare("koi8-r", 6) == 0)
		return B_KOI8R_CONVERSION;
	else if (charset.ICompare("utf-8", 5) == 0)
		return B_UNICODE_CONVERSION;
	else if (charset.ICompare("windows-1251", 12) == 0)
		return B_MS_WINDOWS_1251_CONVERSION;
	else if (charset.ICompare("euc-kr",6) == 0)
		return B_EUC_KR_CONVERSION;
	return B_ISO1_CONVERSION;
}

MimeMessage::MimeMessage()
	:	fSize(0),
		fFlags(0),
		fRoot(0),
		fReferenceCount(0)
{
}

MimeMessage::MimeMessage(const MimeMessage &inMessage)
{
	*this = inMessage;
}

MimeMessage::~MimeMessage()
{
	if (fReferenceCount != 0)
		debugger("Tried to delete MimeMessage with non-zero reference count.");
		
	delete fRoot;
}

MimeMessage &MimeMessage::operator=(const MimeMessage &inMessage)
{
	if (&inMessage != this) {
		fFields[kDate] = inMessage.GetDate();
		fFields[kSubject] = inMessage.GetSubject();
		fFields[kFrom] = inMessage.GetFrom();
		fFields[kFromName] = inMessage.GetFromName();
		fFields[kSender] = inMessage.GetSender();
		fFields[kSenderName] = inMessage.GetSenderName();
		fFields[kRecipient] = inMessage.GetRecipient();
		fFields[kRecipientName] = inMessage.GetRecipientName();
		fFields[kCc] = inMessage.GetCc();
		fFields[kCcName] = inMessage.GetCcName();
		fFields[kReplyTo] = inMessage.GetReplyTo();
		fFields[kReplyToName] = inMessage.GetReplyToName();
		fFields[kInReplyTo] = inMessage.GetInReplyTo();
		fFields[kMessageId] = inMessage.GetMessageID();
		fFields[kUid] = inMessage.GetUid();
		fFields[kMailbox] = inMessage.GetMailbox();
		fFields[kContentType] = inMessage.GetContentType();
		fFields[kMainPartId] = inMessage.GetMainPartID();
		fSize = inMessage.GetSize();
		fFlags = inMessage.GetFlags();
		fReferenceCount = 0;
		// The hierarchy needs to be copied deeply
		if (inMessage.GetRoot()) {
			fRoot = new MessagePart(*(inMessage.GetRoot()));
			CopySubParts(inMessage.GetRoot(), fRoot);
		}
		else
			fRoot = 0;
	}
	return *this;
}

void MimeMessage::AcquireReference()
{
	atomic_add(&fReferenceCount, 1);
}

void MimeMessage::ReleaseReference()
{
	int32 prev = atomic_add(&fReferenceCount, -1);
	if (prev - 1 == 0)
		delete this;
}

void MimeMessage::CopySubParts(const MessagePart *source, MessagePart *destination)
{
	for (int32 i = 0; i < source->subParts.CountItems(); i++) {
		MessagePart *spart = static_cast<MessagePart *>(source->subParts.ItemAt(i));
		MessagePart *dpart = new MessagePart(*spart);
		destination->subParts.AddItem(dpart);
		if (spart->isContainer)
			CopySubParts(spart, dpart);
	}
}

const char* MimeMessage::GetContentType() const
{
	return fFields[kContentType].String();
}

const char *MimeMessage::GetMainPartID() const
{
	return fFields[kMainPartId].String();
}

const MessagePart* MimeMessage::GetRoot() const
{
	return fRoot;
}

MessagePart* MimeMessage::GetMainDoc(const char *preferredType)
{
	return SearchForMain(fRoot, preferredType);
}

MessagePart* MimeMessage::SearchForMain(MessagePart *parent, const char *preferredType)
{
	if (!parent)
		return 0;

	MessagePart *best = NULL;
	if (parent->type.ICompare("text/", 5) == 0)	// Only default to top document if it is text.
		best = parent;
	
	for (int i = 0; i < parent->subParts.CountItems(); i++) {
		MessagePart *check = static_cast<MessagePart*>(parent->subParts.ItemAt(i));
		if (check->isContainer)
			check = SearchForMain(check, preferredType);
		
		if (check == 0)
			continue;
		
		if (best == 0 && check->type.ICompare("text/", 5) == 0)
			best = check;
		else if ((best == 0 || best->type.ICompare(preferredType) != 0) &&
					((check->type.ICompare(preferredType) == 0) && (check->disposition != MessagePart::kAttached)))
			best = check;
	}

	return best;
}

MessagePart* MimeMessage::FindPart(const char *name)
{
	MessagePart *current = fRoot;
	for (const char *c = name; *c && current;) {
		if (!current->isContainer)
			break;
	
		int index = atoi(c);
		while (*c && *c != '.')
			c++;
			
		if (*c == '.')
			c++;

		current = static_cast<MessagePart*>(current->subParts.ItemAt(index - 1));
	}

	return current;
}

MessagePart* MimeMessage::FindPartByContentID(const char *contentID)
{
	return SearchForContentID(fRoot, contentID);
}

MessagePart* MimeMessage::SearchForContentID(MessagePart *part, const char *contentID)
{
	if (part->isContainer) {
		for (int i = 0; i < part->subParts.CountItems(); i++) {
			MessagePart *check = static_cast<MessagePart*>(part->subParts.ItemAt(i));
			check = SearchForContentID(check, contentID);
			if (check)
				return check;
		}
	} else if (part->contentID == contentID)
		return part;
		
	return 0;
}

const char* MimeMessage::GetMailbox() const
{
	return fFields[kMailbox].String();
}

const char* MimeMessage::GetUid() const
{
	return fFields[kUid].String();
}

const char* MimeMessage::GetDate() const
{
	return fFields[kDate].String();
}

const char* MimeMessage::GetSubject() const
{
	return fFields[kSubject].String();
}

const char* MimeMessage::GetFrom() const
{
	return fFields[kFrom].String();
}

const char* MimeMessage::GetFromName() const
{
	return fFields[kFromName].String();
}

const char* MimeMessage::GetSender() const
{
	return fFields[kSender].String();
}

const char* MimeMessage::GetSenderName() const
{
	return fFields[kSenderName].String();
}

const char* MimeMessage::GetRecipient() const
{
	return fFields[kRecipient].String();
}

const char *MimeMessage::GetRecipientName() const
{
	const char *str = fFields[kRecipientName].String();
	// If it's an empty string, return the address...
	if (strcmp(str, "") == 0)
		str = GetRecipient();
	return str;
}

int MimeMessage::GetSize() const
{
	return fSize;
}

uint32 MimeMessage::GetFlags() const
{
	return fFlags;
}

const char* MimeMessage::GetMessageID() const
{
	return fFields[kMessageId].String();
}

bool MimeMessage::IsMultipartMixed() const
{
	return (fFields[kContentType].ICompare("multipart/mixed", 15) == 0);
}


const char* MimeMessage::GetCc() const
{
	return fFields[kCc].String();
}

const char* MimeMessage::GetCcName() const
{
	return fFields[kCcName].String();
}

const char* MimeMessage::GetInReplyTo() const
{
	return fFields[kInReplyTo].String();
}

const char* MimeMessage::GetReplyTo() const
{
	return fFields[kReplyTo].String();
}

const char* MimeMessage::GetReplyToName() const
{
	return fFields[kReplyToName].String();
}

void MimeMessage::SetRoot(MessagePart *root)
{
	if (fRoot && fRoot != root)
		delete fRoot;

	fRoot = root;
}

void MimeMessage::SetContentType(const char *type)
{
	fFields[kContentType] = type;
}

void MimeMessage::SetMainPartID(const char *part)
{
	fFields[kMainPartId] = part;
}

void MimeMessage::SetMailbox(const char *mailbox)
{
	fFields[kMailbox] = mailbox;
}

void MimeMessage::SetUid(const char *uid)
{
	fFields[kUid] = uid;
}

void MimeMessage::SetDate(const char *date)
{
	fFields[kDate] = date;
}

void MimeMessage::SetSubject(const char *subject)
{
	fFields[kSubject] = subject;
}

void MimeMessage::AddFrom(const char *from)
{
	if (fFields[kFrom].Length())
		fFields[kFrom] << "; " << from;
	else
		fFields[kFrom] << from;
}

void MimeMessage::AddFromName(const char *from)
{
	if (fFields[kFromName].Length())
		fFields[kFromName] << "; " << from;
	else
		fFields[kFromName] << from;
}

void MimeMessage::AddSender(const char *sender)
{
	if (fFields[kSender].Length())
		fFields[kSender] << "; " << sender;
	else
		fFields[kSender] << sender;
}

void MimeMessage::AddSenderName(const char *sender)
{
	if (fFields[kSenderName].Length())
		fFields[kSenderName] << "; " << sender;
	else
		fFields[kSenderName] << sender;
}

void MimeMessage::AddRecipient(const char *recipient)
{
	if (fFields[kRecipient].Length())
		fFields[kRecipient] << "; " << recipient;
	else
		fFields[kRecipient] << recipient;
}

void MimeMessage::SetRecipient(const char *recipient)
{
	fFields[kRecipient] = recipient;
}

void MimeMessage::AddRecipientName(const char *name)
{
	if (fFields[kRecipientName].Length())
		fFields[kRecipientName] << "; " << name;
	else
		fFields[kRecipientName] << name;
}

void MimeMessage::AddCc(const char *cc)
{
	if (fFields[kCc].Length())
		fFields[kCc] << "; " << cc;
	else
		fFields[kCc] << cc;
}

void MimeMessage::AddCcName(const char *name)
{
	if (fFields[kCcName].Length())
		fFields[kCcName] << "; " << name;
	else
		fFields[kCcName] << name;
}

void MimeMessage::AddReplyTo(const char *replyto)
{
	if (fFields[kReplyTo].Length())
		fFields[kReplyTo] << "; " << replyto;
	else
		fFields[kReplyTo] = replyto;
}

void MimeMessage::AddReplyToName(const char *name)
{
	if (fFields[kReplyToName].Length())
		fFields[kReplyToName] << "; " << name;
	else
		fFields[kReplyToName] = name;
}

void MimeMessage::SetInReplyTo(const char *replyto)
{
	fFields[kInReplyTo] = replyto;
}

void MimeMessage::SetSize(int size)
{
	fSize = size;
}

void MimeMessage::SetFlags(uint32 flags)
{
	fFlags = flags;
}

void MimeMessage::SetMessageID(const char *messageID)
{
	fFields[kMessageId] = messageID;
}

status_t MimeMessage::WriteToStream(BDataIO *stream)
{
	stream->Write((const void *)&fFlags, sizeof(uint32));
	stream->Write((const void *)&fSize, sizeof(int));
	int32 size = 0;
	
	// Editors note: This looks rather long and wordy, but it
	// becomes more efficient then doing the obivious
	// stream->Write("foo", strlen("foo"));
	// When you want to read this back in, if we know the length
	// of the string, then we can just lock the BString buffer
	// and do a memcpy into. If we didn't know the length,
	// then we'd have to sit in a while() type loop looking for
	// a null, copying each charater at a time...
	
	// This is totally hard-coded, which is Not Good. It should
	// be broken down so that a MimeMessage can handle an
	// arbitrary number of fields...
	
	for (int32 i = 0; i < 18; i++) {
		size = fFields[i].Length();
		stream->Write((const void *)&size, sizeof(int32));
		stream->Write((const void *)fFields[i].String(), size);
	}
	WritePartsToStream(stream, fRoot);
	stream->Write((const void *)"\0", 1);
	return B_OK;
}

status_t MimeMessage::WriteToXmlStream(B::XML:: BCodifier *stream, bool isDirty)
{
	BStringMap map;
	map.Add("dirty", isDirty ? "1" : "0");
	for (int32 i = 0; i < 18; i++)
		map.Add(kFieldNames[i], fFields[i].String());
	stream->StartTag("message", map);
	WritePartsToXmlStream(stream, fRoot);
	stream->EndTag("message");
	return B_OK;
}

status_t MimeMessage::WritePartsToXmlStream(B::XML::BCodifier *stream, MessagePart *part)
{
	if (part == NULL)
		return B_ERROR;

	BStringMap map;
	map.Add("type", part->type.String());
	map.Add("id", part->id.String());
	map.Add("characterSet", part->characterSet.String());
	map.Add("encoding", part->encoding.String());
	map.Add("name", part->name.String());
	map.Add("contentID", part->contentID.String());
	map.Add("size", part->size);
	map.Add("isContainer", part->isContainer ? "1" : "0");
	map.Add("containerType", (uint32)(part->containerType));
	map.Add("disposition", (uint32)(part->disposition));
	stream->StartTag("attachment", map);
	if (part->isContainer) {
		for (int32 i = 0; i < part->subParts.CountItems(); i++) {
			MessagePart *subPart = static_cast<MessagePart*>(part->subParts.ItemAt(i));
			WritePartsToXmlStream(stream, subPart);
		}
	}
	stream->EndTag("attachment");
	return B_OK;
}

status_t MimeMessage::WritePartsToStream(BDataIO *stream, MessagePart *part)
{
	if (part == NULL) {
		printf(("PART IS NULL\n"));
		return B_ERROR;
	}

	stream->Write((const void *)"(", 1);
	int32 size = 0;
	
	// Part type
	size = part->type.Length();
	stream->Write((const void *)&size, sizeof(int32));
	stream->Write((const void *)part->type.String(), strlen(part->type.String()));
	// Part ID
	size = part->id.Length();
	stream->Write((const void *)&size, sizeof(int32));
	stream->Write((const void *)part->id.String(), strlen(part->id.String()));
	// Character Set
	size = part->characterSet.Length();
	stream->Write((const void *)&size, sizeof(int32));
	stream->Write((const void *)part->characterSet.String(), strlen(part->characterSet.String()));
	// Encoding style
	size = part->encoding.Length();
	stream->Write((const void *)&size, sizeof(int32));
	stream->Write((const void *)part->encoding.String(), strlen(part->encoding.String()));
	// Part name
	size = part->name.Length();
	stream->Write((const void *)&size, sizeof(int32));
	stream->Write((const void *)part->name.String(), strlen(part->name.String()));
	// Content ID
	size = part->contentID.Length();
	stream->Write((const void *)&size, sizeof(int32));
	stream->Write((const void *)part->contentID.String(), strlen(part->contentID.String()));

	stream->Write((const void *)&(part->size), sizeof(int));
	stream->Write((const void *)&(part->isContainer), sizeof(bool));
	stream->Write((const void *)&(part->containerType), sizeof(MessagePart::ContainerType));
	stream->Write((const void *)&(part->disposition), sizeof(MessagePart::DispositionType));

	if (part->isContainer) {
		for (int32 i = 0; i < part->subParts.CountItems(); i++) {
			MessagePart *subPart = static_cast<MessagePart*>(part->subParts.ItemAt(i));
			WritePartsToStream(stream, subPart);
		}
	}

	stream->Write((const void *)")", 1);
	return B_OK;
}

status_t MimeMessage::LoadFromBuffer(BDataIO &stream)
{
	int32  version;
	int32  start = 0xFFFFFFFF;
	
//	stream.Read((void *)&fFlags, sizeof(uint32));  // The OLD way

	stream.Read((void*)&start, sizeof (start));
	
	if ((uint32)start == kVersionOneCookie)
	  {
	    version = 1;
	    stream.Read((void*)&fFlags, sizeof (fFlags));
	  }
	else if (start <= 64)   // Flags for Version Zero
	  {
	    version = 0;
	    fFlags = start;
	  }
	else return B_ERROR;    
	
	stream.Read((void *)&fSize, sizeof(int));

	for (int32 i = 0; i < 18; i++)
		LoadField(stream, fFields[i]);

	fRoot = NULL;
	LoadPartsFromBuffer(stream, fRoot);
	// Skip null seperator
	char null[1];
	stream.Read((void *)&(null[0]), 1);
	return B_OK;
}

status_t MimeMessage::LoadPartsFromBuffer(BDataIO &stream, MessagePart *part)
{
	
	for (;;) {
		// Check for end of stream
		BufferedFileAdapter *a = dynamic_cast<BufferedFileAdapter *>(&stream);
		if (a->Peek() == '\0')
			break;
		// Eat open paranthesis
		char paren;
		stream.Read((void *)&paren, 1);
		// Extract the part
		MessagePart *newPart = new MessagePart;
		if (part == NULL) {
			fRoot = newPart;
			part = fRoot;
		} else {
			part->subParts.AddItem(newPart);
		}
		ExtractPart(stream, newPart);
		if (newPart->isContainer)
			LoadPartsFromBuffer(stream, newPart);
		// Eat closing parenthesis
		stream.Read((void *)&paren, 1);
		if (a->Peek() == ')')
			break;
	}
	
	return B_OK;
}

inline void MimeMessage::LoadField(BDataIO &stream, BString &field)
{
	int32 size = 0;
	stream.Read((void *)&size, sizeof(int32));
	
	if (size > 0) {
		char *buffer = field.LockBuffer(size);
		if (buffer) {
			stream.Read((void *)buffer, size);
			field.UnlockBuffer(size);
		} else {
			field = B_EMPTY_STRING;
		}
	} else {
		field = B_EMPTY_STRING;
	}
}

void MimeMessage::ExtractPart(BDataIO &stream, MessagePart *part)
{
	LoadField(stream, part->type);
	LoadField(stream, part->id);
	LoadField(stream, part->characterSet);
	LoadField(stream, part->encoding);
	LoadField(stream, part->name);
	LoadField(stream, part->contentID);

	// Size of part
	stream.Read((void *)&(part->size), sizeof(int));
	// Is part a container
	stream.Read((void *)&(part->isContainer), sizeof(bool));
	// Type of container (or not a container)
	stream.Read((void *)&(part->containerType), sizeof(MessagePart::ContainerType));
	// Attachment disposition type
	stream.Read((void *)&(part->disposition), sizeof(MessagePart::DispositionType));
}

void MimeMessage::Print(bool deep)
{
	printf("MimeMessage:\n");
	printf("\tDate:%s\n", GetDate());
	printf("\tSubject:%s\n", GetSubject());
	printf("\tFrom:%s\n", GetFrom());
	printf("\tFrom Name:%s\n", GetFromName());
	printf("\tSender:%s\n", GetSender());
	printf("\tSender Name:%s\n", GetSenderName());
	printf("\tRecipient:%s\n", GetRecipient());
	printf("\tRecipient Name:%s\n", GetRecipientName());
	printf("\tCc:%s\n", GetCc());
	printf("\tCc Name:%s\n", GetCcName());
	printf("\tInReplyTo:%s\n", GetInReplyTo());
	printf("\tContent-Type:%s\n", GetContentType());
	DumpPart(fRoot, 0, deep);
}

inline void indent(int count)
{
	for (int i = 0; i < count; i++)
		printf("  ");
}

void MimeMessage::DumpPart(MessagePart *part, int depth, bool deep)
{
	if (part == 0) {
		indent(depth);
		printf("NULL\n");
		return;
	}

	if (part->isContainer) {
		indent(depth);
		printf("Container (%s)\n", part->containerType == MessagePart::kMixed ? "Mixed" : "Alternative");
		for (int i = 0; i < part->subParts.CountItems(); i++) {
			MessagePart *subPart = static_cast<MessagePart*>(part->subParts
				.ItemAt(i));
			DumpPart(subPart, depth + 1, deep);
		}
	} else {
		indent(depth);
		if (deep)
			part->Print();
		else
			printf("Body type=%s id=%s charset=%s encoding=%s size=%d\n",
			part->type.String(), part->id.String(), part->characterSet.String(),
			part->encoding.String(), part->size);
	}
}


