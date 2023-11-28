/* nvidia.c */
extern void *get_accelerant_hook(uint32 feature, void *data);
/* thunk.c */
extern status_t _set_display_mode(display_mode *mode_to_set);
extern uint32 _get_accelerant_mode_count(void);
extern status_t _get_mode_list(display_mode *dm);
extern status_t _propose_display_mode(display_mode *target, display_mode *low, display_mode *high);
extern status_t _get_frame_buffer_config(frame_buffer_config *a_frame_buffer);
extern status_t _get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high);
extern status_t _move_display_area(uint16 h_display_start, uint16 v_display_start);
extern void _set_indexed_colors(uint count, uint8 first, uint8 *color_data, uint32 flags);
extern uint32 _get_dpms_mode(void);
extern status_t _set_dpms_mode(uint32 dpms_flags);
extern status_t _set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask);
extern void _show_cursor(bool on);
/* accel.c */
extern uint32 getenginecount(void);
extern status_t acquireengine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et);
extern status_t releaseengine(engine_token *et, sync_token *st);
extern void waitengineidle(void);
extern status_t getsynctoken(engine_token *et, sync_token *st);
extern status_t synctotoken(sync_token *st);
extern void rectfill(engine_token *et, register uint32 color, register fill_rect_params *list, register uint32 count);
extern void rectinvert(engine_token *et, register fill_rect_params *list, register uint32 count);
extern void blit(engine_token *et, register blit_params *list, register uint32 count);
extern status_t AccelInit(register struct gfx_card_info *ci);
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
