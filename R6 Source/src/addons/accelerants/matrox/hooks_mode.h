//////////////////////////////////////////////////////////////////////////////
// Video Mode: Function Prototypes
//////////////////////////////////////////////////////////////////////////////

uint32 AccelerantModeCount(void);

status_t GetModeList(display_mode *dm);

status_t ProposeVideoMode(display_mode *target,
                          const display_mode *low,
                          const display_mode *high);

status_t SetVideoMode(display_mode *dm);

status_t GetVideoMode(display_mode *dm);

status_t GetFramebufferConfig(frame_buffer_config *fbc);

status_t GetPixelClockLimits(display_mode *dm,
                             uint32 *low,
                             uint32 *high);

status_t MoveDisplayArea(uint16 h_display_start,
                               uint16 v_display_start);

void SetClipRect(uint32 cxl,
                 uint32 cyt,
                 uint32 cxr,
                 uint32 cyb,
                 uint32 v_width,
                 uint32 ydstorg);

void SetIndexedColors(uint count,
                      uint8 first,
                      uint8 *color_data,
                      uint32 flags);


bool SetScreenSaver(bool enable);

void create_mode_list(void);

void calc_max_clocks(void);


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
