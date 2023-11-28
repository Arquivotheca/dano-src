#include "PowerMgmtController.h"
#include <driver/bklight.h>
#include <Application.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <string.h>
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Roster.h>
#include <Screen.h>
#include <stdio.h>
#include <unistd.h>
#include <Debug.h>

PowerMgmtController::PowerMgmtController()
 : BLooper("PowerMgmt Looper"), last(system_time()), lastwake(system_time()),
   saving(false),fadetime(PM_DELAY),pkg(DT300)
{

	BMessenger me(this);
	start = system_time();
	
	saving_sem = create_sem(1, "saving_sem");

	// generate events every two seconds to check idleness
	idlecheck = new BMessageRunner(me, new BMessage('idle'), 1000000);

}

bool PowerMgmtController::QuitRequested()
{
	delete idlecheck;
	delete_sem (saving_sem);
	return true;
}

void PowerMgmtController::MessageReceived(BMessage *msg)
{
	int err;
	switch(msg->what)
	{
		case 'idle' :
			if ((err = acquire_sem_etc(saving_sem,1, B_CAN_INTERRUPT,0))==B_NO_ERROR)
			{
				if( !saving )
				{
					if(fadetime && system_time() - last > fadetime * 1000000LL) {
						saving = Power_Off_Devs(pkg);
					}
				}
				release_sem_etc(saving_sem, 1, 0);	
			}
			break;

		default :
			BLooper::MessageReceived(msg);
			break;
	}
}

bool PowerMgmtController::Power_Off_Devs (int32 pkg)
{
	switch(pkg)
	{
		case  DT300:
			{
				int bkldrvr = open("/dev/misc/bklight", O_RDWR);
				if (bkldrvr != -1) 
				{
					int32 bklparm = 0;
					ioctl(bkldrvr,BK_LIGHT_OFF,&bklparm);
					close(bkldrvr);
				}
				return true;    /*power saving active*/
			}
		
		default:
			fadetime = 0;		/*disable PM from this pt on */
			return false;	
	}
}

void PowerMgmtController::Power_On_Devs (int32 pkg)
{
	switch(pkg)
	{
		case  DT300:
			{
				int bkldrvr = open("/dev/misc/bklight", O_RDWR);
				if (bkldrvr != -1) 
				{
					int32 bklparm = 0;
					ioctl(bkldrvr,BK_LIGHT_ON,&bklparm);
					close(bkldrvr);
				}
			
			} break;
		
		default:	
			break;
	}
}


bool
PowerMgmtController::ProcessInput(BMessage *)
{
	int err;
	last = system_time();
	if ((err = acquire_sem_etc(saving_sem,1, B_CAN_INTERRUPT,0))==B_NO_ERROR)
	{
		if(saving)
		{
			saving = false;
			Power_On_Devs(pkg);
			lastwake = last;
		}
		release_sem_etc(saving_sem, 1, 0);
	}	

	return true;
}


