#!/boot/beos/bin/sh
mkdir /boot/_packages_
mkdir /boot/_packages_/japanese
mkdir /boot/_packages_/us_english
mkdir /boot/_packages_/optional
obj.${HOSTTYPE}/packageattr /boot/_packages_/japanese 'Japanese' 'Japanese language extensions' 'Languages' 0 0 1200000 japan_flag.tga
obj.${HOSTTYPE}/packageattr /boot/_packages_/us_english 'U.S. English' 'U.S. English language support' 'Languages' 1 1 0 usa_flag.tga
obj.${HOSTTYPE}/packageattr /boot/_packages_/optional 'Optional' 'Sample code, sample media files, experimental drivers, and more' 'Options' 0 0 85000000
