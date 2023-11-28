SRC_DIR := src/commands

include $(call INCLUDE_IF_AVAILABLE, \
	addattr \
	ali \
	asdbg \
	be \
	beinstall \
	boot \
	cal \
	calibrate \
	catattr \
	chop \
	clear \
	clockconfig \
	copyattr \
	copyres \
	cpq_led \
	debug_glue \
	desklink \
	df \
	disktest \
	dstcheck \
	dumpmpct \
	dumppt \
	eject \
	error \
	ffm \
	forcerm \
	format \
	gameaudiomix \
	gameaudioplay \
	gfx_harness \
	hd \
	installbootsector \
	intraff \
	isabridgehack \
	isvolume \
	KeyMapMaker \
	listarea \
	listattr \
	listimage \
	listport \
	listres \
	listsem \
	logger \
	mail \
	makeres \
	mkdos \
	mount \
	mountmemfs \
	netcmds \
	parrot \
	pc \
	pcmcia \
	poke \
	prio \
	ps \
	psloop \
	query \
	release \
	restools \
	rlsecurityid \
	rmattr \
	safemode \
	screentoppm \
	setdprintf \
	setvolumesize \
	strings \
	stripres \
	sysinfo \
	testmike \
	top \
	tput \
	tweakmode \
	unmount \
	ush \
	waitfor \
	xres \
, src/commands)

### Script section

FILES := makebootfloppy

FILES += cc c++

ifneq ($(call MFLAG_IS_SET, COMPATIBILITY_COMPILER_SCRIPTS),)
FILES+= ar ld ranlib
endif

SRC_DIR:=src/commands

MY_INSTALL_DIR:=$(INSTALL_BIN)

PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk

