
#define BLooper BAppSessionDrawable
#include <Handler.h>
#undef BLooper
#define BWindow BAppSessionDrawable
#include <Looper.h>
#undef BWindow
#define BTabView BAppSessionDrawable
#include <View.h>
#undef BTabView
#define BDirectWindow BAppSessionDrawable
#include <Window.h>
#undef BDirectWindow

#include "BAppSessionDrawable.h"

#include <interface/Bitmap.h>
#include <interface/Picture.h>
#include <interface/Shape.h>
#include <support/StreamIO.h>

#include <app_server_p/messages.h>
#include <app_server_p/shared_fonts.h>
#include <app_server_p/shared_defaults.h>
#include <app_p/token.h>

BAppSessionDrawable::BAppSessionDrawable(	GehmlSession* session,
											int32 viewToken,
											BView *hackView)
	: fSession(NULL), fToken(NO_TOKEN),
	  fAutoPushState(false)
{
	SetTo(session, viewToken, false);
	fHack = hackView;
	fHack->fState = &fState;
}

BAppSessionDrawable::~BAppSessionDrawable()
{
}

void BAppSessionDrawable::SetTo(	GehmlSession* session,
									int32 viewToken,
									bool autoPushState)
{
	if (fSession) debugger("BAppSessionDrawable already set");
	fSession = session;
	fAutoPushState = autoPushState;
	SetViewToken(viewToken);
}

void BAppSessionDrawable::SetViewToken(int32 viewToken)
{
	if (fToken != NO_TOKEN) {
		if (!fAutoPushState) debugger("Can't change view of non-autoPushState drawable");
		PopState();
	}
	fToken = viewToken;
	
	if (fToken != NO_TOKEN) {
		fState.set_default_values();
		if (fAutoPushState) PushState();
	}
}

int32 BAppSessionDrawable::ViewToken() const
{
	return fToken;
}

BView *
BAppSessionDrawable::HackView()
{
	return fHack;
}

status_t BAppSessionDrawable::InitCheck() const
{
	return (fSession != NULL ? B_OK : B_NO_INIT);
}

void BAppSessionDrawable::Flush() const
{
	fSession->flush();
}

void BAppSessionDrawable::Sync() const
{
	fSession->flush();
}

void BAppSessionDrawable::PushState()
{
	if (fState.f_nonDefault != 0) {
		write_font();
	}
	fSession->swrite_l(GR_PUSH_STATE);
	fState.valid_flags |= B_ORIGIN_VALID;
	fState.origin = B_DEFAULT_ORIGIN;
}

void BAppSessionDrawable::PopState()
{
	fSession->swrite_l(GR_POP_STATE);
	fState.valid_flags = 0;
	fState.f_mask = 0;
	fState.f_nonDefault = 0;
}

void BAppSessionDrawable::MoveOrigin(BPoint delta)
{
	if (!(fState.valid_flags & B_ORIGIN_VALID)) {
		fSession->swrite_l(GR_GET_ORIGIN);
		fSession->flush();
		fSession->sread(sizeof(BPoint),&fState.origin);
		fState.valid_flags |= B_ORIGIN_VALID;
	}
	
	fState.origin += delta;
	fSession->swrite_l(GR_SET_ORIGIN);
	fSession->swrite(sizeof(BPoint),&fState.origin);
}

void BAppSessionDrawable::ScaleBy(float xAmount, float /*yAmount*/)
{
	// TO DO: Need to deal with scale changing when state is popped.
	fSession->swrite_l(GR_SET_SCALE);
	fSession->swrite_coo_a(&xAmount);
}

void BAppSessionDrawable::ClipToPicture(	BPicture *picture,
											BPoint where,
											bool sync)
{
	(void)picture;
	(void)where;
	(void)sync;
}

void BAppSessionDrawable::ClipToInversePicture(	BPicture *picture,
												BPoint where,
												bool sync)
{
	(void)picture;
	(void)where;
	(void)sync;
}

void BAppSessionDrawable::SetDrawingMode(drawing_mode mode)
{
	if ((fState.valid_flags & B_DRAW_MODE_VALID) &&
		(fState.draw_mode == mode))
		return;

	fState.draw_mode = mode;
	fState.valid_flags |= B_DRAW_MODE_VALID;
		
	fSession->swrite_l(GR_SET_DRAW_MODE);
	short tmpMode = mode;
	fSession->swrite(2, &tmpMode);
}

drawing_mode BAppSessionDrawable::DrawingMode() const
{
	if (!(fState.valid_flags & B_DRAW_MODE_VALID)) {
		fSession->swrite_l(GR_GET_DRAW_MODE);
		fSession->flush();
		short tmpMode;
		fSession->sread(2, &tmpMode);
		fState.draw_mode = (drawing_mode)tmpMode;
		fState.valid_flags |= B_DRAW_MODE_VALID;
	}
	return fState.draw_mode;
}

void BAppSessionDrawable::SetBlendingMode(	source_alpha srcAlpha,
											alpha_function alphaFunc)
{
	if ((fState.valid_flags & B_BLENDING_MODE_VALID) &&
		(fState.srcAlpha == srcAlpha) &&
		(fState.alphaFunc == alphaFunc))
		return;

	fState.srcAlpha = srcAlpha;
	fState.alphaFunc = alphaFunc;
	fState.valid_flags |= B_BLENDING_MODE_VALID;
		
	fSession->swrite_l(GR_SET_BLENDING_MODE);
	short tmpMode = srcAlpha;
	fSession->swrite(2, &tmpMode);
	tmpMode = alphaFunc;
	fSession->swrite(2, &tmpMode);
}

void BAppSessionDrawable::GetBlendingMode(	source_alpha *srcAlpha,
											alpha_function *alphaFunc) const
{
	if (!(fState.valid_flags & B_BLENDING_MODE_VALID)) {
		fSession->swrite_l(GR_GET_BLENDING_MODE);
		fSession->flush();
		short tmpMode;
		fSession->sread(2, &tmpMode);
		fState.srcAlpha = (source_alpha)tmpMode;
		fSession->sread(2, &tmpMode);
		fState.alphaFunc = (alpha_function)tmpMode;
		fState.valid_flags |= B_BLENDING_MODE_VALID;
	}
	
	*srcAlpha = fState.srcAlpha;
	*alphaFunc = fState.alphaFunc;
}

void BAppSessionDrawable::SetHighColor(rgb_color a_color)
{
	if ((fState.valid_flags & B_HIGH_COLOR_VALID) &&
		((uint32*)&fState.high_color == (uint32*)&a_color))
		return;

	fState.high_color = a_color;
	fState.valid_flags |= B_HIGH_COLOR_VALID;
		
	fSession->swrite_l(GR_FORE_COLOR);
	fSession->swrite(sizeof(rgb_color), &a_color);
}

rgb_color BAppSessionDrawable::HighColor() const
{
	if (!(fState.valid_flags & B_HIGH_COLOR_VALID)) {
		fSession->swrite_l(GR_GET_FORE_COLOR);
		fSession->flush();
		fSession->sread(sizeof(rgb_color), &fState.high_color);
		fState.valid_flags |= B_HIGH_COLOR_VALID;
	}
	return fState.high_color;
}

void BAppSessionDrawable::SetLowColor(rgb_color a_color)
{
	if ((fState.valid_flags & B_LOW_COLOR_VALID) &&
		((uint32*)&fState.low_color == (uint32*)&a_color))
		return;

	fState.low_color = a_color;
	fState.valid_flags |= B_LOW_COLOR_VALID;
		
	fSession->swrite_l(GR_BACK_COLOR);
	fSession->swrite(sizeof(rgb_color), &a_color);
}

rgb_color BAppSessionDrawable::LowColor() const
{
	if (!(fState.valid_flags & B_LOW_COLOR_VALID)) {
		fSession->swrite_l(GR_GET_BACK_COLOR);
		fSession->flush();
		fSession->sread(sizeof(rgb_color), &fState.low_color);
		fState.valid_flags |= B_LOW_COLOR_VALID;
	}
	return fState.low_color;
}

void BAppSessionDrawable::SetFont(const BFont *font, uint32 mask)
{
	if (!mask) return;

	fState.font.SetTo(*font, mask);
	fState.f_mask |= mask;
	fState.f_nonDefault |= mask;
}

void BAppSessionDrawable::GetFont(BFont *font) const
{
	if ((fState.f_mask & B_FONT_ALL) != B_FONT_ALL) {
		read_font();
	}

	*font = fState.font;
}

void BAppSessionDrawable::write_font()
{
/*
	fSession->swrite_l(GR_SET_FONT_CONTEXT);
	IKAccess::WriteFont(&fState.font, fSession);
	fSession->swrite_l(fState.f_nonDefault);
	
	fState.f_nonDefault = 0;
*/
}

void BAppSessionDrawable::read_font() const
{
/*
	fSession->swrite_l(GR_GET_FONT_CONTEXT);
	fSession->flush();
	IKAccess::ReadFont(&fState.font, fSession);

	fState.f_mask = B_FONT_ALL;
	fState.f_nonDefault = 0;
*/
}

void BAppSessionDrawable::ForceFontAliasing(bool enable)
{
//	fSession->swrite_l(GR_SET_FONT_ALIASING);
//	fSession->swrite(1, &enable);
}

void BAppSessionDrawable::SetPenSize(float size)
{
	if ((fState.valid_flags & B_PEN_SIZE_VALID) &&
		(fState.pen_size == size))
		return;

	fState.pen_size = size;
	fState.valid_flags |= B_PEN_SIZE_VALID;
		
	fSession->swrite_l(GR_SET_PEN_SIZE);
	fSession->swrite_coo_a(&size);
}

float BAppSessionDrawable::PenSize() const
{
	if (!(fState.valid_flags & B_PEN_SIZE_VALID)) {
		fSession->swrite_l(GR_GET_PEN_SIZE);
		fSession->flush();
		fSession->sread_coo_a(&fState.pen_size);
		fState.valid_flags |= B_PEN_SIZE_VALID;
	}
	return fState.pen_size;
}

void BAppSessionDrawable::SetLineMode(	cap_mode lineCap,
										join_mode lineJoin,
										float miterLimit)
{
	if ((fState.valid_flags & B_LINE_MODE_VALID) && 
		(fState.line_cap == lineCap) &&
		(fState.line_join == lineJoin) &&
		(fState.miter_limit == miterLimit))
		return;
		
	fState.line_cap = lineCap;
	fState.line_join = lineJoin;
	fState.miter_limit = miterLimit;
	fState.valid_flags |= B_LINE_MODE_VALID;
	
	fSession->swrite_l(GR_SET_PEN_SIZE);
	short	modes[2];
	modes[0] = lineCap;
	modes[1] = lineJoin;
	fSession->swrite_l(GR_SET_LINE_MODE);
	fSession->swrite(sizeof(short)*2, modes);
	fSession->swrite(sizeof(float), &miterLimit);
}

void BAppSessionDrawable::GetLineMode(	cap_mode* lineCap,
										join_mode* lineJoin,
										float* miterLimit) const
{
	if (!(fState.valid_flags & B_LINE_MODE_VALID)) {
		fSession->swrite_l(GR_GET_JOIN_MODE);
		fSession->swrite_l(GR_GET_CAP_MODE);
		fSession->swrite_l(GR_GET_MITER_LIMIT);
		fSession->flush();
		short mode;
		fSession->sread(2,&mode);
		fState.line_join = (join_mode)mode;
		fSession->sread(2,&mode);
		fState.line_cap = (cap_mode)mode;
		fSession->sread(4,&fState.miter_limit);
		fState.valid_flags |= B_LINE_MODE_VALID;
	}
	*lineCap = fState.line_cap;
	*lineJoin = fState.line_join;
	*miterLimit = fState.miter_limit;
}

void BAppSessionDrawable::MovePenTo(BPoint pt)
{
	if ((fState.valid_flags & B_PEN_LOCATION_VALID) &&
		(fState.pen_loc == pt))
		return;

	fState.pen_loc = pt;
	fState.valid_flags |= B_PEN_LOCATION_VALID;
		
	fSession->swrite_l(GR_MOVETO);
	fSession->swrite_point_a(&pt);
}

BPoint BAppSessionDrawable::PenLocation() const
{
	if (!(fState.valid_flags & B_PEN_LOCATION_VALID)) {
		fSession->swrite_l(GR_GET_PEN_LOC);
		fSession->flush();
		fSession->sread_point_a(&fState.pen_loc);
		fState.valid_flags |= B_PEN_SIZE_VALID;
	}
	return fState.pen_loc;
}

void BAppSessionDrawable::StrokeLineTo(BPoint toPt)
{
	fSession->swrite_l(GR_LINETO);
	fSession->swrite_point_a(&toPt);
	fState.pen_loc = toPt;
	fState.valid_flags |= B_PEN_LOCATION_VALID;
}

void BAppSessionDrawable::StrokePolygon(	const BPoint *ptArray,
											int32 numPts,
											bool  closed)
{
	if (numPts < 3 && closed) return;
	
	short tempClosed = closed;
	short tempNum = numPts < 32767 ? numPts : 32767;
	fSession->swrite_l(GR_POLYFRAME);
	fSession->swrite(2, &tempClosed);
	fSession->swrite(2, &tempNum);
	fSession->swrite(tempNum * sizeof(BPoint), (void *) ptArray);
}

void BAppSessionDrawable::FillPolygon(const BPoint *ptArray, int32 numPts)
{
	if (numPts < 3) return;
	
	short tempNum = numPts < 32767 ? numPts : 32767;
	fSession->swrite_l(GR_POLYFILL);
	fSession->swrite(2, &tempNum);
	fSession->swrite(tempNum * sizeof(BPoint), (void *) ptArray);
}

void BAppSessionDrawable::StrokeRect(BRect r)
{
	fSession->swrite_l(GR_RECTFRAME);
	fSession->swrite_rect_a(&r);
}

void BAppSessionDrawable::FillRect(BRect r)
{
	fSession->swrite_l(GR_RECTFILL);
	fSession->swrite_rect_a(&r);
}

void BAppSessionDrawable::InvertRect(BRect r)
{
	fSession->swrite_l(GR_RECT_INVERT);
	fSession->swrite_rect_a(&r);
}

void BAppSessionDrawable::StrokeRoundRect(BRect r, float xRadius, float yRadius)
{
	fSession->swrite_l(GR_ROUND_RECT_FRAME);
	fSession->swrite_rect_a(&r);
	fSession->swrite_coo_a(&xRadius);
	fSession->swrite_coo_a(&yRadius);
}

void BAppSessionDrawable::FillRoundRect(BRect r, float xRadius, float yRadius)
{
	fSession->swrite_l(GR_ROUND_RECT_FILL);
	fSession->swrite_rect_a(&r);
	fSession->swrite_coo_a(&xRadius);
	fSession->swrite_coo_a(&yRadius);
}

void BAppSessionDrawable::StrokeEllipse(BPoint center, float xRadius, float yRadius)
{
	fSession->swrite_l(GR_ELLIPSE_STROKE);
	fSession->swrite(8,&center);
	fSession->swrite(4,&xRadius);
	fSession->swrite(4,&yRadius);
}

void BAppSessionDrawable::FillEllipse(BPoint center, float xRadius, float yRadius)
{
	fSession->swrite_l(GR_ELLIPSE_FILL);
	fSession->swrite(8,&center);
	fSession->swrite(4,&xRadius);
	fSession->swrite(4,&yRadius);
}

void BAppSessionDrawable::StrokeArc(	BPoint center,
										float xRadius,
										float yRadius,
										float startAngle,
										float arcAngle)
{
	fSession->swrite_l(GR_ARC_STROKE);
	fSession->swrite(8,&center);
	fSession->swrite(4,&xRadius);
	fSession->swrite(4,&yRadius);
	fSession->swrite(4,&startAngle);
	fSession->swrite(4,&arcAngle);
}

void BAppSessionDrawable::FillArc(BPoint center,
								float xRadius,
								float yRadius,
								float startAngle,
								float arcAngle)
{
	fSession->swrite_l(GR_ARC_FILL);
	fSession->swrite(8,&center);
	fSession->swrite(4,&xRadius);
	fSession->swrite(4,&yRadius);
	fSession->swrite(4,&startAngle);
	fSession->swrite(4,&arcAngle);
}

void BAppSessionDrawable::StrokeBezier(BPoint* controlPoints)
{
	fSession->swrite_l(GR_DRAW_BEZIER);
	fSession->swrite(sizeof(BPoint)*4,controlPoints);
}

void BAppSessionDrawable::FillBezier(BPoint *controlPoints)
{
	fSession->swrite_l(GR_FILL_BEZIER);
	fSession->swrite(sizeof(BPoint)*4,controlPoints);
}

void BAppSessionDrawable::StrokeShape(BShape *shape)
{
	int32 opCount,ptCount;
	uint32 *opList;
	BPoint *ptList;
	IKAccess::GetShapeData(shape,&opCount,&ptCount,&opList,&ptList);
	uint32 buildingOp = IKAccess::ShapeBuildingOp(shape);
	
	if ((!opCount && !buildingOp) || !ptCount) return;
	
	fSession->swrite_l(GR_STROKE_SHAPE);
	fSession->swrite_l(opCount+1);
	fSession->swrite_l(ptCount);
	fSession->swrite(sizeof(uint32)*opCount,opList);
	if (buildingOp & 0x30000000)
		fSession->swrite_l(buildingOp);
	else
		fSession->swrite_l(buildingOp | 0x30000000);
	fSession->swrite(sizeof(BPoint)*ptCount,ptList);
}

void BAppSessionDrawable::FillShape(BShape *shape)
{
	int32 opCount,ptCount;
	uint32 *opList;
	BPoint *ptList;
	IKAccess::GetShapeData(shape,&opCount,&ptCount,&opList,&ptList);
	uint32 buildingOp = IKAccess::ShapeBuildingOp(shape);
	
	if ((!opCount && !buildingOp) || !ptCount) return;
	
	fSession->swrite_l(GR_FILL_SHAPE);
	fSession->swrite_l(opCount+1);
	fSession->swrite_l(ptCount);
	fSession->swrite(sizeof(uint32)*opCount,opList);
	if (buildingOp & 0x30000000)
		fSession->swrite_l(buildingOp);
	else
		fSession->swrite_l(buildingOp | 0x30000000);
	fSession->swrite(sizeof(BPoint)*ptCount,ptList);
}

void BAppSessionDrawable::DrawPicture(const BPicture *aPicture)
{
	DrawPictureAt(aPicture, PenLocation());
}

void BAppSessionDrawable::DrawString(const char *aString,
								   int32 length,
								   escapement_delta *delta)
{
	if (length < 0) length = strlen(aString);
	if (length == 0) return;
	if (length > 32766)
		length = 32766;
	
	uint				buffer[3];
	static const char	zero = 0;
	const short			real_length = (short)(length+1);
	
	buffer[0] = GR_DRAW_STRING;
	if (delta == NULL) {
		((float*)buffer)[1] = 0.0;
		((float*)buffer)[2] = 0.0;
	}
	else {
		((float*)buffer)[1] = delta->nonspace;
		((float*)buffer)[2] = delta->space;
	}
	
	if (fState.f_nonDefault != 0) {
		write_font();
	}
	fSession->swrite(12, buffer);
	fSession->swrite(2, (short int*)&real_length);
	fSession->swrite(length, (char*)aString);
	fSession->swrite(1, (char*)&zero);
	fState.valid_flags &= ~B_PEN_LOCATION_VALID;
}

void BAppSessionDrawable::DrawClippedScaledBitmap(	const BBitmap *aBitmap,
													BRect srcRect,
													BRect dstRect)
{
	fSession->swrite_l(GR_SCALE_BITMAP1_A);
	fSession->swrite_rect_a(&srcRect);
	fSession->swrite_rect_a(&dstRect);
	fSession->swrite_l(IKAccess::BitmapToken(aBitmap));
}

void BAppSessionDrawable::MovePenBy(BPoint pt)
{
	fSession->swrite_l(GR_MOVEBY);
	fSession->swrite_point_a(&pt);
	fState.valid_flags &= ~B_PEN_LOCATION_VALID;
}

void BAppSessionDrawable::StrokeLine(BPoint pt0, BPoint pt1)
{
	fSession->swrite_l(GR_LINE);
	fSession->swrite_point_a(&pt0);
	fSession->swrite_point_a(&pt1);
	fState.pen_loc = pt1;
	fState.valid_flags |= B_PEN_LOCATION_VALID;
}

void BAppSessionDrawable::BeginLineArray(int32 count)
{
	BDrawable::BeginLineArray(count);
}

void BAppSessionDrawable::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	BDrawable::AddLine(pt0, pt1, col);
}

void BAppSessionDrawable::EndLineArray()
{
	BDrawable::EndLineArray();
}

void BAppSessionDrawable::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3)
{
	BDrawable::StrokeTriangle(pt1, pt2, pt3);
}

void BAppSessionDrawable::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3)
{
	BDrawable::FillTriangle(pt1, pt2, pt3);
}

void BAppSessionDrawable::StrokeInscribedEllipse(BRect r)
{
	fSession->swrite_l(GR_ELLIPSE_INSCRIBE_STROKE);
	fSession->swrite_rect_a(&r);
}

void BAppSessionDrawable::FillInscribedEllipse(BRect r)
{
	fSession->swrite_l(GR_ELLIPSE_INSCRIBE_FILL);
	fSession->swrite_rect_a(&r);
}

void BAppSessionDrawable::StrokeInscribedArc(	BRect r,
												float startAngle,
												float arcAngle)
{
	fSession->swrite_l(GR_ARC_INSCRIBE_STROKE);
	fSession->swrite_rect_a(&r);
	fSession->swrite(4,&startAngle);
	fSession->swrite(4,&arcAngle);
}

void BAppSessionDrawable::FillInscribedArc(	BRect r,
											float startAngle,
											float arcAngle)
{
	fSession->swrite_l(GR_ARC_INSCRIBE_FILL);
	fSession->swrite_rect_a(&r);
	fSession->swrite(4,&startAngle);
	fSession->swrite(4,&arcAngle);
}

void BAppSessionDrawable::DrawBitmapAt(const BBitmap *aBitmap, BPoint where)
{
	fSession->swrite_l(GR_DRAW_BITMAP_A);
	fSession->swrite_point_a(&where);
	fSession->swrite_l(IKAccess::BitmapToken(aBitmap));
}

void BAppSessionDrawable::DrawBitmap(const BBitmap *aBitmap)
{
	fSession->swrite_l(GR_DRAW_BITMAP_A);
	BPoint where(PenLocation());
	fSession->swrite_point_a(&where);
	fSession->swrite_l(IKAccess::BitmapToken(aBitmap));
}

void BAppSessionDrawable::DrawScaledBitmap(const BBitmap *aBitmap, BRect dstRect)
{
	fSession->swrite_l(GR_SCALE_BITMAP_A);
	fSession->swrite_rect_a(&dstRect);
	fSession->swrite_l(IKAccess::BitmapToken(aBitmap));
}

void BAppSessionDrawable::SetFontSize(float size)
{
	fState.font.SetSize(size);
	fState.f_mask |= B_FONT_SIZE;
	fState.f_nonDefault |= B_FONT_SIZE;
}

void BAppSessionDrawable::GetFontHeight(font_height* height) const
{
	if ((fState.f_mask & B_FONT_ALL) != B_FONT_ALL) {
		read_font();
	}
	fState.font.GetHeight(height);
}

float BAppSessionDrawable::StringWidth(const char* str, int32 length) const
{
	if ((fState.f_mask & B_FONT_ALL) != B_FONT_ALL) {
		read_font();
	}
	if (length < 0) return fState.font.StringWidth(str);
	else return fState.font.StringWidth(str, length);
}

void BAppSessionDrawable::TruncateString(	BString* in_out,
											uint32 mode,
											float width) const
{
	if ((fState.f_mask & B_FONT_ALL) != B_FONT_ALL) {
		read_font();
	}
	fState.font.TruncateString(in_out, mode, width);
}

void BAppSessionDrawable::DrawPictureAt(const BPicture *aPicture, BPoint where)
{
	if (IKAccess::PictureToken(aPicture) <= 0) return;
	
	fSession->swrite_l(GR_PLAY_PICTURE);
	fSession->swrite_l(IKAccess::PictureToken(aPicture));
	fSession->swrite_point_a(&where);
	
	MovePenTo(where);
}
