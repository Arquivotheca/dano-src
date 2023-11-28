x$ac_file" != x..; then
  # Support "outfile[:infile[:infile...]]", defaulting infile="outfile.in".
  case "$ac_file" in
  *:*) ac_file_in=`echo "$ac_file"|sed 's%[^:]*:%%'`
       ac_file=`echo "$ac_file"|sed 's%:.*%%'` ;;
  *) ac_file_in="${ac_file}.in" ;;
  esac

  echo creating $ac_file

  rm -f conftest.frag conftest.in conftest.out
  ac_file_inputs=`echo $ac_file_in|sed -e "s%^%$ac_given_srcdir/%" -e "s%:% $ac_given_srcdir/%g"`
  cat $ac_file_inputs > conftest.in

  cat > conftest.frag <<CEOF
${ac_dA}PRODUCT${ac_dB}PRODUCT${ac_dC}"m4"${ac_dD}
${ac_uA}PRODUCT${ac_uB}PRODUCT${ac_uC}"m4"${ac_uD}
${ac_eA}PRODUCT${ac_eB}PRODUCT${ac_eC}"m4"${ac_eD}
${ac_dA}VERSION${ac_dB}VERSION${ac_dC}"1.4"${ac_dD}
${ac_uA}VERSION${ac_uB}VERSION${ac_uC}"1.4"${ac_uD}
${ac_eA}VERSION${ac_eB}VERSION${ac_eC}"1.4"${ac_eD}
${ac_dA}PROTOTYPES${ac_dB}PROTOTYPES${ac_dC}1${ac_dD}
${ac_uA}PROTOTYPES${ac_uB}PROTOTYPES${ac_uC}1${ac_uD}
${ac_eA}PROTOTYPES${ac_eB}PROTOTYPES${ac_eC}1${ac_eD}
${ac_dA}HAVE_LIMITS_H${ac_dB}HAVE_LIMITS_H${ac_dC}1${ac_dD}
${ac_uA}HAVE_LIMITS_H${ac_uB}HAVE_LIMITS_H${ac_uC}1${ac_uD}
${ac_eA}HAVE_LIMITS_H${ac_eB}HAVE_LIMITS_H${ac_eC}1${ac_eD}
CEOF
  sed -f conftest.frag conftest.in > conftest.out
  rm -f conftest.in
  mv conftest.out conftest.in

  cat > conftest.frag <<CEOF
${ac_dA}HAVE_MEMORY_H${ac_dB}HAVE_MEMORY_H${ac_dC}1${ac_dD}
${ac_uA}HAVE_MEMORY_H${ac_uB}HAVE_MEMORY_H${ac_uC}1${ac_uD}
${ac_eA}HAVE_MEMORY_H${ac_eB}HAVE_MEMORY_H${ac_eC}1${ac_eD}
${ac_dA}HAVE_STRING_H${ac_dB}HAVE_STRING_H${ac_dC}1${ac_dD}
${ac_uA}HAVE_STRING_H${ac_uB}HAVE_STRING_H${ac_uC}1${ac_uD}
${ac_eA}HAVE_STRING_H${ac_eB}HAVE_STRING_H${ac_eC}1${ac_eD}
${ac_dA}HAVE_UNISTD_H${ac_dB}HAVE_UNISTD_H${ac_dC}1${ac_dD}
${ac_uA}HAVE_UNISTD_H${ac_uB}HAVE_UNISTD_H${ac_uC}1${ac_uD}
${ac_eA}HAVE_UNISTD_H${ac_eB}HAVE_UNISTD_H${ac_eC}1${ac_eD}
${ac_dA}STDC_HEADERS${ac_dB}STDC_HEADERS${ac_dC}1${ac_dD}
${ac_uA}STDC_HEADERS${ac_uB}STDC_HEADERS${ac_uC}1${ac_uD}
${ac_eA}STDC_HEADERS${ac_eB}STDC_HEADERS${ac_eC}1${ac_eD}
CEOF
  sed -f conftest.frag conftest.in > conftest.out
  rm -f conftest.in
  mv conftest.out conftest.in

  cat > conftest.frag <<CEOF
${ac_dA}RETSIGTYPE${ac_dB}RETSIGTYPE${ac_dC}void${ac_dD}
${ac_uA}RETSIGTYPE${ac_uB}RETSIGTYPE${ac_uC}void${ac_uD}
${ac_eA}RETSIGTYPE${ac_eB}RETSIGTYPE${ac_eC}void${ac_eD}
${ac_dA}HAVE_MKSTEMP${ac_dB}HAVE_MKSTEMP${ac_dC}1${ac_dD}
${ac_uA}HAVE_MKSTEMP${ac_uB}HAVE_MKSTEMP${ac_uC}1${ac_uD}
${ac_eA}HAVE_MKSTEMP${ac_eB}HAVE_MKSTEMP${ac_eC}1${ac_eD}
${ac_dA}HAVE_SIGACTION${ac_dB}HAVE_SIGACTION${ac_dC}1${ac_dD}
${ac_uA}HAVE_SIGACTION${ac_uB}HAVE_SIGACTION${ac_uC}1${ac_uD}
${ac_eA}HAVE_SIGACTION${ac_eB}HAVE_SIGACTION${ac_eC}1${ac_eD}
${ac_dA}HAVE_STRERROR${ac_dB}HAVE_STRERROR${ac_dC}1${ac_dD}
${ac_uA}HAVE_STRERROR${ac_uB}HAVE_STRERROR${ac_uC}1${ac_uD}
${ac_eA}HAVE_STRERROR${ac_eB}HAVE_STRERROR${ac_eC}1${ac_eD}
CEOF
  sed -f conftest.frag conftest.in > conftest.out
  rm -f conftest.in
  mv conftest.out conftest.in

  cat > conftest.frag <<CEOF
${ac_dA}HAVE_TMPFILE${ac_dB}HAVE_TMPFILE${ac_dC}1${ac_dD}
${ac_uA}HAVE_TMPFILE${ac_uB}HAVE_TMPFILE${ac_uC}1${ac_uD}
${ac_eA}HAVE_TMPFILE${ac_eB}HAVE_TMPFILE${ac_eC}1${ac_eD}
${ac_dA}HAVE_ALLOCA_H${ac_dB}HAVE_ALLOCA_H${ac_dC}1${ac_dD}
${ac_uA}HAVE_ALLOCA_H${ac_uB}HAVE_ALLOCA_H${ac_uC}1${ac_uD}
${ac_eA}HAVE_ALLOCA_H${ac_eB}HAVE_ALLOCA_H${ac_eC}1${ac_eD}
${ac_dA}HAVE_ALLOCA${ac_dB}HAVE_ALLOCA${ac_dC}1${ac_dD}
${ac_uA}HAVE_ALLOCA${ac_uB}HAVE_ALLOCA${ac_uC}1${ac_uD}
${ac_eA}HAVE_ALLOCA${ac_eB}HAVE_ALLOCA${ac_eC}1${ac_eD}
${ac_dA}HAVE_VPRINTF${ac_dB}HAVE_VPRINTF${ac_dC}1${ac_dD}
${ac_uA}HAVE_VPRINTF${ac_uB}HAVE_VPRINTF${ac_uC}1${ac_uD}
${ac_eA}HAVE_VPRINTF${ac_eB}HAVE_VPRINTF${ac_eC}1${ac_eD}
CEOF
  sed -f conftest.frag conftest.in > conftest.out
  rm -f conftest.in
  mv conftest.out conftest.in

  cat > conftest.frag <<CEOF
${ac_dA}HAVE_STRTOL${ac_dB}HAVE_STRTOL${ac_dC}1${ac_dD}
${ac_uA}HAVE_STRTOL${ac_uB}HAVE_STRTOL${ac_uC}1${ac_uD}
${ac_eA}HAVE_STRTOL${ac_eB}HAVE_STRTOL${ac_eC}1${ac_eD}
${ac_dA}HAVE_ECVT${ac_dB}HAVE_ECVT${ac_dC}1${ac_dD}
${ac_uA}HAVE_ECVT${ac_uB}HAVE_ECVT${ac_uC}1${ac_uD}
${ac_eA}HAVE_ECVT${ac_eB}HAVE_ECVT${ac_eC}1${ac_eD}
s%^[ 	]*#[ 	]*undef[ 	][ 	]*[a-zA-Z_][a-zA-Z_0-9]*%/* & */%
CEOF
  sed -f conftest.frag conftest.in > conftest.out
  rm -f conftest.in
  mv conftest.out conftest.in

  rm -f conftest.frag conftest.h
  echo "/* $ac_file.  Generated automatically by configure.  */" > conftest.h
  cat conftest.in >> conftest.h
  rm -f conftest.in
  if cmp -s 