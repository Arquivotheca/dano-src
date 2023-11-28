/* ++++++++++
	FILE:	find_dir.c
	REVS:	$Revision: 1.22 $
	NAME:	herold
	DATE:	Mon Apr 28 17:20:31 PDT 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <Errors.h>
#include <fs_info.h>
#include <FindDirectory.h>
#include <Errors.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#include <pwd.h>

#include "priv_syscalls.h"

#define HOM "$h"


/* COMMON and USER config areas are both under /boot/home/... when this is set */
#define COMBINED_CONFIG 1

/* ----------
	create_directory - create all components of a full path.

	Assumes path is absolute, no trailing slash, and the last element
	is a directory to be made.
----- */
	
static status_t
create_directory (char *path, mode_t mode)
{
	char		*stake;
	status_t	err;

	if (access(path, F_OK) == 0)
		return B_OK;
	
	stake = path;
	while (1) {

		stake = strchr (stake + 1, '/');	/* find next slash */

		if (stake)							/* remove slash */
			*stake = '\0';

		err = B_OK;
		if (mkdir(path, mode))
			err = errno;					/* make what we have so far */

		if (stake)							/* replace slash */
			*stake = '/';

		if (err != B_OK && err != EEXIST)	/* problems? */
			return err;

		if (!stake)							/* leaf has no trailing slash */
			break;
	}
	return B_OK;
}  


/* ----------
	find_directory - return the path for a well know directory.
----- */

status_t
find_directory (
	directory_which	which,
	dev_t			vol_dev,
	bool			create_it,
	char			*path,
	int32			path_size)
{
	typedef const char *cp_array[];		/* typdef for readability */

	static cp_array per_vol_dirs_bfs = {
		"home/Desktop",					/* B_DESKTOP_DIRECTORY */
		"home/Desktop/Trash"			/* B_TRASH_DIRECTORY */
	};

	static cp_array beos_dirs = {
		"beos",							/* B_BEOS_DIRECTORY */
		"beos/system",					/* B_BEOS_SYSTEM_DIRECTORY */
		"beos/system/add-ons",			/* B_BEOS_ADDONS_DIRECTORY */
		"beos/system/boot",				/* B_BEOS_BOOT_DIRECTORY */
		"beos/etc/fonts",				/* B_BEOS_FONTS_DIRECTORY */
		"beos/system/lib",				/* B_BEOS_LIB_DIRECTORY */
		"beos/system/servers",			/* B_BEOS_SERVERS_DIRECTORY */
		"beos/apps",					/* B_BEOS_APPS_DIRECTORY */
		"beos/bin",						/* B_BEOS_BIN_DIRECTORY */
		"beos/etc",						/* B_BEOS_ETC_DIRECTORY */
		"beos/documentation",			/* B_BEOS_DOCUMENTATION_DIRECTORY */
		"beos/preferences",				/* B_BEOS_PREFERENCES_DIRECTORY */
		"beos/system/add-ons/Translators",
						/* B_BEOS_TRANSLATORS_DIRECTORY */
		"beos/system/add-ons/media",		/* B_BEOS_MEDIA_NODES_DIRECTORY */
		"beos/etc/sounds",				/* B_BEOS_SOUNDS_DIRECTORY */
	};
#if COMBINED_CONFIG
	static cp_array com_dirs = {
		HOM,							/* B_COMMON_DIRECTORY */
		HOM"/config",					/* B_COMMON_SYSTEM_DIRECTORY */
		HOM"/config/add-ons",			/* B_COMMON_ADDONS_DIRECTORY */
		HOM"/config/boot",				/* B_COMMON_BOOT_DIRECTORY */
		HOM"/config/fonts",				/* B_COMMON_FONTS_DIRECTORY */
		HOM"/config/lib",				/* B_COMMON_LIB_DIRECTORY */
		HOM"/config/servers",			/* B_COMMON_SERVERS_DIRECTORY */
		HOM"/config/bin",				/* B_COMMON_BIN_DIRECTORY */
		HOM"/config/etc",				/* B_COMMON_ETC_DIRECTORY */
		HOM"/config/documentation",		/* B_COMMON_DOCUMENTATION_DIRECTORY */
		HOM"/config/settings",			/* B_COMMON_SETTINGS_DIRECTORY */
		"develop",						/* B_COMMON_DEVELOP_DIRECTORY */
		"var/log",						/* B_COMMON_LOG_DIRECTORY */
		"var/spool",					/* B_COMMON_SPOOL_DIRECTORY */
		"var/tmp",						/* B_COMMON_TEMP_DIRECTORY */
		"var",							/* B_COMMON_VAR_DIRECTORY */
		HOM"/config/add-ons/Translators",
										/* B_COMMON_TRANSLATORS_DIRECTORY */
		HOM"/config/add-ons/media",		/* B_COMMON_MEDIA_NODES_DIRECTORY */
		HOM"/config/sounds",			/* B_COMMON_SOUNDS_DIRECTORY */
	};
#else
	static cp_array com_dirs = {
		"",						    /* B_COMMON_DIRECTORY */
		"config",					/* B_COMMON_SYSTEM_DIRECTORY */
		"config/add-ons",			/* B_COMMON_ADDONS_DIRECTORY */
		"config/boot",			    /* B_COMMON_BOOT_DIRECTORY */
		"config/fonts",			    /* B_COMMON_FONTS_DIRECTORY */
		"config/lib",				/* B_COMMON_LIB_DIRECTORY */
		"config/servers",			/* B_COMMON_SERVERS_DIRECTORY */
		"config/bin",				/* B_COMMON_BIN_DIRECTORY */
		"config/etc",				/* B_COMMON_ETC_DIRECTORY */
		"config/documentation",	    /* B_COMMON_DOCUMENTATION_DIRECTORY */
		"config/settings",		    /* B_COMMON_SETTINGS_DIRECTORY */
		"develop",						/* B_COMMON_DEVELOP_DIRECTORY */
		"var/log",						/* B_COMMON_LOG_DIRECTORY */
		"var/spool",					/* B_COMMON_SPOOL_DIRECTORY */
		"var/tmp",						/* B_COMMON_TEMP_DIRECTORY */
		"var",							/* B_COMMON_VAR_DIRECTORY */
		"config/add-ons/Translators",
										/* B_COMMON_TRANSLATORS_DIRECTORY */
		"config/add-ons/media",	    /* B_COMMON_MEDIA_NODES_DIRECTORY */
		"config/sounds",			/* B_COMMON_SOUNDS_DIRECTORY */
	};
#endif
	static cp_array user_dirs = {
		HOM"",							/* B_USER_DIRECTORY */
		HOM"/config",					/* B_USER_CONFIG_DIRECTORY */
		HOM"/config/add-ons",			/* B_USER_ADDONS_DIRECTORY */
		HOM"/config/boot",				/* B_USER_BOOT_DIRECTORY */
		HOM"/config/fonts",				/* B_USER_FONTS_DIRECTORY */
		HOM"/config/lib",				/* B_USER_LIB_DIRECTORY */
		HOM"/config/settings",			/* B_USER_SETTINGS_DIRECTORY */
		HOM"/config/be",				/* B_USER_DESKBAR_DIRECTORY */
		HOM"/config/settings/printers",	/* B_USER_PRINTERS_DIRECTORY */
		HOM"/config/add-ons/Translators",
										/* B_USER_TRANSLATORS_DIRECTORY */
		HOM"/config/add-ons/media",		/* B_USER_MEDIA_NODES_DIRECTORY */
		HOM"/config/sounds",			/* B_USER_SOUNDS_DIRECTORY */
	};
	static cp_array global_dirs = {
		"apps",							/* B_APPS_DIRECTORY */
		"preferences",					/* B_PREFERENCES_DIRECTORY */
		"utilities",					/* B_UTILITIES_DIRECTORY */
	};

	static cp_array per_vol_dirs_ofs = {
		"desktop",						/* B_DESKTOP_DIRECTORY */
		"desktop/Trash"					/* B_TRASH_DIRECTORY */
	};

	static cp_array per_vol_dirs_hfs = {
		"Desktop Folder",				/* B_DESKTOP_DIRECTORY */
		"Trash"							/* B_TRASH_DIRECTORY */
	};

	static cp_array per_vol_dirs_dos = {
		NULL,							/* B_DESKTOP_DIRECTORY */
		"RECYCLED/_BEOS_"				/* B_TRASH_DIRECTORY */
	};

	static struct tbl {
		cp_array	*paths;
		int			max_path;
	} tables[] = {
		&per_vol_dirs_bfs, sizeof (per_vol_dirs_bfs) / sizeof (per_vol_dirs_bfs[0]),
		&beos_dirs, sizeof (beos_dirs) / sizeof (beos_dirs[0]),
		&com_dirs, sizeof (com_dirs) / sizeof (com_dirs[0]),
		&user_dirs, sizeof (user_dirs) / sizeof (user_dirs[0]),
		&global_dirs, sizeof (global_dirs) / sizeof (global_dirs[0])
	};

	static struct pervol {
		const char		*name;
		cp_array		*tbl;
	} pv_tbl[] = {
		"bfs", 	&per_vol_dirs_bfs,
		"hfs", 	&per_vol_dirs_hfs,
		"ofs", 	&per_vol_dirs_ofs,
		"dos", 	&per_vol_dirs_dos
	};

	status_t		retval = B_OK;
	struct tbl		*t;
	int				i;
	const char		*p = NULL;
	const char		*mount_point = "/boot/";
	int				mount_point_len = strlen ("/boot/");
	char			*vol_name = NULL;
	fs_info			fs;
	int				is_boot_volume = FALSE;

	/* special case for the root directory */

	if (which==B_ROOT_DIRECTORY) {
		strcpy(path,"/");
		return B_OK;
	}

	/* first see which table we will use */

	i = which / 1000;
	if (i < 0 || i >= sizeof(tables)/sizeof(tables[0])) {
		retval = B_BAD_VALUE;
		goto done;
	}

	t = tables + i;
	which %= 1000;

	/* now check if in range for this table */

	if (which >= t->max_path || which < 0) {
		retval = B_BAD_VALUE;
		goto done;
	}
		
	vol_name = (char*) malloc(path_size);

	/* handle per-volume directories */
	if (i == 0 && vol_dev >= 0) {
		/* lookup boot volume, see if requested volume is boot */
		status_t err = fs_stat_dev(dev_for_path("/boot"), &fs);
		if (err == B_OK && fs.dev == vol_dev) {
			strcpy (vol_name, "/boot/");
			is_boot_volume = true;
		} else {
			char dirent_buffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
			int vnfd;
			int dir_fd;
			char *end;
							
			/* lookup volume name, volume type */
			if (fs_stat_dev(vol_dev, &fs) != B_OK) {
				retval = ENODEV;
				goto done;
			}
			
			/* Get the dirent for the first entry in this directory. */
			vnfd = _kopen_vn_(vol_dev, fs.root, NULL, O_RDONLY, true);
			if (vnfd < 0) {
				retval = vnfd;
				goto done;
			}
		
			dir_fd = _kopendir_(vnfd, NULL, true);
			_kclose_(vnfd);

			if (dir_fd < 0) {
				retval = dir_fd;
				goto done;
			}

			err = _kreaddir_(dir_fd, (struct dirent*) dirent_buffer,
				sizeof(struct dirent) + B_FILE_NAME_LENGTH, 1);
			_kclosedir_(dir_fd);

			if (err < 0) {
				retval = err;
				goto done;
			}
							
			if (err == 0) {
				retval = ENOENT;
				goto done;
			}
				
			/* Get the path for the dirent I just got */
			err = get_path_for_dirent((struct dirent*) dirent_buffer, vol_name,
				path_size);
			if (err < 0) {
				retval = err;
				goto done;
			}
					
			/* Strip off the entry name to get the path to this directory */
			for (end = vol_name + strlen(vol_name); end >= vol_name; end--) {
				if (*end == '/')
					break;
			}

			*(end + 1) = '\0';					
		}

		mount_point = vol_name;
		mount_point_len = strlen (mount_point);

		for (i = 0; i < sizeof (pv_tbl) / sizeof (pv_tbl[0]); i++) {
			if (!strcmp (fs.fsh_name, pv_tbl[i].name)) {
				p = (*pv_tbl[i].tbl)[which];
				break;
			}
		}
	} else
		p = (*t->paths)[which];

	if (p == NULL) {
		retval = ENOENT;
		goto done;
	}
	/*
	 *	If this is the home directory, it is always on the
	 *	boot volume. ... or something -bjs
	 */
	i = strlen (HOM);
	if (!strncmp (HOM, p, i)) {
		struct passwd *pw;
		p += i;
		if(pw = getpwuid(getuid())){
			mount_point = pw->pw_dir;
		} else {
			if (!(mount_point = getenv ("HOME")))
				mount_point = "/boot/home";
		}
		mount_point_len = strlen (mount_point);
	}
	
	/* now check if passed buffer is big enough */
	if (mount_point_len + strlen(p) > path_size - 1) {
		retval = E2BIG;
		goto done;
	}

	strcpy (path, mount_point);
	strcpy (path + mount_point_len, p);
	if (create_it)
		retval = create_directory (path, 0755);
		
done:
	free(vol_name);

	return retval;
}
