#ifndef db_h
#define db_h

#include "versioncache.h"

int db_get_version(const char *versionid, int fields);
int db_put_version(const struct version_info *info);
int db_delete_version(const char *versionid);
char ** db_get_device_classes();
int db_put_upgrade_path(const char *versionID, const char *fromVersionID,int flags);
int devclass_deploy(const char *devclassid,const char *versionid,const char *imgpath);
int devclass_getimgsize(const char *devclassid);

#endif
