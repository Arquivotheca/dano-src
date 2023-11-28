#ifndef __MAILDAEMON_DEFS__
#define __MAILDAEMON_DEFS__

#include <SupportDefs.h>

// email add-ons
#define	MAIL_DAEMON_ADDON_IMAP		"email/imap.so"
#define	MAIL_DAEMON_ADDON_SMTP		"email/smtp.so"
#define	MAIL_DAEMON_ADDON_POP3		"email/pop3.so"
#define	MAIL_DAEMON_ADDON_LOCAL		"email/local.so"

// contacts add-ons
#define	MAIL_DAEMON_ADDON_CONTACT_LOCAL		"email/contact_local.so"

#define	MAIL_DAEMON_MAIN_ACCOUNT				"main_account"
#define	MAIL_DAEMON_MAIN_ADDRESSBOOK			"main"
#define	MAIL_DAEMON_FOLDERS_ACCOUNT				"mail_daemon_folders"

#define MAIL_DAEMON_SIGNATURE					"application/x-vnd.Be-BEIAPOST"

#define	MAIL_DAEMON_USER_CHANGE_USER_NAME		"name"

#define	MAIL_DAEMON_ACCOUNT_NAME					"account_name"
#define	MAIL_DAEMON_ACCOUNT_USERNAME				"account_username"
#define	MAIL_DAEMON_ACCOUNT_EMAIL					"account_email"
#define	MAIL_DAEMON_ACCOUNT_LOGIN					"account_login"
#define	MAIL_DAEMON_ACCOUNT_PASSWORD				"account_password"
#define	MAIL_DAEMON_ACCOUNT_INCOMING_ADDON			"account_incoming_addon"
#define	MAIL_DAEMON_ACCOUNT_INCOMING_SERVER			"account_incoming_server"
#define	MAIL_DAEMON_ACCOUNT_INCOMING_SERVER_PORT	"account_incoming_server_port"
#define	MAIL_DAEMON_ACCOUNT_INCOMING_SERVER_AUTH	"account_incoming_server_auth"
#define	MAIL_DAEMON_ACCOUNT_OUTGOING_ADDON			"account_outgoing_addon"
#define	MAIL_DAEMON_ACCOUNT_OUTGOING_SERVER			"account_outgoing_server"
#define	MAIL_DAEMON_ACCOUNT_OUTGOING_SERVER_PORT	"account_outgoing_server_port"
#define	MAIL_DAEMON_ACCOUNT_OUTGOING_SERVER_AUTH	"account_outgoing_server_auth"
#define	MAIL_DAEMON_ACCOUNT_DRAFT_ADDON				"account_draft_addon"
#define	MAIL_DAEMON_ACCOUNT_DRAFT_SERVER			"account_draft_server"
#define	MAIL_DAEMON_ACCOUNT_DRAFT_SERVER_PORT		"account_draft_server_port"
#define	MAIL_DAEMON_ACCOUNT_DRAFT_SERVER_AUTH		"account_draft_server_auth"
#define	MAIL_DAEMON_ACCOUNT_OUTBOX_ADDON			"account_outbox_addon"
#define	MAIL_DAEMON_ACCOUNT_OUTBOX_SERVER			"account_outbox_server"
#define	MAIL_DAEMON_ACCOUNT_OUTBOX_SERVER_PORT		"account_outbox_server_port"
#define	MAIL_DAEMON_ACCOUNT_OUTBOX_SERVER_AUTH		"account_outbox_server_auth"

#define	MAIL_DAEMON_ACCOUNTS_NUMBER					"account_number"

#define	MAIL_DAEMON_CONTACT_ACCOUNT_NAME			"account_name"
#define	MAIL_DAEMON_CONTACT_ACCOUNT_LOGIN			"account_login"
#define	MAIL_DAEMON_CONTACT_ACCOUNT_PASSWORD		"account_password"
#define	MAIL_DAEMON_CONTACT_ACCOUNT_ADDON			"account_addon"
#define	MAIL_DAEMON_CONTACT_ACCOUNT_SERVER			"account_server"
#define	MAIL_DAEMON_CONTACT_ACCOUNT_SERVER_PORT		"account_port"
#define	MAIL_DAEMON_CONTACT_ACCOUNT_SERVER_AUTH		"account_auth"

#define	MAIL_DAEMON_FILTERS_NUMBER					"filter_number"

#define	MAIL_DAEMON_FILTER_NAME						"filter_name"
#define	MAIL_DAEMON_FILTER_ACCOUNT					"filter_account"
#define	MAIL_DAEMON_FILTER_PLAY_SOUND				"filter_playsound"
#define	MAIL_DAEMON_FILTER_PLAY_SOUND_FILE			"filter_playsound_file"
#define	MAIL_DAEMON_FILTER_SET_LABEL				"filter_set_label"
#define	MAIL_DAEMON_FILTER_LABEL					"filter_label"

#define	MAIL_DAEMON_MAILBOX_NAME					"mailbox_name"
#define	MAIL_DAEMON_MAILBOX_NEW_NAME				"mailbox_new_name"
#define	MAIL_DAEMON_MAILBOX_REF_NAME				"mailbox_name_ref"
#define	MAIL_DAEMON_MAILBOX_TOTAL_MESSAGES			"mailbox_total_messages"
#define	MAIL_DAEMON_MAILBOX_NEW_MESSAGES			"mailbox_new_messages"

#define	MAIL_DAEMON_ADDRESSBOOK_NAME				"addressbook_name"
#define	MAIL_DAEMON_ADDRESSBOOK_NEW_NAME			"addressbook_new_name"
#define	MAIL_DAEMON_ADDRESSBOOK_REF_NAME			"addressbook_name_ref"
#define	MAIL_DAEMON_ADDRESSBOOK_TOTAL				"addressbook_total"

#define	MAIL_DAEMON_ADDRESSBOOK_ENTRY_NAME			"addressbook_entry_name"
#define	MAIL_DAEMON_ADDRESSBOOK_ENTRY_VALUE			"addressbook_value"
#define	MAIL_DAEMON_ADDRESSBOOK_ENTRY_ID			"addressbook_entry_id"

#define MAIL_DAEMON_MESSAGE_FIELD_SUBJECT			"mail_subject"
#define MAIL_DAEMON_MESSAGE_FIELD_FROM				"mail_from"
#define MAIL_DAEMON_MESSAGE_FIELD_REPLYTO			"mail_replyto"
#define MAIL_DAEMON_MESSAGE_FIELD_TO				"mail_to"
#define MAIL_DAEMON_MESSAGE_FIELD_CC				"mail_cc"
#define MAIL_DAEMON_MESSAGE_FIELD_BCC				"mail_bcc"
#define MAIL_DAEMON_MESSAGE_FIELD_PRIORITY			"mail_priority"
#define MAIL_DAEMON_MESSAGE_FIELD_BODY				"mail_body"
#define MAIL_DAEMON_MESSAGE_FIELD_DATE				"mail_date"
#define MAIL_DAEMON_MESSAGE_FIELD_SIZE				"mail_size"
#define MAIL_DAEMON_MESSAGE_FIELD_UID				"mail_uid"

#define MAIL_DAEMON_MESSAGE_FIELD_CONTENT_TYPE 		"mail_ct"
#define MAIL_DAEMON_MESSAGE_FIELD_SUBCONTENT_TYPE 	"mail_subct"
#define MAIL_DAEMON_MESSAGE_FIELD_CHARSET		 	"mail_charset"
#define MAIL_DAEMON_MESSAGE_FIELD_ENCODING		 	"mail_encoding"
#define MAIL_DAEMON_MESSAGE_FIELD_FILENAME		 	"mail_filename"
#define MAIL_DAEMON_MESSAGE_FIELD_SIZE		 		"mail_size"
#define MAIL_DAEMON_MESSAGE_FIELD_LOCALSIZE		 	"mail_localsize"
#define MAIL_DAEMON_MESSAGE_FIELD_INDEX			 	"mail_index"

// used to specify the headers required
#define MAIL_DAEMON_MESSAGE_HEADERS					"headers"

#define	MAIL_DAEMON_STATUS						"status"		// value returned to the client

#define	MAIL_DAEMON_PORT						"port"			// port used to return data to the client

#define	MAIL_DAEMON_INDEX						"index"
#define	MAIL_DAEMON_INDEX_START					"id_start"
#define	MAIL_DAEMON_INDEX_END					"id_end"
#define	MAIL_DAEMON_CONTENT_SECTION				"content_section"
#define	MAIL_DAEMON_CONTENT_BUFFER_SIZE			"content_buffer_size"

#define	MAIL_DAEMON_SPACE_STORAGE_CURRENT	"storage_current"
#define	MAIL_DAEMON_SPACE_STORAGE_LIMIT		"storage_limit"
#define	MAIL_DAEMON_SPACE_MESSAGE_CURRENT	"message_current"
#define	MAIL_DAEMON_SPACE_MESSAGE_LIMIT		"message_limit"

// Here is the list of all messages accepted by the BeIA Mail Daemon

/* The reply is always in the "status" (int32) field and that's a status_t value */
enum
{
	// Reply
	MAIL_DAEMON_REPLY = 'REPY',				// reply field to know if the requested action
											// has been well executed

	MAIL_DAEMON_ADDRESSBOOK_CREATE = 'ABCR',				// Create an addressbook
	/* Parameters :
	"addressbook_name" (char *) : addressbook name
	"addressbook_name_ref" (char *) : ref  (where the addressbook is created)
	*/

	MAIL_DAEMON_ADDRESSBOOK_DELETE = 'ABDE',				// Delete an addressbook
	/* Parameters :
	"addressbook_name" (char *) : addressbook name
	*/

	MAIL_DAEMON_ADDRESSBOOK_RENAME = 'ABRE',				// Rename an addressbook
	/* Parameters :
	"addressbook_name" (char *) : addressbook name
	"addressbook_new_name" (char *) : new addressbook name
	*/
	
	MAIL_DAEMON_ADDRESSBOOK_GETSTATS = 'ABST',				// Get addressbook stats
	/* Parameters :
	"addressbook_name" (char *) : addressbook_name name
	*/
	/* Returned Value :
	"addressbook_total" (int32) : number of entries
	*/

	MAIL_DAEMON_ADDRESSBOOK_ADD_CONTACT = 'ABAC',				// Add a contact to the addressbook
	/* Parameters :
	"addressbook_name" (char *) : addressbook_name name
	
	(n times)
	"addressbook_entry_name" (char *) : entry name (firstname for example)
	"addressbook_value" (char *) : value of the entry
	*/

	MAIL_DAEMON_ADDRESSBOOK_DELETE_CONTACT = 'ABDC',			// Delete a contact to the addressbook
	/* Parameters :
	"addressbook_name" (char *) : addressbook_name name
	"addressbook_entry_id" (uint32) : id of the entry
	*/

	MAIL_DAEMON_ADDRESSBOOK_GET_CONTACT = 'ABGC',			// Get a contact to the addressbook
	/* Parameters :
	"addressbook_name" (char *) : addressbook_name name
	"addressbook_entry_id" (uint32) : id of the entry
	*/
	/* Returned Value :
	(n times)
	"addressbook_entry_name" (char *) : entry name (firstname for example)
	"addressbook_value" (char *) : value of the entry
	*/

	MAIL_DAEMON_ADDRESSBOOK_MODIFY_CONTACT = 'ABMC',			// Modify a contact to the addressbook
	/* Parameters :
	"addressbook_name" (char *) : addressbook_name name
	"addressbook_entry_id" (uint32) : id of the entry
	
	(n times)
	"addressbook_entry_name" (char *) : entry name (firstname for example)
	"addressbook_value" (char *) : value of the entry
	*/
	
	MAIL_DAEMON_ADDRESSBOOK_QUERY_CONTACT = 'ABQC',				//  Query contacts from the addressbook
	/* Parameters :
	"addressbook_name" (char *) : addressbook_name name
	"addressbook_entry_id" (uint32) : id of the entry (if necessary, if not everything will be retrieved)
	(n times)
	"addressbook_entry_name" (char *) : entry name needed in the reply (firstname for example)
	*/
	
	// Mail Accounts
	MAIL_DAEMON_ACCOUNTS_COUNT = 'ACCC',
	/* Returned Value :
	"account_number" (int32) : number of accounts
	*/
	MAIL_DAEMON_ACCOUNTS_ADD = 'ACCA',
	/* Parameters :
	"account_name" (char *)						: name of the account
	"account_username" (char *)					: name of the user
	"account_email" (char *)					: email of the user
	"account_login" (char *)					: login of the user
	"account_password" (char *)					: password of the user
	"account_incoming_addon" (char *)			: addon used by the server
	"account_incoming_server" (char *)			: address of the incoming mail server
	"account_incoming_server_port" (int32)		: port of the incoming mail server
	"account_incoming_server_auth" (int16)		: authentication used by the incoming mail server
	"account_outgoing_addon" (char *)			: addon used by the server
	"account_outgoing_server" (char *)			: address of the outgoing mail server
	"account_outgoing_server_port" (int32)		: port of the outgoing mail server
	"account_outgoing_server_auth" (int16)		: authentication used by the outgoing mail server
	"account_draft_addon" (char *)				: addon used by the server
	"account_draft_server" (char *)				: address of the outgoing mail server
	"account_draft_server_port" (int32)			: port of the outgoing mail server
	"account_draft_server_auth" (int16)			: authentication used by the outgoing mail server
	"account_outbox_addon" (char *)				: addon used by the server
	"account_outbox_server" (char *)			: address of the outgoing mail server
	"account_outbox_server_port" (int32)		: port of the outgoing mail server
	"account_outbox_server_auth" (int16)		: authentication used by the outgoing mail server
	*/

	MAIL_DAEMON_ACCOUNTS_GET = 'ACCG',
	/* Parameters :
	"account_name" (char *)						: name of the account to get
	*/
	/* Returned Value :
	"account_name" (char *)						: name of the account to get
	"account_username" (char *)					: name of the user
	"account_email" (char *)					: email of the user
	"account_login" (char *)					: login of the user
	"account_password" (char *)					: password of the user
	"account_incoming_addon" (char *)			: addon used by the server
	"account_incoming_server" (char *)			: address of the incoming mail server
	"account_incoming_server_port" (int32)		: port of the incoming mail server
	"account_incoming_server_auth" (int16)		: authentication used by the incoming mail server
	"account_outgoing_addon" (char *)			: addon used by the server
	"account_outgoing_server" (char *)			: address of the outgoing mail server
	"account_outgoing_server_port" (int32)		: port of the outgoing mail server
	"account_outgoing_server_auth" (int16)		: authentication used by the outgoing mail server
	"account_draft_addon" (char *)				: addon used by the server
	"account_draft_server" (char *)				: address of the outgoing mail server
	"account_draft_server_port" (int32)			: port of the outgoing mail server
	"account_draft_server_auth" (int16)			: authentication used by the outgoing mail server
	"account_outbox_addon" (char *)				: addon used by the server
	"account_outbox_server" (char *)			: address of the outgoing mail server
	"account_outbox_server_port" (int32)		: port of the outgoing mail server
	"account_outbox_server_auth" (int16)		: authentication used by the outgoing mail server
	*/
	MAIL_DAEMON_ACCOUNTS_MODIFY = 'ACCM',
	/* Parameters :
	"account_name" (char *)						: name of the account to modify
	"account_username" (char *)					: name of the user
	"account_email" (char *)					: email of the user
	"account_login" (char *)					: login of the user
	"account_password" (char *)					: password of the user
	"account_incoming_addon" (char *)			: addon used by the server
	"account_incoming_server" (char *)			: address of the incoming mail server
	"account_incoming_server_port" (int32)		: port of the incoming mail server
	"account_incoming_server_auth" (int16)		: authentication used by the incoming mail server
	"account_outgoing_addon" (char *)			: addon used by the server
	"account_outgoing_server" (char *)			: address of the outgoing mail server
	"account_outgoing_server_port" (int32)		: port of the outgoing mail server
	"account_outgoing_server_auth" (int16)		: authentication used by the outgoing mail server
	"account_draft_addon" (char *)				: addon used by the server
	"account_draft_server" (char *)				: address of the outgoing mail server
	"account_draft_server_port" (int32)			: port of the outgoing mail server
	"account_draft_server_auth" (int16)			: authentication used by the outgoing mail server
	"account_outbox_addon" (char *)				: addon used by the server
	"account_outbox_server" (char *)			: address of the outgoing mail server
	"account_outbox_server_port" (int32)		: port of the outgoing mail server
	"account_outbox_server_auth" (int16)		: authentication used by the outgoing mail server
	*/

	MAIL_DAEMON_ACCOUNTS_DELETE = 'ACCD',
	/* Parameters :
	"account_name" (char *)						: name of the account to modify
	*/

	// Filters
	MAIL_DAEMON_FILTERS_COUNT = 'FILC',
	/* Returned Value :
	"account_number" (int32) : number of filters
	*/
	
	MAIL_DAEMON_FILTERS_ADD = 'FILA',
	/* Parameters :
	"filter_name" (char *)						: name of the filter
	"filter_account" (char *)					: account of the filter
	"filter_playsound" (bool)					: play sound
	"filter_playsound_file" (char *)			: play sound file path
	"filter_set_label" (bool)					: set label
	"filter_label" (int8)						: label
	*/
	
	MAIL_DAEMON_FILTERS_DELETE = 'FILD',
	/* Parameters :
	"filter_name" (char *)						: name of the filter
	*/
	
	MAIL_DAEMON_FILTERS_GET = 'FILG',
	/* Parameters :
	"filter_name" (char *)						: name of the filter to get
	*/
	/* Returned Value :
	"filter_name" (char *)						: name of the filter
	"filter_account" (char *)					: account of the filter
	"filter_playsound" (bool)					: play sound
	"filter_playsound_file" (char *)			: play sound file path
	"filter_set_label" (bool)					: set label
	"filter_label" (int8)						: label
	*/

	MAIL_DAEMON_FILTERS_MODIFY = 'FILM',
	/* Parameters :
	"filter_name" (char *)						: name of the filter to modify
	"filter_account" (char *)					: account of the filter
	"filter_playsound" (bool)					: play sound
	"filter_playsound_file" (char *)			: play sound file path
	"filter_set_label" (bool)					: set label
	"filter_label" (int8)						: label
	*/
	
	// Common functions
	MAIL_DAEMON_COMMON_CAPABILITES = 'COMC',			// not done
	
	// Add-ons
	MAIL_DAEMON_ADDONS_LIST = 'ADDL',
	MAIL_DAEMON_ADDONS_CAPABILITES = 'ADDC',
	
	MAIL_DAEMON_MAILBOX_GETSTATS = 'MBST',						// Get mailbox stats
	/* Parameters :
	"mailbox_name" (char *) : mailbox name
	*/
	/* Returned Value :
	"total_messages" (int32) : number of messages
	"new_messages" (int32) : number of new messages
	*/
	
	MAIL_DAEMON_MAILBOX_CREATE = 'MBCR',						// Create a mailbox
	/* Parameters :
	"mailbox_name_ref" (char *) : ref  (where the mailbox is created)
	"mailbox_name" (char *) : mailbox name
	*/

	MAIL_DAEMON_MAILBOX_DELETE = 'MBDE',						// Delete a mailbox
	/* Parameters :
	"mailbox_name" (char *) : mailbox name
	*/

	MAIL_DAEMON_MAILBOX_RENAME = 'MBRE',						// Rename a mailbox
	/* Parameters :
	"mailbox_name" (char *) : mailbox name
	"mailbox_new_name" (char *) : new mailbox name
	*/

	MAIL_DAEMON_MAILBOX_SUBSCRIBE = 'MBSU',						// Subscribe to a mailbox
	/* Parameters :
	"mailbox_name" (char *) : mailbox name
	*/

	MAIL_DAEMON_MAILBOX_UNSUBSCRIBE = 'MBUS',					// Unsubscribe to a mailbox
	/* Parameters :
	"mailbox_name" (char *) : mailbox name
	*/
	
	MAIL_DAEMON_MAILBOX_LIST_MAILBOX = 'MBLM',					// Retrieve mailboxes list
	/* Parameters :
	"port" (port_id) 		: port used to transfer the data back to the client
	*/

	MAIL_DAEMON_MAILBOX_LIST_SUBSCRIBED_MAILBOX = 'MBLS',		// Retrieve subscribed mailboxes list
	/* Parameters :
	"port" (port_id) 		: port used to transfer the data back to the client
	*/
	
	// Mail saving as draft
	MAIL_DAEMON_SAVE_DRAFT_MESSAGE = 'SAVD',
	/* Parameters :
	"account_name" (char *) : account name
	"mail_subject" (char *) : subject of the message
	"mail_to" (char *) : to of the message
	"mail_cc" (char *) : cc of the message
	"mail_bcc" (char *) : bcc of the message
	"mail_priority" (int8) : priority of the message
	"mail_body" (char *) : text body of the message
	"mail_uid" (char *) : uid of the message (not present if it's a new message)
	*/
	
	// Mail Sending
	MAIL_DAEMON_SEND_MESSAGE = 'SEND',
	/* Parameters :
	"account_name" (char *) : account name
	"mail_subject" (char *) : subject of the message
	"mail_to" (char *) : to of the message
	"mail_cc" (char *) : cc of the message
	"mail_bcc" (char *) : bcc of the message
	"mail_priority" (int8) : priority of the message
	"mail_body" (char *) : text body of the message
	*/
	
	// Mail Deletion
	MAIL_DAEMON_DELETE_MESSAGE = 'DELE',
	/* Parameters :
	"mailbox_name" (char *) : mailbox
	"id_mail" (const char *) : id of the mail
	*/
	
	// Mail Retrieving
	MAIL_DAEMON_RETRIEVE_HEADERS = 'RTHE',
	/* Parameters :
	"port" (port_id) : port used to transfer the data back to the client
	"mailbox_name" (char *) : mailbox
	"id_start" (const char *) : id of the first mail
	"id_end" (const char *) : id of the last mail
	
	note : if id_start = "-1" and id_end = "-1", everything is retrieved
	*/
	
	MAIL_DAEMON_RETRIEVE_STRUCTURE = 'RTST',
	/* Parameters :
	"port" (port_id) : port used to transfer the data back to the client
	"index" (const char *) : id of the mail
	"mailbox_name" (char *) : mailbox
	*/
	
	MAIL_DAEMON_RETRIEVE_CONTENT = 'RTCO',
	/* Parameters :
	"port" (port_id) : port used to transfer the data back to the client
	"index" (const char *) : id of the mail
	"content_section" (const char *) : section (start at 1)
	"content_buffer_size" (int64) : size of the buffer used to stream the content
	"mailbox_name" (char *) : mailbox
	*/
	
	MAIL_DAEMON_SELECT_USER = 'USER',
	/* Parameters :
	"name" (char *) : account name
	*/
	
	MAIL_DAEMON_NETWORK_ACTIVE = 'NETA',			// sent to the daemon to say "Connected to the internet" (Obsolete : The network status is sent by the net_server automatically)
	MAIL_DAEMON_NETWORK_INACTIVE = 'NETI',			// sent to the daemon to say "Disconnected to the internet" (Obsolete : The network status is sent by the net_server automatically)
	
	MAIL_DAEMON_CHECK_MAIL = 'CHEM',
	/* Parameters :
	"mailbox_name" (char *) : mailbox (if not present, inbox is used)
	*/
	
	MAIL_DAEMON_GET_SPACE = 'GSPC',
	/* Parameters :
	"account_name" (char *) : account name
	*/
	/* Returned Value :
	"storage_current" (int64) : current used storage (in bytes)
	"storage_limit" (int64) : maximum storage (in bytes)
	"message_current" (int32) : current used storage (in messages)
	"message_limit" (int32) : maximum storage (in messages)
	*/
};

#endif
