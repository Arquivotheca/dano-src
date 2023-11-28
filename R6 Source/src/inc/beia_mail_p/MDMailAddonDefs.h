#ifndef __MDMAILADDON_DEFS_H__
#define __MDMAILADDON_DEFS_H__

#define	MAIL_CLIENT_CAPABILITY_NOTHING			0

#define	MAIL_CLIENT_HEADERS_MESSAGE				'head'
#define MAIL_CLIENT_HEADERS						"headers"

typedef enum md_mailaddon_capability
{
	md_mailaddon_capability_nothing 				= 0x0,
	md_mailaddon_capability_login_logout 			= 0x1,
	md_mailaddon_capability_mailbox_listing 		= 0x2,
	md_mailaddon_capability_mailbox_creation 		= 0x4,
	md_mailaddon_capability_mailbox_deletion 		= 0x8,
	md_mailaddon_capability_mailbox_renaming 		= 0x10,
	md_mailaddon_capability_mailbox_cleaning 		= 0x20,

	md_mailaddon_capability_data_sending 			= 0x100,

	md_mailaddon_capability_headers_retrieving 		= 0x200,
	md_mailaddon_capability_text_headers_retrieving = 0x400,
	md_mailaddon_capability_structure_retrieving 	= 0x800,
	md_mailaddon_capability_content_retrieving 		= 0x1000,

	md_mailaddon_capability_message_deletion 		= 0x2000,

	md_mailaddon_capability_server_space 			= 0x20000,

} md_mailaddon_capability;

typedef enum md_mailaddon_direction
{
	md_mailaddon_direction_incoming = 1,
	md_mailaddon_direction_outgoing = 2,
} md_mailaddon_direction;

typedef enum md_mailaddon_authentication
{
	md_mailaddon_authentication_none = 0,
	md_mailaddon_authentication_plain = 1,
	md_mailaddon_authentication_login = 2,
	md_mailaddon_authentication_cram_md5 = 3,
	md_mailaddon_authentication_digest_md5 = 4,
} md_mailaddon_authentication;

typedef enum md_mailaddon_status
{
	md_mailaddon_status_new = 0x0,
	md_mailaddon_status_read = 0x1,
	md_mailaddon_status_replied = 0x2,
	md_mailaddon_status_forwarded = 0x4,
	md_mailaddon_status_deleted = 0x8,

	md_mailaddon_status_flagged = 0x10,

	md_mailaddon_status_pending = 0x20,
	md_mailaddon_status_draft = 0x30,
	md_mailaddon_status_sent = 0x40,
} md_mailaddon_status;

#endif
