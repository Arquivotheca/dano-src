#include "QScopeSubscriber.h"

#include <Looper.h>
#include <Message.h>

/*
 *  Subscriber constructor
 */

QScopeSubscriber::QScopeSubscriber(BLooper *looper) 
	: the_looper(looper)
{
	TriggerRightChannel = false;
	TriggerSlopeNeg = false;
	TriggerLevel = 0;
	SetTimePerDiv(2E-3);

	state = STATE_RECORD;

	active_buf = 0;
	scope_counter = 0;
	record_counter = 0;
	next_frame = 0.0;
	old_input = 0;
	left_min = right_min = 32767;
	left_max = right_max = left_peak = right_peak = -32768;

	hold_off_counter = 0;
	SetHoldOff(0);

	trigger_start_frame = 0;
	trigger_total_frames = 0;
	SetTriggerMode(TRIGGER_LEVEL);
}


/*
 *  Subscriber destructor
 */

QScopeSubscriber::~QScopeSubscriber()
{
}


/*
 *  Set time per division
 */

void QScopeSubscriber::SetTimePerDiv(float time)
{
	time_per_div = time;
	frame_add = time * SAMPLE_RATE * float(NUM_X_DIVS) / float(SCOPE_WIDTH);
	SetHoldOff(hold_off);
}


/*
 *  Set hold-off time
 */

void QScopeSubscriber::SetHoldOff(float hold)
{
	hold_off = hold;
	hold_off_frames = hold * time_per_div * SAMPLE_RATE;
}


/*
 *  Set trigger mode
 */

void QScopeSubscriber::SetTriggerMode(int mode)
{
	trigger_mode = mode;
}


/*
 *  Stream function
 */

void QScopeSubscriber::ProcessBuffer(int16 *buf, size_t count)
{
	// Number of sample frames in input buffer
	count >>= 2;

	// Act according to current state
	switch (state) {
		case STATE_HOLD_OFF:	// Wait before next trigger
hold_off:
			if (hold_off_counter >= count) {

				// Still waiting
				hold_off_counter -= count;

			} else {

				// Time elapsed, now search for trigger level (or start recording if not triggered)
				if (trigger_mode != TRIGGER_OFF) {
					state = STATE_WAIT_FOR_TRIGGER;
					trigger_start_frame = hold_off_counter;
					trigger_total_frames = 0;
					goto wait_for_trigger;
				} else {
					state = STATE_RECORD;
					next_frame = float(hold_off_counter);
					goto record;
				}
			}
			break;

		case STATE_WAIT_FOR_TRIGGER: {	// Search for trigger level
wait_for_trigger:
			int i;
			int16 input;
			if (trigger_mode == TRIGGER_PEAK) {
				if (TriggerRightChannel) {
					int16 compare = right_peak - 256;
					for (i=trigger_start_frame; i<count; i++) {
						input = buf[(i << 1) + 1];
						if (input >= compare)
							goto trigger_found;
					}
				} else {
					int16 compare = left_peak - 256;
					for (i=trigger_start_frame; i<count; i++) {
						input = buf[i << 1];
						if (input >= compare)
							goto trigger_found;
						old_input = input;
					}
				}
			} else {
				bool trigger_right = TriggerRightChannel;
				int trigger_level = TriggerLevel;
				if (TriggerSlopeNeg) {
					for (i=trigger_start_frame; i<count; i++) {
						input = buf[(i << 1) + trigger_right];
						if (input < trigger_level && old_input > trigger_level)
							goto trigger_found;
						old_input = input;
					}
				} else {
					for (i=trigger_start_frame; i<count; i++) {
						input = buf[(i << 1) + trigger_right];
						if (input > trigger_level && old_input < trigger_level)
							goto trigger_found;
						old_input = input;
					}
				}
			}
			trigger_start_frame = 0;

			// Trigger anyway if we have waited more than 1/30s
			trigger_total_frames += count;
			if (trigger_total_frames > SAMPLE_RATE / 30)
				goto trigger_found;
			break;

trigger_found:
			state = STATE_RECORD;
			record_counter = i;
			next_frame = float(i) + frame_add;
			left_min = left_max = buf[i << 1];
			right_min = right_max = buf[(i << 1) + 1];
			left_peak = right_peak = -32768;
			old_input = input;
			goto record;
		}

		case STATE_RECORD: {	// Get samples and stuff them into scope_buf
record:
			int next = int(next_frame);
			bool reaches_next_frame = true;
			if (next > count) {
				next = count;
				reaches_next_frame = false;
			}

			// Search minimum and maximum
			for (int i=record_counter; i<next; i++) {
				int16 left = buf[i << 1];
				int16 right = buf[(i << 1) + 1];
				if (left < left_min)
					left_min = left;
				if (left > left_max) {
					left_max = left;
					if (left > left_peak)
						left_peak = left;
				}
				if (right < right_min)
					right_min = right;
				if (right > right_max) {
					right_max = right;
					if (right > right_peak)
						right_peak = right;
				}
			}

			if (reaches_next_frame) {

				// Record one sample
				scope_buf[active_buf][scope_counter] = left_max;
				scope_buf[active_buf][scope_counter + SCOPE_WIDTH * 2] = right_max;
				scope_counter++;
				scope_buf[active_buf][scope_counter] = left_min;
				scope_buf[active_buf][scope_counter + SCOPE_WIDTH * 2] = right_min;
				scope_counter++;

				// scope_buf full? Then send message to looper
				if (scope_counter == SCOPE_WIDTH * 2) {
					scope_counter = 0;

					BMessage msg(MSG_NEW_BUFFER);
					msg.AddPointer("buffer", scope_buf[active_buf]);
					the_looper->PostMessage(&msg);
					active_buf = !active_buf;

					state = STATE_HOLD_OFF;
					hold_off_counter = hold_off_frames + int(next_frame);
					goto hold_off;
				}

				// Advance to next frame
				record_counter = next;
				next_frame += frame_add;
				if (record_counter < count) {
					left_min = left_max = buf[record_counter << 1];
					right_min = right_max = buf[(record_counter << 1) + 1];
					goto record;
				} else {

					// Input buffer used up
					left_min = left_max = buf[(count-1) << 1];
					right_min = right_max = buf[((count-1) << 1) + 1];
					record_counter = 0;
					next_frame -= count;
				}

			} else {

				// Input buffer used up
				record_counter = 0;
				next_frame -= count;
			}
			break;
		}
	}
}
