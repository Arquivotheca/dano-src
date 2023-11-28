/* ++++++++++
	FILE:	location.h
	REVS:	$Revision: $
	NAME:	bronson
	DATE:	13 Jun 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _LOCATION_H
#define _LOCATION_H

// these come from NIM:OSUtils 4-29

typedef struct {
  float latitude;	// fractions of a great circle
  float longitude;	// fractions of a great circle
  int   gmt_delta;	// Greenwich mean time
  int	dls_delta;	// hour offset for daylight savings time
} location_info;

#endif
