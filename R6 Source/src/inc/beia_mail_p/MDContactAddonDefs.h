#ifndef __MDCONTACTADDON_DEFS_H__
#define __MDCONTACTADDON_DEFS_H__

#define	MD_CONTACT_ADDON_PREDICATE_ID	"id"
#define	MD_CONTACT_ADDON_FIELD			"field"

//typedef enum md_contactaddon_capability
//{
//	md_mailaddon_capability_nothing 				= 0x0,
//	md_mailaddon_capability_login_logout 			= 0x1,
//	md_mailaddon_capability_mailbox_listing 		= 0x2,
//	md_mailaddon_capability_mailbox_creation 		= 0x4,
//	md_mailaddon_capability_mailbox_deletion 		= 0x8,
//	md_mailaddon_capability_mailbox_renaming 		= 0x10,
//	md_mailaddon_capability_mailbox_cleaning 		= 0x20,
//
//	md_mailaddon_capability_data_sending 			= 0x100,
//
//	md_mailaddon_capability_headers_retrieving 		= 0x200,
//	md_mailaddon_capability_text_headers_retrieving = 0x400,
//	md_mailaddon_capability_structure_retrieving 	= 0x800,
//	md_mailaddon_capability_content_retrieving 		= 0x1000,
//
//	md_mailaddon_capability_message_deletion 		= 0x2000,
//
//	md_mailaddon_capability_server_space 			= 0x20000,
//
//} md_mailaddon_capability;
//
//typedef enum md_mailaddon_direction
//{
//	md_mailaddon_direction_incoming = 1,
//	md_mailaddon_direction_outgoing = 2,
//} md_mailaddon_direction;

typedef enum md_contactaddon_authentication
{
	md_contactaddon_authentication_none = 0,
} md_contactaddon_authentication;

#endif
