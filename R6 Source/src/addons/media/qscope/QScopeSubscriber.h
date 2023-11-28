#ifndef QSCOPESUBSCRIBER_H
#define QSCOPESUBSCRIBER_H

#include "QScope_Parameters.h"

class BLooper;

// QScope audio stream subscriber
class QScopeSubscriber 
{
public:
			QScopeSubscriber(BLooper *looper);
	virtual	~QScopeSubscriber();

	void SetTimePerDiv(float time);
	void SetTriggerMode(int mode);
	void SetHoldOff(float time);

	void ProcessBuffer(int16 *buf, size_t count);

	bool TriggerRightChannel;
	bool TriggerSlopeNeg;
	int TriggerLevel;

private:

	BLooper *the_looper;

	int16 scope_buf[2][SCOPE_WIDTH*4];	// Two buffers containing min/max values for left/right channels
	int active_buf;			// For double buffering

	int state;				// Current state (STATE_...)

	int scope_counter;				// Number of samples accumulated in scope_buf
	int record_counter;				// Current sample frame index in input buffer
	float time_per_div;				// Time per division
	float next_frame;				// Next sample frame in input buffer
	float frame_add;				// Added to next_frame for each scope_buf sample
	int16 old_input;				// Previous input for trigger slope detection
	int16 left_min, left_max;		// Current minimum/maximum sample elongation
	int16 right_min, right_max;		// Current minimum/maximum sample elongation
	int16 left_peak, right_peak;	// Peak levels found during recording

	float hold_off;				// Hold-off time in multiples of the time/div time
	int hold_off_frames;		// Number of sample frames to hold off
	int hold_off_counter;		// Counter for remaining number of sample frames to wait

	int trigger_start_frame;	// First sample frame index for trigger
	int trigger_total_frames;	// Total number of frames waited for trigger
	int trigger_mode;			// Trigger mode (TRIGGER_...)
};

#endif

