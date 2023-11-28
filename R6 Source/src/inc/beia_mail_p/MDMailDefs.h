#ifndef __MD_MAIL_DEFS_H__
#define __MD_MAIL_DEFS_H__

typedef enum md_mail_capability
{
	md_mail_capability_nothing = 0x0,
	md_mail_capability_login_logout = 0x1,
	md_mail_capability_mailbox_listing = 0x2,
	md_mail_capability_mailbox_creation = 0x4,
	md_mail_capability_mailbox_deletion = 0x8,
	md_mail_capability_mailbox_renaming = 0x10,
	md_mail_capability_mailbox_cleaning = 0x20,

	md_mail_capability_data_sending = 0x100,
	md_mail_capability_headers_retrieving = 0x200,
	md_mail_capability_text_headers_retrieving = 0x400,

} md_mail_capability;

typedef enum md_mail_direction
{
	md_mail_direction_incoming = 1,
	md_mail_direction_outgoing = 2,
} md_mail_direction;

typedef enum md_mail_authentication
{
	md_mail_authentication_none = 0,
	md_mail_authentication_login = 1,
	md_mail_authentication_plain = 2,
	md_mail_authentication_cram_md5 = 3,
} md_mail_authentication;

typedef enum md_mail_status
{
	md_mail_status_nothing = 0x0,
	md_mail_status_new = 0x1,
	md_mail_status_read = 0x2,
	md_mail_status_replied = 0x4,
	md_mail_status_forwarded = 0x8,
	md_mail_status_deleted = 0x10,

	md_mail_status_flagged = 0x20,

	md_mail_status_pending = 0x40,
	md_mail_status_draft = 0x80,
	md_mail_status_sent = 0x100,
} md_mail_status;

typedef enum md_mail_loading_state
{
	md_mail_loading_state_unloaded = 0,
	md_mail_loading_state_loaded = 1,
	md_mail_loading_state_partially_loaded = 2,
} md_mail_loading_state;

#endif
