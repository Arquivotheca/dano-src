#! /bin/sh
# Testing the implementation of localedata.
# Copyright (C) 1998 Free Software Foundation, Inc.
# This file is part of the GNU C Library.
# Contributed by Andreas Jaeger, <aj@arthur.rhein-neckar.de>, 1998.
#
# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If
# not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

common_objpfx=$1; shift

test_locale ()
{
    charmap=$1
    input=$2
    out=$3
    rep=$4
    I18NPATH=. \
    ${common_objpfx}elf/ld.so --library-path $common_objpfx \
    ${common_objpfx}locale/localedef --quiet -c -f $charmap -i $input \
      --repertoire-map $rep ${common_objpfx}localedata/$out

    if [ $? -ne 0 ]; then
	echo "Charmap: \"${charmap}\" Inputfile: \"${input}\"" \
	     "Outputdir: \"${out}\" failed"
	exit 1
    fi
}

# I take this out for now since it is a known problem
# (see [PR libc/229] and [PR libc/454]. --drepper
# test_locale IBM437 de_DE de_DE.437 mnemonic.ds
test_locale tests/test1.cm tests/test1.def test1 mnemonic.ds
test_locale tests/test2.cm tests/test2.def test2 mnemonic.ds
test_locale tests/test3.cm tests/test3.def test3 mnemonic.ds
test_locale tests/test4.cm tests/test4.def test4 mnemonic.ds
# I know that multi-byte charsets do not yet work. --drepper
# test_locale tests/test5.cm tests/test5.def test5 mnemonic.ds

exit 0

# Local Variables:
#  mode:shell-script
# End:
