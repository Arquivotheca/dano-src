#ifndef _SYSLOGD_H_
#define _SYSLOGD_H_

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#define SYSLOG_PRINT		'sypr'
#define KERN_SYSLOG_PRINT	'kspr'
#define KERN_SYSLOG_FLUSH	'flsh'

#define MAXLINE			1024

#define SYSLOGD_PORT_NAME	"syslog_daemon_port"

typedef struct syslog_message {
	int 	len;
	char	buf[MAXLINE + 1];
} syslog_message;


#endif /* _SYSLOGD_H_ */
