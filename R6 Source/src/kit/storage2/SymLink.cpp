s=NONE
x_libraries=NONE
bindir='${exec_prefix}/bin'
sbindir='${exec_prefix}/sbin'
libexecdir='${exec_prefix}/libexec'
datadir='${prefix}/share'
sysconfdir='${prefix}/etc'
sharedstatedir='${prefix}/com'
localstatedir='${prefix}/var'
libdir='${exec_prefix}/lib'
includedir='${prefix}/include'
oldincludedir='/usr/include'
infodir='${prefix}/info'
mandir='${prefix}/man'

# Initialize some other variables.
subdirs=
MFLAGS= MAKEFLAGS=
SHELL=${CONFIG_SHELL-/bin/sh}
# Maximum number of lines to put in a shell here document.
ac_max_here_lines=12

ac_prev=
for ac_option
do

  # If the previous option needs an argument, assign it.
  if test -n "$ac_prev"; then
    eval "$ac_prev=\$ac_option"
    ac_prev=
    continue
  fi

  case "$ac_option" in
  -*=*) ac_optarg=`echo "$ac_option" | sed 's/[-_a-zA-Z0-9]*=//'` ;;
  *) ac_optarg= ;;
  esac

  # Accept the important Cygnus configure options, so we can diagnose typos.

  case "$ac_option" in

  -bindir | --bindir | --bindi | --bind | --bin | --bi)
    ac_prev=bindir ;;
  -bindir=* | --bindir=* | --bindi=* | --bind=* | --bin=* | --bi=*)
    bindir="$ac_optarg" ;;

  -build | --build | --buil | --bui | --bu)
    ac_prev=build ;;
  -build=* | --build=* | --buil=* | --bui=* | --bu=*)
    build="$ac_optarg" ;;

  -cache-file | --cache-file | --cache-fil | --cache-fi \
  | --cache-f | --cache- | --cache | --cach | --cac | --ca | --c)
    ac_prev=cache_file ;;
  -cache-file=* | --cache-file=* | --cache-fil=* | --cache-fi=* \
  | --cache-f=* | --cache-=* | --cache=* | --cach=* | --cac=* | --ca=* | --c=*)
    cache_file="$ac_optarg" ;;

  -datadir | --datadir | --datadi | --datad | --data | --dat | --da)
    ac_prev=datadir ;;
  -datadir=* | --datadir=* | --datadi=* | --datad=* | --data=* | --dat=* \
  | --da=*)
    datadir="$ac_optarg" ;;

  -disable-* | --disable-*)
    ac_feature=`echo $ac_option|sed -e 's/-*disable-//'`
    # Reject names that are not valid shell variable names.
    if test -n "`echo $ac_feature| sed 's/[-a-zA-Z0-9_]//g'`"; then
      { echo "configure: error: $ac_feature: invalid feature name" 1>&2; exit 1; }
    fi
    ac_feature=`echo $ac_feature| sed 's/-/_/g'`
    eval "enable_${ac_feature}=no" ;;

  