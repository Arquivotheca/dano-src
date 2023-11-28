#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#define DIR_LEFT 0
#define DIR_RIGHT 1

class Controller : public BMidi {
    BMidiPort port;
    uint dial;
	uint _channel;
public:
    Controller(char *device);
    ~Controller();
	virtual void SystemExclusive(void *data, size_t dataLength, uint32 time);
    virtual void ControlChange(uchar channel,
        uchar number, uchar value, uint32 time);
	virtual void NoteOn(uchar channel, uchar note, 
         uchar velocity, uint32 time);
	virtual void NoteOff(uchar channel, uchar note, 
         uchar velocity, uint32 time);

	virtual void ButtonChanged(uint button, uint on) = 0;
    virtual void SliderChanged(uint slider, double value) = 0;
    virtual void DialChanged(uint direction) = 0;
};

#endif
