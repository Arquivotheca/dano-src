# Makefile

TARGET_NAME:=pdf.so

SRC_DIR:=src/addons/www_content/pdf

SRCS := PDFContent.cpp \
	pdf_doc.cpp \
 	Atomizer.cpp Transform2D.cpp \
	md5_dgst.c md5_one.c rc4_enc.c rc4_skey.c crypto.cpp \
	MultiPositionIO.cpp CachingPositionIO.cpp ConnectedIO.cpp \
	DocFramework.cpp ToolView.cpp \
	PDFPageView.cpp \
	NumControl.cpp ZoomControl.cpp\
	FontEngine.cpp FontUtils.cpp \
	Filter.cpp PositionIOFilter.cpp \
	ArrayBuilder.cpp \
	BreakAwayPusher.cpp \
	IndirectBuilder.cpp \
	ObjectParser.cpp \
	ObjectSink.cpp \
	Objectizer.cpp \
	Pusher.cpp \
	PushContents.cpp \
	ASCIIHexDecode.cpp \
	ASCII85Decode.cpp \
	LZWDecode.cpp \
	RunLengthDecode.cpp \
	CCITTFaxDecode.cpp \
	DCTDecode.cpp \
	RC4Decode.cpp \
	PredictedDecode.cpp \
	FlateDecode.cpp \
	DataIOSink.cpp \
	DescriptionEngine.cpp \
	RenderEngine.cpp \
	PDFObjectFinder.cpp \
	PDFDisplay.cpp \
	SampleExpander.cpp \
	LUTPusher.cpp \
	GreyToRGB.cpp \
	CMYKtoRGB.cpp \
	SimpleImage.cpp \
	StencilImage.cpp \
	ImageChain.cpp \
	HashCache.cpp \
	Object2.cpp \
	PDFAlert.cpp \
	cachemgr.c
#	PDFPrintJob.cpp \
#	TeePusher.cpp 	# for debugging, not part of a regular build
#	CMapEngine.cpp \
#	NullPusher.cpp	# for debugging, not part of a regular build

#MY_CFLAGS := $(F_NO_EXCEPTIONS) -Wall -W -Wno-multichar -fno-omit-frame-pointer  -Wno-ctor-dtor-privacy -DDEBUG=1 -fno-inline -g -DUSE_TEE_PUSHER=0
#MY_CFLAGS := $(F_NO_EXCEPTIONS) -Wall -W -Wno-multichar -fno-omit-frame-pointer  -Wno-ctor-dtor-privacy -DDEBUG=1 -fno-inline -g -DUSE_TEE_PUSHER=0 -DNDEBUG=1
#MY_CFLAGS += -fcheck-memory-usage -D_KERNEL_MODE
MY_CFLAGS := $(F_NO_EXCEPTIONS) -Wall -W -Wno-multichar -Wno-unused  -Wno-ctor-dtor-privacy -DNDEBUG=1

MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/src/kit/font/fontfusion/core -I$(BUILDHOME)/src/kit/jpegread
#MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/srcx/bitstream/fontfusion2/core -I$(BUILDHOME)/src/inc/jpegread_p
#MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/$(SRC_DIR)/freetype -I$(BUILDHOME)/src/inc/jpegread_p

SYSTEM_LIBS:= libroot.so libbe.so libwww.so libfont.so libz.a libjpegread.so
#SYSTEM_LIBS:= libroot.so libbe.so libwww.so libfontfusion2.a libz.a libjpegread.so
#SYSTEM_LIBS:= libroot.so libbe.so libwww.so libfreetype.a libz.a libjpegread.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

# Install the identifier attributes for this add-on.
# BE AWARE: Any changes to these attributes should also
# be made to the factory class.
# Also note that we set a modification time of 0, so
# the attributes will never be overwritten from the
# executable, even if it is later changed.
ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/pdf"				\
			"extensions" "pdf"						\
			"plugin_ids" "Adobe Acrobat"					\
			"plugin_desc" "BeIA PDF Content Viewer (Adobe Acrobat v4.0)"				\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

#include $(BUILDHOME)/src/addons/www_content/pdf/fontfusion/$(MAKEFILE_LEAFNAME)
