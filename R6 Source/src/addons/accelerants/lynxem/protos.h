/* hooks.c */
extern void *get_accelerant_hook(uint32 feature, void *data);
/* thunk.c */
extern status_t _set_display_mode(display_mode *mode_to_set);
extern uint32 _get_accelerant_mode_count(void);
extern status_t _get_mode_list(display_mode *dm);
extern status_t _propose_display_mode(display_mode *target, display_mode *low, display_mode *high);
extern status_t _get_frame_buffer_config(frame_buffer_config *a_frame_buffer);
extern status_t _get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high);
extern void _set_indexed_colors(uint count, uint8 first, uint8 *color_data, uint32 flags);
extern uint32 _get_dpms_mode(void);
extern status_t _set_dpms_mode(uint32 dpms_flags);
extern status_t _set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask);
extern void _show_cursor(bool on);
/* render.c */
extern uint32 getenginecount(void);
extern status_t acquireengine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et);
extern status_t releaseengine(engine_token *et, sync_token *st);
extern void waitengineidle(void);
extern status_t getsynctoken(engine_token *et, sync_token *st);
extern status_t synctotoken(sync_token *st);
extern void rectfill_gen(uint32 color, uint32 drawctl, register fill_rect_params *list, register uint32 count);
extern void rectfill(engine_token *et, uint32 color, register fill_rect_params *list, register uint32 count);
extern void rectinvert(engine_token *et, register fill_rect_params *list, register uint32 count);
extern void blit(engine_token *et, register blit_params *list, register uint32 count);
extern status_t AccelInit(struct gfx_card_info *ci);
/* init.c */
extern status_t init(int the_fd);
extern ssize_t clone_info_size(void);
extern void get_clone_info(void *data);
extern status_t init_clone(void *data);
extern void uninit(void);
