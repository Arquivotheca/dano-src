 "$ac_dir" && mkdir "$ac_dir"
    ac_dir_suffix="/`echo $ac_dir|sed 's%^\./%%'`"
    # A "../" for each directory in $ac_dir_suffix.
    ac_dots=`echo $ac_dir_suffix|sed 's%/[^/]*%../%g'`
  else
    ac_dir_suffix= ac_dots=
  fi

  case "$ac_given_srcdir" in
  .)  srcdir=.
      if test -z "$ac_dots"; then top_srcdir=.
      else top_srcdir=`echo $ac_dots|sed 's%/$%%'`; fi ;;
  /*) srcdir="$ac_given_srcdir$ac_dir_suffix"; top_srcdir="$ac_given_srcdir" ;;
  *) # Relative path.
    srcdir="$ac_dots$ac_given_srcdir$ac_dir_suffix"
    top_srcdir="$ac_dots$ac_given_srcdir" ;;
  esac

  case "$ac_given_INSTALL" in
  [/$]*) INSTALL="$ac_given_INSTALL" ;;
  *) INSTALL="$ac_dots$ac_given_INSTALL" ;;
  esac

  echo creating "$ac_file"
  rm -f "$ac_file"
  configure_input="Generated automatically from `echo $ac_file_in|sed 's%.*/%%'` by configure."
  case "$ac_file" in
  *Makefile*) ac_comsub="1i\\
# $configure_input" ;;
  *) ac_comsub= ;;
  esac

  ac_file_inputs=`echo $ac_file_in|sed -e "s%^%$ac_given_srcdir/%" -e "s%:% $ac_given_srcdir/%g"`
  sed -e "$ac_comsub
s%@configure_input@%$configure_input%g
s%@srcdir@%$srcdir%g
s%@top_srcdir@%$top_srcdir%g
s%@INSTALL@%$INSTALL%g
" $ac_file_inputs | (eval "$ac_sed_cmds") > $ac_file
fi; done
rm -f conftest.s*

# These sed commands are passed to sed as "A NAME B NAME C VALUE D", where
# NAME is the cpp macro being defined and VALUE is the value it is being given.
#
# ac_d sets the value in "#define NAME VALUE" lines.
ac_dA='s%^\([ 	]*\)#\([ 	]*define[ 	][ 	]*\)'
ac_dB='\([ 	][ 	]*\)[^ 	]*%\1#\2'
ac_dC='\3'
ac_dD='%g'
# ac_u turns "#undef NAME" with trailing blanks into "#define NAME VALUE".
ac_uA='s%^\([ 	]*\)#\([ 	]*\)undef\([ 	][ 	]*\)'
ac_uB='\([ 	]\)%\1#\2def