
#ifndef MASTER_H
#define MASTER_H

#define STATION_PATH								"/boot/station/"
#define MY_CONFIG_PATH								"/boot/station/config"
#define CACHE_PATHNAME 								"/boot/station/cache"
#define DEFAULTRECOVERYSETTINGS_PATHNAME 			"/boot/station/vendor/recovery_settings"
#define DELTA_BOOTSTRAP_PATHNAME 					"/boot/station/templates/update_bootstrap"
#define FULL_BOOTSTRAP_PATHNAME 					"/boot/station/templates/full_bootstrap"

#define BINDIR 					"/boot/station/bin/"
#define CP_PATHNAME 			(BINDIR "cp")
#define DD_PATHNAME 			(BINDIR "dd")
#define ZIP_PATHNAME 			(BINDIR "zip")
#define ELFHANDLER_PATHNAME 	(BINDIR "ElfHandler")
#define BEFIRST_PATHNAME 		(BINDIR "BeFirst")
#define ELF2CELF_PATHNAME 		(BINDIR "elf_to_celf")
#define MKCFS_PATHNAME 			(BINDIR "mkcfs")
#define MKFLASH_PATHNAME 		(BINDIR "mkflash")
#define MKLBX_PATHNAME 			(BINDIR "mklbx")
#define RLEIMAGE_PATHNAME 		(BINDIR "rle_image")
#define DSASIG_PATHNAME 		(BINDIR "dsasig")
#define EDITRECOVERY_PATHNAME	(BINDIR "edit_recovery_settings")

#define UNPACK_DICT_PATHNAME 	"beos/system/dict"

extern int myPG;

#endif
