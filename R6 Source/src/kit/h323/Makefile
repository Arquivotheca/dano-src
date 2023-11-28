################################################################################
#
# Makefile for the h323 (network telephony) kit
# (C) 2001 Be Incorporated, All Rights Reserved.
#
# Author(s): Jac Goudsmit
#
################################################################################
#
# The h323 kit is derived from the openh323 project, see www.openh323.org.
# At the time of this writing, the openh323 sources are stable enough to
# work, but not stable enough to take a snapshot and leave it that way.
# Also, it is heavily integrated with pwlib (portable windows library), which
# can be obtained from the same website but has a lot of code that is redundant
# to us (e.g. audio/video codecs, string and container classes).
#
# In order to rebuild the h323 kit from the latest pwlib and openh323 sources,
# download the sources from the website into the pwlib and openh323 directories
# under the directory where this Makefile resides, and then run make in this
# directory. It should not be necessary to actually build the pwlib and openh323
# projects before running make here.
#
# Naming conventions in variables:
# XXX_DS	directory for source-files (all sources except .h files)
# XXX_DI	directory for include-files (.h)
# XXX_Y		list of bison (.y) files without path
# XXX_L		list of flex (.l) files without path
# XXX_C		list of c/c++ (.c .cxx .cpp) files without path
# XXX_H		list of include (.h) files without path
# XXX_FS	list of all source files (everything except .h) with their paths
# XXX_FI	list of all include files with their paths
# XXX_O		list of all object files without paths
# XXX_G		list of c/c++ files that are generated (e.g. cpp files generated
#			from .y files), without path
#
################################################################################
#
# History:
# 08-Feb-2001 JG Created (using openh323.org cvs archive of today)
#

# overrides for make's internal variables
# should be replaced by an include
LEX=flex
#YACC=bison -y
CC=gcc
CXX=g++
CFLAGS+=-I.
CXXFLAGS=$(CFLAGS)

# default rule at top of file so make doesn't get confused by the other rules
default: all

# a rule to generate c++ files from bison sources
%.cpp: %.y
	bison -o $@ $<

# Define the places for the pwlib and openh323 directories.
# If you follow the openh323 website's instructions, or if you want to
# actually build the pwlib and openh323 libraries, you will probably need to
# override these to ~/pwlib and ~/openh323
PWLIB_DIR=pwlib
OPENH323_DIR=openh323

# Define the files that we need for the ASN.1 (Abstract Syntax Notation) parser
# ASN is a data definition language that is used by the ITU (International
# Telecommunication Union) to define the information exchange between computers.
# Some of the protocols used by the h323 kit are defined with ASN.
ASNP_DS=$(PWLIB_DIR)/tools/asnparser
ASNP_DI=$(PWLIB_DIR)/tools/asnparser
ASNP_Y=asn_grammar.y
ASNP_L=asn_lex.l
ASNP_C=main.cxx
ASNP_H=main.h
ASNP_X=asnparser

ASNP_FS=$(patsubst %,$(ASNP_DS)/%,$(ASNP_Y) $(ASNP_L) $(ASNP_C))
ASNP_FI=$(patsubst %,$(ASNP_DI)/%,$(ASNP_H))
ASNP_O=$(ASNP_Y:%.y=%.o) $(ASNP_L:%.l=%.o) $(ASNP_C:%.cxx=%.o)
ASNP_G=$(ASNP_Y:%.y=%.cpp)


$(ASNP_FS):
	cp $? .

$(ASNP_G): %.cpp: %.y

$(ASNP_X): $(ASNP_O)



# List of files to delete on a make clean
CLEANFILES=

# Targets
all: $(ASNP_X)

clean: FORCE
	rm -f $(CLEANFILES)

FORCE:

# End
