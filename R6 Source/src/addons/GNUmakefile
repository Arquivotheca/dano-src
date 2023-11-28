
include $(call INCLUDE_IF_AVAILABLE, \
	accelerants \
	app_server \
	binder \
	drive_setup \
	email \
	input_server \
	media \
	pcsc \
	print_server \
	reseditor \
	screen_saver \
	smoove \
	tracker \
	www_content \
	www_protocol \
, src/addons)

ifeq ($(call MFLAG_IS_SET,USE_IAD_TRANSLATORS_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	translators \
, src/addons)
else
include $(call INCLUDE_IF_AVAILABLE, \
	translators2 \
, src/addons)
endif

