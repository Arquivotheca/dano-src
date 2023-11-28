#ifdef __cplusplus
extern "C" {
#endif

void	fft(int n, float *real, float *imag);
void	ifft(int n, float *real,float *imag);

int 	fht();
int 	realfft();
int 	realifft();
int 	cas2d();
int 	fht2d();
int 	conv2d();

#ifdef __cplusplus
}
#endif
