# Makefile

SOURCE:=flash.so

SRC_DIR:=src/addons/www_content/flashbin

TARGET:= $(INSTALL_ADD_ONS)/web/$(SOURCE)

ELF_FILES+=$(TARGET)

ATTRIBUTES:=-complex raw -t time 0 -t message				\
			"mime_types" "application/x-shockwave-flash"	\
			"extensions" "swf"								\
			"plugin_ids" "Shockwave Flash"					\
			"plugin_desc" "Shockwave Flash 4.0"				\
			-end											\
			-name "be:content"

PERMISSIONS:=$(PERMS_SYSTEM_DATA)

MY_DEPS:=$(MAKEFILE_LEAFNAME) \
	$(BELIBRARIES)/libroot.so \
	$(BELIBRARIES)/libbe.so \
	$(BELIBRARIES)/libmedia.so \
	$(BELIBRARIES)/libwww.so \
	$(BELIBRARIES)/libtextencoding.so

include $(SDIR_TEMPLATES)/DataFileTemplate.mk

