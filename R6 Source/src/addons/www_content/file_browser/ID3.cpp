/*-----------------------------------------------------------------*/
//
//	File:		ID3.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include <ByteOrder.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ID3.h"

#define kHEAD_ROOM		512


//========================================================================

static void			pad						(BString*, char*, int32);
static void			remove_trailing_spaces	(BString*, char*, int32);
static int32		int28toint32			(uchar*);
static void			int32toint28			(int32, uchar*);
static int32		add_tag					(uint32, uchar*&, uint32&, BString);
static int32		update_tag				(uchar*&, uchar*, uint32&, BString);
static status_t		copy_file				(BFile*, BFile*, uint32);
static status_t		write_header			(BFile*, uchar*, uint32, bool, int32);

/*
void dump(uchar* buffer, int32 length)
{
	char	string[17];
	int32	loop;

	for (loop = 0; loop < length; loop++)
	{
		if ((loop) && !(loop & 0xf))
			printf("     %s\n", string);
		printf("%.2x ", buffer[loop]);
		if (buffer[loop] >= ' ')
			string[loop & 0xf] = buffer[loop];
		else
			string[loop & 0xf] = '.';
		string[(loop & 0xf) + 1] = 0;
	}
	for (loop = (16 - (length & 0xf)) & 0xf; loop > 0; loop--)
		printf("   ");
	printf("     %s\n\n", string);
}
*/


//========================================================================

status_t ReadID3Tags(const char* path, id3_tags* tags)
{
	BFile		file(path, O_RDONLY);
	status_t	result = B_NO_ERROR;

	if ((result = file.InitCheck()) == B_NO_ERROR)
	{
		uchar		buffer[128];
		ssize_t		read;

		if ((read = file.Read(buffer, 10)) >= 0)
		{
			bool	have_artist = false;
			bool	have_album = false;
			bool	have_title = false;
			bool	have_length = false;
			bool	have_track = false;

			/* some files start with a 0xff - skip it */
			if (buffer[0] == 0xff)
			{
				file.Seek(1, SEEK_SET);
				read = file.Read(buffer, 10);
			}

			/* refer to http://www.id3.org */
			if ((buffer[0] == 'I') &&
				(buffer[1] == 'D') &&
				(buffer[2] == '3') &&
				(buffer[3] >= 0x03) &&
				(buffer[3] < 0xff) &&
				(buffer[4] < 0xff) &&
				(buffer[6] < 0x80) &&
				(buffer[7] < 0x80) &&
				(buffer[8] < 0x80) &&
				(buffer[9] < 0x80))
			{
				tags->artist = "";
				tags->album = "";
				tags->title = "";
				tags->track = 0;
				tags->length = 0;
				tags->genre = 0;
				tags->version = id3_vers_2_3_0;

				/* id3v2 header (http://www.id3.org/id3v2.3.0.html) */
				size_t	tag_size = int28toint32(&buffer[6]);
				uchar*	header = (uchar*)malloc(tag_size);

				//printf("%s\n", path);
				if (header)
				{
					uchar*	offset = header;

					file.Read(header, tag_size);
					while ((tag_size > 0) &&
							((!have_album) ||
							(!have_artist) ||
							(!have_title) ||
							(!have_length) ||
							(!have_track)))
					{
						uchar	encoding;
						uint32	frame = B_HOST_TO_BENDIAN_INT32(*((uint32*)offset));
						size_t	size;

						//printf("%c%c%c%c (%x)\n", frame >> 24, frame >> 16, frame >> 8, frame, frame);
						if (!frame)
						{
							//printf("hmmm, still expecting %d bytes\n", tag_size);
							break;
						}

						offset += 4;
						size = int28toint32(offset);
						if (size > tag_size)
						{
							//printf("hmmm, frame size is unreasonable: %d\n", (int) size);
							break;
						}

						offset += 4;
						offset += 2;	/* skip flags */
						tag_size -= (10 + size);
						switch (frame)
						{
							case 'TIT2':	/* title */
								encoding = *offset;
								tags->title.SetTo((char*)offset + 1, size - 1);
								have_title = true;
								break;

							case 'TPE1':	/* artist */
								encoding = *offset;
								tags->artist.SetTo((char*)offset + 1, size - 1);
								have_artist = true;
								break;

							case 'TALB':	/* album */
								encoding = *offset;
								tags->album.SetTo((char*)offset + 1, size - 1);
								have_album = true;
								break;

							case 'TLEN':	/* length  in milleseconds */
							case 'TRCK':	/* track number */
								{
									char*	length;

									encoding = *offset;
									length = (char*)malloc(size);
									strncpy(length, (char*)offset + 1, size - 1);
									length[size - 1] = '\0';
									if (frame == 'TLEN')
									{
										tags->length = strtoll(length, NULL, 0);
										have_length = true;
									}
									else
									{
										tags->track = strtol(length, NULL, 0);
										have_track = true;
									}
									free(length);
								}
								break;
						}
						offset += size;
					}
					free(header);	
				}
			}
			else
			{
				file.Seek(-128, SEEK_END);
				if ((file.Read(buffer, 128) == 128) && (strncmp((char*)buffer, "TAG", 3) == 0))
				{
					/* id3v1 header (http://www.id3.org/id3v1.html) */
					remove_trailing_spaces(&tags->title, (char*)buffer + 3, 30);
					remove_trailing_spaces(&tags->artist, (char*)buffer + 33, 30);
					remove_trailing_spaces(&tags->album, (char*)buffer + 63, 30);
					tags->track = 0;
					tags->length = 0;
					tags->genre = buffer[127];
					tags->version = id3_vers_1;
				}
				else
					result = B_ERROR;
			}
		}
		else
			result = read;
	}
	return result;
}


/*-----------------------------------------------------------------*/

status_t WriteIDTags(const char* path, id3_tags* tags)
{
	BFile		file(path, O_RDWR);
	status_t	result;

	if ((result = file.InitCheck()) == B_NO_ERROR)
	{
		uchar		buffer[128];

		result = B_ERROR;
		switch (tags->version)
		{
			case id3_vers_unknown:
			case id3_vers_1:
			case id3_vers_2_2_0:
				{
					file.Seek(-128, SEEK_END);
					if ((file.Read(buffer, 128) == 128) && (strncmp((char*)buffer, "TAG", 3) == 0))
					/* overwrite existing ID3 tags */
						file.Seek(-128, SEEK_END);
					memcpy(buffer, "TAG", 3);
					pad(&tags->title, (char*)buffer + 3, 30);
					pad(&tags->artist, (char*)buffer + 33, 30);
					pad(&tags->album, (char*)buffer + 63, 30);
					buffer[127] = tags->genre;
					if ((result = file.Write(buffer, 128)) > 0)
						result = B_NO_ERROR;
				}
				break;

			case id3_vers_2_3_0:
				{
					bool		has_ff = false;
					uchar*		new_header = (uchar*)malloc(10);
					uint32		new_length = 10;
					ssize_t		read;

					if (new_header)
					{
						if ((read = file.Read(buffer, 10)) >= 0)
						{
							/* some files start with a 0xff - skip it */
							if (buffer[0] == 0xff)
							{
								has_ff = true;
								file.Seek(1, SEEK_SET);
								read = file.Read(buffer, 10);
							}
							// copy old header to new header
							memcpy(new_header, buffer, 10);

							size_t	old_length = int28toint32(&buffer[6]);
							size_t	saved_length = old_length;
							uchar*	old_header = (uchar*)malloc(old_length);

							if (old_header)
							{
								bool	updated_artist = false;
								bool	updated_album = false;
								bool	updated_title = false;
								bool	updated_track = false;
								uchar*	offset = old_header;

								file.Read(old_header, old_length);

								result = B_NO_MEMORY;
								while (old_length)
								{
									uint32	frame = B_HOST_TO_BENDIAN_INT32(*((uint32*)offset));
									size_t	size;

									if (!frame)
										break;

									size = int28toint32(offset + 4);
									if (size > old_length)
										break;
			
									old_length -= (10 + size);
									switch (frame)
									{
										case 'TIT2':	/* title */
											if (update_tag(new_header, offset, new_length, tags->title) == B_ERROR)
												goto fail;
											updated_title = true;
											break;
			
										case 'TPE1':	/* artist */
											if (update_tag(new_header, offset, new_length, tags->artist) == B_ERROR)
												goto fail;
											updated_artist = true;
											break;
			
										case 'TALB':	/* album */
											if (update_tag(new_header, offset, new_length, tags->album) == B_ERROR)
												goto fail;
											updated_album = true;
											break;
			
										case 'TRCK':	/* track number */
											{
												BString	track;

												track << tags->track;
												if (update_tag(new_header, offset, new_length, track) == B_ERROR)
													goto fail;
												updated_track = true;
											}
											break;

										default:		/* copy unknown frame */
											{
												uchar*	tmp;

												tmp = (uchar*)realloc(new_header, new_length + 10 + size);
												if (tmp)
												{
													new_header = tmp;
													memcpy(&new_header[new_length], offset, 10 + size);
													new_length += 10 + size;
												}
												else
													goto fail;
											}
											
									}
									offset += (10 + size);
								}

								/* if old header didn't have any interesting tags, add them */
								if (!updated_title)
								{
									int32	len;

									if ((len = add_tag(B_HOST_TO_BENDIAN_INT32('TIT2'), new_header, new_length, tags->title)) == B_ERROR)
										goto fail;
									new_length += len;
								}
								if (!updated_artist)
								{
									int32	len;

									if ((len = add_tag(B_HOST_TO_BENDIAN_INT32('TPE1'), new_header, new_length, tags->artist)) == B_ERROR)
										goto fail;
									new_length += len;
								}
								if (!updated_album)
								{
									int32	len;

									if ((len = add_tag(B_HOST_TO_BENDIAN_INT32('TALB'), new_header, new_length, tags->album)) == B_ERROR)
										goto fail;
									new_length += len;
								}
								if (!updated_track)
								{
									int32	len;
									BString	track;

									track << tags->track;
									if ((len = add_tag(B_HOST_TO_BENDIAN_INT32('TRCK'), new_header, new_length, track)) == B_ERROR)
										goto fail;
									new_length += len;
								}

								if ((new_length - 10) <= saved_length)
								{
									/* easy case, just overwrite existing header and we're done */
									int32	null = 0;

									file.Seek((has_ff) ? 1 : 0, SEEK_SET);
									if ((result = file.Write(new_header, new_length)) < (int)new_length)
										goto fail;
									/* zero rest of old header */
									for (int32 loop = 0; loop < (int32)(saved_length - (new_length - 10)); loop++)
									{
										if ((result = file.Write(&null, 1)) < 1)
											goto fail;
									}
								}
								else
								{
									/* bad case, the new header is bigger than the old one so we
									   need to re-write the complete file - yuck */
									/* give another kHEAD_ROOM bytes of headroom */
									int32	index = 1;
									BFile	f;
									BPath	p1(path);
									BPath	p2;
									BString	tmp;

									p1.GetParent(&p2);
									while (1)
									{
										status_t	result;

										tmp = "";
										tmp << p2.Path() << '/' << "tmp" << index++;
										result = f.SetTo(tmp.String(), B_READ_WRITE | B_FAIL_IF_EXISTS | B_CREATE_FILE);
										if (result == B_NO_ERROR)
											break;
										if ((result != B_FILE_EXISTS) && (result != B_NOT_ALLOWED))
											goto fail;
									}
									int32toint28(new_length - 10 + kHEAD_ROOM + ((has_ff) ? 1 : 0), &new_header[6]);
									if (((result = write_header(&f, new_header, new_length, has_ff, kHEAD_ROOM)) == B_NO_ERROR) &&
										((result = copy_file(&f, &file, saved_length + 10)) == B_NO_ERROR))
									{
										BEntry	entry(path);

										entry.Remove();
										entry.SetTo(tmp.String());
										entry.Rename(path);
									}
									else
									{
										BEntry	entry(tmp.String());

										entry.Remove();
										goto fail;
									}
								}
								result = B_NO_ERROR;

fail:;
								free(old_header);
							}
							free(new_header);
						}
					}
				}
				break;			
		}
	}
	return result;
}


/*-----------------------------------------------------------------*/

int32 int28toint32(uchar* int28)
{
	return ((int28[0] << 21) | (int28[1] << 14) | (int28[2] << 7) | int28[3]);
}


/*-----------------------------------------------------------------*/

void int32toint28(int32 n, uchar* int28)
{
	int28[0] = (n >> 21) & 0x7f;
	int28[1] = (n >> 14) & 0x7f;
	int28[2] = (n >> 7) & 0x7f;
	int28[3] = n & 0x7f;
}


/*-----------------------------------------------------------------*/

void pad(BString* src, char* dst, int32 count)
{
	int32	cnt = min_c((int32)strlen(src->String()), count);
	int32	padding = count - cnt;

	strncpy(dst, src->String(), cnt);
	while (padding > 0)
	{
		dst[cnt++] = ' ';	
		padding--;
	}
}


/*-----------------------------------------------------------------*/

int32 add_tag(uint32 tag, uchar*& new_header, uint32& new_length, BString new_string)
{
	uchar*	tmp;

	tmp = (uchar*)realloc(new_header, new_length + 11 + new_string.Length());
	if (tmp)
	{
		new_header = tmp;
		// tag
		memcpy(&new_header[new_length], &tag, 4);
		// length
		int32toint28(new_string.Length() + 1, &new_header[new_length + 4]);
		// flags
		new_header[new_length + 8] = 0;
		new_header[new_length + 9] = 0;
		// encoding
		new_header[new_length + 10] = 0;
		// data
		memcpy(&new_header[new_length + 11], new_string.String(), new_string.Length());
		new_length += 11 + new_string.Length();
		return B_NO_ERROR;
	}
	else
		return B_ERROR;
}


/*-----------------------------------------------------------------*/

int32 update_tag(uchar*& new_header, uchar* old_header, uint32& new_length, BString new_string)
{
	uchar*	tmp;

	tmp = (uchar*)realloc(new_header, new_length + 11 + new_string.Length());
	if (tmp)
	{
		new_header = tmp;
		// tag
		memcpy(&new_header[new_length], old_header, 4);
		// length
		int32toint28(new_string.Length() + 1, &new_header[new_length + 4]);
		// flags
		memcpy(&new_header[new_length + 8], &old_header[8], 2);
		// encoding
		new_header[new_length + 10] = 0;
		// data
		memcpy(&new_header[new_length + 11], new_string.String(), new_string.Length());
		new_length += 11 + new_string.Length();
		return B_NO_ERROR;
	}
	else
		return B_ERROR;
}


/*-----------------------------------------------------------------*/

void remove_trailing_spaces(BString* target, char* str, int32 count)
{
	while (count)
	{
		if (str[count - 1] != ' ')
		{
			target->SetTo(str, count);
			break;
		}
		count--;
	}
}


/*-----------------------------------------------------------------*/

status_t copy_file(BFile* dst, BFile* src, uint32 offset)
{
	char*		buffer;
	int32		buffer_size = 64 * 1024;
	ssize_t		read;
	status_t	result = B_NO_MEMORY;

	while ((buffer = (char*)malloc(buffer_size)) == NULL)
	{
		buffer_size = buffer_size >> 1;
		if (!buffer_size)
			return result;
	}

	src->Seek(offset, SEEK_SET);
	while (1)
	{
		if ((read = src->Read(buffer, buffer_size)) < 0)
		{
			result = read;
			break;
		}
		if ((result = dst->Write(buffer, read)) != read)
			break;
		if (read < buffer_size)
		{
			result = B_NO_ERROR;
			break;
		}
	}
	free(buffer);
	return result;
}


/*-----------------------------------------------------------------*/

status_t write_header(BFile* file, uchar* new_header, uint32 new_length, bool has_ff, int32 head_room)
{
	int32		null = 0;
	status_t	result;

	if (has_ff)
	{
		char	ff = 0xff;
		if ((result = file->Write(&ff, 1)) < 1)
			return result;
	}

	if ((result = file->Write(new_header, new_length)) < (int)new_length)
		return result;

	for (int32 loop = 0; loop < head_room; loop++)
	{
		if ((result = file->Write(&null, 1)) < 1)
			return result;
	}
	return B_NO_ERROR;
}
