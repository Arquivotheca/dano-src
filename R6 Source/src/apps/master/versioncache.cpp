#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <image.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <OS.h>

#include "master.h"
#include "versioncache.h"
#include "db.h"

/* *** */
int
touchdir(char *path)
{
	if (mkdir(path,0777) != 0) {
 		switch (errno) {
		case EEXIST:	return ERR_OK;	
						break;

		default:		perror(path);
						return ERR_UNKNOWN;
						break;
		}
	}
	return ERR_OK;
}

/* *** */

static int
unzip_archive(const char *versionid,char *archive)
{
	int ok;
	char *argv[6];
	char zip[MAXPATHLEN];
	char dest[MAXPATHLEN];
	thread_id tid;
	status_t result;

	ok = ERR_OK;
	snprintf(dest,MAXPATHLEN,CACHE_PATHNAME "/%s/%s",versionid,archive);
	if (touchdir(dest) == ERR_OK) {
		snprintf(zip,MAXPATHLEN,CACHE_PATHNAME "/%s/%s.zip",versionid,archive);
		argv[0] = "/bin/unzip";
		argv[1] = "-qqo"; /* quiet mode, overwrite files WITHOUT prompting */
		argv[2] = zip;
		argv[3] = "-d";
		argv[4] = dest;
		argv[5] = NULL;
		if ((tid = load_image(5,(const char **)argv,(const char **)environ)) != B_ERROR) {
			if (wait_for_thread(tid,&result) == B_OK) {
				if (result == 0) {
					ok = ERR_OK;
				} else {
					ok = ERR_ZIPERROR;
				}
			} else {
				/* wait_for_thread */
				perror("wait_for_thread");
				ok = ERR_UNKNOWN;
			}
		} else {
			/* load_image */
			perror("load_image");
			ok = ERR_UNKNOWN;
		}
	} else {
		/* touchdir */
		ok = ERR_UNKNOWN;
	}
	
	return ok;
}


static int
unpack_archives(const char *versionid, int fields) {
	char path[MAXPATHLEN];
	int got = 0;

	if (fields & AR_FULLUPDATE) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/fullupdate.zip",versionid);
		if (access(path,R_OK) == 0) {
			if (unzip_archive(versionid,"fullupdate") == 0) {
				fields &= ~AR_FULLUPDATE;
				got |= AR_FULLUPDATE;
			}
		}
	}

	if (fields & AR_DELTADATA) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/deltadata.zip",versionid);
		if (access(path,R_OK) == 0) {
			if (unzip_archive(versionid,"deltadata") == 0) {
				fields &= ~AR_DELTADATA;
				got |= AR_DELTADATA;
			}
		}
	}

	if (fields & AR_VERSIONARCHIVE) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/versionarchive.zip",versionid);
		if (access(path,R_OK) == 0) {
			if (unzip_archive(versionid,"versionarchive") == 0) {
				fields &= ~AR_VERSIONARCHIVE;
				got |= AR_VERSIONARCHIVE;
			}
		}
	}
	return got;
}

int
cache_get_version(const char *versionid,int fields)
{
	char path[MAXPATHLEN];
	int got = 0;

	/*
		Check in the upacked cache area first.
	*/
	if (fields & AR_FULLUPDATE) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/fullupdate",versionid);
		if (access(path,R_OK) == 0) {
			fields &= ~AR_FULLUPDATE;
			got |= AR_FULLUPDATE;
		}
	}

	if (fields & AR_DELTADATA) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/deltadata",versionid);
		if (access(path,R_OK) == 0) {
			fields &= ~AR_DELTADATA;
			got |= AR_DELTADATA;
		}
	}

	if (fields & AR_VERSIONARCHIVE) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/versionarchive",versionid);
		if (access(path,R_OK) == 0) {
			fields &= ~AR_VERSIONARCHIVE;
			got |= AR_VERSIONARCHIVE;
		}
	}

	if (fields & AR_FULLBOOTSTRAP) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/bootstrap.js",versionid);
		if (access(path,R_OK) == 0) {
			fields &= ~AR_FULLBOOTSTRAP;
			got |= AR_FULLBOOTSTRAP;
		}
	}

	/*
		Is there anything left to do?
	*/
	if (fields == 0) {
		return got; /* nope */
	}

	/*
		Now download anything we still need from the remote server.
	*/
	int newGot = db_get_version(versionid, fields);
	if(newGot < 0) return newGot;

	/* unpack the stuff we have */
	unpack_archives(versionid, newGot);
	got |= newGot;
	return got;
}

/* *** */

int
cache_put_version(const struct version_info *info)
{
	/* we just pass through, at the moment */
	return db_put_version(info);
}
