# Makefile

SOURCE:=opera.so

SRC_DIR:=src/addons/www_content/operabin

TARGET:= $(INSTALL_ADD_ONS)/web/$(SOURCE)

ELF_FILES+=$(TARGET)

ATTRIBUTES:=-complex raw -t time 0 -t message				\
			"mime_types" "text/html"						\
			"mime_types" "text/plain"						\
			"mime_types" "application/x-wagner-css"			\
			"mime_types" "application/x-wagner-javascript"	\
			"extensions" "html"								\
			"extensions" "htm"								\
			"extensions" "css"								\
			"extensions" "js"								\
			-end											\
			-name "be:content"

PERMISSIONS:=$(PERMS_SYSTEM_DATA)
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
