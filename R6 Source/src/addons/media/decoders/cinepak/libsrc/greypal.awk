# $Header: u:/rcs/cv/rcs/greypal.awk 1.1 1994/05/09 14:01:48 timr Exp $
# $Log: greypal.awk $
# Revision 1.1  1994/05/09 14:01:48  timr
# Initial revision

#
# GREYPAL.AWK -- Generates greypal.h.
#

# GreyByteLookup[k] contains the palette index corresponding to the Y value k.
#
# This is an even mapping of the values [10..235] into the set [0..255].

# GreyDwordLookup[k] contains the four dithered palette indices corresponding
# to a smooth patch with Y value k.

# Code is generated with 4-character tabs, as God intended.

BEGIN {
    printf "unsigned char GreyByteLookup [256] = {\n    ";
    for (i = 0; i < 256; i++)
    {
        printf "%3d, ", int (((236 * i + 128) / 256 + 10))

	if (i % 8 == 7) {
	    printf "\n    ";
	}
    }
    print "};"
    print " "

    print "unsigned char GreyDwordDither [256][4] = {";
    for (i = 0; i < 256; i++)
    {
        j = (i > 3) ? i - 3 : 0;
	k = (i < 252) ? i + 3 : 255;
	printf "    {%3d, %3d, %3d, %3d},\n",
	    int (((236 * i + 128) / 256 + 10)),
	    int (((236 * j + 128) / 256 + 10)),
	    int (((236 * k + 128) / 256 + 10)),
	    int (((236 * i + 128) / 256 + 10))
    }
    print "};"
    exit
}


