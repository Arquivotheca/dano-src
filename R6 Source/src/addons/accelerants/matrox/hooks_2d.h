//////////////////////////////////////////////////////////////////////////////
// 2D Acceleration: Function Prototypes
//////////////////////////////////////////////////////////////////////////////


void ScreenToScreenBlit(engine_token *et,
                        blit_params *list,
                        uint32 count);

void ScreenToScreenTransBlit(engine_token *et,
						uint32 transparent_color,
                        blit_params *list,
                        uint32 count);

void RectangleFill(engine_token *et,
                   uint32 color,
                   fill_rect_params *list,
                   uint32 count);

void RectangleInvert(engine_token *et,
                     fill_rect_params *list,
                     uint32 count);

void SpanFill(engine_token *et,
              uint32 color,
              uint16 *list,
              uint32 count);


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
