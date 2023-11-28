#ifndef FFTSHIFTER_H 
#define FFTSHIFTER_H 


class FFTSHIFTER 
{ 
public: 
        		FFTSHIFTER(); 
        		~FFTSHIFTER(); 
        void 	SetSize(int size); 
        void 	Reset();
        void 	Add(float x); 
        void 	SetParameters( int new_mode ); 
        void 	Perform_FFT(void); 
        short	Get_Result(void); 
        short	Get_Result(long i); 

private: 
		unsigned ReverseBits ( unsigned index, unsigned NumBits ); 
		unsigned NumberOfBitsNeeded ( unsigned PowerOfTwo ); 

        int 	mode; 
        int 	input_pointer; 
        int 	total_points; 
        int 	logPoints, sqrtPoints; 
        int 	output_pointer; 
        int 	sampling_rate; 


        float 	*real;
        float 	*im;
        float 	*real1;
        float 	*im1;
}; 
#endif
