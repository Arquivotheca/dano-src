/*  mode.c  */
extern uint32 _get_accelerant_mode_count (void);
extern status_t _get_mode_list (display_mode *dm);
extern status_t _propose_display_mode (display_mode *target,
				display_mode *low,
				display_mode *high);
extern status_t _set_display_mode (display_mode *mode_to_set);
extern status_t _get_display_mode (display_mode *current_mode);
extern status_t _get_frame_buffer_config (frame_buffer_config *a_frame_buffer);
extern status_t _get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high);
extern status_t _move_display_area (uint16 h_display_start, uint16 v_display_start);
extern status_t _get_display_mode (display_mode *current_mode);

/*  accel.c  */
extern uint32 getenginecount (void);
extern status_t acquireengine (uint32 caps,
			       uint32 max_wait,
			       sync_token *st,
			       engine_token **et);
extern status_t releaseengine (engine_token *et, sync_token *st);
extern void waitengineidle (void);
extern status_t getsynctoken (engine_token *et, sync_token *st);
extern status_t synctotoken (sync_token *st);

/* video.c */
extern status_t neomagic_init ();
extern void _set_indexed_colors (uint count, uint8 first, uint8 *color_data, uint32 flags);
extern status_t _set_dpms_mode(uint32 dpms_flags);
extern uint32 _dpms_capabilities(void);
extern uint32 _dpms_mode(void);

/* cursor.c */
extern status_t _set_cursor_shape (uint16	curswide, uint16	curshigh, uint16	hot_x, uint16	hot_y, uint8	*andbits,uint8	*xorbits);
extern void _show_cursor (bool on);
extern void _move_cursor (int16 x, int16 y);

/* nm2200.c */
extern void nm2200_blit (engine_token *et, register blit_params *list, register uint32 count);
extern void nm2200_rectangle_fill (engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
extern void nm2200_rectangle_invert (engine_token *et, fill_rect_params *list, uint32 count);
extern void nm2200_span_fill (engine_token *et, uint32 color, uint16 *list, uint32 count);

/* nm2097.c */
extern void nm2097_blit (engine_token *et, register blit_params *list, register uint32 count);
extern void nm2097_rectangle_fill (engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
extern void nm2097_rectangle_invert (engine_token *et, fill_rect_params *list, uint32 count);
extern void nm2097_span_fill (engine_token *et, uint32 color, uint16 *list, uint32 count);
