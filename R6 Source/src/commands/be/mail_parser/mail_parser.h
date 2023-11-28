//--------------------------------------------------------------------
//	
//	mail_parser.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <Application.h>
#include <Directory.h>
#include <E-mail.h>
#include <Entry.h>
#include <File.h>
#include <NodeInfo.h>
#include <parsedate.h>

static const char FIELD_TO[] = "to: ";
static const char FIELD_REPLY[] = "reply-to: ";
static const char FIELD_SUBJECT[] = "subject: ";
static const char FIELD_WHEN[] = "date: ";
static const char FIELD_PRIORITY[] = "priority: ";
static const char FIELD_FROM[] = "from: ";
static const char FIELD_MIME[] = "mime-version: ";

#define strsize(x) (sizeof(x) - 1)


//====================================================================

class TMailParser : public BApplication {

private:

public:

				TMailParser(void);
virtual void	ArgvReceived(int32, char**);
virtual void	RefsReceived(BMessage*);
virtual void	ReadyToRun(void);
status_t		Parse(BEntry*);
};
