/*  thunk.c  */
uint32 _get_accelerant_mode_count (void);
status_t _get_mode_list (display_mode *dm);
status_t _propose_display_mode (display_mode *target,
				display_mode *low,
				display_mode *high);
status_t _set_display_mode (display_mode *mode_to_set);
status_t _get_display_mode (display_mode *current_mode);
status_t _get_frame_buffer_config (frame_buffer_config *a_frame_buffer);
status_t _get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high);
status_t _move_display_area (uint16 h_display_start, uint16 v_display_start);
void _set_indexed_colors (uint count,
			  uint8 first,
			  uint8 *color_data,
			  uint32 flags);
extern status_t _set_dpms_mode(uint32 dpms_flags);
extern uint32 _dpms_capabilities(void);
extern uint32 _dpms_mode(void);
extern status_t FreeAuxBuffers();
extern status_t AllocateAuxBuffers();

/* tvout.cpp */
#if __cplusplus
extern "C" {
#endif
extern void enable_tvout(int state);
extern void construct_i2c(int fd);
extern void destroy_i2c(void);
#if __cplusplus
};
#endif

/*  setup.c  */
int SetupGraphicsEngine (register struct thdfx_card_info *ci,
			 int screenWidth,
			 int screenHeight,
			 int bitspp);
void CheckForErrors (register struct thdfx_card_info *ci,
		     char *fileName,
		     int lineNo);

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
extern void rectfill_gen (engine_token *et,
			  register uint32 color,
			  register fill_rect_params *list,
			  register uint32 count);
extern void rectangle_invert (engine_token *et,
			  register fill_rect_params *list,
			  register uint32 count);
extern void blit (engine_token *et,
		  register blit_params *list,
		  register uint32 count);
extern void span_fill(engine_token *et,
              uint32 color,
              uint16 *list,
              uint32 count);
extern status_t Init3DEngine( char *device_path, void *context );
extern status_t Shutdown3DEngine( char *device_path, void *context );
extern void get_device_info( char *device_path, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum );
extern void get_3d_modes( char *device_path, void (* callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum );

/* overlay.c */
extern uint32 OverlayCount(const display_mode *dm);
extern const uint32 *OverlaySupportedSpaces(const display_mode *dm);
extern uint32 OverlaySupportedFeatures(uint32 a_color_space);
extern const overlay_buffer *AllocateOverlayBuffer(color_space cs, uint16 width, uint16 height);
extern status_t ReleaseOverlayBuffer(const overlay_buffer *ob);
extern status_t GetOverlayConstraints(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc);
extern overlay_token AllocateOverlay(void);
extern status_t ReleaseOverlay(overlay_token ot);
extern status_t ConfigureOverlay(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov);

/* fifo.c */
extern status_t InitFifo(int32 fifo_number);
extern uint32	GetFifoHwReadPtr(int32 fifo_number);
extern void FifoMakeSpace(uint32 fifo_number, uint32 slots);
extern void FifoAllocateSlots(int fifo_number, int slots);

/*  video.c  */
extern status_t thdfx_init ();
status_t propose_video_mode (display_mode		*target,
		const display_mode	*low,
		const display_mode	*high);

extern uint32 colorspacebits (uint32 cs);

extern void setpaletteentry (register uint32	regBase,
		int32			idx,
		uint8			r,
		uint8			g,
		uint8			b);


extern status_t SetupCRTC (register display_mode		*dm);

extern status_t vid_selectmode (register display_mode		*dm,uint32				lockflags);
extern status_t movedisplay (uint16 x,uint16 y);

/* cursor.c */
extern status_t _set_cursor_shape (uint16 curswide,
			    uint16 curshigh,
			    uint16 hot_x,
			    uint16 hot_y,
			    uint8 *andbits,
			    uint8 *xorbits);
extern void _move_cursor (int16 x, int16 y);
extern void _show_cursor (bool on);

extern void _V3_SetGamma( float gammaR, float gammaG, float gammaB);
