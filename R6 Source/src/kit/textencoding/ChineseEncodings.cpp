#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "ChineseEncodings.h"
#include "ChineseMappings.cpp"
#define	INDEX_BITS	8
#define	B_SUBSTITUTE	0x1a
static int getUtf8Bytes (int utf8First)
{
	int numBytes;
	if ((utf8First & 0x80) == 0x00)
		numBytes = 1;
	else if ((utf8First & 0xC0) == 0x80)
		numBytes = 0;
	else if ((utf8First & 0xE0) == 0xC0)
		numBytes = 2;
	else if ((utf8First & 0xF0) == 0xE0)
		numBytes = 3;
	else if ((utf8First & 0xF8) == 0xF0)
		numBytes = 4;
	else if ((utf8First & 0xFC) == 0xF8)
		numBytes = 5;
	else if ((utf8First & 0xFE) == 0xFC)
		numBytes = 6;
	else
		numBytes = 0;
	return numBytes;
}

static int GetU8Bytes (uint32 ucs)
{
	int numBytes;
	if (ucs < 0x80)
		numBytes = 1;
	else if (ucs < 0x800)
		numBytes = 2;
	else if (ucs < 0x10000)
		numBytes = 3;
	else if (ucs < 0x200000)
		numBytes = 4;
	else if (ucs < 0x4000000)
		numBytes = 5;
	else if (ucs < 0x80000000)
		numBytes = 6;
	else
		numBytes = 0;
	return numBytes;
}

static void WriteU8 (uint32 ucs4, uchar* dst)
{
	const uchar firstByteMark [] = { 0, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
	int numBytes = GetU8Bytes (ucs4);
	dst += numBytes;
	switch (numBytes)
	{
	case 6:	*--dst = (ucs4 & 0x3f) | 0x80;	ucs4 >>= 6;
	case 5:	*--dst = (ucs4 & 0x3f) | 0x80;	ucs4 >>= 6;
	case 4:	*--dst = (ucs4 & 0x3f) | 0x80;	ucs4 >>= 6;
	case 3:	*--dst = (ucs4 & 0x3f) | 0x80;	ucs4 >>= 6;
	case 2:	*--dst = (ucs4 & 0x3f) | 0x80;	ucs4 >>= 6;
	case 1:	*--dst = ucs4 | firstByteMark [numBytes];
	}
}

static status_t	Dbcs2U8
	(const uint16	table [][DBCS_LO_MAX - DBCS_LO_MIN + 1],
	const uchar *src,
	int32		*srcLen,
	uchar		*dst,
	int32		*dstLen,
	int32		*state,
	char		substitute)
{
	int	srcCount = *srcLen, dstCount = *dstLen;
	bool exitf = false, hzEnabled = (*state & HZ_ENABLED) != 0;
	int _state = (*state &= ~HZ_ENABLED);	// STATE_NORMAL or STATE_ALT_NORMAL
	int firstByte, ucs;
//	assert (_state == STATE_NORMAL || _state == STATE_ALT_NORMAL);
/*zgx--*/
	substitute = B_SUBSTITUTE;
/*--zgx*/
	while (srcCount > 0 && dstCount > 0 && !exitf)
	{
		int c = *src++;
		switch (_state)
		{
		case STATE_NORMAL:
			if (hzEnabled && c == '~')
			{
				_state = STATE_ESC_SEQ;
			}
			else if (isDbcsHi (c))
			{
				firstByte = c;
				_state = STATE_DBCS;
			}
			else
			{
//				printf("0x%04x->", c);
				if (c >= 0x80)
					c = substitute;
//				printf("0x%04x\n", c);
				*dst++ = c;
				--srcCount;
				--dstCount;
			}
			break;

		case STATE_DBCS:
			if (isDbcsLo (c) &&
				(ucs = table [firstByte - DBCS_HI_MIN][c - DBCS_LO_MIN]) != 0)
			{
				// got valid UCS
				int numBytes = GetU8Bytes (ucs);
				if (dstCount < numBytes)
					exitf = true;
				else
				{
					WriteU8 (ucs, dst);
					dst += numBytes;
//					printf("0x%02x%02x->0x%04x\n", firstByte, c, ucs);
					srcCount -= 2;
					dstCount -= numBytes;
				}
			}
			else
			{
				// invalid double-byte code
//				printf("0x%02x%02x->0x%04x\n", firstByte, c, substitute);
				*dst++ = substitute;
				srcCount -= 2;
				--dstCount;
			}
			_state = STATE_NORMAL;
			break;

		case STATE_ESC_SEQ:
			switch (c)
			{
			case '{':
				*state = _state = STATE_ALT_NORMAL;
				srcCount -= 2;
				break;

			case '}':
				*state = _state = STATE_NORMAL;
				srcCount -= 2;
				break;

			case '~':
				if (*state == STATE_NORMAL)
				{
					*dst++ = c;
					srcCount -= 2;
					--dstCount;
					_state = *state;	// back to previous state
				}
				else // if (*state == STATE_ALT_NORMAL)
				{
					firstByte = '~' | 0x80;
					--src;				// force to reread current byte
					_state = STATE_ALT_DBCS;
				}
				break;

			case '\r':
			case '\n':
				_state = *state;
				break;

			default:
				if (*state == STATE_NORMAL)
				{
					// undefined escape sequence
					if (dstCount < 2)
						exitf = true;
					else
					{
						*dst++ = '~';
						*dst++ = c;
						srcCount -= 2;
						dstCount -= 2;
					}
					_state = *state;	// back to previous state
				}
				else // if (*state == STATE_ALT_NORMAL)
				{
					firstByte = '~' | 0x80;
					--src;				// force to reread current byte
					_state = STATE_ALT_DBCS;
				}
				break;
			}
			break;

		case STATE_ALT_NORMAL:
			if (hzEnabled && c == '~')
				_state = STATE_ESC_SEQ;
#if 0
			else if (isDbcsHi (c))
			{
				firstByte = c;
				_state = STATE_ALT_DBCS_8BIT;
			}
#endif
			else if (c == '\r' || c == '\n')
			{
				*dst++ = c;
				--srcCount;
				--dstCount;
				*state = _state = STATE_NORMAL;
			}
			else
			{
				firstByte = c | 0x80;
				_state = STATE_ALT_DBCS;
			}
			break;

		case STATE_ALT_DBCS:
			c |= 0x80;
			if ((ucs = table [firstByte - DBCS_HI_MIN][c - DBCS_LO_MIN]) != 0)
			{
				// got valid UCS
				int numBytes = GetU8Bytes (ucs);
				if (dstCount < numBytes)
					exitf = true;
				else
				{
					WriteU8 (ucs, dst);
					dst += numBytes;
					srcCount -= 2;
					dstCount -= numBytes;
				}
			}
			else
			{
				// invalid double-byte code
				*dst++ = substitute;
				srcCount -= 2;
				--dstCount;
			}
			_state = STATE_ALT_NORMAL;
			break;
		}
	}
//	assert (*state == STATE_NORMAL || *state == STATE_ALT_NORMAL);
	if (*srcLen == srcCount || *dstLen == dstCount)
		return B_ERROR;
	*srcLen -= srcCount;
	*dstLen -= dstCount;
	return B_OK;
}

static void ReadU8 (const uchar* src, uint32 *ucs4)
{
	const uint32 offsets [] = {
		0, 0, 
		                                                    (0xC0<< 6) | 0x80, 
		                                       (0xE0<<12) | (0x80<< 6) | 0x80,
		                          (0xF0<<18) | (0x80<<12) | (0x80<< 6) | 0x80,
		             (0xF8<<24) | (0x80<<18) | (0x80<<12) | (0x80<< 6) | 0x80,
		(0xFC<<30) | (0x80<<24) | (0x80<<18) | (0x80<<12) | (0x80<< 6) | 0x80,
		// last one overflows
	};

	int numBytes = getUtf8Bytes (*src);
	uint32 u = 0;
	switch (numBytes)
	{
	case 6:	u += *src++;	u <<= 6;
	case 5:	u += *src++;	u <<= 6;
	case 4:	u += *src++;	u <<= 6;
	case 3:	u += *src++;	u <<= 6;
	case 2:	u += *src++;	u <<= 6;
	case 1:	u += *src++;
	}
	u -= offsets [numBytes];
	*ucs4 = u;
}

static status_t U82Dbcs
	(const uint16	*index [],
	const uchar	*src,
	int32		*srcLen,
	uchar		*dst,
	int32		*dstLen,
	int32		*state,
	char		substitute)
{
	int	srcCount = *srcLen, dstCount = *dstLen;

/*****zgx*/
	substitute = B_SUBSTITUTE;
/**--zgx*/
	while (srcCount > 0 && dstCount > 0)
	{
		int c1 = *src;
		int numBytes = getUtf8Bytes (c1);
		uint32 ucs4;
		const uint16 *table;
		if (numBytes == 0 || srcCount < numBytes)
			break;
		ReadU8 (src, &ucs4);
		if (ucs4 < 0x80)
		{
			// got ASCII
			src += numBytes;
			srcCount -= numBytes;
			*dst++ = ucs4;
			--dstCount;
		}
		else if (ucs4 < 0x10000 &&
			(table = index [ucs4 >> (16 - INDEX_BITS)]) != NULL &&
			(c1 = table [ucs4 & ((1 << (16 - INDEX_BITS)) - 1)]) != 0)
		{
			// got valid DBCS
			if (dstCount < 2)
				break;
			src += numBytes;
			srcCount -= numBytes;
			*dst++ = c1 >> 8;
			*dst++ = c1;
			dstCount -= 2;
		}
		else
		{
			// invalid
			src += numBytes;
			srcCount -= numBytes;
			*dst++ = substitute;
			--dstCount;
		}
	}
	if (*srcLen == srcCount || *dstLen == dstCount)
		return B_ERROR;
	*srcLen -= srcCount;
	*dstLen -= dstCount;
	return B_OK;
}

status_t 
gbk_to_utf8(const char *src, int32 *srcLen, char *dst, int32 *dstLen, int32 *state, char substitute)
{
		*state |= HZ_ENABLED;
		return Dbcs2U8 (gbk_to_ucs,
			(const uchar *)src, srcLen, 
			(uchar *)dst, dstLen,
			state, substitute);
}

status_t 
utf8_to_gbk(const char *src, int32 *srcLen, char *dst, int32 *dstLen, int32 *state, char substitute)
{
	*state = STATE_DBCS ;
		*state |= HZ_ENABLED;
		return U82Dbcs (ucs_to_gbk_index,
			(const uchar *)src, srcLen, 
			(uchar*)dst, dstLen,
			(int32 *)&state, substitute);
}

status_t 
big5_to_utf8(const char *src, int32 *srcLen, char *dst, int32 *dstLen, int32 *state, char substitute)
{
		return Dbcs2U8 (big5_to_ucs,
			(const uchar *)src, srcLen, 
			(uchar *)dst, dstLen,
			state, substitute);
}

status_t 
utf8_to_big5(const char *src, int32 *srcLen, char *dst, int32 *dstLen, int32 *state, char substitute)
{
	//state = STATE_DBCS ;
		return U82Dbcs (ucs_to_big5_index, 
			(const uchar *)src, srcLen, 
			(uchar*)dst, dstLen,
			state, substitute);
}
