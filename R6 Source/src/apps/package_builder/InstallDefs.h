// InstallDefs.h
#ifndef _INSTALLDEFS_H
#define _INSTALLDEFS_H


#define SIZE_RES_TYPE		'INsz'
#define RESOURCE_TYPE	'INst'
#define INSTALLER_SIZE	84901

enum {
	header_id = 0,
	package_id,
	catalog_id,
	attrib_id,
	readme_id,
	license_id,
	license_size_id,
	size_id
};
#endif
