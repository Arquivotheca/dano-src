

#include <Application.h>
#include <Message.h>
#include <Window.h>
#include <View.h>
#include <math.h>
#include <string.h>
#include <resampler_trilinear.h>

typedef int16 INPUT;

#define RESAMPLER resampler_trilinear<INPUT, float>
#define INRATE 48000.0
#define OUTRATE 44100.0
// 1 or 2 channels only!
#define NUMCHANNELS 1
class mywin: public BWindow
{
	RESAMPLER *r;
	INPUT	inbuffer[250*NUMCHANNELS];
	float	outbuffer[700];
	BView *v;
	int64	totalsamples;
	int32	delay;

	public:
		mywin()
			: BWindow(BRect(100,100,400,400),"test",B_TITLED_WINDOW,0,0)
		{

			r = new RESAMPLER(	INRATE, OUTRATE, NUMCHANNELS);
			v=new BView(Bounds(),"",B_WILL_DRAW,B_FOLLOW_ALL);
			AddChild(v);

			INPUT *p=inbuffer;
			for(int i=0;i<sizeof(inbuffer)/(sizeof(INPUT)*NUMCHANNELS);i++)
			{
				INPUT sample = INPUT(sin(NUMCHANNELS*2*M_PI*i/(sizeof(inbuffer)/sizeof(INPUT)))  *32000.0);
				printf("%d: %f\n",i,sample);
				*p++ = sample;
#if NUMCHANNELS == 2
				*p++ = sample;
#endif
			}
			totalsamples = 0;
			delay = 0;
			Show();
			PostMessage('smpl');
		}

		virtual ~mywin()
		{
		}

		virtual bool QuitRequested()
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			return true;
		}

		virtual void MessageReceived(BMessage *mes)
		{
			switch(mes->what)
			{
				case 'smpl':
					for(int i=0;i<100;i++)
						resample();
					PostMessage('smpl');
					break;
					
				default:
					BWindow::MessageReceived(mes);
					break;
			}
		}

		void resample()
		{
			const float gain[2]={1.0,1.0};
			size_t insize = sizeof(inbuffer)/(sizeof(INPUT)*NUMCHANNELS);
			size_t outsize = sizeof(outbuffer)/(2*NUMCHANNELS);
			memset(outbuffer,0,sizeof(outbuffer));
			totalsamples += insize;
			int64 numseconds = totalsamples/44100;

			r->resample_gain(
            		(void*)inbuffer,
				    insize,
					(void*) outbuffer,
				    outsize,
					gain,
					false // mix
					);

			if((delay++%5000)==0)
			{
				printf("total input: %8Ld (%Ld:%02Ld:%02Ld)\n",totalsamples, numseconds/3600, (numseconds/60)%60, numseconds%60);

//				printf("%f %f %d (%f)\n",	r->m_state.fl_xd_step,
//											r->m_state.fl_xd, 
//											r->m_state.i_xd, 
//											float(INRATE+r->m_state.i_xd)/OUTRATE
//											);

//				if(r->m_state.fl_xd>1.0)
//					r->m_state.fl_xd-=1.0;
				
				v->FillRect(v->Bounds(),B_SOLID_LOW);
				int32 numlines=0;
				v->BeginLineArray(256);
				rgb_color black;
				black.set_to(0,0,0,255);
				for(int i=0;i<outsize/2;i++)
				{
//				printf("i: %d\n",i);
					v->AddLine
							(
								BPoint(i,150),
								BPoint(i,150+outbuffer[i*2]*100),
								black
							);
					if(numlines++>=255);
					{
						v->EndLineArray();
						v->BeginLineArray(256);
						numlines=0;
					}
				}
				v->EndLineArray();
			}
		}
};


int main()
{
	BApplication app("application/x-vnd.Be-resamplertestbed");
	new mywin();
	app.Run();
}
