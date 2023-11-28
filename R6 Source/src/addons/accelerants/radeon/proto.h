#ifndef __PROTO_H__
#define __PROTO_H__

#include <OS.h>
#include <graphics_p/radeon/CardState.h>

#define dprintf(x) _kdprintf_ x

status_t __radeon_DeviceInfo( accelerant_device_info *adi );
extern status_t __radeon_Init( int32 the_fd );
extern void __radeon_Uninit();
extern status_t Radeon_init_clone (void *data);


extern void Radeon_InitEngine (CardInfo *ci);
extern void Radeon_WaitForIdle (CardInfo *ci);
extern void Radeon_WaitForFifo (CardInfo *ci, uint32 entries);
extern void Radeon_ResetEngine (CardInfo *ci);
extern void Radeon_FlushPixelCache (CardInfo *ci);

extern void Radeon_screen_to_screen_blits(engine_token *et, blit_params *list, uint32 count);
extern void Radeon_screen_to_screen_blit( CardInfo *ci, blit_params *bp );
extern void Radeon_fill_rectangle(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
extern void Radeon_fill_rect( CardInfo *ci, uint32 color, fill_rect_params *frp );

extern status_t Radeon_Init3DEngine( char *device_path, void *context );
extern status_t Radeon_Shutdown3DEngine( char *device_path, void *context );
extern void Radeon_get_device_info( char *device_path, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum );
extern void Radeon_get_3d_modes( char *device_path, void (* callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum );

extern int32 Radeon_CPInit (CardInfo * ci, int32 index);
extern void Radeon_CPEnd (CardInfo *ci, int32 waitmode);
extern void Radeon_Flush ( CardInfo *ci );
extern void Radeon_CPInitClone (CardInfo * ci);

extern void Radeon_WriteRegFifo( CardInfo *ci, int32 reg, uint32 data );

extern void Radeon_Fence();


#endif
