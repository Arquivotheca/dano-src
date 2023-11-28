#ifndef __MAILDAEMON_FILTERS_DEFS__
#define __MAILDAEMON_FILTERS_DEFS__

#include <SupportDefs.h>

typedef enum mail_daemon_filters_fields
{
	MAIL_DAEMON_FILTERS_FIELDS_NOTHING = 0,
	MAIL_DAEMON_FILTERS_FIELDS_FROM,
};

typedef enum mail_daemon_filters_test
{
	MAIL_DAEMON_FILTERS_TEST_NOTHING = 0,
	MAIL_DAEMON_FILTERS_TEST_IS,
	MAIL_DAEMON_FILTERS_TEST_IS_NOT,
	MAIL_DAEMON_FILTERS_TEST_CONTAINS,
	MAIL_DAEMON_FILTERS_TEST_DOES_NOT_CONTAIN,
};

#endif