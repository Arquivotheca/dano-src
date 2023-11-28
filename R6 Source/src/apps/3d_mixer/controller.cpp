#include <MidiText.h>
#include <MidiPort.h>
#include <MidiSynth.h>
#include <stdio.h>
#include <string.h>

#include "controller.h"

/* control #102->#117 = Slider #1->#16
** control #118 = Dial #1
*/
void Controller::ControlChange(uchar channel, uchar number, 
	uchar value, uint32 time)
{
	if(channel == _channel){
		double val = ((double) value) / 127.0;
		if(number == 118){
			if(dial == 0 && value == 0) {
				DialChanged(DIR_LEFT);
			} else if(dial == 127 && value == 127) {
				DialChanged(DIR_RIGHT);
			} else {
				DialChanged(value > dial ? DIR_RIGHT : DIR_LEFT);
			}
			dial = value;
			return;
		}
		if((number > 101) && (number < 118)){
			SliderChanged(number - 101, val);
			return;
		}
	}
}

void Controller::NoteOn(uchar channel, uchar note, uchar vel, uint32 time)
{
	if(channel == _channel) ButtonChanged(note+1, 1);
}

void Controller::NoteOff(uchar channel, uchar note, uchar vel, uint32 time)
{
	if(channel = _channel) ButtonChanged(note+1, 0);
}

void Controller::SystemExclusive(void *data, size_t len, uint32 time)
{
	int i;
	uchar *d = (uchar *) data;

	fprintf(stderr,"[ ");
	for(i=0;i<len;i++) fprintf(stderr,"%02x ",d[i]);
	fprintf(stderr,"]\n");

}


Controller::Controller(char *device)
{
	dial  = 0;
	_channel = 1;
	status_t err = port.Open(device);
	if(!err) {
		port.Start(); 
		port.Connect(this);
	}
}

Controller::~Controller()
{
	port.Stop();
}

#if TEST

#include <stdio.h>

class MyController : public Controller {
public:
	MyController(char *device) : Controller(device) {
	}
	virtual void ButtonChanged(uint button, uint on) {
		fprintf(stderr,"button[%02d] is %s\n",button,on?"On":"Off");
	}
	virtual void SliderChanged(uint slider, double value) {
		fprintf(stderr,"slider[%02d] = %6.4f\n",slider,value);
	}
    virtual void DialChanged(uint direction) {
		fprintf(stderr,"dial going %s\n",direction == DIR_LEFT ? "Left" : "Right");
	}
};

void main (int argc, char** argv)
{
	if(argc == 2) {
		MyController C(argv[1]);
		while(1) snooze(1000000);
	}
	return 0;
}
#endif
