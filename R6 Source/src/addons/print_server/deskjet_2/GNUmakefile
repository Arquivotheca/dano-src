# makefile

SRC_DIR := src/addons/print_server/deskjet_2

TARGET_NAME := Deskjet

SRCS :=	Compression.cpp  DJ690_maps.cpp   Font.cpp            TextManager.cpp  cgtimes.cpp        imager.cpp       version.cpp		\
		Context.cpp      DJ6XX.cpp        Header.cpp          TextSupport.cpp  courier.cpp        imager_open.cpp  versioncode.cpp	\
		DJ600.cpp        DJ895.cpp        IO_Utils.cpp        Translator.cpp   create_so.cpp      interp_open.cpp					\
		DJ600_maps.cpp   DJ895_maps.cpp   Job.cpp             aladdin.cpp      creator.cpp        ltrgothic.cpp						\
		DJ630.cpp        DJ895_maps2.cpp  Printer.cpp         ap2100.cpp       filterhpa.cpp      rgbtocmyk.cpp						\
		DJ660.cpp        DJ970_maps.cpp   Registry.cpp        breaks_open.cpp  globals.cpp        scaler_open.cpp					\
		DJ660_maps.cpp   DJ970_maps2.cpp  ResSynth.cpp        broadway.cpp     script.cpp											\
		DJ690.cpp        SystemServices.cpp capture.cpp       htfed_open.cpp   univers.cpp        DeskjetBeOS.cpp

MY_CFLAGS := -D_LITTLE_ENDIAN -D_DJ600 -D_DJ630 -D_DJ6xx -D_DJ6xxPhoto -D_DJ8xx -D_DJ9xx -D_DJ9xxVIP -D_APOLLO2100
MY_INCLUDES := -I$(BUILDHOME)/$(SRC_DIR)/add_ons -I$(BUILDHOME)/$(SRC_DIR)

SYSTEM_LIBS := libroot.so libbe.so
PARENT_IMAGE :=

HAS_MAIN :=

MY_INSTALL_DIR := $(INSTALL_B_BEOS_ADDONS_DIRECTORY)/Print

LINK_DIRS :=
SIGNATURE :=
RESOURCES :=
ATTRIBUTES :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
