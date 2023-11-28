//*****************************************************************************
//
//	File:		Swap.cpp
//
//	Description:
//	
//	Written by:	Peter Potrebic
//
//	Copyright 199r87, Be Incorporated
//
//****************************************************************************/

#include <support2/Debug.h>
#include <sys/types.h>
#include <support2/SupportDefs.h>
#include <render2/Rect.h>
#include <render2/Point.h>
#include <support2/ByteOrder.h>

using namespace B::Support2;

bool is_type_swapped(type_code type)
{
	switch (type) {
		case B_BOOL_TYPE:
		case B_CHAR_TYPE:
		case B_COLOR_8_BIT_TYPE:
		case B_DOUBLE_TYPE:
		case B_FLOAT_TYPE:
		case B_GRAYSCALE_8_BIT_TYPE:
		case B_INT64_TYPE:
		case B_INT32_TYPE:
		case B_INT16_TYPE:
		case B_INT8_TYPE:
		case B_MESSAGE_TYPE:
		case B_MESSENGER_TYPE:
		case B_MIME_TYPE:
		case B_MONOCHROME_1_BIT_TYPE:
		case B_OFF_T_TYPE:
		case B_PATTERN_TYPE:
		case B_POINTER_TYPE:
		case B_POINT_TYPE:
		case B_RECT_TYPE:
		case B_ENTRY_REF_TYPE:
		case B_RGB_32_BIT_TYPE:
		case B_COLOR_32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_STRING_TYPE:
		case B_TIME_TYPE:
		case B_UINT64_TYPE:
		case B_UINT32_TYPE:
		case B_UINT16_TYPE:
		case B_UINT8_TYPE:
			return true;
	}
	return false;
}

status_t swap_data(type_code type, void *d, size_t length,
	swap_action action)
{
	if ((action == B_SWAP_HOST_TO_LENDIAN) && (B_HOST_IS_LENDIAN))
		return B_OK;
	if ((action == B_SWAP_LENDIAN_TO_HOST) && (B_HOST_IS_LENDIAN))
		return B_OK;

	if ((action == B_SWAP_HOST_TO_BENDIAN) && (B_HOST_IS_BENDIAN))
		return B_OK;
	if ((action == B_SWAP_BENDIAN_TO_HOST) && (B_HOST_IS_BENDIAN))
		return B_OK;

	char	*data = (char *) d;
	char	*end = data + length;

	status_t	err = B_OK;

	switch (type) {
		case B_INT16_TYPE:
		case B_UINT16_TYPE: {
			while (data < end) {
				*((int16 *) data) = __swap_int16(*((int16 *) data));
				data += sizeof(int16);
			}
			break;
		}
		case B_SIZE_T_TYPE:			// ??? could be diff size on diff platform
		case B_SSIZE_T_TYPE: {		// ??? could be diff size on diff platform
			ASSERT(sizeof(size_t) == sizeof(int32));
			ASSERT(sizeof(ssize_t) == sizeof(int32));
			while (data < end) {
				*((int32 *) data) = __swap_int32(*((int32 *) data));
				data += sizeof(int32);
			}
			break;
		}
		case B_TIME_TYPE: {
			ASSERT(sizeof(time_t) == sizeof(int32));
			while (data < end) {
				*((int32 *) data) = __swap_int32(*((int32 *) data));
				data += sizeof(int32);
			}
			break;
		}
		case B_POINTER_TYPE:
		case B_INT32_TYPE:
		case B_UINT32_TYPE: {
			while (data < end) {
				*((int32 *) data) = __swap_int32(*((int32 *) data));
				data += sizeof(int32);
			}
			break;
		}
		case B_OFF_T_TYPE: {		// ??? could be diff size on diff platform
			ASSERT(sizeof(off_t) == sizeof(int64));
			while (data < end) {
				*((int64 *) data) = __swap_int64(*((int64 *) data));
				data += sizeof(int64);
			}
			break;
		}
		case B_INT64_TYPE:
		case B_UINT64_TYPE: {
			while (data < end) {
				*((int64 *) data) = __swap_int64(*((int64 *) data));
				data += sizeof(int64);
			}
			break;
		}
		case B_RECT_TYPE: {
			B::Render2::BRect	*r;
			while (data < end) {
				r = (B::Render2::BRect *) data;
				r->left = __swap_float(r->left);
				r->top = __swap_float(r->top);
				r->right = __swap_float(r->right);
				r->bottom = __swap_float(r->bottom);
				data += sizeof(B::Render2::BRect);
			}
			break;
		}
		case B_POINT_TYPE: {
			B::Render2::BPoint	*pt;
			while (data < end) {
				pt = (B::Render2::BPoint *) data;
				pt->x = __swap_float(pt->x);
				pt->y = __swap_float(pt->y);
				data += sizeof(B::Render2::BPoint);
			}
			break;
		}
		case B_FLOAT_TYPE: {
			while (data < end) {
				*((float *) data) = __swap_float(*((float *) data));
				data += sizeof(float);
			}
			break;
		}
		case B_DOUBLE_TYPE: {
			while (data < end) {
				*((double *) data) = __swap_double(*((double *) data));
				data += sizeof(double);
			}
			break;
		}
		default: {
			err = B_BAD_VALUE;	// can't swap this type
			break;
		}
	}
	return err;
}
