//biquad filter

#include <OS.h>
#include <math.h>

	enum{
		LOW_PASS,
		HIGH_PASS,
		BAND_PASS,
		NOTCH,
		PEAKING,
		LOW_SHELF,
		HIGH_SHELF				
	};

class Biquad
{
public:

	Biquad();

	void SetMode(int32 m)
	{
		mode = m;
		changed = true;
	};
	void SetSampleRate(float s)		// in Hz
	{
		sample_rate = s;
		changed = true;
	};
	void SetFrequency(float f)		// in Hz
	{
		frequency = f;
		changed = true;
	};
	void SetBandwidth(float b)		// in octave... (0.3 to 3)
	{
		bandwidth = b;
		changed = true;
	};
	void SetGain(float g)			// db
	{
		gain = g;
		changed = true;
	};
	
	void Process(float *buf, size_t size)
	{
		ReCalc();			
		float temp;
		for(int i=0; i<size; i++){
			*buf += (z1 * -b1) + (z2 * -b2);
			temp = (*buf * a0) + (z1 * a1) + (z2 * a2);
			z2 = z1;
			z1 = *buf;
			*buf = temp;
			buf++;
		}
	};

private:
	void ReCalc();

	int32 mode;
	
	//parameters
	float sample_rate;	//in Hz
	float frequency;	//in Hz
	float bandwidth;	//in octaves at -3dB
	float gain;			//in dB
	
	//intermediate
	float A;			//10^(dBgain/40)
	float omega;		//2*PI*frequency/sample_rate
	float sn;			//sin(omega);
	float cs;			//cos(omega);
	float alpha;		//sn*sinh[ ln(2)/2 * bandwidth * omega/sn ]
	float beta;			//sqrt[ (A^2 + 1)/S - (A-1)^2 ]

	//coefficients
	float a0;
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;
	
	//delay samples
	float z1;
	float z2;
	
	//other
	bool changed;
	
};

Biquad::Biquad()
{
	sample_rate = 44100;
	frequency = 1000;
	bandwidth = 1;
	gain = 1;
	changed = true;
	ReCalc();
}

void
Biquad::ReCalc()
{
	if(changed == false){
		return;	
	}
	omega = 2*M_PI*frequency/sample_rate;
	cs = cos(omega);
	sn = sin(omega);
	alpha = sn*sinh( log(2)/2 * bandwidth * omega/sn );
	beta = (A*A + 1.0)/(32.0) - (A-1.0)*(A-1.0);
	beta = sqrt(beta);

	A = pow(10.0, (gain/40.0));		

	switch(mode){
		case LOW_PASS:		
			//H(s) = 1 / (s^2 + s/Q + 1)
			b0 =  (1 - cs)/2;
			b1 =   1 - cs;
			b2 =  (1 - cs)/2;
			a0 =   1 + alpha;
			a1 =  -2*cs;
			a2 =   1 - alpha;
			break;
		case HIGH_PASS:
			//H(s) = s^2 / (s^2 + s/Q + 1)
			b0 =  (1 + cs)/2;
			b1 = -(1 + cs);
			b2 =  (1 + cs)/2;
			a0 =   1 + alpha;
			a1 =  -2*cs;
			a2 =   1 - alpha;
			break;
		case BAND_PASS:
			//H(s) = (s/Q) / (s^2 + s/Q + 1)
			b0 =   alpha;
			b1 =   0;
			b2 =  -alpha;
			a0 =   1 + alpha;
			a1 =  -2*cs;
			a2 =   1 - alpha;
			break;
		case NOTCH:
		 	//H(s) = (s^2 + 1) / (s^2 + s/Q + 1)
			b0 =   1;
			b1 =  -2*cs;
			b2 =   1;
			a0 =   1 + alpha;
			a1 =  -2*cs;
			a2 =   1 - alpha;
			break;
		case PEAKING:
			//H(s) = (s^2 + s*(A/Q) + 1) / (s^2 + s/(A*Q) + 1)
			b0 =   1 + alpha*A;
			b1 =  -2*cs;
			b2 =   1 - alpha*A;
			a0 =   1 + alpha/A;
			a1 =  -2*cs;
			a2 =   1 - alpha/A;
			break;
		case LOW_SHELF:
		 	//H(s) = A * (A + beta*s + s^2) / (1 + beta*s + A*s^2)
			b0 =    A*( (A+1) - (A-1)*cs + beta*sn );
			b1 =  2*A*( (A-1) - (A+1)*cs );
			b2 =    A*( (A+1) - (A-1)*cs - beta*sn );
			a0 =        (A+1) + (A-1)*cs + beta*sn;
			a1 =   -2*( (A-1) + (A+1)*cs );
			a2 =        (A+1) + (A-1)*cs - beta*sn;
			break;
		case HIGH_SHELF:
			//H(s) = A * (1 + beta*s + A*s^2) / (A + beta*s + s^2)
			b0 =    A*( (A+1) + (A-1)*cs + beta*sn );
			b1 = -2*A*( (A-1) + (A+1)*cs );
			b2 =    A*( (A+1) + (A-1)*cs - beta*sn );
			a0 =        (A+1) - (A-1)*cs + beta*sn;
			a1 =    2*( (A-1) - (A+1)*cs );
			a2 =        (A+1) - (A-1)*cs - beta*sn;
			break;
		default:
			break;
	}
	changed = false;
}


