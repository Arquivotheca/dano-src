

# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/interface

MY_CFLAGS:= \
	$(F_NO_EXCEPTION) \
	-D_BUILDING_be=1 \
	$(call MFLAG_TO_DFLAG, SUPPORTS_BEOS_ABOUT_BOX) \
#

MY_INCLUDES:=	-I$(BUILDHOME)/src/inc/app_p \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/app_server_p \
			-I$(BUILDHOME)/src/inc/support_p \
			-I$(BUILDHOME)/src/inc/interface_p \
			-I$(BUILDHOME)/src/inc/print_server_p \
			-I$(BUILDHOME)/src/inc/print_p \
			-I$(BUILDHOME)/src/inc \
			-I$(BUILDHOME)/src/inc/input_server_p \
			-I$(BUILDHOME)/$(SRC_DIR) \
#

# Hackery to be able to see the libpng header files (for CursorManager).
MY_INCLUDES     += -I${BUILDHOME}/srcx/gnu/libpng -I$(BUILDHOME)/srcx/gnu/zlib-1.1.3

SRCS :=  \
		AboutBox.cpp \
		Alert.cpp	\
		Bitmap.cpp \
		BitmapCollection.cpp \
		Box.cpp	\
		Button.cpp		\
		ChannelControl.cpp	\
		ChannelSlider.cpp	\
		CheckBox.cpp	\
		ColorControl.cpp	\
		Control.cpp	\
		CursorManager.cpp	\
		Deskbar.cpp \
		Dragger.cpp	\
		Font.cpp \
		FontShared.cpp \
		Glyph.cpp \
		GraphicsEnv.cpp \
		GraphicsDefs.cpp \
		Input.cpp \
		Insets.cpp \
		IRegion.cpp \
		ListView.cpp \
		ListItem.cpp \
		Menu.cpp	\
		MenuBar.cpp	\
		MenuField.cpp	\
		MenuItem.cpp	\
		MenuWindow.cpp	\
		OptionControl.cpp \
		OptionPopUp.cpp \
		OutlineListView.cpp	\
		Picture.cpp			\
		PictureButton.cpp	\
		PictureTranslation.cpp \
		Point.cpp		\
		Polygon.cpp	\
		PopUpMenu.cpp	\
		PrivateScreen.cpp \
		PrivateSlideRule.cpp \
		RadioButton.cpp	\
		Rect.cpp		\
		Region.cpp		\
		RenderView.cpp  \
		RulerView.cpp	\
		SaveScreen.cpp	\
		Screen.cpp	\
		ScrollBar.cpp	\
		ScrollView.cpp	\
		Shape.cpp	\
		Shelf.cpp	\
		Slider.cpp \
		SlideRule.cpp \
		StatusBar.cpp	\
		StringView.cpp	\
		TabView.cpp \
		TextControl.cpp	\
		TextListControl.cpp	\
		TextView.cpp	\
		TextViewSupport.cpp	\
		ToolTip.cpp \
		Transform2d.cpp \
		TraverseViews.cpp	\
		View.cpp	\
		Window.cpp 	\
		PrintJob.cpp \
		DrawEngine.cpp \
		LBXFormat0.cpp \
		LBXFormat1.cpp \
		Unpack.cpp

ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_REMOTE_DISPLAY),)
MY_DFLAGS	+= -DREMOTE_DISPLAY
endif

MY_DFLAGS += $(call MFLAG_TO_DFLAG, PRINTING_FOR_DESKTOP)

ifneq ($(call MFLAG_IS_SET, ROTATE_DISPLAY),)
MY_DFLAGS	+= -DROTATE_DISPLAY=1
endif

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk
endif

