#ifndef gCRTC_H
#define gCRTC_H

#include "accelerant.h"

struct data_ioctl_sis_CRT_settings* prepare_CRT(display_mode *dm, uint32 screen_start_address, uint32 pixels_per_row);

#endif
