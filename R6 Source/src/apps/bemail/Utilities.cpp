//--------------------------------------------------------------------
//	
//	Utilities.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Alert.h>
#include "Utilities.h"


//====================================================================
// case-insensitive version of strcmp
//

int32 cistrcmp(const char *str1, const char *str2)
{
	char	c1;
	char	c2;
	int32	len;
	int32	loop;

	len = strlen(str1) + 1;
	for (loop = 0; loop < len; loop++) {
		c1 = str1[loop];
		if ((c1 >= 'A') && (c1 <= 'Z'))
			c1 += ('a' - 'A');
		c2 = str2[loop];
		if ((c2 >= 'A') && (c2 <= 'Z'))
			c2 += ('a' - 'A');
		if (c1 == c2) {
		}
		else if (c1 < c2)
			return -1;
		else if ((c1 > c2) || (!c2))
			return 1;
	}
	return 0;
}


//====================================================================
// case-insensitive version of strncmp
//

int32 cistrncmp(const char *str1, const char *str2, int32 max)
{
	char		c1;
	char		c2;
	int32		loop;

	for (loop = 0; loop < max; loop++) {
		c1 = *str1++;
		if ((c1 >= 'A') && (c1 <= 'Z'))
			c1 += ('a' - 'A');
		c2 = *str2++;
		if ((c2 >= 'A') && (c2 <= 'Z'))
			c2 += ('a' - 'A');
		if (c1 == c2) {
		}
		else if (c1 < c2)
			return -1;
		else if ((c1 > c2) || (!c2))
			return 1;
	}
	return 0;
}


//--------------------------------------------------------------------
// case-insensitive version of strstr
//

char* cistrstr(char *cs, char *ct)
{
	char		c1;
	char		c2;
	int32		cs_len;
	int32		ct_len;
	int32		loop1;
	int32		loop2;

	cs_len = strlen(cs);
	ct_len = strlen(ct);
	for (loop1 = 0; loop1 < cs_len; loop1++) {
		if (cs_len - loop1 < ct_len)
			goto done;
		for (loop2 = 0; loop2 < ct_len; loop2++) {
			c1 = cs[loop1 + loop2];
			if ((c1 >= 'A') && (c1 <= 'Z'))
				c1 += ('a' - 'A');
			c2 = ct[loop2];
			if ((c2 >= 'A') && (c2 <= 'Z'))
				c2 += ('a' - 'A');
			if (c1 != c2)
				goto next;
		}
		return(&cs[loop1]);
next:;
	}
done:;
	return(NULL);
}


//--------------------------------------------------------------------
// Un-fold field and add items to dst
//

void extract(char **dst, char *src)
{
	bool		remove_ws = true;
	int32		comma = 0;
	int32		count = 0;
	int32		index = 0;
	int32		len;
	int32 		srcLen = strlen(src);
	
	if (strlen(*dst))
		comma = 2;

	for (;;) {
		if ((src[index] == '\r') || (src[index] == '\n')) {
			if (count) {
				len = strlen(*dst);
				*dst = (char *)realloc(*dst, len + count + comma + 1);
				if (comma) {
					(*dst)[len++] = ',';
					(*dst)[len++] = ' ';
					comma = 0;
				}
				memcpy(&((*dst)[len]), &src[index - count], count);
				(*dst)[len + count] = 0;
				count = 0;

				if (src[index + 1] == '\n')
					index++;
				if ((src[index + 1] != ' ') && (src[index + 1] != '\t'))
					break;
			}
		}
		else {
			if ((remove_ws) && ((src[index] == ' ') || (src[index] == '\t'))) {
			}
			else {
				remove_ws = false;
				count++;
			}
		}
		index++;
		if(index > srcLen)
			break;
	}
}


//--------------------------------------------------------------------
// get list of recipients
//

void get_recipients(char **str, char *header, int32 len, bool all)
{
	int32		start;

	start = 0;
	for (;;) {
		if (!(cistrncmp("To: ", &header[start], strlen("To: ") - 1))) 
			extract(str, &header[start + strlen("To: ")]);
		else if (!(cistrncmp("Cc: ", &header[start], strlen("Cc: ") - 1)))
			extract(str, &header[start + strlen("Cc: ")]);
		else if ((all) && (!(cistrncmp("From: ", &header[start], strlen("From: ") - 1))))
			extract(str, &header[start + strlen("From: ")]);
		else if ((all) && (!(cistrncmp("Reply-To: ", &header[start], strlen("Reply-To: ") - 1))))
			extract(str, &header[start + strlen("Reply-To: ")]);
		start += linelen(&header[start], len - start, true);

		if (start >= len)
			break;
	}
	verify_recipients(str);
}


//--------------------------------------------------------------------
// verify list of recipients
//

void verify_recipients(char **to)
{
	bool		quote;
	char		*dst;
	char		*src;
	char		*name;
	int32		dst_len = 0;
	int32		index = 0;
	int32		loop;
	int32		offset;
	int32		len;
	int32		l;
	int32		src_len;

	src = *to;
	src_len = strlen(src);
	while (index < src_len) {
		len = 0;
		quote = false;
		while ((src[index + len]) && ((quote) || ((!quote) && (src[index + len] != ',')))) {
			if (src[index + len] == '"')
				quote = !quote;
			len++;
		}

		//
		//	Error in header
		//
		if (quote) {
			(new BAlert("Error", "There is an error in the header of this "
			  "message, some information may not appear correctly.",
			  "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			break;
		}

// remove quoted text...
		if (len) {
			offset = index;
			index += len + 1;
			for (loop = offset; loop < offset + len; loop++) {
				if (src[loop] == '"') {
					len -= (loop - offset) + 1;
					offset = loop + 1;
					while ((len) && (src[offset] != '"')) {
						offset++;
						len--;
					}
					offset++;
					len--;
					break;
				}
			}

// look for bracketed '<...>' text...
			for (loop = offset; loop < offset + len; loop++) {
				if (src[loop] == '<') {
					offset = loop + 1;
					l = 0;
					while ((l < len) && (src[offset + l] != '>')) {
						l++;
					}
					goto add;
				}
			}

// not bracketed? then take it all...
			l = len;
			while ((l) && ((src[offset] == ' ') || (src[offset] == '\t'))) {
				offset++;
				l--;
			}

add:
			if (dst_len) {
				dst = (char *)realloc(dst, dst_len + 1 + l);
				dst[dst_len++] = ',';
			}
			else
				dst = (char *)malloc(l);
			memcpy(&dst[dst_len], &src[offset], l);
			dst_len += l;
		}
		else
			index++;
	}

	if (dst_len) {
		dst = (char *)realloc(dst, dst_len + 1);
		dst[dst_len] = 0;
	}
	else {
		dst = (char *)malloc(1);
		dst[0] = 0;
	}

	free(*to);
	*to = dst;
}


//--------------------------------------------------------------------
// return length of \n terminated line
//

int32 linelen(char *str, int32 len, bool header)
{
	int32		loop;

	for (loop = 0; loop < len; loop++) {
		if (str[loop] == '\n') {
			if ((!header) || (loop < 2) || ((header) && (str[loop + 1] != ' ') &&
										  (str[loop + 1] != '\t')))
				return loop + 1;
		}
	}
	return len;
}


//--------------------------------------------------------------------
// get named parameter from string
//

bool get_parameter(char *src, char *param, char *dst)
{
	char		*offset;
	int32		len;

	if (offset = cistrstr(src, param)) {
		offset += strlen(param);
		len = strlen(src) - (offset - src);
		if (*offset == '"') {
			offset++;
			len = 0;
			while (offset[len] != '"') {
				if (offset[len] == '\0') {
					(new BAlert("Error", "There is an error in the header "
					  "of this message, some information may not appear "
					  "correctly.", "OK", NULL, NULL, B_WIDTH_AS_USUAL,
					  B_WARNING_ALERT))->Go();
					return false;						
				}
			
				len++;
			}
			offset[len] = 0;
		}
		strcpy(dst, offset);
		return true;
	}
	return false;
}


//--------------------------------------------------------------------
// search buffer for boundary
//

char* find_boundary(char *buf, char *boundary, int32 len)
{
	char	*offset;

	offset = buf;
	while (strncmp(boundary, offset, strlen(boundary))) {
		offset += linelen(offset, (buf + len) - offset + 1, false);
		if (*offset == '\r')
			offset++;
		if (offset >= buf + len)
			return NULL;
	}
	return offset;
}
