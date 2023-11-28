#include <View.h>

class TAnalogClock : public BView {
public:
						TAnalogClock(BPoint loc, const char *name, int32 secondsRadius,
							int32 minutesRadius, int32 hoursRadius, int32 offset,
							bool showSeconds);
						
						~TAnalogClock();
						
virtual		void 		AttachedToWindow();
virtual		void 		Draw(BRect);
virtual		void		MouseDown(BPoint);
			
			// short		PointOnLine(int32, int32, int32, int32, int32, int32);
			void		SetTime(int32, int32, int32);
private:

			enum {
				kNone,
				kHour,
				kMinute,
				kSecond
			};
			
			// determine which hand is being clicked based on a point
			int32		DraggedHand(BPoint) const;
			
			// calculate the end point of a hand
			BPoint 		ClockHandEndPoint(short hours, short minutes,
							short seconds, int32 hand, BPoint offset) const;

			// point to time conversion funcitons; point is around a <0, 0> origin
			short		PointToMinute(BPoint) const;
			short		PointToHour(BPoint) const;

			// primitives used by above calls
			BPoint		SixtiethsToPoint(int32 sixtieths, int32 radius) const;
						// given a radius, convert a number in the range
						// 0-59 to a point around the <0, 0> origin

			short		PointToSixtieth(BPoint) const;
						// convert a point to a sixtieths, point may be infintely
						// far from origin


			// mouse tracking callbacks
			void 		MouseTrack(BPoint, uint32);
			void 		DoneTracking(BPoint);

			bool		fShowSeconds;

			short		fMinutesRadius;
			short		fHoursRadius;
			short		fSecondsRadius;
			
			short		fOffset;
				
			short		fHours;
			short		fMinutes;
			short		fSeconds;

			int32		fTrackingHand;

			BView		*fOffscreenView;
			BBitmap		*fOffscreenBits;
			BBitmap		*fClockBits;
};
