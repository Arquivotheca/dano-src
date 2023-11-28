#!/bin/sh
#
# This script will build a zip archive to be unzipped from /boot which
# contains all the neccessary pieces to run Wagner on Maui

source ~/.profile

usage()
{
	echo -e "usage: make-wagner-zip [-c ] TREE\n\n-c\tmake clean\n"
}

CURR_DIR=`pwd`

while getopts "c" opt; do
	case $opt in
		c ) export CLEAN='clean' ;;		# do make clean
		r ) export BUILDREAL=1 ;; 	# do build realplayer
		s ) export SYNC=0 ;;		# don't sync to latest
#		w ) export DESKTOPWAGNER=1 ;;		# do build Wagner for the Desktop
		* ) usage
	esac
done

shift $(($OPTIND - 1 ))

#if [ $1 ="rel" ]; then

# Build desktop libbe
if [ $1 = "rel" ]; then
	$1
	cd ${BUILDHOME}/src/kit
	make -f Make.libbe $CLEAN install
fi

iad

if ! $MY_PREFIX
then
	set_prefix $BUILDHOME/test-test
fi

# Build the WWW Kit
cd ${BUILDHOME}/src/kit
make -f Make.libwww $CLEAN install

# Build the PNG Pieces
cd ${BUILDHOME}/srcx/gnu/libpng
make -f make-full $CLEAN install
make -f make-reader $CLEAN install

# Build the Wagner app
cd ${BUILDHOME}/src/apps/wagner
make clean install
if [ $1 = "rel" ]; then
	mkdir ${MY_PREFIX}/install/i586/beos/apps/wagner
	mkdir ${MY_PREFIX}/install/i586/beos/apps/wagner/lib
	mv ${MY_PREFIX}/install/i586/beos/apps/Wagner ${MY_PREFIX}/install/i586/beos/apps/wagner/Wagner
	WAGNER=beos/apps/wagner
	rm ${MY_PREFIX}/install/i586/home/config/be/Applications/Wagner
	ln -s /boot/beos/apps/wagner/Wagner ${MY_PREFIX}/install/i586/home/config/be/Applications/Wagner
else
	export WAGNER=beos/apps/Wagner
	rm ${MY_PREFIX}/install/i586/home/config/be/Applications/Wagner
	ln -s /boot/beos/apps/Wagner ${MY_PREFIX}/install/i586/home/config/be/Applications/Wagner
fi

# Build the protocols
cd ${BUILDHOME}/src/addons/www_protocol
make $CLEAN install

# Build the content classes
cd ${BUILDHOME}/src/addons/www_content
make $CLEAN install

# Build the resources
cd ${BUILDHOME}/src/resources
make install

# Build the 'extras'
cd ${BUILDHOME}
make extras

ARCHIVE=wagner-$1.zip

rm ${CURR_DIR}/$ARCHIVE

# Now archive everything we need
if [ $1 = "rel" ]; then
	cd "${BUILDHOME}/../$1/install/i586"
	echo grabbing stuff from `pwd`
	cp $BUILDHOME/../$1/install/i586/beos/system/lib/libbe.so* ${MY_PREFIX}/install/i586/beos/apps/wagner/lib/
fi

cd ${MY_PREFIX}/install/i586
echo grabbing stuff from `pwd`
zip -9Tyr ${CURR_DIR}/$ARCHIVE beos/system/lib/libwww.so beos/system/lib/libsettings.so beos/system/lib/libpng.so beos/system/lib/libpng-reader.so custom/* $WAGNER home/config/be/Applications/Wagner beos/system/add-ons/web 
zip -9Tyr ${CURR_DIR}/$ARCHIVE apps/RealPlayer beos/system/lib/libreal.so.6.0
zip -9Tyr ${CURR_DIR}/$ARCHIVE beos/pjava/*
zip -9Tyr ${CURR_DIR}/$ARCHIVE home/config/settings/beia*
cd ${CURR_DIR}

