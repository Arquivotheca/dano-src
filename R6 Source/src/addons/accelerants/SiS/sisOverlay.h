#ifndef SISOVERLAY_H
#define SISOVERLAY_H

#include <SupportDefs.h>

#include <video_overlay.h>

uint32 OVERLAY_COUNT(const display_mode *dm);
const uint32 *OVERLAY_SUPPORTED_SPACES(const display_mode *dm);
uint32 OVERLAY_SUPPORTED_FEATURES(uint32 a_color_space);
const overlay_buffer *ALLOCATE_OVERLAY_BUFFER(color_space cs, uint16 width, uint16 height);
status_t RELEASE_OVERLAY_BUFFER(const overlay_buffer *ob);
status_t GET_OVERLAY_CONSTRAINTS(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc);
overlay_token ALLOCATE_OVERLAY(void);
status_t RELEASE_OVERLAY(overlay_token ot);
status_t CONFIGURE_OVERLAY(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov);

#endif
