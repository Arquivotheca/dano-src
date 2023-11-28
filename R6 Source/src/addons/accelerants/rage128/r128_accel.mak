###############################################################################
## BeOS Graphics Accelerant - r128 Driver
##
## FILE : r128_accel.mak
## HIST :
##    Original version written by Todd Showalter.
##    Modified for use with the generic driver skeleton by Christopher Thomas.
##
###############################################################################


###############################################################################
## Make Settings ##############################################################
###############################################################################

.SUFFIXES: .o .c .cpp


###############################################################################
## Project Targets ############################################################
###############################################################################

NAME       = rage128.accelerant
BINDIR     = bin
DRIVER     = $(BINDIR)/$(NAME)
INSTALLDIR = /boot/beos/system/add-ons/accelerants/$(NAME)


###############################################################################
## Includes ###################################################################
###############################################################################

## Include generic driver skeleton targets.

include ../skeleton_accelerant/accel_objs.make


###############################################################################
## Tools ######################################################################
###############################################################################

###############################################################################
## C Compiler
##    I've turned on maximum warnings, with the requirement for prototypes for
## all functions.  I find this saves me a lot of time when dealing with other
## people's code, and it doesn't hurt with mine, either.  I've turned on the
## -pedantic argument because it catches a few things that -Wall misses, and
## because when code builds cleanly under -Wall -pedantic it is more likely
## to be clean.
##    I like my code to compile at the highest possible warning level with no
## warnings.  I figure if you don't know the C incantations to make the
## warning go away, you don't know what you're doing well enough to leave the
## code generating said warning in place.
##    As a small matter of interest, the CC and CCNAME thing is to seperate
## the actual name of the binary from the cruft that make uses to determine
## how to invoke the command.  In this case, we're sticking an @ in front
## of the commands, which means "execute it without printing the invocation
## line".  The purpose of this is to prevent the screen from being filled
## with unnecessary garbage when compiling.  The calling flags are listed
## at the top (see the header target below for more info), so there's no
## point in filling the screen with them over and over again.  We want the
## warnings and errors to stand out and yell at us.

CCNAME = gcc
CC     = @$(CCNAME)
SKEL_INC = ../skeleton_include
DUMMY_INC = ../r128_include
CFLAGS = -Wall -Wmissing-prototypes -pedantic -O3                 \
        -I../include                                              \
        -I../skeleton_accelerant                                  \
        -I$(SKEL_INC)                                             \
        -I$(DUMMY_INC)											
        
## Add -DDEBUG for a debug build.
DEBUGFLAG = -DDEBUG -DDEBUG_ACCEL=1 

#-DREG_DEBUG

###############################################################################
## Linker
##    The linker and it's flags.  There is probably a direct way to call the
## linker (ie: without this cheezy "going through gcc" thing), but I haven't
## seen it yet and haven't had the time to suss out what needs to be done.  I
## may well do so at some point in the future, but for now this scheme does
## the job, and that's good enough.

LDNAME  = gcc
LD      = @$(LDNAME)
LDFLAGS = -nostart -Xlinker -soname=$(NAME) -L./ -lroot 


###############################################################################
## Suffix Rules ###############################################################
###############################################################################


###############################################################################
##     For those to whom the below looks like line noise, an explanation:
##
##     Suffix rules tell make what to do when trying to create a target; a
## rule like .c.o: tells make "if you need to make foo.o, and you can find
## a foo.c, run foo.c through the following rule set to make foo.o"
##
## -c   -> Compile and assemble, don't link.
## $<   -> the source file (foo.c or foo.cpp)
## -o   -> tells gcc the name of the object file to produce
## $@   -> the target file (foo.o)
##
##      So the .c.o: rule fo foo.o becomes:
##
## echo "     Compiler -- foo.c"
## gcc -flags -c foo.c -o foo,o

.c.o:
	@echo "     Compiler --" $<
	$(CC) $(CFLAGS) $(DEBUGFLAG) -c $< -o $@

.cpp.o:
	@echo "     Compiler --" $<
	$(CC) $(CFLAGS) $(DEBUGFLAG) -c $< -o $@


###############################################################################
## Objects ####################################################################
###############################################################################

DRVOBJS = $(GDS_ACC_OBJS)        \
          r128_2d.o              \
          r128_cursor.o          \
          r128_accel_init.o      \
          r128_modeset.o     	 \
          regdump.o
          
#          Acceleration.o


###############################################################################
## Primary Targets ############################################################
###############################################################################

default: header $(DRVOBJS) $(DRIVER) footer

clean:
	@-rm -rf $(BINDIR)
	@-rm -f $(DRIVER)
	@-rm -f $(DRVOBJS)
	@-rm -f *~

install: installheader
	@cp $(DRIVER) $(INSTALLDIR)

###############################################################################
## Secondary Targets ##########################################################
###############################################################################

$(DRIVER): $(BINDIR) $(DRVOBJS)
	@echo "       Linker --" $(DRIVER)
	$(LD) -o $(DRIVER) $(DRVOBJS) $(LDFLAGS)
	@mimeset -f $(DRIVER)

$(BINDIR):
	@-mkdir $(BINDIR)


###############################################################################
## Default Header
##     This is a pair of nice banners for the compile.  It gives us some info
## about what's going on, and provides a fairly clean seperation between the
## compile and the surroundings.  The major reason for this (other than sheer
## aesthetics) is to make sure that it's easy to spot errors (and where they
## start).

header:
	@echo
	@echo %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	@echo %" "Building $(DRIVER) with $(MAKE).
	@echo %" "CC: $(CCNAME) $(CFLAGS)
	@echo %" "LD: $(LDNAME) $(LDFLAGS)
	@echo %~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

footer:
	@echo %~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo %" "Done.
	@echo %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	@echo


###############################################################################
## Install Header
##     This prints a nice banner to tell us what's been done.  Some day when
## I have a little more spare time I'll hack this up a bit so that it does
## proper error reporting.

installheader:
	@echo
	@echo Installing: " " $(DRIVER)
	@echo To: "         " $(INSTALLDIR)
	@echo


HOST = 192.168.64.0
PUNG:
	ping -c4 $(HOST)

ftp: $(DRIVER) 
	cd ../r128_driver; make -fr128_drv.mak ; cd ../r128_accelerant
	ftp </boot/beos/bin/ftp_load


###############################################################################
## This Is The End Of The File ################################################
###############################################################################
