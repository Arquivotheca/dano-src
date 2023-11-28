/*-----------------------------------------------------------------*/
//
//	File:		ID3.h
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef ID3_H
#define ID3_H

#include <String.h>

//#define DEBUG				1

enum ID3_VERSION
{
	id3_vers_unknown = 0,
	id3_vers_1,
	id3_vers_2_2_0,
	id3_vers_2_3_0
};


//========================================================================

struct id3_tags
{
					id3_tags()
					{
						artist = "";
						album = "";
						title = "";
						track = 0;
						length = 0;
						genre = 0;
						version = id3_vers_unknown;
					}

	void			PrintToStream()
					{
#ifdef DEBUG
						printf("Artist: %s\n", artist.String());
						printf("Album:  %s\n", album.String());
						printf("Title:  %s\n", title.String());
						printf("Track:  %d\n", (int)track);
						printf("Length: %Ld\n", length);
						printf("Genre:  %d\n\n", genre;
#endif
					}

	BString			artist;
	BString			album;
	BString			title;
	int32			track;
	int64			length;
	int8			genre;
	ID3_VERSION		version;
};


//========================================================================

status_t			ReadID3Tags				(const char* path,
											 id3_tags*);
status_t			WriteIDTags				(const char* path,
											 id3_tags*);

#endif
