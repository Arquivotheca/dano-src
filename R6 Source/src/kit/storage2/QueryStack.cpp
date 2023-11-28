s%@mandir@%${prefix}/man%g
s%@PRODUCT@%m4%g
s%@VERSION@%1.4%g
s%@AWK@%gawk%g
s%@CC@%gcc%g
s%@INSTALL_PROGRAM@%${INSTALL}%g
s%@INSTALL_SCRIPT@%${INSTALL_PROGRAM}%g
s%@INSTALL_DATA@%${INSTALL} -m 644%g
s%@SET_MAKE@%%g
s%@RANLIB@%ranlib%g
s%@CPP@%gcc -E%g
s%@U@%%g
s%@ANSI2KNR@%%g
s%@ALLOCA@%%g
s%@LIBOBJS@%%g
s%@STACKOVF@%%g

CEOF

# Split the substitutions into bite-sized pieces for seds with
# small command number limits, like on Digital OSF/1 and HP-UX.
ac_max_sed_cmds=90 # Maximum number of lines to put in a sed script.
ac_file=1 # Number of current file.
ac_beg=1 # First line for current file.
ac_end=$ac_max_sed_cmds # Line after last line for current file.
ac_more_lines=:
ac_sed_cmds=""
while $ac_more_lines; do
  if test $ac_beg -gt 1; then
    sed "1,${ac_beg}d; ${ac_end}q" conftest.subs > conftest.s$ac_file
  else
    sed "${ac_end}q" conftest.subs > conftest.s$ac_file
  fi
  if test ! -s conftest.s$ac_file; then
    ac_more_lines=false
    rm -f conftest.s$ac_file
  else
    if test -z "$ac_sed_cmds"; then
      ac_sed_cmds="sed -f conftest.s$ac_file"
    else
      ac_sed_cmds="$ac_sed_cmds | sed -f conftest.s$ac_file"
    fi
    ac_file=`expr $ac_file + 1`
    ac_beg=$ac_end
    ac_end=`expr $ac_end + $ac_max_sed_cmds`
  fi
done
if test -z "$ac_sed_cmds"; then
  ac_sed_cmds=cat
fi

CONFIG_FILES=${CONFIG_FILES-"Makefile doc/Makefile lib/Makefile src/Makefile checks/Makefile examples/Makefile"}
for ac_file in .. $CONFIG_FILES; do if test "x$ac_file" != x..; then
  # Support "out