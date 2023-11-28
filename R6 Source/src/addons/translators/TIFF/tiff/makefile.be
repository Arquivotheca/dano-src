#
# 	makefile
#
# 	Copyright (C) 1991-95 Be Incorporated  
# 	All rights reserved.
#

# std part
SHELL		= /bin/sh
OFILE_DIR	= obj

MAKE 		= make
ifeq ($(PLATFORM), ppc)
CFLAGS		= -nosyspath -O7 -DPPC603=1 $(INCLUDES)
CFLAGSNOPT	= -nosyspath -DPPC603=1 $(INCLUDES)
LD 			= mwld
CC 			= mwcc
else
CFLAGS		= -O3 $(INCLUDES)
CFLAGSNOPT	= $(INCLUDES)
LD 			= gcc
CC 			= gcc
endif
LDFLAGS		= -L$(LIBS)
ifeq ($(PLATFORM), ppc)
AR		= mwld -xml -o
else
AR		= ar cr
endif

PUSHD = pushed_dir=`pwd` ; cd 
POPD = cd $$pushed_dir


# installation dependent part
LIBS		= $(BUILDHOME)/libraries
INCLUDES	= -I$(BUILDHOME)/headers
VPATH		= ./$(OFILE_DIR)

.SUFFIXES: .out .o .c .cpp


# libtiff part
LIBTIFF_OFILES	= $(OFILE_DIR)/tif_fax3.o \
	$(OFILE_DIR)/tif_fax4.o \
	$(OFILE_DIR)/tif_aux.o \
	$(OFILE_DIR)/tif_ccittrle.o \
	$(OFILE_DIR)/tif_close.o \
	$(OFILE_DIR)/tif_compress.o \
	$(OFILE_DIR)/tif_dir.o \
	$(OFILE_DIR)/tif_dirinfo.o \
	$(OFILE_DIR)/tif_dirread.o \
	$(OFILE_DIR)/tif_dirwrite.o \
	$(OFILE_DIR)/tif_dumpmode.o \
	$(OFILE_DIR)/tif_error.o \
	$(OFILE_DIR)/tif_getimage.o \
	$(OFILE_DIR)/tif_jpeg.o \
	$(OFILE_DIR)/tif_flush.o \
	$(OFILE_DIR)/tif_lzw.o \
	$(OFILE_DIR)/tif_next.o \
	$(OFILE_DIR)/tif_open.o \
	$(OFILE_DIR)/tif_packbits.o \
	$(OFILE_DIR)/tif_print.o \
	$(OFILE_DIR)/tif_read.o \
	$(OFILE_DIR)/tif_strip.o \
	$(OFILE_DIR)/tif_swab.o \
	$(OFILE_DIR)/tif_thunder.o \
	$(OFILE_DIR)/tif_tile.o \
	$(OFILE_DIR)/tif_be.o \
	$(OFILE_DIR)/tif_version.o \
	$(OFILE_DIR)/tif_warning.o \
	$(OFILE_DIR)/tif_write.o \
	$(OFILE_DIR)/TIFFReader.o
#	$(OFILE_DIR)/fd_handler.o


# targets & rules

all::	libtiff.a

libtiff.a.$(PLATFORM):	clean libtiff.a
	cp -p libtiff.a libtiff.a.$(PLATFORM)

libtiff.a: $(OFILE_DIR) $(LIBTIFF_OFILES)
	$(AR) $@ $(LIBTIFF_OFILES)
	@echo Just made TIFF library

$(OFILE_DIR):
	-mkdir $(OFILE_DIR)

clean:
	@echo Removing libtiff $(OFILE_DIR)
	@-rm -f libtiff.a
	@-rm -f $(OFILE_DIR)/*
	@-rmdir $(OFILE_DIR)

.c.o:
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/$*.o

.cpp.o:
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/$*.o
#
#fd_handler.o : fd_handler.cpp fd_handler.h
#	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/fd_handler.o
#
$(OFILE_DIR)/TIFFReader.o : TIFFReader.cpp TIFFReader.h tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/TIFFReader.o

$(OFILE_DIR)/tif_aux.o : tif_aux.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_aux.o

$(OFILE_DIR)/tif_ccittrle.o : tif_ccittrle.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_ccittrle.o

$(OFILE_DIR)/tif_close.o : tif_close.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_close.o

$(OFILE_DIR)/tif_compress.o : tif_compress.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_compress.o

$(OFILE_DIR)/tif_dir.o : tif_dir.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_dir.o

$(OFILE_DIR)/tif_dirinfo.o : tif_dirinfo.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_dirinfo.o

$(OFILE_DIR)/tif_dirread.o : tif_dirread.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_dirread.o

$(OFILE_DIR)/tif_dirwrite.o : tif_dirwrite.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_dirwrite.o

$(OFILE_DIR)/tif_dumpmode.o : tif_dumpmode.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_dumpmode.o

$(OFILE_DIR)/tif_error.o : tif_error.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_error.o

$(OFILE_DIR)/tif_fax4.o : tif_fax4.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_fax4.o

$(OFILE_DIR)/tif_flush.o : tif_flush.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_flush.o

$(OFILE_DIR)/tif_getimage.o : tif_getimage.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_getimage.o

$(OFILE_DIR)/tif_jpeg.o : tif_jpeg.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_jpeg.o

$(OFILE_DIR)/tif_lzw.o : tif_lzw.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_lzw.o

$(OFILE_DIR)/tif_machdep.o : tif_machdep.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_machdep.o

$(OFILE_DIR)/tif_next.o : tif_next.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_next.o

$(OFILE_DIR)/tif_open.o : tif_open.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_open.o

$(OFILE_DIR)/tif_packbits.o : tif_packbits.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_packbits.o

$(OFILE_DIR)/tif_print.o : tif_print.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_print.o

$(OFILE_DIR)/tif_read.o : tif_read.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_read.o

$(OFILE_DIR)/tif_strip.o : tif_strip.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_strip.o

$(OFILE_DIR)/tif_swab.o : tif_swab.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_swab.o

$(OFILE_DIR)/tif_thunder.o : tif_thunder.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_thunder.o

$(OFILE_DIR)/tif_tile.o : tif_tile.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_tile.o

$(OFILE_DIR)/tif_be.o : tif_be.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_be.o

$(OFILE_DIR)/tif_version.o : tif_version.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_version.o

$(OFILE_DIR)/tif_warning.o : tif_warning.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_warning.o

$(OFILE_DIR)/tif_write.o : tif_write.cpp tiff.h tiffcomp.h tiffconf.h tiffio.h tiffiop.h
	$(CC) $(CFLAGS) -I. -c $< -o $(OFILE_DIR)/tif_write.o

$(OFILE_DIR)/tif_fax3.o: tif_fax3.cpp g3states.h t4.h tif_fax3.h
	$(CC) $(CFLAGSNOPT) -I. -c $< -o $(OFILE_DIR)/tif_fax3.o

$(OFILE_DIR)/g3states.h: mkg3states.cpp t4.h
	$(CC) $(CFLAGS) -I. -c mkg3states.cpp -o mkg3states.o
	$(LD) -o mkg3states mkg3states.o -L$(LIBS)
	./mkg3states -c > g3states.h







