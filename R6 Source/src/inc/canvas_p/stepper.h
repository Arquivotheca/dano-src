#ifndef STATE_H
#define STATE_H

#include <SupportDefs.h>

//======================================================
//	Class Interface: BMStateChanger
//
// A StateChanger is an object that knows how to maintain
// state information, and transition from state to state.
// It's a very simple concept and can be used to implement
// multi-state buttons or anything else requiring state.
// The benefit of having such a simple object is that
// you can sub-class it anc create various state transitions
// and hand them around.  Passing a stateChanger around to 
// other objects allows them to inherit similar behavior
// through similar state transitions.
//======================================================
class BMStepFunction
{
public:
				BMStepFunction(const int32 initialValue);
				
	virtual void	Reset();
	virtual void	SetStep(const int32);
	virtual int32	NextStep();
	virtual int32	NextStep(int32);
	virtual int32	CurrentStep() const {return fCurrentStep;};
	
protected:
	virtual void	StepChanged(const int32);
	
	int32	fInitialStep;
	int32	fCurrentStep;
private:
};

//======================================================
// Class Interface: BMRangeStepper
//
// This is a simple linear progression stepper.  It will
// go from the low to the high and back to the low again.
//======================================================
class BMRangeStepper : public BMStepFunction
{
public:
				BMRangeStepper(const int32 initial, const int32 terminating);
				
	virtual void	SetStep(const int32);
	virtual int32	NextStep();
	virtual int32	NextStep(const int32);
	
protected:
	int32	fLow;
	int32	fHigh;
private:
};

#endif
