# ex:set sw=2 ts=8 wm=0:
# $Header: u:/rcs/cv/rcs/version.awk 2.6 1995/03/14 08:35:19 bog Exp $

# (C) Copyright 1992-1995 Radius Inc.
# All rights reserved

# This source code and any compilation or derivative thereof is the sole
# property of Radius Inc. and is provided pursuant to a Software
# License Agreement.  This code is the proprietary information of Radius
# and is confidential in nature.  Its use and dissemination by any party
# other than Radius is strictly limited by the confidential information
# provisions of the Agreement referenced above.

# $Log: version.awk $
# Revision 2.6  1995/03/14 08:35:19  bog
# version.n RCS revision of <major>.<minor> now is used to directly
# produce release version 1.<major>.0.<minor>.
# Revision 2.5  1995/02/22  12:32:49  bog
# Fix copright message.
# 
# Revision 2.4  1995/02/22  12:24:47  bog
# Roll to 1.9.REVMAJOR.REVMINOR.
#
# Revision 2.3  1993/06/17  11:27:24  bog
# Turn versions with every build.
#
# Revision 2.2  93/06/17  11:02:20  bog
# Need to do macro substitution.
#
# Revision 2.1  93/06/17  09:19:20  bog
# Get it working.
#
# Revision 2.0  93/06/17  09:16:37  bog
# Bump to version 2.0.
#
#
# Script to make version #define stuff

BEGIN {
  getline Revision < "version.n"
  split(Revision, RevNum)
  split(RevNum[2], Rev, "\.")
}
{
  # If version.n is RCS revision <major>.<minor>, then
  #   the product version is 1.<major>.0.0, and
  #   the file version is 1.<major>.0.<minor>

  sub("\?REVMAJOR\?", Rev[1])
  sub("\?REVMINOR\?", Rev[2])
  sub("\?PROD_VERSION\?", PROD_VERSION)
  sub("\?PROD_RELEASE\?", PROD_RELEASE)
  sub("\?PROD_TURN\?", PROD_TURN)
  sub("\?PROD_BUILD\?", PROD_BUILD)
  sub("\?FILE_VERSION\?", FILE_VERSION)
  sub("\?FILE_RELEASE\?", FILE_RELEASE)
  sub("\?FILE_TURN\?", FILE_TURN)
  sub("\?FILE_BUILD\?", FILE_BUILD)
}
/!PROD_VERSION!/ {
  PROD_VERSION = $2
  next
}
/!PROD_RELEASE!/ {
  PROD_RELEASE = $2
  next
}
/!PROD_TURN!/ {
  PROD_TURN = $2
  next
}
/!PROD_BUILD!/ {
  PROD_BUILD = $2
  next
}
/!FILE_VERSION!/ {
  FILE_VERSION = $2
  next
}
/!FILE_RELEASE!/ {
  FILE_RELEASE = $2
  next
}
/!FILE_TURN!/ {
  FILE_TURN = $2
  next
}
/!FILE_BUILD!/ {
  FILE_BUILD = $2
  next
}
{
  print
}
