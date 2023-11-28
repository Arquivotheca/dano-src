#if ! defined MAILPREFS_INCLUDED
#define MAILPREFS_INCLUDED

typedef struct
{
	bool log;
	float pt_x;
	float pt_y;
	int32 workspace;
} server_data;


typedef struct
{
	bool enabled;
	char default_domain[256];
} mail_prefs_data;

status_t get_mail_prefs_data(mail_prefs_data *data);
status_t get_server_data(server_data *data);
status_t set_server_data(server_data *data, bool save);

#endif

