#ifndef _PWD_H_
#define _PWD_H_

#include <be_setup.h>
#include <sys/types.h>

__extern_c_start

struct passwd {
        char    *pw_name;
		char    *pw_passwd;
        uid_t   pw_uid;
        gid_t   pw_gid;
        char    *pw_dir;
        char    *pw_shell;
		char    *pw_gecos;
};

extern void           setpwent(void);
extern void           endpwent(void);
extern struct passwd *getpwent(void);
extern struct passwd *getpwnam(const char *name);
extern struct passwd *getpwuid(uid_t uid);

__extern_c_end

#endif /* _PWD_H_ */
