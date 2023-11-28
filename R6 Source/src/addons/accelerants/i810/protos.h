/* hooks.c */
extern void *get_accelerant_hook(uint32 feature, void *data);
/* thunk.c */
extern uint32 _get_accelerant_mode_count(void);
extern status_t _get_mode_list(display_mode *dm);
extern status_t _propose_display_mode(display_mode *target, display_mode *low, display_mode *high);
extern status_t _get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high);
extern void _set_indexed_colors(uint count, uint8 first, uint8 *color_data, uint32 flags);
extern uint32 _get_dpms_mode(void);
extern status_t _set_dpms_mode(uint32 dpms_flags);
extern status_t _set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask);
extern void _show_cursor(bool on);
/* mode.c */
extern status_t setdisplaymode(display_mode *dm);
extern status_t setfbbase(uint32 fbbase);
extern status_t getframebufferconfig(frame_buffer_config *fbuf);
/* render.c */
extern uint32 getenginecount(void);
extern status_t acquireengine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et);
extern status_t releaseengine(engine_token *et, sync_token *st);
extern void waitengineidle(void);
extern status_t getsynctoken(engine_token *et, sync_token *st);
extern status_t synctotoken(sync_token *st);
extern void rectfill_gen(uint32 color, uint32 bltctl, register fill_rect_params *list, register uint32 count);
extern void rectfill(engine_token *et, uint32 color, register fill_rect_params *list, register uint32 count);
extern void rectinvert(engine_token *et, register fill_rect_params *list, register uint32 count);
extern void blit(engine_token *et, register blit_params *list, register uint32 count);
extern void spanfill(engine_token *et, uint32 color, register uint16 *list, register uint32 count);
extern status_t AccelInit(register struct gfx_card_info *ci);
extern void flushdropcookie(uint32 cookie);
extern void writepacket(void *cl_buf, register int32 size, int execute, int which);
extern void _synclog_(uint32 code, uint32 val);
/* init.c */
extern status_t init(int the_fd);
extern ssize_t clone_info_size(void);
extern void get_clone_info(void *data);
extern status_t init_clone(void *data);
extern void uninit(void);
extern status_t init3d(const char *devpath, void *arg);
extern void uninit3d(const char *devpath, void *gc);
extern void get_device_info3d(const char *devpath, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum);
extern void get_3d_modes(const char *devpath, void (*callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum);
/* overlay.c */
extern uint32 overlaycount(const display_mode *dm);
extern const uint32 *overlaysupportedspaces(const display_mode *dm);
extern uint32 overlaysupportedfeatures(uint32 a_color_space);
extern const overlay_buffer *allocateoverlaybuffer(color_space cs, uint16 width, uint16 height);
extern status_t releaseoverlaybuffer(const overlay_buffer *ob);
extern status_t getoverlayconstraints(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc);
extern overlay_token allocateoverlay(void);
extern status_t releaseoverlay(overlay_token ot);
extern status_t configureoverlay(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov);
