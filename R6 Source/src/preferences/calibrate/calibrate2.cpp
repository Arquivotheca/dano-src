
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <Application.h>
#include <View.h>
#include <Window.h>
#include <Screen.h>
#include <Slider.h>
#include <Path.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <Button.h>
#include <TextView.h>
#include <Box.h>

const long CALIBRATION_CLICK='clbr';
const long CALIBRATION_MOVE='move';


class ConfirmationWindow: public BWindow
{
	private:
		BMessageRunner *runner;
		BBox *box;
		BTextView *tv;
		char *config;
		int	counter;
		port_id port;

	public:
		ConfirmationWindow(const char *cfg, port_id prt)
			: BWindow(BRect(100,100,299,199),"Confirm",B_TITLED_WINDOW,B_NOT_RESIZABLE)
		{
			config=strdup(cfg);
			port=prt;
			box=new BBox(BRect(0,0,200,100));
			AddChild(box);
			tv=new BTextView(BRect(10,10,180,50),"",BRect(0,0,170,40),B_FOLLOW_NONE);
			tv->MakeEditable(false);
			tv->MakeSelectable(false);
			tv->SetWordWrap(true);
			tv->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			box->AddChild(tv);

			BButton *ok=new BButton(BRect(10,65,95,70),"","OK",new BMessage('okok'));
			BButton *cancel=new BButton(BRect(105,65,190,70),"","Cancel",new BMessage('not!'));
			box->AddChild(ok);
			box->AddChild(cancel);
			counter=10;
			runner=new BMessageRunner(BMessenger(this),new BMessage('ping'),1000000);

			write_port(port,4,config,strlen(config)+1); // temp-recalibrate
		}
		~ConfirmationWindow()
		{
			write_port(port,2,NULL,0); // true recalibrate
			delete runner;
			free(config);
		}
		bool QuitRequested()
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			return true;
		}
		void WriteConfig(directory_which where, const char *config)
		{
			BPath path;
			find_directory(where, &path);
			path.Append("dt300_calibration");
            FILE *file = fopen(path.Path(), "w");
            if (file != NULL)
			{
				fprintf(file,"%s\n",config);
				fclose(file);
			}
			else
				fprintf(stderr,"error writing calibration\n"); 
		}
		void MessageReceived(BMessage *mes)
		{
			switch(mes->what)
			{
				case 'ping':
					if(--counter==0)
						PostMessage(B_QUIT_REQUESTED);
					else
					{
						char msg[1000];
						sprintf(msg,"Click OK to confirm new calibration, or Cancel to keep previous setting. Auto-cancel in %d seconds.",counter);
						tv->SetText(msg);
					}
					break;

				case 'okok':
					// finalize calibration
					WriteConfig(B_USER_SETTINGS_DIRECTORY, config);
					PostMessage(B_QUIT_REQUESTED);
					break;

				case 'not!':
					// cancel calibration
					PostMessage(B_QUIT_REQUESTED);
					break;
				
				default:
					BWindow::MessageReceived(mes);
					break;
			}		
		}
};

class CalibrateView: public BView
{
	private:
		float	saveX[4], saveY[4];
		int count;
		// leave contrast and brightness ordered like this!
		int32 contrast;
		int32 brightness;
		BSlider *contrastslider;
		BSlider *brightslider;
		port_id port;
		bigtime_t clicktime;
	
	public:
		CalibrateView()
			: BView(BRect(0,0,50,50),"",B_FOLLOW_NONE,B_WILL_DRAW)
		{
			SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			count=0;
			port=B_BAD_VALUE;
		}
		virtual ~CalibrateView()
		{
		}
		virtual void DetachedFromWindow()
		{
		}
		virtual void AttachedToWindow()
		{
			port=find_port("dt300 control port");
			BMessenger msngr(this);
			write_port(port,1,&msngr,sizeof(msngr)); // raw mode

			BRect size=Window()->Bounds();
			ResizeTo(size.Width(),size.Height());
			size.InsetBy(size.Width()*0.4,size.Height()*0.40);
			contrastslider=new BSlider(size,"contrast","Contrast",new BMessage('cntr'),100,150);
			contrastslider->SetLimitLabels("Low","High");
	//		AddChild(contrastslider);
	//		contrastslider->SetTarget(this);

			size.top+=(size.bottom-size.top)/2;
			brightslider=new BSlider(size,"brightness","Brightness",new BMessage('brte'),100,150);
			brightslider->SetLimitLabels("Low","High");
	//		AddChild(brightslider);
	//		brightslider->SetTarget(this);
			clicktime=system_time();
		}
		virtual void RawMouseMoved(float x, float y)
		{
			if(count<4)
			{
				saveX[count]=x;
				saveY[count]=y;
				count++;
				Invalidate();
			}
			if(count==4)
			{
					count=5;
					float	xDelta, xScale, xMuly;
					float	yDelta, yScale, yMulx;

					xDelta = -saveX[0];
					yDelta = -saveY[0];
					yScale = 1 / (saveY[1] + yDelta);
					xScale = 1 / (saveX[2] + xDelta);
					xMuly = (1 / ((saveX[3] + xDelta) * xScale) - 1) / ((saveY[3] + yDelta) * yScale);
					yMulx = (1 / ((saveY[3] + yDelta) * yScale) - 1) / ((saveX[3] + xDelta) * xScale);
					printf("%f %f %f %f %f %f %f %f\n",
						xDelta, xScale, 0.0, xMuly,
						yDelta, yScale, 0.0, yMulx);
					fprintf(stderr, "validation:\n");
					for(int i=0; i<4; i++)
						fprintf(stderr, " (%f,%f)\n",
							((saveX[i] + xDelta) * xScale) * (1 + ((saveY[i] + yDelta) * yScale) * xMuly),
							((saveY[i] + yDelta) * yScale) * (1 + ((saveX[i] + xDelta) * xScale) * yMulx));

					char config[1000];
					sprintf(config,"%f %f %f %f %f %f %f %f",
							xDelta, xScale, 0.0, xMuly,
							yDelta, yScale, 0.0, yMulx);
					write_port(port,0,NULL,0); // switch back to translated mode
					(new ConfirmationWindow(config,port))->Show();
					Window()->PostMessage(B_QUIT_REQUESTED);
			}
		}	
		virtual void Draw(BRect rect)
		{
			BRect bigbounds=Bounds();
			BRect smallbounds=bigbounds;
			smallbounds.InsetBy(bigbounds.Width()*0.2,bigbounds.Height()*0.2666666);
			SetPenSize(3);
			const char *str="Please click here";
			BFont font;
			GetFont(&font);
			font.SetSize(20);
			SetFont(&font);
			switch(count)
			{
				case 0:
					StrokeLine(smallbounds.LeftTop(),bigbounds.LeftTop());
					FillTriangle(	bigbounds.LeftTop(),
									bigbounds.LeftTop()+BPoint(5,20),
									bigbounds.LeftTop()+BPoint(20,5));
					MovePenTo(smallbounds.LeftTop()+BPoint(-40,20));
					DrawString(str);
					break;
				case 1:
					StrokeLine(smallbounds.LeftBottom(),bigbounds.LeftBottom());
					FillTriangle(	bigbounds.LeftBottom(),
									bigbounds.LeftBottom()+BPoint(5,-20),
									bigbounds.LeftBottom()+BPoint(20,-5));
					MovePenTo(smallbounds.LeftBottom()+BPoint(-40,-20));
					DrawString(str);
					break;
				case 2:
					StrokeLine(smallbounds.RightTop(),bigbounds.RightTop());
					FillTriangle(	bigbounds.RightTop(),
									bigbounds.RightTop()+BPoint(-5,20),
									bigbounds.RightTop()+BPoint(-20,5));
					MovePenTo(smallbounds.RightTop()+BPoint(-40,20));
					DrawString(str);
					break;
				case 3:
					StrokeLine(smallbounds.RightBottom(),bigbounds.RightBottom());
					FillTriangle(	bigbounds.RightBottom(),
									bigbounds.RightBottom()+BPoint(-5,-20),
									bigbounds.RightBottom()+BPoint(-20,-5));
					MovePenTo(smallbounds.RightBottom()+BPoint(-40,-20));
					DrawString(str);
					break;
			}

		}
		virtual void MessageReceived(BMessage *mes)
		{
			switch(mes->what)
			{
				case 'rawm':
					{
						float x,y;
						bigtime_t now=system_time();
						if(	mes->FindFloat("x",&x)==B_OK &&	mes->FindFloat("y",&y)==B_OK && (now-clicktime)>200000)
							RawMouseMoved(x,y);
						clicktime=now;
					}
					break;

				case '_c+b':
					{
						if(mes->FindInt32("contrast",&contrast)==B_OK && mes->FindInt32("brightness",&brightness))
						{
							contrastslider->SetValue(contrast);	
							brightslider->SetValue(brightness);	
						}
					}
					break;

				case 'cntr':
				case 'brte':
					contrast=contrastslider->Value();
					brightness=brightslider->Value();
					write_port(port,3,&contrast,2*sizeof(int));
					break;
			}
		}

		void SetBandC()
		{
			char data[2];
			data[0]=contrastslider->Value();
			data[1]=brightslider->Value();
		}
};



class CalibrateWindow: public BWindow
{
	private:
		CalibrateView *view;
	public:
		CalibrateWindow()
			: BWindow(BRect(0,0,100,100),"",B_TITLED_WINDOW,0)
		{
			BScreen screen;
			BRect frame=screen.Frame();
			MoveTo(frame.LeftTop());
			ResizeTo(frame.Width(),frame.Height());
			AddChild(view=new CalibrateView);
		}
	
		virtual ~CalibrateWindow()
		{}
	
		virtual bool QuitRequested()
		{
			return true;
		}
};


int main()
{
	BApplication calibrateapp("application/x-vnd.Be-Calibrate");

	CalibrateWindow *win=new CalibrateWindow();
	win->Show();
	calibrateapp.Run();
}
