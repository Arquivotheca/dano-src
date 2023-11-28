#ifndef _VERSION_RECORD_CACHE_H_
#define _VERSION_RECORD_CACHE_H_

#define ERR_OK					0
#define ERR_UNKNOWN				-1	/* some sort of LDAP error */
#define ERR_NOVERSION			-2	/* no version record found */
#define ERR_DUPVERSIONS			-3	/* more than one of those exists (should never happen) */
#define ERR_BIND				-4	/* couldn't bind to the LDAP server */
#define ERR_ZIPERROR			-5	/* zip couldn't unpack the archive */
#define ERR_INTERNAL			-6	/* an internal error (also should never happen) */
#define ERR_NOCONFIG			-7	/* can't read my configuration */
#define ERR_EMPTY				-8	/* needed attributes are missing from the record */
#define ERR_INVALID				-9	/* an attribute had an invalid format */
#define ERR_FILENOTFOUND		-10	

#define AR_FULLUPDATE 		0x0001
#define AR_VERSIONARCHIVE	0x0002
#define AR_DELTADATA		0x0004
#define AR_FULLBOOTSTRAP	0x0008
#define AR_ALL				(AR_FULLUPDATE|AR_VERSIONARCHIVE|AR_DELTADATA|AR_FULLBOOTSTRAP)

#define UF_REBOOT			0x0001
#define UF_REDIAL			0x0002
#define UF_RESTART			0x0004
#define UF_FREEZE			0x0008
#define UF_FLUSH			0x0010

struct version_info {
	const char *versionid;
	const char *installby;
	const char *checkout_time;
	const char *checkin_time;
	const char *finish_time;
	const char *lastversionid;
};


#ifdef __cplusplus
extern "C" {
#endif

int cache_put_version(const struct version_info *info);
int cache_get_version(const char *versionid,int fields);

int touchdir(char *path);
int write_file(char *path, void *data, unsigned long len);

#ifdef __cplusplus
};
#endif


#endif
