/* camera.h
 *
 * AV/C Camera subunit definitions
 *
 */

#ifndef _1394_AVC_CAMERA_H
#define _1394_AVC_CAMERA_H

#define CAMERA_AE_MODE					0x40
#define CAMERA_AE_SHIFT					0x42
#define CAMERA_IRIS						0x43
#define CAMERA_SHUTTER_SPEED			0x44
#define CAMERA_AGC_GAIN					0x45
#define CAMERA_FLASH					0x48
#define CAMERA_VIDEO_LIGHT				0x49
#define CAMERA_GAMMA					0x52
#define CAMERA_SETUP_LEVEL				0x54
#define CAMERA_CONTRAST					0x55
#define CAMERA_SHARPNESS				0x56
#define CAMERA_SATURATION				0x5b
#define CAMERA_HUE						0x5c
#define CAMERA_WHITE_BALANCE			0x5d
#define CAMERA_DIGITAL_ZOOM				0x60
#define CAMERA_DIGITAL_ZOOM_MAX_LIMIT	0x61
#define CAMERA_FREEZE					0x62
#define CAMERA_REVERSE					0x64
#define CAMERA_RANGE					0x70
#define CAMERA_SUPPORT_LEVEL_PROFILE	0x72
#define CAMERA_AGC_MAXIMUM_GAIN			0x74
#define CAMERA_IRIS_RANGE				0x75
#define CAMERA_CCD_SCAN_MODE			0x7a
#define CAMERA_FOCUS					0xc1
#define CAMERA_FOCUSSING_POSITION		0xc2
#define CAMERA_FOCAL_LENGTH				0xc3
#define CAMERA_ZOOM						0xc4
#define CAMERA_AF_MODE					0xc8
#define CAMERA_ND_FILTER				0xcb
#define CAMERA_PAN						0xda
#define CAMERA_TILT						0xdb
#define CAMERA_IMAGE_STABILIZER			0xdc

/* Automatic exposure */

#define CAMERA_AE_MODE_FULL_AUTOMATIC			0x00
#define CAMERA_AE_MODE_GAIN_PRIORITY_MODE		0x01
#define CAMERA_AE_MODE_SHUTTER_PRIORITY_MODE	0x02
#define CAMERA_AE_MODE_IRIS_PRIORITY_MODE		0x04
#define CAMERA_AE_MODE_MANUAL					0x0f

status_t camera_get_ae_mode(
				avc_subunit		*subunit,
				uchar			*ae_mode,
				bigtime_t		timeout);

status_t camera_set_ae_mode(
				avc_subunit		*subunit,
				uchar			ae_mode,
				bigtime_t		timeout);

#define CAMERA_AE_SHIFT_DEFAULT			0x00
#define CAMERA_AE_SHIFT_INCREMENT		0x01
#define CAMERA_AE_SHIFT_DECREMENT		0x02

status_t camera_get_ae_shift(
				avc_subunit		*subunit,
				uchar			*ae_shift,
				bigtime_t		timeout);

status_t camera_set_ae_shift(
				avc_subunit		*subunit,
				uchar			ae_shift,
				bigtime_t		timeout);

/* Automatic focussing */

#define CAMERA_AF_MODE_FOCUSSING_AUTO	0x00
#define CAMERA_AF_MODE_FOCUSSING_MANUAL	0x01

status_t camera_get_af_mode(
				avc_subunit		*subunit,
				uchar			*af_mode,
				bigtime_t		timeout);

status_t camera_set_af_mode(
				avc_subunit		*subunit,
				uchar			af_mode,
				bigtime_t		timeout);

/* Automatic gain control */

#define CAMERA_AGC_GAIN_DEFAULT			0x00
#define CAMERA_AGC_GAIN_INCREMENT		0x01
#define CAMERA_AGC_GAIN_DECREMENT		0x02

status_t camera_get_agc_gain(
				avc_subunit		*subunit,
				uchar			*agc_gain,
				bigtime_t		timeout);

status_t camera_set_agc_gain(
				avc_subunit		*subunit,
				uchar			agc_gain,
				bigtime_t		timeout);

status_t camera_get_agc_maximum_gain(
				avc_subunit		*subunit,
				uchar			*agc_gain,
				bigtime_t		timeout);

#define CAMERA_CCD_SCAN_MODE_INTERLACE		0x00
#define CAMERA_CCD_SCAN_MODE_PROGRESSIVE	0x01

status_t camera_get_ccd_scan_mode(
				avc_subunit		*subunit,
				uchar			*ccd_scan_mode,
				bigtime_t		timeout);

status_t camera_set_ccd_scan_mode(
				avc_subunit		*subunit,
				uchar			ccd_scan_mode,
				bigtime_t		timeout);

#define CAMERA_CONTRAST_DEFAULT				0x00
#define CAMERA_CONTRAST_INCREMENT			0x01
#define CAMERA_CONTRAST_DECREMENT			0x02

status_t camera_get_contrast(
				avc_subunit		*subunit,
				uchar			*contrast,
				bigtime_t		timeout);

status_t camera_set_contrast(
				avc_subunit		*subunit,
				uchar			contrast,
				bigtime_t		timeout);

#define CAMERA_DIGITAL_ZOOM_DISABLE		0x60
#define CAMERA_DIGITAL_ZOOM_ENABLE		0x70

status_t camera_get_digital_zoom_state(
				avc_subunit		*subunit,
				uchar			*zoom_state,
				bigtime_t		timeout);

status_t camera_set_digital_zoom_state(
				avc_subunit		*subunit,
				uchar			zoom_state,
				bigtime_t		timeout);

#define CAMERA_DIGITAL_ZOOM_MAX_LIMIT_DEFAULT	0x00
#define CAMERA_DIGITAL_ZOOM_MAX_LIMIT_INCREMENT	0x01
#define CAMERA_DIGITAL_ZOOM_MAX_LIMIT_DECREMENT	0x02

status_t camera_get_digital_zoom_max_limit(
				avc_subunit		*subunit,
				uchar			*limit,
				bigtime_t		timeout);

status_t camera_set_digital_zoom_max_limit(
				avc_subunit		*subunit,
				uchar			limit,
				bigtime_t		timeout);

#define CAMERA_FLASH_OFF				0x60
#define CAMERA_FLASH_CHARGING			0x71
#define CAMERA_FLASH_COMPLETE			0x72

status_t camera_get_flash(
				avc_subunit		*subunit,
				uchar			*state, uchar *charge,
				bigtime_t		timeout);

/*************** more commands *****************/

#define CAMERA_ZOOM_TELE_FASTEST		0x31
#define CAMERA_ZOOM_TELE_6				0x33
#define CAMERA_ZOOM_TELE_5				0x35
#define CAMERA_ZOOM_TELE_4				0x37
#define CAMERA_ZOOM_TELE_3				0x39
#define CAMERA_ZOOM_TELE_2				0x3B
#define CAMERA_ZOOM_TELE_1				0x3D
#define CAMERA_ZOOM_TELE_SLOWEST		0x3F
#define CAMERA_ZOOM_STOP				0x60
#define CAMERA_ZOOM_WIDE_SLOWEST		0x41
#define CAMERA_ZOOM_WIDE_1				0x43
#define CAMERA_ZOOM_WIDE_2				0x45
#define CAMERA_ZOOM_WIDE_3				0x47
#define CAMERA_ZOOM_WIDE_4				0x49
#define CAMERA_ZOOM_WIDE_5				0x4B
#define CAMERA_ZOOM_WIDE_6				0x4D
#define CAMERA_ZOOM_WIDE_FASTEST		0x4F

#define CAMERA_ZOOM_UNSPECIFIED_TELE	0x31
#define CAMERA_ZOOM_UNSPECIFIED_WIDE	0x41

status_t camera_get_zoom(
				avc_subunit		*subunit,
				uchar			*zoom_mode,
				bigtime_t		timeout);

status_t camera_set_zoom(
				avc_subunit		*subunit,
				uchar			zoom_mode,
				bigtime_t		timeout);

#endif
