#! /usr/bin/perl -w

# Copyright (C) 1997 Free Software Foundation, Inc.
# This file is part of the GNU C Library.
# Contributed by Andreas Jaeger <aj@arthur.rhein-neckar.de>, 1997.

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


$PACKAGE = "libc";
$progname = $0;
if ($ENV{CC}) {
  $CC = $ENV{CC};
} else {
  $CC= "gcc";
}

sub usage {
  print "Usage: test-installation [soversions.mk]\n";
  print "  --help       print this help, then exit\n";
  print "  --version    print version number, then exit\n";
  exit 0;
}

sub installation_problem {
  print "The script has found some problems with your installation!\n";
  print "Please read the FAQ and the README file and check the following:\n";
  print "- Did you change the gcc specs file (neccessary after upgrading from\n";
  print "  Linux libc5)?\n";
  print "- Are there any symbolic links of the form libXXX.so to old libraries?\n";
  print "  Links like libm.so -> libm.so.5 (where libm.so.5 is an old library) are wrong,\n";
  print "  libm.so should point to the newly installed glibc file - and there should be\n";
  print "  only one such link (check e.g. /lib and /usr/lib)\n";
  print "You should restart this script from your build directory after you've\n";
  print "fixed all problems!\n";
  print "Btw. the script doesn't work if you're installing GNU libc not as your\n";
  print "primary library!\n";
  exit 1;
}

arglist: while (@ARGV) {
  if ($ARGV[0] eq "--v" || $ARGV[0] eq "--ve" || $ARGV[0] eq "--ver" ||
      $ARGV[0] eq "--vers" || $ARGV[0] eq "--versi" ||
      $ARGV[0] eq "--versio" || $ARGV[0] eq "--version") {
    print "test-installation (GNU $PACKAGE)\n";
    print "Copyright (C) 1997 Free Software Foundation, Inc.\n";
    print "This is free software; see the source for copying conditions.  There is NO\n";
    print "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n";
    print "Written by Andreas Jaeger <aj\@arthur.rhein-neckar.de>\n";

    exit 0;
  } elsif ($ARGV[0] eq "--h" || $ARGV[0] eq "--he" || $ARGV[0] eq "--hel" ||
	   $ARGV[0] eq "--help") {
    &usage;
  } elsif ($ARGV[0] =~ /^-/) {
    print "$progname: unrecognized option `$ARGV[0]'\n";
    print "Try `$progname --help' for more information.\n";
    exit 1;
  } else {
    last arglist;
  }
}

# We expect none or one argument.
if ($#ARGV == -1) {
    $soversions="soversions.mk";
} elsif ($#ARGV == 0) {
    if (-d $ARGV[0]) {
      $soversions = "$ARGV[0]/soversions.mk";
    } else {
      $soversions = $ARGV[0];
    }
} else {
    die "Wrong number of arguments.";
}


# Read names and versions of all shared libraries that are part of
# glibc
open SOVERSIONS, $soversions
  or die ("Couldn't open $soversions in build directory!");

$link_libs = "";
%versions = ();

while (<SOVERSIONS>) {
  next if (/^all-sonames/);
  chop;
  if (/^lib/) {
    ($name, $version)= /^lib(.*)\.so-version=\.(.*)$/;
    if ($name ne "nss_ldap") {
      $link_libs .= " -l$name";
      $versions{$name} = $version;
    }
  } else {
    if (/^ld\.so/) {
      ($ld_so_name, $ld_so_version)= /=(.*)\.so\.(.*)$/;
    }
  }
}

close SOVERSIONS;

# Create test program and link it against all
# shared libraries

open PRG, ">/tmp/test-prg$$.c"
  or die ("Couldn't write test file /tmp/test-prg$$.c");

print PRG '
#include <stdio.h>
#include <stdlib.h>
int main(void) {
  printf ("Your new glibc installation seems to be ok.\n");
  exit (0);
}
';
close PRG;

open GCC, "$CC /tmp/test-prg$$.c $link_libs -o /tmp/test-prg$$ 2>&1 |"
  or die ("Couldn't execute $CC!");

while (<GCC>) {
  print $_ if (! /warning/);
}
close GCC;
if ($?) {
  print "Execution of $CC failed!\n";
  &installation_problem;
}

# Test if test program is linked against the right versions of
# shared libraries

$ok = 1;
%found = ();

open LDD, "ldd /tmp/test-prg$$ |"
  or die ("Couldn't execute ldd");
while (<LDD>) {
  if (/^\s*lib/) {
    ($name, $version1, $version2) =
      /^\s*lib(\w*)\.so\.([0-9\.]*)\s*=>.*\.so\.([0-9\.]*)/;
    $found{$name} = 1;
    if ($versions{$name} ne $version1 || $version1 ne $version2) {
      print "Library lib$name is not correctly installed.\n";
      print "Please check your installation!\n";
      print "Offending line of ldd output: $_\n";
      $ok = 0;
    }
  }
  if (/$ld_so_name/) {
    ($version1, $version2) =
      /$ld_so_name\.so\.([0-9\.]*)\s*=>.*\.so\.([0-9\.]*)/;
    if ($version1 ne $version2 || $version1 ne $ld_so_version) {
      print "The dynamic linker $ld_so_name.so is not correctly installed.\n";
      print "Please check your installation!\n";
      print "Offending line of ldd output: $_\n";
      $ok = 0;
    }
  }
}

close LDD;
die "ldd execution failed" if $?;

foreach (keys %versions) {
  unless ($found{$_}) {
    print "Library lib$_ is not correctly installed since the test program\n";
    print "was not linked dynamically against it.\n";
    print "Do you have a file/link lib$_.so?\n";
    $ok = 0;
  }
}

&installation_problem unless $ok;

# Finally execute the test program
system ("/tmp/test-prg$$") == 0
  or die ("Execution of test program failed");

# Clean up after ourselves
unlink ("/tmp/test-prg$$", "/tmp/test-prg$$.c");
