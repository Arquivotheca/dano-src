#ifndef QSCOPE_PARAMETERS_H
#define QSCOPE_PARAMETERS_H

#include <SupportDefs.h>

const uint32 MSG_NEW_BUFFER = 'nbuf';

const uint32 CONTROL_STREAM = 1;
const uint32 MSG_DAC_STREAM = 'dacs';
const uint32 MSG_ADC_STREAM = 'adcs';

const uint32 CONTROL_CHANNEL = 2;
const uint32 MSG_LEFT_CHANNEL = 'left';
const uint32 MSG_RIGHT_CHANNEL = 'rght';
const uint32 MSG_STEREO_CHANNELS = 'dual';

const uint32 CONTROL_TIMEDIV = 3;
const uint32 MSG_TIME_DIV_100us = '100u';
const uint32 MSG_TIME_DIV_200us = '200u';
const uint32 MSG_TIME_DIV_500us = '500u';
const uint32 MSG_TIME_DIV_1ms = '1ms ';
const uint32 MSG_TIME_DIV_2ms = '2ms ';
const uint32 MSG_TIME_DIV_5ms = '5ms ';
const uint32 MSG_TIME_DIV_10ms = '10ms';

const uint32 CONTROL_TRIGGER = 4;
const uint32 MSG_TRIGGER_OFF = 'trof';
const uint32 MSG_TRIGGER_LEVEL = 'trlv';
const uint32 MSG_TRIGGER_PEAK= 'trpk';
const uint32 MSG_TRIGGER_LEFT = 'trlt';
const uint32 MSG_TRIGGER_RIGHT = 'trrt';

const uint32 CONTROL_SLOPE = 5;
const uint32 MSG_SLOPE_POS = 'slp+';
const uint32 MSG_SLOPE_NEG = 'slp-';

const uint32 CONTROL_ILLUMINATE = 6;
const uint32 MSG_ILLUMINATION = 'illu';


const int SCOPE_WIDTH = 320;	// Scope grid parameters
const int SCOPE_HEIGHT = 256;
const int NUM_X_DIVS = 10;
const int NUM_Y_DIVS = 8;
const int TICKS_PER_DIV = 5;

const float SAMPLE_RATE = 44100.0;

enum {	// Subscriber states
	STATE_HOLD_OFF,
	STATE_WAIT_FOR_TRIGGER,
	STATE_RECORD
};

enum {	// Trigger modes
	TRIGGER_OFF,
	TRIGGER_LEVEL,
	TRIGGER_PEAK
};

#endif
