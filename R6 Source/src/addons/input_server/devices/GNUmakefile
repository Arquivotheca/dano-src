include $(call INCLUDE_IF_AVAILABLE, \
	acer_wp_touch \
	aqua_buttons \
	cop8 \
	dt300 \
	elo \
	input_recorder_device \
	keyboard \
	mouse \
	mk712 \
	netron_display \
	softkeyboard \
	serial_mouse \
, $(BUILDHOME)/src/addons/input_server/devices)

# The wacom driver in the tree isn't up to date -- the third-party version is better
# Also, our wacom driver was never supported.
# It's also documented (right here) as not being in the old-style makefile.
# include $(BUILDHOME)/src/addons/input_server/devices/wacom/GNUmakefile
