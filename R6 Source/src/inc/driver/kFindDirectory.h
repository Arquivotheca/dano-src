#ifndef _kFIND_DIRECTORY_H
#define _kFIND_DIRECTORY_H

#ifdef __cplusplus
extern "C" {
#endif

#define K_DESKTOP_DIRECTORY					"home/Desktop"
#define K_TRASH_DIRECTORY					"home/Desktop/Trash"
#define K_BEOS_DIRECTORY					"beos"
#define K_BEOS_SYSTEM_DIRECTORY				"beos/system"
#define K_BEOS_ADDONS_DIRECTORY				"beos/system/add-ons"
#define K_BEOS_BOOT_DIRECTORY				"beos/system/boot"
#define K_BEOS_FONTS_DIRECTORY				"beos/etc/fonts"
#define K_BEOS_LIB_DIRECTORY				"beos/system/lib"
#define K_BEOS_SERVERS_DIRECTORY			"beos/system/servers"
#define K_BEOS_APPS_DIRECTORY				"beos/apps"
#define K_BEOS_BIN_DIRECTORY				"beos/bin"
#define K_BEOS_ETC_DIRECTORY				"beos/etc"
#define K_BEOS_DOCUMENTATION_DIRECTORY		"beos/documentation"
#define K_BEOS_PREFERENCES_DIRECTORY		"beos/preferences"
#define K_BEOS_TRANSLATORS_DIRECTORY		"beos/system/add-ons/Translators"
#define K_BEOS_MEDIA_NODES_DIRECTORY		"beos/system/addons/media"
#define K_BEOS_SOUNDS_DIRECTORY				"beos/etc/sounds"

#define K_COMMON_DIRECTORY					"home"
#define K_COMMON_SYSTEM_DIRECTORY			"home/config"
#define K_COMMON_ADDONS_DIRECTORY			"home/config/add-ons"
#define K_COMMON_BOOT_DIRECTORY				"home/config/boot"
#define K_COMMON_FONTS_DIRECTORY			"home/config/fonts"
#define K_COMMON_LIB_DIRECTORY				"home/config/lib"
#define K_COMMON_SERVERS_DIRECTORY			"home/config/servers"
#define K_COMMON_BIN_DIRECTORY				"home/config/bin"
#define K_COMMON_ETC_DIRECTORY				"home/config/etc"
#define K_COMMON_DOCUMENTATION_DIRECTORY	"home/config/documentation"
#define K_COMMON_SETTINGS_DIRECTORY			"home/config/settings"
#define K_COMMON_DEVELOP_DIRECTORY			"develop"
#define K_COMMON_LOG_DIRECTORY				"var/log"
#define K_COMMON_SPOOL_DIRECTORY			"var/spool"
#define K_COMMON_TEMP_DIRECTORY				"var/tmp"
#define K_COMMON_VAR_DIRECTORY				"var"
#define K_COMMON_TRANSLATORS_DIRECTORY		"home/config/add-ons/Translators"
#define K_COMMON_MEDIA_NODES_DIRECTORY		"home/config/add-ons/media"
#define K_COMMON_SOUNDS_DIRECTORY			"home/config/sounds"

/* It is unwise to reference the user directories except through the userlevel
 * API. */
#define K_USER_DIRECTORY					"home"
#define K_USER_CONFIG_DIRECTORY				"home/config"
#define K_USER_ADDONS_DIRECTORY				"home/config/add-ons"
#define K_USER_BOOT_DIRECTORY				"home/config/boot"
#define K_USER_FONTS_DIRECTORY				"home/config/fonts"
#define K_USER_LIB_DIRECTORY				"home/config/lib"
#define K_USER_SETTINGS_DIRECTORY			"home/config/settings"
#define K_USER_DESKBAR_DIRECTORY			"home/config/be"
#define K_USER_PRINTERS_DIRECTORY			"home/config/settings/printers"
#define K_USER_TRANSLATORS_DIRECTORY		"home/config/add-ons/Translators"
#define K_USER_MEDIA_NODES_DIRECTORY		"home/config/add-ons/media"
#define K_USER_SOUNDS_DIRECTORY				"home/config/sounds"

#define K_APPS_DIRECTORY					"apps"
#define K_PREFERENCES_DIRECTORY				"preferences"
#define K_UTILITIES_DIRECTORY				"utilities"

#ifdef __cplusplus
}
#endif

#endif
