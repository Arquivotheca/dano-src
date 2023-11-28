
include $(call INCLUDE_IF_AVAILABLE, \
	bmp \
	bookmarks \
	download \
	contact_manager \
	email_address \
	file_browser \
	file_browser2 \
	gif \
	japanese_im \
	jpeg \
	keytrap \
	lbx \
	mediaplayer2 \
	merlin \
	midi_rmf \
	minitel \
	multipart \
	pdf \
	png \
	pulse \
	real \
	status \
	softkeyboard \
	update \
	upload \
	videoplayer \
, $(BUILDHOME)/src/addons/www_content)


# As I was doing something similar for opera, I decided to impliment this
# for flash as well. (steven 00-09-11)
ifeq ($(wildcard $(BUILDHOME)/src/addons/www_content/flash/GNUmakefile),)
include $(BUILDHOME)/src/addons/www_content/flashbin/$(MAKEFILE_LEAFNAME)
else
include $(BUILDHOME)/src/addons/www_content/flash/$(MAKEFILE_LEAFNAME)
endif

# ns_plugin is documented (right here) as not existing in the old makefile
# include $(BUILDHOME)/src/addons/www_content/ns_plugin/GNUmakefile

# This mimics the behavior of 'ifeq ($(USER),geh)' in the old makefile
# in a more appropriate manner. (steven 00-09-11)
ifeq ($(wildcard $(BUILDHOME)/src/addons/www_content/opera/GNUmakefile),)
include $(BUILDHOME)/src/addons/www_content/operabin/$(MAKEFILE_LEAFNAME)
else
include $(BUILDHOME)/src/addons/www_content/opera/$(MAKEFILE_LEAFNAME)
endif

