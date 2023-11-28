
include $(call INCLUDE_IF_AVAILABLE, \
	TellBrowser \
	alert \
	bdbproxy \
	beep \
	binder \
	bootcomplete \
	bootmode \
	bprof \
	cifsmount \
	console \
	craw \
	db \
	diffa \
	doch_format \
	dsasig \
	durango_tv \
	edline \
	finddir \
	howdy \
	index \
	installsound \
	iroster \
	keymap \
	kill \
	listdev \
	mail_parser \
	mimeset \
	mountvolume \
	nut \
	play \
	playfile \
	prepare \
	print \
	profile \
	quit \
	rescan \
	roster \
	screen_blanker \
	seminfo \
	shutdown \
	sndcmd \
	staticip \
	strace \
	translate \
	updated \
	version \
	xtou \
, src/commands/be)

ifeq ($(call MFLAG_IS_SET,USE_IAD_BOOTMAN_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	bootman \
, src/commands/be)
else
include $(call INCLUDE_IF_AVAILABLE, \
	bootman_iad \
, src/commands/be)
endif

ifeq ($(call MFLAG_IS_SET,USE_IAD_MAKEBOOTABLE_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	makebootable \
, src/commands/be)
else
include $(call INCLUDE_IF_AVAILABLE, \
	makebootable_iad \
, src/commands/be)
endif

ifeq ($(call MFLAG_IS_SET,USE_IAD_WRITEMBR_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	writembr \
, src/commands/be)
else
include $(call INCLUDE_IF_AVAILABLE, \
	writembr_iad \
, src/commands/be)
endif

