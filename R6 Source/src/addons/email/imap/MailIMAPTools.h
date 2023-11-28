#ifndef __MAIL_IMAP_UTILS_H__
#define __MAIL_IMAP_UTILS_H__

class BString;
class MDMailHeader;
class MDMailRecipient;
class MDMailRecipientsList;
class MDMail;
class MDMailContainer;

void	populate_header_with_imap(const char *,MDMailHeader *);
void	populate_structure_with_imap(const char *,MDMail *, bool);

void	balance_string(const char *,BString *,char,char);
void	cut_string(BString *,char);
void	extract_parameter(BString *,BString *,int32);

void 	extract_recipient(BString *string,MDMailRecipient *header);
void 	extract_recipient_entry(BString *string,MDMailHeader *, const char *fieldName);

void 	extract_recipients_list(BString *string,MDMailRecipientsList *list);
void 	extract_recipients_list_entry(BString *string,MDMailHeader *, const char *fieldName);

void	extract_mono_block_data(BString *,MDMail *, int32 section, int32 sub_section = 0L);
void 	extract_mono_block_data(BString *, BString *,char separator,char separator2 = -1);
void 	extract_mono_block_parameters(BString *,MDMailContainer *);

void	extract_dual_block_data(BString *,MDMail *);

void	extract_triple_block_data(BString *,MDMail *,bool);

#endif
