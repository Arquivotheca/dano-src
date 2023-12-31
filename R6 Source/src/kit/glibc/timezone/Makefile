# Copyright (C) 1998 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

#
#	Makefile for timezone information
#
subdir	:= timezone

distribute := tzfile.h private.h scheck.c ialloc.c yearistype	\
	      iso3166.tab zone.tab tzselect.ksh checktab.awk	\
	      README

extra-objs := scheck.o ialloc.o

others	:= zdump zic
tests	:= test-tz tst-timezone

tzbases := africa antarctica asia australasia europe northamerica \
	   southamerica etcetera factory systemv \
	   solar87 solar88 solar89
tzlinks := backward
tzfiles := $(tzbases) $(tzlinks)
# pacificnew doesn't compile; if it is to be used, it should be included in
# northamerica.
distribute += $(tzfiles) leapseconds pacificnew simplebackw

generated := $(addprefix z.,$(tzfiles))
install-sbin := zic zdump

generated-dirs = testdata

all: # Make this the default target; it will be defined in Rules.

include ../Makeconfig	# Get objpfx defined so we can use it below.

# z.* use this variable.
define nl


endef
ifndef avoid-generated
-include $(addprefix $(objpfx)z.,$(tzfiles))
endif

# Make these absolute file names.
installed-localtime-file := $(firstword $(filter /%,$(inst_localtime-file)) \
					$(addprefix $(inst_zonedir)/, \
						    $(localtime-file)))
installed-posixrules-file := $(firstword $(filter /%,$(posixrules-file)) \
					 $(addprefix $(inst_zonedir)/, \
						     $(posixrules-file)))

ifeq ($(cross-compiling),no)
# Don't try to install the zoneinfo files since we can't run zic.
#install-others = $(addprefix $(inst_zonedir)/,$(zonenames) \
					      $(zonenames:%=posix/%) \
					      $(zonenames:%=right/%)) \
#		 $(installed-localtime-file) $(installed-posixrules-file)
endif

ifeq ($(have-ksh),yes)
install-others += $(inst_zonedir)/iso3166.tab $(inst_zonedir)/zone.tab
install-bin += tzselect
generated += tzselect
endif

include ../Rules


$(tzfiles:%=$(objpfx)z.%): $(objpfx)z.%: % Makefile
# Kludge alert: we use an implicit rule (in what we are generating here)
# because that is the only way to tell Make that the one command builds all
# the files.
# The extra kludge for the $(tzlinks) files is necessary since running zic
# this file requires all other files to exist.  Blech!
	(echo 'define $*-zones'						    ;\
	 $(AWK) '$$1 == "Zone" { print $$2 } $$1 == "Link" { print $$3 }' $^ ;\
	 echo 'endef'							    ;\
	 echo '$*-zones := $$(subst $$(nl), ,$$($*-zones))'		    ;\
	 echo 'ifdef $*-zones'						    ;\
	 if test x$(findstring $*, $(tzlinks)) != x; then		     \
	   echo '$$(addprefix $$(inst_zonedir)/right/,$$($*-zones)): \';\
	   echo '$$(foreach t,$$(tzbases),$$(addprefix $$(inst_zonedir)/right/,$$($$t-zones)))' ;\
	   echo '$$(addprefix $$(inst_zonedir)/posix/,$$($*-zones)): \';\
	   echo '$$(foreach t,$$(tzbases),$$(addprefix $$(inst_zonedir)/posix/,$$($$t-zones)))' ;\
	   echo '$$(addprefix $$(inst_zonedir)/,$$($*-zones)): \'	    ;\
	   echo '$$(foreach t,$$(tzbases),$$(addprefix $$(inst_zonedir)/,$$($$t-zones)))' ;\
	 fi								    ;\
	 echo '$$(addprefix $$(inst_datadir)/zone%/right/,$$($*-zones)): \' ;\
	 echo '$< $$(objpfx)zic leapseconds yearistype'			    ;\
	 echo '	$$(tzcompile)'						    ;\
	 echo '$$(addprefix $$(inst_datadir)/zone%/posix/,$$($*-zones)): \' ;\
	 echo '$< $$(objpfx)zic /dev/null yearistype'			    ;\
	 echo '	$$(tzcompile)'						    ;\
	 echo '$$(addprefix $$(inst_datadir)/zone%/,$$($*-zones)): \'	    ;\
	 echo '$< $$(objpfx)zic $$(leapseconds) yearistype'		    ;\
	 echo '	$$(tzcompile)'						    ;\
	 echo 'endif'							    ;\
	 echo 'zonenames := $$(zonenames) $$($*-zones)'			    ;\
	) > $@.new
	mv $@.new $@

.PHONY: echo-zonenames
echo-zonenames:
	@echo 'Known zones: $(zonenames)'


# We have to use `-d $(inst_zonedir)' to explictly tell zic where to
# place the output files although $(zonedir) is compiled in.  But the
# user might have set $(install_root) on the command line of `make install'.
zic-cmd = $(built-program-cmd) -d $(inst_zonedir)
tzcompile = $(zic-cmd)$(target-zone-flavor) -L $(word 3,$^) \
	    -y $(dir $(word 4,$^))$(notdir $(word 4,$^)) $<

# The source files specify the zone names relative to the -d directory,
# so for the posix/ and right/ flavors we need to pass -d $(inst_zonedir)/posix
# and the like.  This magic extracts /posix or /right if it's the first
# component after $(inst_zonedir) in the target name $@.
target-zone-flavor = $(filter /posix /right, \
			      /$(firstword $(subst /, , \
					  $(patsubst $(inst_zonedir)/%,%,$@))))

ifdef localtime
$(installed-localtime-file): $(inst_zonedir)/$(localtime) $(objpfx)zic \
			     $(+force)
	$(make-target-directory)
	if test -r $@; then \
	  echo Site timezone NOT reset to Factory.; \
	else \
	  rm -f $@T; \
	  $(SHELL) $(..)scripts/rellns-sh $< $@T; \
	  mv -f $@T $@; \
	fi
endif
ifdef posixrules
$(installed-posixrules-file): $(inst_zonedir)/$(posixrules) $(objpfx)zic \
			      $(+force)
	$(zic-cmd) -p $(posixrules)
endif


$(objpfx)zic: $(objpfx)scheck.o $(objpfx)ialloc.o

tz-cflags = -DTZDIR='"$(zonedir)"' \
	    -DTZDEFAULT='"$(localtime-file)"' \
	    -DTZDEFRULES='"$(posixrules-file)"' \
	    -DTM_GMTOFF=tm_gmtoff -DTM_ZONE=tm_zone

CFLAGS-zdump.c = -Wno-strict-prototypes -DNOID $(tz-cflags)
CFLAGS-zic.c = -Wno-strict-prototypes -DNOID $(tz-cflags)
CFLAGS-ialloc.c = -Wno-strict-prototypes -DNOID
CFLAGS-scheck.c = -Wno-strict-prototypes -DNOID

# We have to make sure the data for testing the tz functions is available.
testdata = $(objpfx)testdata
define build-testdata
$(built-program-cmd) -d $(testdata) -L $(leapseconds) -y ./yearistype $<
endef

$(objpfx)test-tz.out: $(addprefix $(testdata)/, America/New_York Etc/UTC UTC)
$(objpfx)tst-timezone.out: $(addprefix $(testdata)/, \
				       Europe/Berlin Universal \
				       Australia/Melbourne \
				       America/Sao_Paulo)

test-tz-ENV = TZDIR=$(testdata)
tst-timezone-ENV = TZDIR=$(testdata)

$(testdata)/America/New_York: northamerica $(objpfx)zic $(leapseconds) \
			      yearistype
	$(build-testdata)
$(testdata)/Etc/UTC: etcetera $(objpfx)zic $(leapseconds) yearistype
	$(build-testdata)
$(testdata)/UTC: simplebackw $(objpfx)zic $(testdata)/Etc/UTC \
		 $(leapseconds) yearistype
	$(build-testdata)
$(testdata)/Europe/Berlin: europe $(objpfx)zic $(leapseconds) yearistype
	$(build-testdata)
$(testdata)/Universal: simplebackw $(objpfx)zic $(testdata)/Etc/UTC \
		       $(leapseconds) yearistype
	$(build-testdata)
$(testdata)/Australia/Melbourne: australasia $(objpfx)zic $(leapseconds) \
				 yearistype
	$(build-testdata)
$(testdata)/America/Sao_Paulo: southamerica $(objpfx)zic $(leapseconds) \
			       yearistype
	$(build-testdata)


$(objpfx)tzselect: tzselect.ksh $(common-objpfx)config.make
	sed -e 's%@KSH@%$(KSH)%g' \
	    -e 's%@TZDIR@%$(zonedir)%g' < $< > $@.new
	chmod 555 $@.new
	mv -f $@.new $@

$(addprefix $(inst_zonedir)/,iso3166.tab zone.tab): \
    $(inst_zonedir)/%: % $(+force)
	$(do-install)
