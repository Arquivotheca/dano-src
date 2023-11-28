#ifndef __MAILDAEMON_XML__
#define __MAILDAEMON_XML__

// ========================= Mailboxes Retrieving =========================

// XML tags
#define	MAILDAEMON_XML_MAILBOXLIST					"<MailboxList>"
#define	MAILDAEMON_XML_MAILBOX						"<Mailbox"
#define	MAILDAEMON_XML_MAILBOX_CLOSE				"</Mailbox>"
#define	MAILDAEMON_XML_MAILBOXLIST_CLOSE			"</MailboxList>"

#define	MAILDAEMON_XML_MAILBOX_NAME					"Mailbox_name"
#define	MAILDAEMON_XML_MAILBOX_TOTAL_MESSAGES		"Total_messages"
#define	MAILDAEMON_XML_MAILBOX_NEW_MESSAGES			"New_messages"

// Port code
#define	MAILDAEMON_XML_PORT_MAILBOX					'mbox'
#define	MAILDAEMON_XML_PORT_MAILBOX_DONE			'MBOX'

// ========================= Headers Retrieving =========================

// XML tags
#define MAILDAEMON_XML_HEADERLIST					"<HeaderList>"
#define MAILDAEMON_XML_HEADER						"<Header"
#define MAILDAEMON_XML_HEADER_CLOSE					"</Header>"
#define MAILDAEMON_XML_HEADERLIST_CLOSE				"</HeaderList>"

// XML fields
#define MAILDAEMON_XML_HEADER_SUBJECT				"Subject"
#define MAILDAEMON_XML_HEADER_FROM					"From"
#define MAILDAEMON_XML_HEADER_REPLYTO				"Reply-to"
#define MAILDAEMON_XML_HEADER_DATE					"Date"
#define MAILDAEMON_XML_HEADER_TO					"To"
#define MAILDAEMON_XML_HEADER_CC					"Cc"
#define MAILDAEMON_XML_HEADER_BCC					"Bcc"
#define MAILDAEMON_XML_HEADER_UID					"Uid"
#define MAILDAEMON_XML_HEADER_SIZE					"Size"
#define MAILDAEMON_XML_HEADER_LABEL					"Label"
#define MAILDAEMON_XML_HEADER_STATUS				"Status"
#define MAILDAEMON_XML_HEADER_LOAD					"Load"
#define MAILDAEMON_XML_HEADER_ENCLOSURES			"Enclosures"
#define MAILDAEMON_XML_HEADER_ACCOUNT				"Account"
#define MAILDAEMON_XML_HEADER_LOGIN					"Login"
#define MAILDAEMON_XML_HEADER_PASSWORD				"Password"
#define MAILDAEMON_XML_HEADER_SERVER				"Server"
#define MAILDAEMON_XML_HEADER_MAILBOX				"Mailbox"

// Port code
#define	MAILDAEMON_XML_PORT_HEADER					'head'
#define	MAILDAEMON_XML_PORT_HEADER_DONE				'HEAD'

// ========================= Structure Retrieving =========================

// XML tags
#define MAILDAEMON_XML_STRUCTURE					"<MessageStructure"
#define MAILDAEMON_XML_STRUCTURE_CLOSE				"</MessageStructure>"

#define MAILDAEMON_XML_STRUCTURE_CONTAINER			"<Container"
#define MAILDAEMON_XML_STRUCTURE_CONTAINER_CLOSE	"</Container>"

// XML fields
#define MAILDAEMON_XML_STRUCTURE_ID					"Id"
#define MAILDAEMON_XML_STRUCTURE_TYPE				"Type"
#define MAILDAEMON_XML_STRUCTURE_SECTION			"Section"
#define MAILDAEMON_XML_STRUCTURE_SIZE				"Size"

// Port code
#define	MAILDAEMON_XML_PORT_STRUCTURE				'stru'
#define	MAILDAEMON_XML_PORT_STRUCTURE_DONE			'STRU'

// ========================= Content Retrieving =========================

// Port code
#define	MAILDAEMON_XML_PORT_CONTENT					'cont'
#define	MAILDAEMON_XML_PORT_CONTENT_DONE			'CONT'

// ========================= Contacts Retrieving =========================

// XML tags
#define MAILDAEMON_XML_CONTACTLIST					"<ContactList>"
#define MAILDAEMON_XML_CONTACT						"<Contact"
#define MAILDAEMON_XML_CONTACT_CLOSE				"</Contact>"
#define MAILDAEMON_XML_CONTACTLIST_CLOSE			"</ContactList>"

#define MAILDAEMON_XML_CONTACT_ID					"Id"
#define MAILDAEMON_XML_CONTACT_VALUE				"Value"

// Port code
#define	MAILDAEMON_XML_PORT_CONTACT					'cont'
#define	MAILDAEMON_XML_PORT_CONTACT_DONE			'CONT'

#endif
