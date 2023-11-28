###############################################################################
## BeOS Generic Driver Skeleton
##
##    This file contains a list of objects that need to be built for the
## generic accelerant.
##
###############################################################################


###############################################################################
## Objects ####################################################################
###############################################################################

## Path of the generic accelerant source files.

GDS_ACC_PATH = ../alt.software/skel

## List of generic accelerant objects.

GDS_ACC_OBJS = $(GDS_ACC_PATH)/accel_util.o         \
               $(GDS_ACC_PATH)/entrypoint.o         \
               $(GDS_ACC_PATH)/globals.o            \
               $(GDS_ACC_PATH)/hooks_cursor.o       \
               $(GDS_ACC_PATH)/hooks_dpms.o         \
               $(GDS_ACC_PATH)/hooks_init.o         \
               $(GDS_ACC_PATH)/hooks_mode.o         \
               $(GDS_ACC_PATH)/hooks_sync.o         \
               $(GDS_ACC_PATH)/standard_modes.o


###############################################################################
## This Is The End Of The File ################################################
###############################################################################
