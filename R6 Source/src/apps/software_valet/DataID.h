
// general header tags
enum {
	ID_PKGHEADER =		'PhIn',

	ID_FORMAT_VERS =	'FVer',
	ID_ARCFLAGS =		'AFla',
	ID_FILESIZE =		'FSiz',
	ID_CATALOG_OFFSET =	'COff',
	ID_ATTRIB_OFFSET =	'AOff'
};

// catalog entry tags
enum {
	// folder entry
	ID_FOLDERITEM =		'FldI',
	
	// list of folder attributes
	ID_FOLDERDATA =		'FDat',
	
	// list of items in folder
	ID_FOLDITEMS =		'FIts',
	
	// items in folder
	ID_FILEITEM =		'FilI',
	ID_PATCHITEM =		'PtcI',
	ID_SCRIPTITEM =		'ScrI',
	ID_EXECITEM = 		'ExcI',
	ID_LINKITEM =		'LnkI'
};

// general entry tags
enum {
	ID_NAME	=	'Name',
	ID_MIME =	'Mime',
	ID_MODE	=	'Mode',
	ID_DEST =	'Dest',
	ID_CUST =	'Cust',
	ID_REPL =	'Repl',
	ID_CTIME =	'CTim',
	ID_MTIME =	'MTim',
	ID_VERS =	'Vers',
	ID_GROUPS =	'Grps',
	ID_PLAT =	'Plat'
};

// file tags
enum {
	ID_OFFSET =		'OffT',
	ID_CMPSIZE	=	'CmpS',
	ID_ORGSIZE =	'OrgS',
	ID_OLDSIZE =	'OldS',
	ID_APP_SIG =	'ASig',
	ID_VERSINFO =	'VrsI'		// PR2 style version info
};

// symlink tags
enum {
	ID_LINK =		'Link'
};

// begin package attributes
enum {
	ID_PKGATTR	=	'PkgA'
};

// attributes stored in the general attributes section
enum {
	A_MASTER_GROUP_LIST = 1,
	A_VIEW_GROUP_LIST,
	A_DEFAULT_DEST_LIST,
	A_CUST_DEST_LIST,
	A_INST_DESCRIPTION,
	A_INSTALLFOLDER,
	A_LICENSE,
	A_SPLASH_SCREEN,
	A_FILECOUNT,
	A_PACKAGE_HELP,
	A_GROUP_HELP,
	A_FOLDER_POPUP,
	A_PACK_NAME,
	A_PACK_VERSION,
	A_PACK_DEVELOPER,
	A_PACK_REL_DATE,
	A_PACK_DESCRIPTION,
	A_LICENSE_FILE,
	A_PACK_FLAGS,
	A_PACK_DEPOT_SERIAL =	20,
	A_LICENSE_STYLE =		21,
	A_SOFT_TYPE =			22,
	A_PREFIX_ID	=			23,
	A_VERSION_ID =			24
};

// attributes sub section
enum {
	ID_GRP_ITEM =		'IGrp',
	ID_GRP_NAME =		'GrpN',
	ID_GRP_DESC =		'GrpD',
	ID_GRP_HTXT =		'GrHt',
	ID_GRP_INDX =		'GrId',
	
	ID_DEST_PATH =		'DPat',
	ID_PATHNAME =		'PaNa',
	ID_FIND_DEST =		'FDst', // PR2 style destination index

	ID_DEST_QUERY =		'DQue',
	ID_QUERY_TITLE	=	'DQTi',
	ID_QUERY_SIZE	=	'DQSz',
	ID_QUERY_MIME	=	'DQMi',
	
	ID_DO_LICENSE =		'Lic?',
	ID_LICENSE_PATH =	'LicP'
};
